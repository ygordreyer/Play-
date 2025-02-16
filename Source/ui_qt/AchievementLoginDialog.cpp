#include "AchievementLoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

CAchievementLoginDialog::CAchievementLoginDialog(QWidget* parent)
    : QDialog(parent)
    , m_loginState(LoginState::Idle)
{
    setWindowTitle(tr("RetroAchievements Login"));
    setModal(true);

    CreateLayout();
    ConnectSignals();
    ValidateInput();
}

CAchievementLoginDialog::~CAchievementLoginDialog() = default;

QString CAchievementLoginDialog::GetUsername() const
{
    return m_usernameInput->text();
}

QString CAchievementLoginDialog::GetPassword() const
{
    return m_passwordInput->text();
}

void CAchievementLoginDialog::CreateLayout()
{
    auto mainLayout = new QVBoxLayout(this);

    // Form layout for inputs
    auto formLayout = new QFormLayout();
    m_usernameInput = new QLineEdit(this);
    m_passwordInput = new QLineEdit(this);
    m_passwordInput->setEchoMode(QLineEdit::Password);

    formLayout->addRow(tr("Username:"), m_usernameInput);
    formLayout->addRow(tr("Password:"), m_passwordInput);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("QLabel { color: red; }");
    m_statusLabel->hide();

    // Buttons
    auto buttonLayout = new QHBoxLayout();
    m_loginButton = new QPushButton(tr("Login"), this);
    m_cancelButton = new QPushButton(tr("Cancel"), this);
    m_loginButton->setEnabled(false);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_cancelButton);

    // Add all layouts
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void CAchievementLoginDialog::ConnectSignals()
{
    connect(m_loginButton, &QPushButton::clicked, this, &CAchievementLoginDialog::OnLoginClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &CAchievementLoginDialog::OnCancelClicked);
    connect(m_usernameInput, &QLineEdit::textChanged, this, &CAchievementLoginDialog::ValidateInput);
    connect(m_passwordInput, &QLineEdit::textChanged, this, &CAchievementLoginDialog::ValidateInput);
}

void CAchievementLoginDialog::OnLoginClicked()
{
    m_loginState = LoginState::Authenticating;
    m_loginButton->setEnabled(false);
    m_cancelButton->setEnabled(false);
    m_usernameInput->setEnabled(false);
    m_passwordInput->setEnabled(false);

    m_statusLabel->setText(tr("Authenticating..."));
    m_statusLabel->show();

    // TODO: Implement actual authentication using AchievementManager
    accept();
}

void CAchievementLoginDialog::OnCancelClicked()
{
    reject();
}

void CAchievementLoginDialog::ValidateInput()
{
    bool isValid = !m_usernameInput->text().isEmpty() && !m_passwordInput->text().isEmpty();
    m_loginButton->setEnabled(isValid);
}
