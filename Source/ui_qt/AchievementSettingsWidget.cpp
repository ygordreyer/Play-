#include "AchievementSettingsWidget.h"
#include "AchievementLoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QMessageBox>

CAchievementSettingsWidget::CAchievementSettingsWidget(QWidget* parent)
    : QWidget(parent)
    , m_isLoggedIn(false)
{
    CreateLayout();
    ConnectSignals();
    LoadSettings();
    UpdateLoginState();
}

CAchievementSettingsWidget::~CAchievementSettingsWidget() = default;

void CAchievementSettingsWidget::CreateLayout()
{
    auto mainLayout = new QVBoxLayout(this);

    // Core settings group
    auto coreGroup = new QGroupBox(tr("Core Settings"), this);
    auto coreLayout = new QVBoxLayout();
    m_enableAchievements = new QCheckBox(tr("Enable Achievements"), this);
    m_hardcoreMode = new QCheckBox(tr("Hardcore Mode"), this);
    m_encoreMode = new QCheckBox(tr("Encore Mode"), this);
    m_spectatorMode = new QCheckBox(tr("Spectator Mode"), this);
    m_unofficialMode = new QCheckBox(tr("Unofficial Achievements"), this);

    coreLayout->addWidget(m_enableAchievements);
    coreLayout->addWidget(m_hardcoreMode);
    coreLayout->addWidget(m_encoreMode);
    coreLayout->addWidget(m_spectatorMode);
    coreLayout->addWidget(m_unofficialMode);
    coreGroup->setLayout(coreLayout);

    // Notification settings group
    auto notifyGroup = new QGroupBox(tr("Notification Settings"), this);
    auto notifyLayout = new QVBoxLayout();
    auto durationLayout = new QFormLayout();

    m_enableNotifications = new QCheckBox(tr("Show Achievement Notifications"), this);
    m_enableLeaderboardNotifications = new QCheckBox(tr("Show Leaderboard Notifications"), this);
    m_enableSoundEffects = new QCheckBox(tr("Enable Sound Effects"), this);
    m_enableOverlays = new QCheckBox(tr("Show Achievement Overlays"), this);

    m_notificationDuration = new QSlider(Qt::Horizontal, this);
    m_notificationDuration->setMinimum(1);
    m_notificationDuration->setMaximum(10);
    m_notificationDuration->setValue(3);

    m_leaderboardDuration = new QSlider(Qt::Horizontal, this);
    m_leaderboardDuration->setMinimum(1);
    m_leaderboardDuration->setMaximum(10);
    m_leaderboardDuration->setValue(3);

    durationLayout->addRow(tr("Notification Duration (seconds):"), m_notificationDuration);
    durationLayout->addRow(tr("Leaderboard Duration (seconds):"), m_leaderboardDuration);

    notifyLayout->addWidget(m_enableNotifications);
    notifyLayout->addWidget(m_enableLeaderboardNotifications);
    notifyLayout->addWidget(m_enableSoundEffects);
    notifyLayout->addWidget(m_enableOverlays);
    notifyLayout->addLayout(durationLayout);
    notifyGroup->setLayout(notifyLayout);

    // Profile group
    auto profileGroup = new QGroupBox(tr("Profile"), this);
    auto profileLayout = new QVBoxLayout();
    auto profileInfoLayout = new QFormLayout();

    m_loginButton = new QPushButton(tr("Login"), this);
    m_viewProfile = new QPushButton(tr("View Profile"), this);
    m_usernameLabel = new QLabel(tr("Not logged in"), this);
    m_pointsLabel = new QLabel("0", this);
    m_unreadLabel = new QLabel("0", this);

    profileInfoLayout->addRow(tr("Username:"), m_usernameLabel);
    profileInfoLayout->addRow(tr("Points:"), m_pointsLabel);
    profileInfoLayout->addRow(tr("Unread Messages:"), m_unreadLabel);

    auto buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_viewProfile);

    profileLayout->addLayout(profileInfoLayout);
    profileLayout->addLayout(buttonLayout);
    profileGroup->setLayout(profileLayout);

    // Add all groups to main layout
    mainLayout->addWidget(coreGroup);
    mainLayout->addWidget(notifyGroup);
    mainLayout->addWidget(profileGroup);
    mainLayout->addStretch();

    setLayout(mainLayout);
}

void CAchievementSettingsWidget::ConnectSignals()
{
    connect(m_loginButton, &QPushButton::clicked, this, &CAchievementSettingsWidget::OnLoginClicked);
    connect(m_viewProfile, &QPushButton::clicked, this, &CAchievementSettingsWidget::OnViewProfileClicked);
    connect(m_hardcoreMode, &QCheckBox::toggled, this, &CAchievementSettingsWidget::OnHardcoreModeChanged);

    // Connect all settings changes
    connect(m_enableAchievements, &QCheckBox::toggled, this, &CAchievementSettingsWidget::OnSettingChanged);
    connect(m_encoreMode, &QCheckBox::toggled, this, &CAchievementSettingsWidget::OnSettingChanged);
    connect(m_spectatorMode, &QCheckBox::toggled, this, &CAchievementSettingsWidget::OnSettingChanged);
    connect(m_unofficialMode, &QCheckBox::toggled, this, &CAchievementSettingsWidget::OnSettingChanged);
    connect(m_enableNotifications, &QCheckBox::toggled, this, &CAchievementSettingsWidget::OnSettingChanged);
    connect(m_enableLeaderboardNotifications, &QCheckBox::toggled, this, &CAchievementSettingsWidget::OnSettingChanged);
    connect(m_enableSoundEffects, &QCheckBox::toggled, this, &CAchievementSettingsWidget::OnSettingChanged);
    connect(m_enableOverlays, &QCheckBox::toggled, this, &CAchievementSettingsWidget::OnSettingChanged);
    connect(m_notificationDuration, &QSlider::valueChanged, this, &CAchievementSettingsWidget::OnSettingChanged);
    connect(m_leaderboardDuration, &QSlider::valueChanged, this, &CAchievementSettingsWidget::OnSettingChanged);
}

void CAchievementSettingsWidget::LoadSettings()
{
    // TODO: Load settings from AchievementsConfig
}

void CAchievementSettingsWidget::SaveSettings()
{
    // TODO: Save settings to AchievementsConfig
}

void CAchievementSettingsWidget::OnLoginClicked()
{
    CAchievementLoginDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        // TODO: Handle login with AchievementManager
        UpdateLoginState();
        UpdateProfileDisplay();
    }
}

void CAchievementSettingsWidget::OnViewProfileClicked()
{
    // TODO: Open web browser to user's profile
}

void CAchievementSettingsWidget::OnHardcoreModeChanged(bool checked)
{
    if (checked)
    {
        QMessageBox::warning(this, tr("Hardcore Mode"),
            tr("Enabling hardcore mode will disable save states. Are you sure?"),
            QMessageBox::Ok);
    }
    OnSettingChanged();
}

void CAchievementSettingsWidget::OnSettingChanged()
{
    SaveSettings();
}

void CAchievementSettingsWidget::UpdateLoginState()
{
    // TODO: Get login state from AchievementManager
    bool isLoggedIn = false;
    m_isLoggedIn = isLoggedIn;

    m_loginButton->setText(isLoggedIn ? tr("Logout") : tr("Login"));
    m_viewProfile->setEnabled(isLoggedIn);

    // Disable settings if not logged in
    m_hardcoreMode->setEnabled(isLoggedIn);
    m_encoreMode->setEnabled(isLoggedIn);
    m_spectatorMode->setEnabled(isLoggedIn);
    m_unofficialMode->setEnabled(isLoggedIn);
}

void CAchievementSettingsWidget::UpdateProfileDisplay()
{
    // TODO: Get profile info from AchievementManager
    m_usernameLabel->setText(tr("Not logged in"));
    m_pointsLabel->setText("0");
    m_unreadLabel->setText("0");
}