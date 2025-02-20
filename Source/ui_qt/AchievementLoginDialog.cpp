#include "AchievementLoginDialog.h"
#include "AchievementHTTPDownloader.h"
#include "AchievementsConfig.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
    constexpr char RETROACHIEVEMENTS_API_URL[] = "https://retroachievements.org/API/API_Login.php";
}

AchievementLoginDialog::AchievementLoginDialog(QWidget* parent, bool tokenExpired)
    : QDialog(parent)
    , m_isProcessingLogin(false)
    , m_tokenExpired(tokenExpired)
{
    setWindowTitle(tr("RetroAchievements Login"));
    setModal(true);

    // Initialize systems
    m_httpDownloader = AchievementHTTPDownloader::Create();
    m_config = CAchievementsConfig::LoadConfig();

    setupUI();
    connectSignals();

    // Set custom message if token expired
    if (m_tokenExpired) {
        ui.messageLabel->setText(tr("Your login token has expired. Please log in again to continue tracking achievements."));
        ui.messageLabel->setStyleSheet("QLabel { color: #E74C3C; }");
        ui.messageLabel->setVisible(true);
    }

    updateUI(true);
}

AchievementLoginDialog::~AchievementLoginDialog() = default;

void AchievementLoginDialog::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Message area for errors/status
    ui.messageLabel = new QLabel(this);
    ui.messageLabel->setWordWrap(true);
    ui.messageLabel->setVisible(false);
    mainLayout->addWidget(ui.messageLabel);

    // Form layout for inputs
    auto formLayout = new QFormLayout();
    formLayout->setSpacing(8);

    ui.usernameInput = new QLineEdit(this);
    ui.usernameInput->setPlaceholderText(tr("Enter your RetroAchievements username"));

    ui.passwordInput = new QLineEdit(this);
    ui.passwordInput->setEchoMode(QLineEdit::Password);
    ui.passwordInput->setPlaceholderText(tr("Enter your password"));

    formLayout->addRow(tr("Username:"), ui.usernameInput);
    formLayout->addRow(tr("Password:"), ui.passwordInput);
    mainLayout->addLayout(formLayout);

    // Status label
    ui.statusLabel = new QLabel(this);
    ui.statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(ui.statusLabel);

    // Buttons
    auto buttonLayout = new QHBoxLayout();
    ui.loginButton = new QPushButton(tr("Login"), this);
    ui.loginButton->setEnabled(false);
    ui.loginButton->setDefault(true);

    ui.cancelButton = new QPushButton(tr("Cancel"), this);

    buttonLayout->addStretch();
    buttonLayout->addWidget(ui.loginButton);
    buttonLayout->addWidget(ui.cancelButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    setMinimumWidth(400);
}

void AchievementLoginDialog::connectSignals()
{
    connect(ui.loginButton, &QPushButton::clicked, this, &AchievementLoginDialog::onLoginRequested);
    connect(ui.cancelButton, &QPushButton::clicked, this, &AchievementLoginDialog::onCancelRequested);
    connect(ui.usernameInput, &QLineEdit::textChanged, this, &AchievementLoginDialog::onInputChanged);
    connect(ui.passwordInput, &QLineEdit::textChanged, this, &AchievementLoginDialog::onInputChanged);
}

void AchievementLoginDialog::onLoginRequested()
{
    if (m_isProcessingLogin) return;

    m_isProcessingLogin = true;
    updateUI(false);
    updateLoginStatus(tr("Authenticating..."));

    // Prepare request data
    QJsonObject requestData;
    requestData["u"] = ui.usernameInput->text();
    requestData["p"] = ui.passwordInput->text();

    // Create HTTP request
    m_httpDownloader->CreatePostRequest(
        RETROACHIEVEMENTS_API_URL,
        QJsonDocument(requestData).toJson().toStdString(),
        [this](int32 status_code, const std::string& content_type, const std::vector<uint8>& data) {
            // Process response on main thread
            QMetaObject::invokeMethod(this, [this, status_code, data]() {
                processLoginResult(status_code,
                    data.empty() ? "" : std::string(reinterpret_cast<const char*>(data.data()), data.size()));
            });
        }
    );
}

void AchievementLoginDialog::processLoginResult(int32 statusCode, const std::string& errorMessage)
{
    m_isProcessingLogin = false;

    if (statusCode != AchievementHTTPDownloader::HTTP_STATUS_OK) {
        showError(tr("Network error: %1").arg(QString::fromStdString(errorMessage)));
        updateUI(true);
        return;
    }

    // Parse response
    QJsonDocument response = QJsonDocument::fromJson(QByteArray::fromStdString(errorMessage));
    if (!response.isObject()) {
        showError(tr("Invalid server response"));
        updateUI(true);
        return;
    }

    QJsonObject responseObj = response.object();
    if (!responseObj["Success"].toBool()) {
        showError(tr("Invalid username or password"));
        updateUI(true);
        return;
    }

    // Store authentication info
    const std::string token = responseObj["Token"].toString().toStdString();
    const std::string username = ui.usernameInput->text().toStdString();

    CAchievementsConfig::Auth auth;
    auth.token = token;
    auth.username = username;
    auth.points = responseObj["Points"].toInt();
    auth.unreadMessages = responseObj["UnreadMessages"].toInt();

    m_config->SetAuth(auth);
    m_config->Save();

    handleSuccessfulLogin(token);
}

void AchievementLoginDialog::handleSuccessfulLogin(const std::string& token)
{
    // If user initiated login (not token expired), offer to enable features
    if (!m_tokenExpired) {
        auto settings = m_config->GetSettings();

        // Prompt to enable achievements if disabled
        if (!settings.enabled) {
            if (QMessageBox::question(this,
                tr("Enable Achievements"),
                tr("Would you like to enable achievement tracking?"),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            {
                settings.enabled = true;
            }
        }

        // Prompt for hardcore mode
        if (!settings.hardcoreMode) {
            if (QMessageBox::question(this,
                tr("Hardcore Mode"),
                tr("Would you like to enable hardcore mode?\n\n"
                   "This mode disables save states and ensures competitive integrity."),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            {
                settings.hardcoreMode = true;
            }
        }

        if (settings.enabled != m_config->GetSettings().enabled ||
            settings.hardcoreMode != m_config->GetSettings().hardcoreMode)
        {
            m_config->SetSettings(settings);
            m_config->Save();
        }
    }

    accept();
}

void AchievementLoginDialog::onCancelRequested()
{
    if (m_tokenExpired) {
        // If token expired and user cancels, disable hardcore mode
        auto settings = m_config->GetSettings();
        settings.hardcoreMode = false;
        m_config->SetSettings(settings);
        m_config->Save();
    }
    reject();
}

void AchievementLoginDialog::onInputChanged()
{
    ui.loginButton->setEnabled(validateInput());
    ui.statusLabel->clear();
}

void AchievementLoginDialog::updateUI(bool enabled)
{
    ui.usernameInput->setEnabled(enabled);
    ui.passwordInput->setEnabled(enabled);
    ui.loginButton->setEnabled(enabled && validateInput());
    ui.cancelButton->setEnabled(enabled);
}

bool AchievementLoginDialog::validateInput() const
{
    return !ui.usernameInput->text().isEmpty() &&
           !ui.passwordInput->text().isEmpty();
}

void AchievementLoginDialog::showError(const QString& message)
{
    updateLoginStatus(message, true);
}

void AchievementLoginDialog::updateLoginStatus(const QString& message, bool isError)
{
    ui.statusLabel->setText(message);
    ui.statusLabel->setStyleSheet(isError ?
        "QLabel { color: #E74C3C; }" :
        "QLabel { color: #2980B9; }");
}
