#ifndef PASSWORDHASHER_H
#define PASSWORDHASHER_H

#include <QString>

class PasswordHasher {
public:
    // Создать хеш пароля и вернуть его в виде строки, готовой для хранения в БД
    static QString createHash(const QString &password);

    // Проверить пароль против сохранённого хеша (true — совпадает)
    static bool verify(const QString &password, const QString &storedHash);

    // Проверить, нужно ли пересчитать хеш с новыми параметрами (true — нужно)
    static bool needsRehash(const QString &storedHash);
};

#endif // PASSWORDHASHER_H
