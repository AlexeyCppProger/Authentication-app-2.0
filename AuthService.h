#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include <QString>

class UserRepository;

class AuthService {
public:
    explicit AuthService(UserRepository &repo);

    // По вашему плану: возвращают bool
    bool registerUser(const QString &username, const QString &password);
    bool login(const QString &username, const QString &password);

private:
    UserRepository &m_repo;
};

#endif // AUTHSERVICE_H
