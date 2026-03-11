#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QVariant>

#include "DBConnection.h"
#include "UserRepository.h"

// Поддержка optional заголовка Initialization_db.h и InitializationDefaults
#if __has_include("Initialization_db.h")
#  include "Initialization_db.h"
#  define HAVE_INITIALIZATION_DB 1
#else
#  define HAVE_INITIALIZATION_DB 0
// Запасные значения, если заголовок недоступен
namespace InitializationDefaults {
    static const QString DEFAULT_CONNECTION_NAME = QStringLiteral("main_conn");
    static const QString DEFAULT_DRIVER = QStringLiteral("QSQLITE"); // безопасный fallback
    static const QString DEFAULT_SERVER = QStringLiteral("localhost");
    static const QString DEFAULT_PORT = QStringLiteral("0"); // строка — будет корректно конвертирована
    static const QString DEFAULT_DATABASE = QStringLiteral("app.db");
    static const QString DEFAULT_USER = QStringLiteral("");
    static const QString DEFAULT_PASSWORD = QStringLiteral("");
    static const QString DEFAULT_OPTIONS = QStringLiteral("");
}
#endif

// make_unique для старых стандартов (если нужно)
#if __cplusplus < 201402L
#include <memory>
namespace std {
    template<typename T, typename ...Args>
    unique_ptr<T> make_unique(Args&&... args) {
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();
#if HAVE_INITIALIZATION_DB
    qDebug() << "Using InitializationDefaults::DEFAULT_DRIVER =" << InitializationDefaults::DEFAULT_DRIVER;
#else
    qDebug() << "Initialization_db.h not found — using fallback defaults, driver:" << InitializationDefaults::DEFAULT_DRIVER;
#endif

    // Создаём объект подключения
    DBConnection dbConn(InitializationDefaults::DEFAULT_CONNECTION_NAME);

    // Приводим порт к int независимо от того, строка это или число
    int portInt = QVariant(InitializationDefaults::DEFAULT_PORT).toInt();

    // Пытаемся открыть соединение (подстроить параметры под вашу сигнатуру open)
    bool opened = dbConn.open(
        InitializationDefaults::DEFAULT_DRIVER,
        InitializationDefaults::DEFAULT_SERVER,
        portInt,                                   // <-- порт как int
        InitializationDefaults::DEFAULT_DATABASE,
        InitializationDefaults::DEFAULT_USER,
        InitializationDefaults::DEFAULT_PASSWORD,
        InitializationDefaults::DEFAULT_OPTIONS
    );

    // Если не удалось и драйвер QODBC — пробуем собрать DSN и вызвать openWithDsn (если метод есть)
    if (!opened && InitializationDefaults::DEFAULT_DRIVER == QStringLiteral("QODBC")) {
        QString dsn = QStringLiteral("DRIVER={%1};SERVER=%2;PORT=%3;DATABASE=%4;UID=%5;PWD=%6;")
            .arg(InitializationDefaults::DEFAULT_OPTIONS.isEmpty() ? QStringLiteral("MySQL ODBC 8.0 Unicode Driver")
                                                                    : InitializationDefaults::DEFAULT_OPTIONS)
            .arg(InitializationDefaults::DEFAULT_SERVER)
            .arg(portInt)
            .arg(InitializationDefaults::DEFAULT_DATABASE)
            .arg(InitializationDefaults::DEFAULT_USER)
            .arg(InitializationDefaults::DEFAULT_PASSWORD);

        qDebug() << "Attempting to open via DSN:" << dsn;
        opened = dbConn.openWithDsn(dsn);
    }

    if (!opened) {
        QString errText = dbConn.lastErrorText();
        if (errText.isEmpty()) {
            QSqlDatabase db = dbConn.database();
            if (db.isValid())
                errText = db.lastError().text();
        }

        qCritical() << "Failed to open DB connection:" << errText;
        QMessageBox::critical(nullptr,
                              QStringLiteral("Ошибка подключения к БД"),
                              QStringLiteral("Не удалось открыть соединение с базой данных:\n%1").arg(errText));
        dbConn.close();
        return 1;
    }

#if HAVE_INITIALIZATION_DB
    // Инициализация БД (если есть заголовок/функция)
    QString initErr;
    if (!Initialization_db::initialize(dbConn.database(), &initErr)) {
        qCritical() << "Database initialization failed:" << initErr;
        QMessageBox::critical(nullptr,
                              QStringLiteral("Ошибка инициализации БД"),
                              QStringLiteral("Инициализация БД не удалась:\n%1").arg(initErr));
        dbConn.close();
        return 2;
    }
#else
    qDebug() << "Initialization_db not available — пропускаем инициализацию схемы.";
#endif

    // Создаём репозиторий пользователей и держим его в unique_ptr
    std::unique_ptr<UserRepository> repo = std::make_unique<UserRepository>(InitializationDefaults::DEFAULT_CONNECTION_NAME);

    // Передаём указатель в главное окно (предполагается конструктор MainWindow(UserRepository*, QWidget*))
    MainWindow w(repo.get());
    w.show();

    int rc = app.exec();
    qDebug() << "drivers:" << QSqlDatabase::drivers();
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", "test_odbc");
    db.setDatabaseName("DSN=MyDsnName;UID=myuser;PWD=mypass"); // или полный Driver=... string
    if (!db.open()) {
        qCritical() << "ODBC error:" << db.lastError().text() << db.lastError().nativeErrorCode();
    } else {
        qDebug() << "ODBC OK";
    }
    QSqlDatabase::removeDatabase("test_odbc");
    dbConn.close();
    return rc;
}
