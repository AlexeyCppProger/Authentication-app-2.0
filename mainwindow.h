#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once

#include <QMainWindow>
#include <QString>

class UserRepository;
class QStackedWidget;
class QWidget;
class QLineEdit;
class QPushButton;
class QLabel;
class QTableView;
class QSqlTableModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Делаем repo опциональным (по умолчанию nullptr) — можно создавать MainWindow без аргументов
    explicit MainWindow(UserRepository* repo = nullptr, QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    // UI setup
    void setupUi();
    void applyStyles();
    void showStatusMessage(const QString& msg, int timeout = 3000);

private slots:
    // Слоты, используемые в mainwindow.cpp
    void onLogin();
    void onShowRegister();
    void onRegister();
    void onLogout();
    void onShowChangePassword();
    void onCreateUser();
    void onRefreshUsers();
    void onDeleteUser();

private:
    // Внешний репозиторий/модель
    UserRepository* repo_{nullptr};

    // Главные виджеты
    QStackedWidget* stacked_{nullptr};
    QWidget* loginPage_{nullptr};
    QWidget* registerPage_{nullptr};
    QWidget* profilePage_{nullptr};
    QWidget* adminPage_{nullptr};

    // Login controls
    QLineEdit* leLoginUsername_{nullptr};
    QLineEdit* leLoginPassword_{nullptr};
    QPushButton* btnLogin_{nullptr};
    QPushButton* btnGoRegister_{nullptr};
    QLabel* lblLoginStatus_{nullptr};

    // Register controls
    QLineEdit* leRegUsername_{nullptr};
    QLineEdit* leRegPassword_{nullptr};
    QLineEdit* leRegPasswordConfirm_{nullptr};
    QPushButton* btnRegister_{nullptr};
    QPushButton* btnBackToLogin_{nullptr};
    QLabel* lblRegStatus_{nullptr};

    // Profile controls
    QLabel* lblWelcome_{nullptr};
    QPushButton* btnLogout_{nullptr};
    QPushButton* btnShowChangePassword_{nullptr};
    QLabel* lblChangeStatus_{nullptr};

    // Admin controls
    QTableView* tvUsers_{nullptr};
    QPushButton* btnRefreshUsers_{nullptr};
    QPushButton* btnCreateUser_{nullptr};
    QPushButton* btnDeleteUser_{nullptr};
    QLabel* lblAdminStatus_{nullptr};

    // Model for users table
    QSqlTableModel* usersModel_{nullptr};

    // Current session
    QString currentUsername_;
};
#endif // MAINWINDOW_H
