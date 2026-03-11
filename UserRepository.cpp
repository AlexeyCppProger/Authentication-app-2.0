#include "UserRepository.h"
#include "PasswordHasher.h"
#include <QSqlQuery>                        // Для выполнения SQL-запросов
#include <QSqlError>                        // Для получения описания ошибок БД
#include <QVariant>                         // Для работы с QVariant (lastInsertId и т.д.)
#include <QDateTime>                        // Для отметки времени создания
#include <stdexcept>                        // Для std::runtime_error
#include <QDebug>                           // Для отладки (qWarning и т.п.)A

UserRepository::UserRepository(const QString &connectionName)
    : m_connectionName(connectionName)     // Инициализируем поле с именем подключения
{
    // Пустое тело конструктора — всё сделано в списке инициализации
}

QSqlDatabase UserRepository::db() const {
    // Возвращаем объект QSqlDatabase по имени подключения; QSqlDatabase::database выбросит исключение,
    // если подключения с таким именем нет — но обычно в приложении подключение создаётся заранее.
    return QSqlDatabase::database(m_connectionName);
}

void UserRepository::ensureTableExists()
{
    if (!QSqlDatabase::contains(m_connectionName)) {
        qCritical() << "Connection not found:" << m_connectionName;
        throw std::runtime_error("DB connection not found");
    }

    QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
    if (!db.isValid() || !db.isOpen()) {
        qCritical() << "DB is not open for connection:" << m_connectionName
                    << " valid:" << db.isValid() << " open:" << db.isOpen()
                    << " driver:" << db.driverName();
        throw std::runtime_error("DB not open");
    }

    // Выбираем SQL в зависимости от драйвера (для QSQLITE — sqlite, иначе — mysql/odbc)
    QString createSql;
    QString drv = db.driverName().toLower();
    if (drv.contains("sqlite")) {
        createSql = QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL UNIQUE,
                password_hash TEXT NOT NULL,
                created_at TEXT NOT NULL
            )
        )");
    } else {
        // Подходит для MySQL (native или ODBC->MySQL)
        createSql = QStringLiteral(R"(
            CREATE TABLE IF NOT EXISTS users (
                id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
                username VARCHAR(255) NOT NULL UNIQUE,
                password_hash VARCHAR(255) NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
        )");
    }

    qDebug() << "Creating users table. Connection:" << m_connectionName
             << " driver:" << db.driverName();
    qDebug() << "Create SQL:" << createSql;

    QSqlQuery q(db);
    if (!q.exec(createSql)) {
        qCritical() << "Failed to create users table:" << q.lastError().text();
        qCritical() << "Query was:" << q.lastQuery();
        throw std::runtime_error(q.lastError().text().toStdString());
    }
}

int UserRepository::createUser(const QString &username, const QString &password)
{
    if (username.trimmed().isEmpty()) {
        qWarning() << "username is empty";
        return -1;
    }
    if (userExists(username)) {
        qWarning() << "user already exists";
        return -1;
    }

    QString hash = PasswordHasher::createHash(password);

    QSqlDatabase database = db();
    QSqlQuery q(database);

    // Вариант: не вставляем created_at, если БД сама проставляет текущий timestamp
    q.prepare(QStringLiteral("INSERT INTO users (username, password_hash) VALUES (:username, :password_hash)"));
    q.bindValue(QStringLiteral(":username"), username);
    q.bindValue(QStringLiteral(":password_hash"), hash);

    if (!database.transaction()) {
        qWarning() << "Failed to start transaction:" << database.lastError().text();
        // продолжаем попробовать выполнить вставку, либо вернуть ошибку:
    }

    if (!q.exec()) {
        database.rollback();
        qWarning() << "Failed to insert user:" << q.lastError().text() << ", query:" << q.lastQuery();
        return -1;
    }

    if (!database.commit()) {
        database.rollback();
        qWarning() << "Failed to commit transaction:" << database.lastError().text();
        return -1;
    }

    QVariant lastId = q.lastInsertId();
    if (lastId.isNull()) {
        return -1;
    }
    return lastId.toInt();
}

bool UserRepository::authenticate(const QString &username, const QString &password) {
    // Подготовим SELECT, чтобы получить password_hash для данного username
    QSqlDatabase database = db();                  // Получаем подключение
    QSqlQuery q(database);                          // QSqlQuery, связанный с подключением
    q.prepare(QStringLiteral("SELECT id, password_hash FROM users WHERE username = :username LIMIT 1")); // Выбираем id и password_hash
    q.bindValue(QStringLiteral(":username"), username); // Привязываем username

    if (!q.exec()) {                                // Выполняем запрос; если ошибка —
        qWarning() << "Failed to query user for authentication:" << q.lastError().text(); // Логируем ошибку
        return false;                               // Возвращаем false — не удалось аутентифицировать
    }

    if (!q.next()) {                                // Если нет строк — пользователь не найден
        return false;                               // Возвращаем false — аутентификация неуспешна
    }

    int userId = q.value(QStringLiteral("id")).toInt();         // Читаем id пользователя (может понадобиться для обновления хеша)
    QString storedHash = q.value(QStringLiteral("password_hash")).toString(); // Читаем сохранённый хеш пароля

    // Проверяем пароль с помощью PasswordHasher; verify возвращает true при совпадении
    bool ok = PasswordHasher::verify(password, storedHash);     // Проверка пароля
    if (!ok) {                                                  // Если пароль неверен —
        return false;                                           // Возвращаем false
    }

    // Если пароль верен, проверим, не устарел ли хеш (например, если вы повысили параметры KDF)
    if (PasswordHasher::needsRehash(storedHash)) {              // Если нужно пересчитать хеш —
        try {
            // Генерируем новый хеш и обновляем запись в БД
            QString newHash = PasswordHasher::createHash(password); // Создаём новый хеш с текущими параметрами

            QSqlQuery uq(database);                            // Создаём QSqlQuery для обновления
            uq.prepare(QStringLiteral("UPDATE users SET password_hash = :ph WHERE id = :id")); // Подготовка UPDATE
            uq.bindValue(QStringLiteral(":ph"), newHash);      // Привязка нового хеша
            uq.bindValue(QStringLiteral(":id"), userId);      // Привязка id пользователя

            if (!uq.exec()) {                                 // Выполняем UPDATE; если не удалось —
                qWarning() << "Failed to update password hash during rehash:" << uq.lastError().text(); // Логируем проблему, но не разлогиниваем пользователя
            }
            // Заметьте: ошибку при обновлении хеша мы не пробрасываем дальше — аутентификация прошла успешно
        } catch (const std::exception &e) {
            // Ловим возможные исключения от createHash и логируем их, но не препятствуем успешной аутентификации
            qWarning() << "Rehash failed:" << e.what();
        }
    }

    return true;                                               // При успешной проверке пароля возвращаем true
}

bool UserRepository::changePassword(const QString &username, const QString &oldPassword, const QString &newPassword) {
    // Сначала проверяем аутентификацию по старому паролю
    if (!authenticate(username, oldPassword)) {              // Если аутентификация не прошла —
        return false;                                       // Возвращаем false — смена пароля запрещена
    }

    // Получаем новый хеш пароля
    QString newHash = PasswordHasher::createHash(newPassword); // Создаём хеш для нового пароля

    // Обновляем запись в БД
    QSqlDatabase database = db();                            // Получаем подключение
    QSqlQuery q(database);                                    // Создаём QSqlQuery
    q.prepare(QStringLiteral("UPDATE users SET password_hash = :ph WHERE username = :username")); // Подготовка UPDATE
    q.bindValue(QStringLiteral(":ph"), newHash);              // Привязываем новый хеш
    q.bindValue(QStringLiteral(":username"), username);       // Привязываем username

    if (!q.exec()) {                                          // Выполняем UPDATE; если не удалось —
        qWarning() << "Failed to change password:" << q.lastError().text(); // Логируем ошибку
        return false;                                         // Возвращаем false
    }

    return true;                                              // Возвращаем true при успешном обновлении
}

bool UserRepository::userExists(const QString &username) {
    // Простой запрос COUNT(*) для проверки существования username
    QSqlDatabase database = db();                            // Получаем подключение
    QSqlQuery q(database);                                    // Создаём QSqlQuery
    q.prepare(QStringLiteral("SELECT COUNT(1) as cnt FROM users WHERE username = :username")); // COUNT = количество записей
    q.bindValue(QStringLiteral(":username"), username);       // Привязываем username

    if (!q.exec()) {                                          // Выполняем запрос; если ошибка —
        qWarning() << "Failed to check user existence:" << q.lastError().text(); // Логируем
        return false;                                         // Возвращаем false (безопасно трактовать как "не существует" или "оШибка")
    }

    if (!q.next()) {                                          // Если нет строки с результатом (маловероятно) —
        return false;                                         // Возвращаем false
    }

    int count = q.value(QStringLiteral("cnt")).toInt();       // Читаем значение cnt
    return count > 0;                                         // Возвращаем true, если count > 0
}
