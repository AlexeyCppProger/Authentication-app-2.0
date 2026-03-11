#include "mainwindow.h"
#include "UserRepository.h"

#include <QApplication>
#include <QCoreApplication>
#include <QStackedWidget>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableView>
#include <QSqlTableModel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDebug>
#include <QStatusBar>

MainWindow::MainWindow(UserRepository* repo, QWidget* parent)
    : QMainWindow(parent),
      repo_(repo),
      stacked_(nullptr),
      loginPage_(nullptr),
      registerPage_(nullptr),
      profilePage_(nullptr),
      adminPage_(nullptr),
      leLoginUsername_(nullptr),
      leLoginPassword_(nullptr),
      btnLogin_(nullptr),
      btnGoRegister_(nullptr),
      lblLoginStatus_(nullptr),
      leRegUsername_(nullptr),
      leRegPassword_(nullptr),
      leRegPasswordConfirm_(nullptr),
      btnRegister_(nullptr),
      btnBackToLogin_(nullptr),
      lblRegStatus_(nullptr),
      lblWelcome_(nullptr),
      btnLogout_(nullptr),
      btnShowChangePassword_(nullptr),
      lblChangeStatus_(nullptr),
      tvUsers_(nullptr),
      btnRefreshUsers_(nullptr),
      btnCreateUser_(nullptr),
      btnDeleteUser_(nullptr),
      lblAdminStatus_(nullptr),
      usersModel_(nullptr)
{
    if (!repo_) {
        qWarning() << "MainWindow: UserRepository pointer is null. Some features will be disabled.";
    }

    setWindowTitle("User Manager");
    resize(900, 600);

    // Создаём стек и страницы
    stacked_ = new QStackedWidget(this);

    setupUi();
    applyStyles();

    setCentralWidget(stacked_);

    // Default page: login
    if (loginPage_) stacked_->setCurrentWidget(loginPage_);
}

MainWindow::~MainWindow()
{
    // QObjects с parent'ом будут удалены автоматически, но модель без parent'а (если была) — удалим
    if (usersModel_) {
        delete usersModel_;
        usersModel_ = nullptr;
    }
}

/* ---------- UI setup ---------- */
void MainWindow::setupUi()
{
    //
    // --- LOGIN PAGE ---
    //
    loginPage_ = new QWidget(this);
    {
        auto *v = new QVBoxLayout(loginPage_);
        QLabel *title = new QLabel("<b>Sign in</b>");
        title->setAlignment(Qt::AlignCenter);
        title->setMinimumHeight(40);

        leLoginUsername_ = new QLineEdit();
        leLoginUsername_->setPlaceholderText("Username");

        leLoginPassword_ = new QLineEdit();
        leLoginPassword_->setPlaceholderText("Password");
        leLoginPassword_->setEchoMode(QLineEdit::Password);

        btnLogin_ = new QPushButton("Login");
        btnGoRegister_ = new QPushButton("Create account");

        lblLoginStatus_ = new QLabel();
        lblLoginStatus_->setStyleSheet("color: #E53935;"); // red for errors

        auto *form = new QFormLayout();
        form->addRow("Username:", leLoginUsername_);
        form->addRow("Password:", leLoginPassword_);

        auto *hbtn = new QHBoxLayout();
        hbtn->addWidget(btnLogin_);
        hbtn->addWidget(btnGoRegister_);

        v->addWidget(title);
        v->addLayout(form);
        v->addLayout(hbtn);
        v->addWidget(lblLoginStatus_);
        v->addStretch();
    }

    connect(btnLogin_, &QPushButton::clicked, this, &MainWindow::onLogin);
    connect(btnGoRegister_, &QPushButton::clicked, this, &MainWindow::onShowRegister);

    //
    // --- REGISTER PAGE ---
    //
    registerPage_ = new QWidget(this);
    {
        auto *v = new QVBoxLayout(registerPage_);
        QLabel *title = new QLabel("<b>Create account</b>");
        title->setAlignment(Qt::AlignCenter);
        title->setMinimumHeight(40);

        leRegUsername_ = new QLineEdit();
        leRegUsername_->setPlaceholderText("Username");

        leRegPassword_ = new QLineEdit();
        leRegPassword_->setPlaceholderText("Password");
        leRegPassword_->setEchoMode(QLineEdit::Password);

        leRegPasswordConfirm_ = new QLineEdit();
        leRegPasswordConfirm_->setPlaceholderText("Confirm password");
        leRegPasswordConfirm_->setEchoMode(QLineEdit::Password);

        btnRegister_ = new QPushButton("Register");
        btnBackToLogin_ = new QPushButton("Back to login");

        lblRegStatus_ = new QLabel();
        lblRegStatus_->setStyleSheet("color: #E53935;");

        auto *form = new QFormLayout();
        form->addRow("Username:", leRegUsername_);
        form->addRow("Password:", leRegPassword_);
        form->addRow("Confirm:", leRegPasswordConfirm_);

        auto *hbtn = new QHBoxLayout();
        hbtn->addWidget(btnRegister_);
        hbtn->addWidget(btnBackToLogin_);

        v->addWidget(title);
        v->addLayout(form);
        v->addLayout(hbtn);
        v->addWidget(lblRegStatus_);
        v->addStretch();
    }

    connect(btnRegister_, &QPushButton::clicked, this, &MainWindow::onRegister);
    connect(btnBackToLogin_, &QPushButton::clicked, [this]() {
        if (lblRegStatus_) lblRegStatus_->clear();
        if (loginPage_) stacked_->setCurrentWidget(loginPage_);
    });

    //
    // --- PROFILE PAGE (user) ---
    //
    profilePage_ = new QWidget(this);
    {
        auto *v = new QVBoxLayout(profilePage_);
        lblWelcome_ = new QLabel();
        lblWelcome_->setAlignment(Qt::AlignCenter);
        lblWelcome_->setMinimumHeight(40);

        btnLogout_ = new QPushButton("Logout");
        btnShowChangePassword_ = new QPushButton("Change password");

        lblChangeStatus_ = new QLabel();
        lblChangeStatus_->setStyleSheet("color: #E53935;");

        auto *hbtn = new QHBoxLayout();
        hbtn->addStretch();
        hbtn->addWidget(btnShowChangePassword_);
        hbtn->addWidget(btnLogout_);
        hbtn->addStretch();

        v->addWidget(lblWelcome_);
        v->addLayout(hbtn);
        v->addWidget(lblChangeStatus_);
        v->addStretch();
    }

    connect(btnLogout_, &QPushButton::clicked, this, &MainWindow::onLogout);
    connect(btnShowChangePassword_, &QPushButton::clicked, this, &MainWindow::onShowChangePassword);

    //
    // --- ADMIN PAGE (list of users) ---
    //
    adminPage_ = new QWidget(this);
    {
        auto *v = new QVBoxLayout(adminPage_);

        QLabel *title = new QLabel("<b>Users</b>");
        title->setAlignment(Qt::AlignCenter);
        title->setMinimumHeight(40);

        tvUsers_ = new QTableView();
        tvUsers_->setSelectionBehavior(QAbstractItemView::SelectRows);
        tvUsers_->setSelectionMode(QAbstractItemView::SingleSelection);
        tvUsers_->verticalHeader()->hide();
        tvUsers_->setAlternatingRowColors(true);
        tvUsers_->setEditTriggers(QAbstractItemView::NoEditTriggers);

        btnRefreshUsers_ = new QPushButton("Refresh");
        btnCreateUser_ = new QPushButton("Create user...");
        btnDeleteUser_ = new QPushButton("Delete selected");

        auto *h = new QHBoxLayout();
        h->addWidget(btnRefreshUsers_);
        h->addStretch();
        h->addWidget(btnCreateUser_);
        h->addWidget(btnDeleteUser_);

        lblAdminStatus_ = new QLabel();
        lblAdminStatus_->setStyleSheet("color: #E53935;");

        v->addWidget(title);
        v->addWidget(tvUsers_);
        v->addLayout(h);
        v->addWidget(lblAdminStatus_);
    }

    connect(btnRefreshUsers_, &QPushButton::clicked, this, &MainWindow::onRefreshUsers);
    connect(btnCreateUser_, &QPushButton::clicked, this, &MainWindow::onCreateUser);
    connect(btnDeleteUser_, &QPushButton::clicked, this, &MainWindow::onDeleteUser);

    // Add pages to stack
    stacked_->addWidget(loginPage_);
    stacked_->addWidget(registerPage_);
    stacked_->addWidget(profilePage_);
    stacked_->addWidget(adminPage_);
}

/* ---------- Styles ---------- */
void MainWindow::applyStyles()
{
    QString qss = R"(
        QWidget {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                                       stop:0 #f6fbff, stop:1 #eef7ff);
            font-family: "Segoe UI", Roboto, Arial;
            color: #263238;
        }
        QLabel { font-size: 14px; }
        QLineEdit {
            background: white;
            border: 1px solid #cfd8dc;
            border-radius: 6px;
            padding: 6px;
            min-height: 28px;
        }
        QPushButton {
            background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #42a5f5, stop:1 #1e88e5);
            color: white;
            border-radius: 6px;
            padding: 6px 12px;
            min-height: 30px;
            font-weight: 600;
        }
        QPushButton:disabled {
            background: #90caf9;
            color: rgba(255,255,255,0.7);
        }
        QTableView {
            background: rgba(255,255,255,0.9);
            border: 1px solid #cfd8dc;
            selection-background-color: #cfe9ff;
        }
    )";

    QCoreApplication* core = QCoreApplication::instance();
    if (core) {
        QApplication* app = qobject_cast<QApplication*>(core);
        if (app) {
            app->setStyleSheet(qss);
        } else {
            qWarning() << "applyStyles: QCoreApplication::instance() is not QApplication";
        }
    } else {
        qWarning() << "applyStyles: QApplication instance is null - stylesheet not applied";
    }
}

/* ---------- Helpers ---------- */
void MainWindow::showStatusMessage(const QString& msg, int timeout)
{
    if (statusBar()) {
        statusBar()->showMessage(msg, timeout);
    } else {
        qWarning() << "No status bar available. Message:" << msg;
    }
}

/* ---------- Slots ---------- */

void MainWindow::onLogin()
{
    if (lblLoginStatus_) lblLoginStatus_->clear();
    const QString username = leLoginUsername_ ? leLoginUsername_->text().trimmed() : QString();
    const QString password = leLoginPassword_ ? leLoginPassword_->text() : QString();

    if (username.isEmpty() || password.isEmpty()) {
        if (lblLoginStatus_) lblLoginStatus_->setText("Fill username and password.");
        return;
    }

    if (!repo_) {
        if (lblLoginStatus_) lblLoginStatus_->setText("Internal error: repository not available.");
        qWarning() << "onLogin: repo_ is null";
        return;
    }

    bool ok = repo_->authenticate(username, password);
    if (!ok) {
        if (lblLoginStatus_) lblLoginStatus_->setText("Authentication failed.");
        return;
    }

    currentUsername_ = username;
    if (lblWelcome_) lblWelcome_->setText(QString("Welcome, <b>%1</b>").arg(username));
    showStatusMessage("Signed in");

    if (username.toLower() == "admin") {
        onRefreshUsers();
        if (adminPage_) stacked_->setCurrentWidget(adminPage_);
    } else {
        if (profilePage_) stacked_->setCurrentWidget(profilePage_);
    }

    if (leLoginPassword_) leLoginPassword_->clear();
}

void MainWindow::onShowRegister()
{
    if (lblRegStatus_) lblRegStatus_->clear();
    if (leRegUsername_) leRegUsername_->clear();
    if (leRegPassword_) leRegPassword_->clear();
    if (leRegPasswordConfirm_) leRegPasswordConfirm_->clear();
    if (loginPage_ && registerPage_) stacked_->setCurrentWidget(registerPage_);
}

void MainWindow::onRegister()
{
    if (lblRegStatus_) lblRegStatus_->clear();
    QString username = leRegUsername_ ? leRegUsername_->text().trimmed() : QString();
    QString pass = leRegPassword_ ? leRegPassword_->text() : QString();
    QString pass2 = leRegPasswordConfirm_ ? leRegPasswordConfirm_->text() : QString();

    if (username.isEmpty() || pass.isEmpty()) {
        if (lblRegStatus_) lblRegStatus_->setText("Please fill username and password.");
        return;
    }
    if (pass != pass2) {
        if (lblRegStatus_) lblRegStatus_->setText("Passwords do not match.");
        return;
    }

    if (!repo_) {
        if (lblRegStatus_) lblRegStatus_->setText("Internal error: repository not available.");
        qWarning() << "onRegister: repo_ is null";
        return;
    }

    if (repo_->userExists(username)) {
        if (lblRegStatus_) lblRegStatus_->setText("Username is already taken.");
        return;
    }

    int newId = repo_->createUser(username, pass);
    if (newId <= 0) {
        if (lblRegStatus_) lblRegStatus_->setText("Failed to create user. Check logs.");
        return;
    }

    QMessageBox::information(this, "Success", "Account created. You can log in now.");
    if (registerPage_ && loginPage_) stacked_->setCurrentWidget(loginPage_);
}

void MainWindow::onLogout()
{
    currentUsername_.clear();
    showStatusMessage("Signed out");
    if (loginPage_) stacked_->setCurrentWidget(loginPage_);
}

void MainWindow::onShowChangePassword()
{
    if (!repo_) {
        QMessageBox::warning(this, "Error", "Internal error: repository not available.");
        qWarning() << "onShowChangePassword: repo_ is null";
        return;
    }

    bool ok;
    QString oldP = QInputDialog::getText(this, "Old password", "Enter old password:", QLineEdit::Password, QString(), &ok);
    if (!ok) return;
    QString newP = QInputDialog::getText(this, "New password", "Enter new password:", QLineEdit::Password, QString(), &ok);
    if (!ok) return;

    if (oldP.isEmpty() || newP.isEmpty()) {
        QMessageBox::warning(this, "Error", "Passwords cannot be empty.");
        return;
    }

    bool changed = repo_->changePassword(currentUsername_, oldP, newP);
    if (!changed) {
        QMessageBox::warning(this, "Error", "Failed to change password. Make sure old password is correct.");
        return;
    }
    QMessageBox::information(this, "Success", "Password changed.");
}

void MainWindow::onCreateUser()
{
    if (!repo_) {
        if (lblAdminStatus_) lblAdminStatus_->setText("Internal error: repository not available.");
        qWarning() << "onCreateUser: repo_ is null";
        return;
    }

    bool ok;
    QString username = QInputDialog::getText(this, "Create user", "Username:", QLineEdit::Normal, QString(), &ok);
    if (!ok || username.trimmed().isEmpty()) return;
    QString pass = QInputDialog::getText(this, "Create user", "Password:", QLineEdit::Password, QString(), &ok);
    if (!ok) return;

    if (repo_->userExists(username)) {
        if (lblAdminStatus_) lblAdminStatus_->setText("Username already exists.");
        return;
    }

    int id = repo_->createUser(username.trimmed(), pass);
    if (id <= 0) {
        if (lblAdminStatus_) lblAdminStatus_->setText("Failed to create user.");
    } else {
        if (lblAdminStatus_) {
            lblAdminStatus_->setStyleSheet("color: #2e7d32;"); // green
            lblAdminStatus_->setText("User created.");
        }
        onRefreshUsers();
    }
}

void MainWindow::onRefreshUsers()
{
    if (lblAdminStatus_) lblAdminStatus_->clear();

    if (!QSqlDatabase::database().isValid()) {
        if (lblAdminStatus_) lblAdminStatus_->setText("No valid database connection.");
        qWarning() << "onRefreshUsers: no valid QSqlDatabase connection";
        return;
    }

    if (!usersModel_) {
        usersModel_ = new QSqlTableModel(this, QSqlDatabase::database());
        usersModel_->setTable("users");
        usersModel_->setEditStrategy(QSqlTableModel::OnManualSubmit);
        usersModel_->select();

        int colCount = usersModel_->columnCount();
        for (int c = 0; c < colCount; ++c) {
            QString header = usersModel_->headerData(c, Qt::Horizontal).toString().toLower();
            if (header.contains("password") || header.contains("password_hash")) {
                if (tvUsers_) tvUsers_->setColumnHidden(c, true);
            }
        }

        if (tvUsers_) {
            tvUsers_->setModel(usersModel_);
            tvUsers_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        }
    } else {
        usersModel_->select();
    }
}

void MainWindow::onDeleteUser()
{
    if (lblAdminStatus_) lblAdminStatus_->clear();

    if (!repo_) {
        if (lblAdminStatus_) lblAdminStatus_->setText("Internal error: repository not available.");
        qWarning() << "onDeleteUser: repo_ is null";
        return;
    }

    if (!tvUsers_) {
        if (lblAdminStatus_) lblAdminStatus_->setText("No users view available.");
        return;
    }

    QModelIndex idx = tvUsers_->currentIndex();
    if (!idx.isValid()) {
        if (lblAdminStatus_) lblAdminStatus_->setText("Select a user to delete.");
        return;
    }

    int row = idx.row();
    int idCol = -1;
    if (!usersModel_) {
        if (lblAdminStatus_) lblAdminStatus_->setText("Internal error: model not available.");
        return;
    }

    for (int c = 0; c < usersModel_->columnCount(); ++c) {
        QString header = usersModel_->headerData(c, Qt::Horizontal).toString().toLower();
        if (header == "id" || header == "user id" || header == "userid") {
            idCol = c;
            break;
        }
    }

    int id = -1;
    if (idCol >= 0) {
        QVariant idVal = usersModel_->data(usersModel_->index(row, idCol));
        id = idVal.toInt();
    } else {
        QVariant idVal = usersModel_->data(usersModel_->index(row, 0));
        bool ok = false;
        id = idVal.toInt(&ok);
        if (!ok) {
            if (lblAdminStatus_) lblAdminStatus_->setText("Failed to determine user id.");
            return;
        }
    }

    if (QMessageBox::question(this, "Confirm", "Delete selected user?") != QMessageBox::Yes) return;

    QSqlQuery q(QSqlDatabase::database());
    q.prepare("DELETE FROM users WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        if (lblAdminStatus_) lblAdminStatus_->setText("Failed to delete user.");
        qWarning() << "onDeleteUser: delete query failed:" << q.lastError().text();
        return;
    }

    onRefreshUsers();
    if (lblAdminStatus_) {
        lblAdminStatus_->setStyleSheet("color: #2e7d32;");
        lblAdminStatus_->setText("User deleted.");
    }
}
