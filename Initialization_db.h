#ifndef INITIALIZATION_DB_H
#define INITIALIZATION_DB_H

#include <QString>
#include <QtSql/QSqlDatabase>

/*
  Initialization_db.h
  - Объявляет константы по умолчанию (extern) — их определения находятся в Initialization_db.cpp.
  - Объявляет класс Initialization_db с методом initialize, который выполняет создание таблиц/начальные данные.
*/

namespace InitializationDefaults {
    extern const QString DEFAULT_DRIVER;          // имя драйвера (например "QODBC" или "QMYSQL")
    extern const QString DEFAULT_SERVER;          // адрес сервера (например "127.0.0.1")
    extern const int     DEFAULT_PORT;            // порт (например 3306)
    extern const QString DEFAULT_DATABASE;        // имя базы данных (например "H123")
    extern const QString DEFAULT_USER;            // логин (например "root")
    extern const QString DEFAULT_PASSWORD;        // пароль (например "admin")
    extern const QString DEFAULT_CONNECTION_NAME; // имя подключения в QSqlDatabase (например "main_conn")
    extern const QString DEFAULT_OPTIONS;         // дополнительные опции (если нужны)
}

class Initialization_db
{
public:
    // Выполняет инициализацию схемы/начальных данных в уже открытом QSqlDatabase db.
    // При ошибке возвращает false и при наличии outError записывает текст ошибки.
    static bool initialize(const QSqlDatabase &db, QString *outError = nullptr);
};

#endif // INITIALIZATION_DB_H
