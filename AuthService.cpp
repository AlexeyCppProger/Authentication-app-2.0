#include "AuthService.h"
#include "UserRepository.h"
#include <stdexcept>

AuthService::AuthService(UserRepository &repo)
    : m_repo(repo)
{
}

bool AuthService::registerUser(const QString &username, const QString &password) {
    // Базовая валидация (можно расширить по правилам проекта)
    if (username.trimmed().isEmpty()) return false;
    if (password.size() < 8) return false; // минимальная длина — пример

    // Репозиторий уже проверяет существование и хеширует пароль,
    // но можно проверить заранее, чтобы дать более предсказуемый ответ.
    try {
        if (m_repo.userExists(username)) return false;

        int id = m_repo.createUser(username, password); // createUser делает хеширование
        Q_UNUSED(id);
        return true;
    } catch (const std::exception &e) {
        // здесь можно логировать e.what()
        return false;
    }
}

bool AuthService::login(const QString &username, const QString &password) {
    if (username.trimmed().isEmpty() || password.isEmpty()) return false;

    try {
        bool ok = m_repo.authenticate(username, password);
        if (ok) {
            // При необходимости: обновить last_login или создать сессию.
            // Сейчас у репозитория нет метода updateLastLogin(id),
            // можно добавить в репозиторий и вызвать его тут.
        }
        return ok;
    } catch (const std::exception &e) {
        // обработка/логирование ошибки
        return false;
    }
}
