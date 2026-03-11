#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

#include <QString>
#include <QSqlDatabase>                   // Для доступа к подключению к БД через Qt

class UserRepository {
public:
    // Конструктор принимает имя подключения QSqlDatabase; по умолчанию используется дефолтное подключение
    explicit UserRepository(const QString &connectionName = QLatin1String(QSqlDatabase::defaultConnection)); // Явный конструктор с именем подключения

    // Создаёт таблицу users, если её ещё нет
    void ensureTableExists();            // Метод создания таблицы

    // Создать пользователя и вернуть его id (при ошибке бросает std::runtime_error)
    int createUser(const QString &username, const QString &password); // Создание пользователя

    // Аутентифицировать пользователя по имени и паролю (true — если пароль верен)
    bool authenticate(const QString &username, const QString &password); // Аутентификация

    // Изменить пароль (вернёт true при успехе, false если аутентификация не удалась)
    bool changePassword(const QString &username, const QString &oldPassword, const QString &newPassword); // Смена пароля

    // Проверить, существует ли пользователь с таким именем
    bool userExists(const QString &username); // Проверка существования пользователя

private:
    // Вспомогательный метод для получения объекта QSqlDatabase по имени подключения
    QSqlDatabase db() const;               // Получение объекта БД

    QString m_connectionName;              // Храним имя подключения в поле
};


#endif // USERREPOSITORY_H
