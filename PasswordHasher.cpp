#include "PasswordHasher.h"
#include <sodium.h>
#include <QByteArray>            // Для преобразований QString <-> байты
#include <stdexcept>            // Для выбрасывания исключений
#include <QString>

QString PasswordHasher::createHash(const QString &password) {
    // Преобразуем входной пароль в UTF-8 байты — QByteArray владеет данными и гарантирует null-терминатор при необходимости
    QByteArray pw = password.toUtf8();                             // pw содержит байтовое представление пароля

    // Буфер для результата: crypto_pwhash_STRBYTES — размер строки, возвращаемой crypto_pwhash_str (включая '\0')
    char out[crypto_pwhash_STRBYTES];                              // out — C-строка для хранения результирующего хеша

    // Вызов высшего уровня KDF: генерирует соль автоматически и записывает ASCII-строку формата "$argon2id$..." в out
    // Параметры OPSLIMIT/MEMLIMIT выбираем интерактивные по умолчанию (баланс безопасности и производительности)
    if (crypto_pwhash_str(
            out,                                                   // куда записать результат
            pw.constData(),                                        // пароль (C-строка/буфер)
            static_cast<unsigned long long>(pw.size()),            // длина пароля
            crypto_pwhash_OPSLIMIT_INTERACTIVE,                    // число операций (рекомендуемое интерактивное)
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {            // лимит памяти (рекомендуемое интерактивное)
        // Если функция вернула ненулевой код — не удалось выделить ресурсы (например, недостаточно памяти)
        throw std::runtime_error("crypto_pwhash_str failed");       // Бросаем исключение с сообщением
    }

    // Возвращаем результат как QString; используем fromLatin1 т.к. out содержит ASCII-символы (формат хеша)
    return QString::fromLatin1(out);                               // Возвращаем строку хеша (для хранения в БД)
}

bool PasswordHasher::verify(const QString &password, const QString &storedHash) {
    // Преобразуем пароль в UTF-8 байты
    QByteArray pw = password.toUtf8();                             // pw — байты пароля
    // Преобразуем сохранённый хеш в латинский байтовый буфер (C-строка)
    QByteArray st = storedHash.toLatin1();                         // st — байтовое представление хеша

    // crypto_pwhash_str_verify возвращает 0 при совпадении, -1 при несовпадении или ошибке
    // Поэтому мы сравниваем с 0 и возвращаем true если успех
    return crypto_pwhash_str_verify(
            st.constData(),                                        // сохранённый хеш (C-строка)
            pw.constData(),                                        // пароль для проверки
            static_cast<unsigned long long>(pw.size())) == 0;     // длина пароля; проверяем равенство с 0
}

bool PasswordHasher::needsRehash(const QString &storedHash) {
    // Преобразуем сохранённый хеш в байты
    QByteArray st = storedHash.toLatin1();                         // st — байтовое представление хеша

    // crypto_pwhash_str_needs_rehash возвращает 1 если требуется пересчитать хеш под новые параметры,
    // и 0 если текущие параметры хеша соответствуют указанным.
    // Здесь мы передаём желаемые текущие параметры (INTERACTIVE), можно заменить на более строгие.
    return crypto_pwhash_str_needs_rehash(
            st.constData(),                                        // сохранённый хеш
            crypto_pwhash_OPSLIMIT_INTERACTIVE,                    // желаемый OPSLIMIT
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0;             // желаемый MEMLIMIT; !=0 -> needs rehash
}
