#include "Initialization_db.h"

#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDebug>

namespace InitializationDefaults {
    const QString DEFAULT_DRIVER = QStringLiteral("QODBC");
    const QString DEFAULT_SERVER = QStringLiteral("127.0.0.1");
    const int     DEFAULT_PORT = 3306;
    const QString DEFAULT_DATABASE = QStringLiteral("h123");
    const QString DEFAULT_USER = QStringLiteral("root");
    const QString DEFAULT_PASSWORD = QStringLiteral("admin");
    const QString DEFAULT_CONNECTION_NAME = QStringLiteral("main_conn");
    const QString DEFAULT_OPTIONS = QStringLiteral("");
}

bool Initialization_db::initialize(const QSqlDatabase &db, QString *outError)
{
    if (!db.isValid()) {
        if (outError) *outError = QStringLiteral("Недействительный объект QSqlDatabase.");
        qDebug() << "Initialization_db::initialize: invalid QSqlDatabase";
        return false;
    }

    if (!db.isOpen()) {
        if (outError) *outError = QStringLiteral("Соединение с базой данных не открыто.");
        qDebug() << "Initialization_db::initialize: database is not open";
        return false;
    }

    // Всё успешно
    return true;
}
/*
    QSqlQuery query(db);

    // Пример создания таблицы users (MySQL-синтаксис).
    const QString createUsersTable =
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INT AUTO_INCREMENT PRIMARY KEY, "
            "username VARCHAR(64) NOT NULL UNIQUE, "
            "password_hash VARCHAR(128) NOT NULL, "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ")"
        );

    if (!query.exec(createUsersTable)) {
        QString err = query.lastError().text();
        if (outError) *outError = QStringLiteral("Ошибка создания таблицы users: %1").arg(err);
        qDebug() << "Initialization_db::initialize - create table error:" << err;
        qDebug() << "lastQuery:" << query.lastQuery();
        return false;
    }
*/
