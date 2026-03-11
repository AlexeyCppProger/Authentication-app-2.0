#include "DBConnection.h"
#include <QDebug>
#include <QtSql/QSqlError>

DBConnection::DBConnection(const QString &connectionName)
    : m_connectionName(connectionName)
{
    // Отложенное создание QSqlDatabase — делаем addDatabase только при необходимости (в open/openWithDsn или во втором конструкторе)
}

DBConnection::DBConnection(const QString &driver,
                           const QString &connectionName,
                           const QString &user,
                           const QString &password)
    : m_connectionName(connectionName)
{
    // Создаём QSqlDatabase сразу. driver ожидается как Qt‑драйвер (например "QODBC" или "QMYSQL").
    // Если передан пустой driver — по умолчанию используем QODBC.
    const QString qtDriver = driver.isEmpty() ? QStringLiteral("QODBC") : driver;

    // Удалим старое соединение с таким именем, если оно существует
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    m_db = QSqlDatabase::addDatabase(qtDriver, m_connectionName);
    m_db.setUserName(user);
    m_db.setPassword(password);
}

DBConnection::~DBConnection()
{
    close();
}

bool DBConnection::open(const QString &driver,
                        const QString &server,
                        int port,
                        const QString &database,
                        const QString &user,
                        const QString &password,
                        const QString &options)
{
    // Сформируем DSN (для QODBC). Параметр driver здесь — имя ODBC-драйвера,
    // например "MySQL ODBC 8.0 Unicode Driver".
    // Сам Qt‑драйвер для подключения через ODBC — "QODBC".
    const QString qtDriver = QStringLiteral("QODBC");

    // Удаляем возможное существующее подключение с таким именем.
    // Важно: убедиться, что у вас нет других живых QSqlDatabase объектов, ссылающихся на это подключение.
    if (QSqlDatabase::contains(m_connectionName)) {
        // Сбрасываем локальный handle (если есть) перед removeDatabase
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    m_db = QSqlDatabase::addDatabase(qtDriver, m_connectionName);

    // Формируем строку DSN, как ожидает ODBC-драйвер
    QString dsn = QString("DRIVER={%1};SERVER=%2;PORT=%3;DATABASE=%4;UID=%5;PWD=%6;%7")
                      .arg(driver)
                      .arg(server)
                      .arg(port)
                      .arg(database)
                      .arg(user)
                      .arg(password)
                      .arg(options);

    m_db.setDatabaseName(dsn);

    if (!m_db.open()) {
        captureLastError(m_db);
        qCritical() << "Database open failed:" << m_lastError << "(native:)" << m_lastNativeError;
        // Сбрасываем локальный handle — не держим повреждённое соединение
        m_db = QSqlDatabase();
        return false;
    }

    // Сбросим сообщение об ошибке (если было)
    m_lastError.clear();
    m_lastNativeError.clear();
    qDebug() << "Database opened (connectionName =" << m_connectionName << ")";
    return true;
}

bool DBConnection::openWithDsn(const QString &dsn)
{
    // Открываем соединение, используя заранее сформированную DSN‑строку.
    // Используем Qt драйвер QODBC для ODBC‑строки.
    const QString qtDriver = QStringLiteral("QODBC");

    if (QSqlDatabase::contains(m_connectionName)) {
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    m_db = QSqlDatabase::addDatabase(qtDriver, m_connectionName);
    m_db.setDatabaseName(dsn);

    if (!m_db.open()) {
        captureLastError(m_db);
        qCritical() << "openWithDsn: Database open failed:" << m_lastError << "(native:)" << m_lastNativeError;
        m_db = QSqlDatabase();
        return false;
    }

    m_lastError.clear();
    m_lastNativeError.clear();
    qDebug() << "Database opened with DSN (connectionName =" << m_connectionName << ")";
    return true;
}

void DBConnection::close()
{
    // Закрываем соединение и корректно удаляем его из QSqlDatabase.
    if (m_db.isValid()) {
        if (m_db.isOpen()) {
            m_db.close();
        }
        // Сбрасываем локальный handle (обязательно) перед removeDatabase
        m_db = QSqlDatabase();
    }

    if (!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

QSqlDatabase DBConnection::database() const
{
    // Возвращаем handle на текущее соединение, если оно существует.
    if (!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName)) {
        return QSqlDatabase::database(m_connectionName);
    }
    // Иначе — вернём пустой QSqlDatabase
    return QSqlDatabase();
}

QString DBConnection::lastErrorText() const
{
    return m_lastError;
}

QString DBConnection::lastNativeErrorText() const
{
    return m_lastNativeError;
}

QString DBConnection::connectionName() const
{
    return m_connectionName;
}

bool DBConnection::isOpen() const
{
    if (m_db.isValid()) {
        return m_db.isOpen();
    }
    if (!m_connectionName.isEmpty() && QSqlDatabase::contains(m_connectionName)) {
        return QSqlDatabase::database(m_connectionName).isOpen();
    }
    return false;
}

void DBConnection::captureLastError(const QSqlDatabase &db)
{
    if (!db.isValid()) {
        m_lastError.clear();
        m_lastNativeError.clear();
        return;
    }

    QSqlError err = db.lastError();
    m_lastError = err.text();
    m_lastNativeError = err.driverText();
}
