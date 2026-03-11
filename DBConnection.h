#ifndef DBCONNECTION_H
#define DBCONNECTION_H
#include <QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
/*
  Класс-обёртка для работы с QODBC / QSqlDatabase.
  Интерфейс минимален и безопасен в отношении требований Qt:
  - перед вызовом QSqlDatabase::removeDatabase необходимо сбрасывать все локальные QSqlDatabase объекты.
  - здесь m_db хранится как QSqlDatabase, и метод close() сбрасывает его перед removeDatabase.
*/
class DBConnection
{
public:
    // Конструкторы
    explicit DBConnection(const QString &connectionName = QString());
    DBConnection(const QString &driver,
                 const QString &connectionName,
                 const QString &user,
                 const QString &password);
    ~DBConnection();

    // Открыть соединение, передавая составляющие параметров
    // (driver — например "MySQL ODBC 8.0 Unicode Driver")
    bool open(const QString &driver,
              const QString &server,
              int port,
              const QString &database,
              const QString &user,
              const QString &password,
              const QString &options = QString());

    // Открыть по уже сформированной DSN-строке:
    // "DRIVER={...};SERVER=...;PORT=...;DATABASE=...;UID=...;PWD=...;OPTION=3;"
    bool openWithDsn(const QString &dsn);

    // Закрыть соединение и удалить его из QSqlDatabase
    void close();

    // Возвращает handle на текущее QSqlDatabase (может быть невалиден)
    QSqlDatabase database() const;

    // Текст последней ошибки (человеко-читаемый)
    QString lastErrorText() const;

    // Текст нативной (драйверной) ошибки
    QString lastNativeErrorText() const;

    // Имя соединения, используемое в QSqlDatabase
    QString connectionName() const;

    // Проверка, открыто ли соединение
    bool isOpen() const;

private:
    QString m_connectionName;
    QSqlDatabase m_db;
    QString m_lastError;
    QString m_lastNativeError;

    // Вспомогательная функция для сохранения ошибок из QSqlDatabase
    void captureLastError(const QSqlDatabase &db);
};

#endif // DBCONNECTION_H
