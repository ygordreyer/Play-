#include "AchievementSettingsWidget.h"
#include "AchievementLoginDialog.h"
#include "AchievementsConfig.h"
#include "AchievementSystem.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

namespace {
    constexpr int MIN_DURATION = 1;
    constexpr int MAX_DURATION = 10;
}

AchievementSettingsWidget::AchievementSettingsWidget(QWidget* parent)
    : QWidget(parent)
    , m_blockSettingsUpdate(false)
{
    m_config = CAchievementsConfig::LoadConfig();
    m_achievementSystem = CAchievementSystem::GetInstance();

    setupUI();
    connectSignals();
    createTooltips();
    loadSettings();
    updateUIState();
}

AchievementSettingsWidget::~AchievementSettingsWidget() = default;

void AchievementSettingsWidget::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Add all major groups
    mainLayout->addWidget(createCoreSettingsGroup());
    mainLayout->addWidget(createNotificationSettingsGroup());
    mainLayout->addWidget(createProfileGroup());
    mainLayout->addWidget(createGameInfoGroup());
    mainLayout->addStretch();

    setLayout(mainLayout);
}

QGroupBox* AchievementSettingsWidget::createCoreSettingsGroup()
{
    auto group = new QGroupBox(tr("Core Settings"), this);
    auto layout = new QVBoxLayout();

    ui.enableAchievements = new QCheckBox(tr("Enable Achievements"), this);
    ui.hardcoreMode = new QCheckBox(tr("Hardcore Mode"), this);
    ui.encoreMode = new QCheckBox(tr("Encore Mode"), this);
    ui.spectatorMode = new QCheckBox(tr("Spectator Mode"), this);
    ui.unofficialMode = new QCheckBox(tr("Unofficial Achievements"), this);

    layout->addWidget(ui.enableAchievements);
    layout->addWidget(ui.hardcoreMode);
    layout->addWidget(ui.encoreMode);
    layout->addWidget(ui.spectatorMode);
    layout->addWidget(ui.unofficialMode);

    group->setLayout(layout);
    return group;
}

QGroupBox* AchievementSettingsWidget::createNotificationSettingsGroup()
{
    auto group = new QGroupBox(tr("Notification Settings"), this);
    auto layout = new QVBoxLayout();

    // Checkboxes
    ui.showNotifications = new QCheckBox(tr("Show Achievement Notifications"), this);
    ui.showLeaderboardNotifications = new QCheckBox(tr("Show Leaderboard Notifications"), this);
    ui.enableSoundEffects = new QCheckBox(tr("Enable Sound Effects"), this);
    ui.showOverlays = new QCheckBox(tr("Show Achievement Overlays"), this);

    layout->addWidget(ui.showNotifications);
    layout->addWidget(ui.showLeaderboardNotifications);
    layout->addWidget(ui.enableSoundEffects);
    layout->addWidget(ui.showOverlays);

    // Duration sliders
    auto durationLayout = new QFormLayout();

    ui.notificationDuration = new QSlider(Qt::Horizontal, this);
    ui.notificationDuration->setRange(MIN_DURATION, MAX_DURATION);
    ui.notificationDurationLabel = new QLabel(this);

    ui.leaderboardDuration = new QSlider(Qt::Horizontal, this);
    ui.leaderboardDuration->setRange(MIN_DURATION, MAX_DURATION);
    ui.leaderboardDurationLabel = new QLabel(this);

    durationLayout->addRow(tr("Achievement Notification Duration:"), ui.notificationDuration);
    durationLayout->addRow(ui.notificationDurationLabel);
    durationLayout->addRow(tr("Leaderboard Notification Duration:"), ui.leaderboardDuration);
    durationLayout->addRow(ui.leaderboardDurationLabel);

    layout->addLayout(durationLayout);
    group->setLayout(layout);
    return group;
}

QGroupBox* AchievementSettingsWidget::createProfileGroup()
{
    auto group = new QGroupBox(tr("Profile"), this);
    auto layout = new QVBoxLayout();

    // Profile info
    auto infoLayout = new QFormLayout();
    ui.usernameLabel = new QLabel(tr("Not logged in"), this);
    ui.pointsLabel = new QLabel("0", this);
    ui.lastLoginLabel = new QLabel(this);

    infoLayout->addRow(tr("Username:"), ui.usernameLabel);
    infoLayout->addRow(tr("Total Points:"), ui.pointsLabel);
    infoLayout->addRow(tr("Last Login:"), ui.lastLoginLabel);

    // Buttons
    auto buttonLayout = new QHBoxLayout();
    ui.loginButton = new QPushButton(tr("Login"), this);
    ui.viewProfileButton = new QPushButton(tr("View Profile"), this);
    ui.viewProfileButton->setEnabled(false);

    buttonLayout->addWidget(ui.loginButton);
    buttonLayout->addWidget(ui.viewProfileButton);

    layout->addLayout(infoLayout);
    layout->addLayout(buttonLayout);
    group->setLayout(layout);
    return group;
}

QGroupBox* AchievementSettingsWidget::createGameInfoGroup()
{
    auto group = new QGroupBox(tr("Current Game"), this);
    auto layout = new QFormLayout();

    ui.gameTitle = new QLabel(tr("No game loaded"), this);
    ui.achievementCount = new QLabel("-", this);
    ui.completionStatus = new QLabel("-", this);
    ui.pointsEarned = new QLabel("-", this);

    layout->addRow(tr("Title:"), ui.gameTitle);
    layout->addRow(tr("Achievements:"), ui.achievementCount);
    layout->addRow(tr("Completion:"), ui.completionStatus);
    layout->addRow(tr("Points Earned:"), ui.pointsEarned);

    group->setLayout(layout);
    return group;
}

void AchievementSettingsWidget::connectSignals()
{
    // Core settings
    connect(ui.enableAchievements, &QCheckBox::toggled, this, &AchievementSettingsWidget::onSettingChanged);
    connect(ui.hardcoreMode, &QCheckBox::toggled, this, &AchievementSettingsWidget::onHardcoreModeToggled);
    connect(ui.encoreMode, &QCheckBox::toggled, this, &AchievementSettingsWidget::onSettingChanged);
    connect(ui.spectatorMode, &QCheckBox::toggled, this, &AchievementSettingsWidget::onSettingChanged);
    connect(ui.unofficialMode, &QCheckBox::toggled, this, &AchievementSettingsWidget::onSettingChanged);

    // Notification settings
    connect(ui.showNotifications, &QCheckBox::toggled, this, &AchievementSettingsWidget::onSettingChanged);
    connect(ui.showLeaderboardNotifications, &QCheckBox::toggled, this, &AchievementSettingsWidget::onSettingChanged);
    connect(ui.enableSoundEffects, &QCheckBox::toggled, this, &AchievementSettingsWidget::onSettingChanged);
    connect(ui.showOverlays, &QCheckBox::toggled, this, &AchievementSettingsWidget::onSettingChanged);

    connect(ui.notificationDuration, &QSlider::valueChanged, this, &AchievementSettingsWidget::onNotificationDurationChanged);
    connect(ui.leaderboardDuration, &QSlider::valueChanged, this, &AchievementSettingsWidget::onLeaderboardDurationChanged);

    // Profile buttons
    connect(ui.loginButton, &QPushButton::clicked, this, &AchievementSettingsWidget::onLoginButtonClicked);
    connect(ui.viewProfileButton, &QPushButton::clicked, this, &AchievementSettingsWidget::onViewProfileClicked);

    // Achievement system connections
    if (m_achievementSystem) {
        m_achievementSystem->RegisterUnlockHandler(
            [this](const std::string& id, const std::string& title) {
                QMetaObject::invokeMethod(this, "onAchievementUnlocked",
                    Qt::QueuedConnection, Q_ARG(std::string, id), Q_ARG(std::string, title));
            }
        );
    }
}

void AchievementSettingsWidget::loadSettings()
{
    m_blockSettingsUpdate = true;

    const auto& settings = m_config->GetSettings();

    // Core settings
    ui.enableAchievements->setChecked(settings.enabled);
    ui.hardcoreMode->setChecked(settings.hardcoreMode);
    ui.encoreMode->setChecked(settings.encoreMode);
    ui.spectatorMode->setChecked(settings.spectatorMode);
    ui.unofficialMode->setChecked(settings.unofficialTestMode);

    // Notification settings
    ui.showNotifications->setChecked(settings.notifications);
    ui.showLeaderboardNotifications->setChecked(settings.leaderboardNotifications);
    ui.enableSoundEffects->setChecked(settings.soundEffects);
    ui.showOverlays->setChecked(settings.overlays);

    ui.notificationDuration->setValue(settings.notificationsDuration / 1000);  // Convert ms to seconds
    ui.leaderboardDuration->setValue(settings.leaderboardsDuration / 1000);

    // Update auth info
    const auto& auth = m_config->GetAuth();
    if (!auth.username.empty()) {
        ui.usernameLabel->setText(QString::fromStdString(auth.username));
        ui.pointsLabel->setText(QString::number(auth.points));
        ui.loginButton->setText(tr("Logout"));
        ui.viewProfileButton->setEnabled(true);
    }

    m_blockSettingsUpdate = false;

    // Update duration labels
    onNotificationDurationChanged(ui.notificationDuration->value());
    onLeaderboardDurationChanged(ui.leaderboardDuration->value());
}

void AchievementSettingsWidget::saveSettings()
{
    if (m_blockSettingsUpdate)
        return;

    CAchievementsConfig::Settings settings;

    // Core settings
    settings.enabled = ui.enableAchievements->isChecked();
    settings.hardcoreMode = ui.hardcoreMode->isChecked();
    settings.encoreMode = ui.encoreMode->isChecked();
    settings.spectatorMode = ui.spectatorMode->isChecked();
    settings.unofficialTestMode = ui.unofficialMode->isChecked();

    // Notification settings
    settings.notifications = ui.showNotifications->isChecked();
    settings.leaderboardNotifications = ui.showLeaderboardNotifications->isChecked();
    settings.soundEffects = ui.enableSoundEffects->isChecked();
    settings.overlays = ui.showOverlays->isChecked();

    settings.notificationsDuration = ui.notificationDuration->value() * 1000;  // Convert to ms
    settings.leaderboardsDuration = ui.leaderboardDuration->value() * 1000;

    m_config->SetSettings(settings);
    m_config->Save();

    // Update achievement system
    if (m_achievementSystem) {
        m_achievementSystem->SetHardcoreMode(settings.hardcoreMode);
    }
}

void AchievementSettingsWidget::onLoginButtonClicked()
{
    if (!m_config->GetAuth().username.empty()) {
        // Currently logged in, so log out
        m_config->SetAuth(CAchievementsConfig::Auth());
        m_config->Save();

        ui.usernameLabel->setText(tr("Not logged in"));
        ui.pointsLabel->setText("0");
        ui.lastLoginLabel->clear();
        ui.loginButton->setText(tr("Login"));
        ui.viewProfileButton->setEnabled(false);

        // Disable hardcore mode when logging out
        if (ui.hardcoreMode->isChecked()) {
            ui.hardcoreMode->setChecked(false);
        }

        return;
    }

    // Show login dialog
    AchievementLoginDialog dialog(this, false);
    if (dialog.exec() == QDialog::Accepted) {
        loadSettings();
        updateUIState();
    }
}

void AchievementSettingsWidget::onViewProfileClicked()
{
    const auto& auth = m_config->GetAuth();
    if (auth.username.empty())
        return;

    QUrl profileUrl(QString("https://retroachievements.org/user/%1")
        .arg(QString::fromStdString(auth.username)));
    QDesktopServices::openUrl(profileUrl);
}

void AchievementSettingsWidget::onHardcoreModeToggled(bool enabled)
{
    if (!m_blockSettingsUpdate && enabled) {
        const QString warningText = tr("Enabling hardcore mode will disable save states and ensure competitive integrity.\n\n"
                                     "Your current game progress will be reset. Do you want to continue?");

        if (QMessageBox::warning(this, tr("Enable Hardcore Mode"),
                               warningText,
                               QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            m_blockSettingsUpdate = true;
            ui.hardcoreMode->setChecked(false);
            m_blockSettingsUpdate = false;
            return;
        }
    }

    onSettingChanged();
}

void AchievementSettingsWidget::onNotificationDurationChanged(int value)
{
    ui.notificationDurationLabel->setText(tr("%n second(s)", "", value));
    onSettingChanged();
}

void AchievementSettingsWidget::onLeaderboardDurationChanged(int value)
{
    ui.leaderboardDurationLabel->setText(tr("%n second(s)", "", value));
    onSettingChanged();
}

void AchievementSettingsWidget::onSettingChanged()
{
    saveSettings();
    updateUIState();
}

void AchievementSettingsWidget::updateUIState()
{
    const bool achievementsEnabled = ui.enableAchievements->isChecked();
    const bool loggedIn = !m_config->GetAuth().username.empty();

    // Core settings
    ui.hardcoreMode->setEnabled(achievementsEnabled && loggedIn);
    ui.encoreMode->setEnabled(achievementsEnabled && loggedIn);
    ui.spectatorMode->setEnabled(achievementsEnabled && loggedIn);
    ui.unofficialMode->setEnabled(achievementsEnabled && loggedIn);

    // Notification settings
    const bool notificationsEnabled = achievementsEnabled && ui.showNotifications->isChecked();
    const bool leaderboardNotificationsEnabled = achievementsEnabled && ui.showLeaderboardNotifications->isChecked();

    ui.showNotifications->setEnabled(achievementsEnabled);
    ui.showLeaderboardNotifications->setEnabled(achievementsEnabled);
    ui.enableSoundEffects->setEnabled(achievementsEnabled);
    ui.showOverlays->setEnabled(achievementsEnabled);

    ui.notificationDuration->setEnabled(notificationsEnabled);
    ui.notificationDurationLabel->setEnabled(notificationsEnabled);
    ui.leaderboardDuration->setEnabled(leaderboardNotificationsEnabled);
    ui.leaderboardDurationLabel->setEnabled(leaderboardNotificationsEnabled);
}

void AchievementSettingsWidget::createTooltips()
{
    ui.enableAchievements->setToolTip(tr("Enable achievement tracking and online features"));
    ui.hardcoreMode->setToolTip(tr("Disable save states and ensure competitive integrity"));
    ui.encoreMode->setToolTip(tr("Replay achievements as if they were never unlocked"));
    ui.spectatorMode->setToolTip(tr("View achievements without unlocking them"));
    ui.unofficialMode->setToolTip(tr("Enable unofficial achievement sets"));

    ui.showNotifications->setToolTip(tr("Display pop-up notifications when achievements are unlocked"));
    ui.showLeaderboardNotifications->setToolTip(tr("Display notifications for leaderboard submissions"));
    ui.enableSoundEffects->setToolTip(tr("Play sound effects for achievements and notifications"));
    ui.showOverlays->setToolTip(tr("Show achievement progress overlays during gameplay"));
}

void AchievementSettingsWidget::onAchievementUnlocked(const std::string& id, const std::string& title)
{
    // Update achievement count and points if needed
    auto achievements = m_achievementSystem->GetAchievements();
    int unlockedCount = 0;
    int totalPoints = 0;
    int earnedPoints = 0;

    for (const auto& achievement : achievements) {
        totalPoints += achievement.points;
        if (achievement.unlocked) {
            unlockedCount++;
            earnedPoints += achievement.points;
        }
    }

    ui.achievementCount->setText(tr("%1/%2").arg(unlockedCount).arg(achievements.size()));
    ui.pointsEarned->setText(tr("%1/%2").arg(earnedPoints).arg(totalPoints));

    // Calculate and update completion percentage
    if (!achievements.empty()) {
        float completion = (unlockedCount * 100.0f) / achievements.size();
        ui.completionStatus->setText(tr("%1%").arg(completion, 0, 'f', 1));
    }
}

void AchievementSettingsWidget::onGameInfoUpdated(uint32 gameId, const QString& gameInfo)
{
    if (gameId == 0) {
        ui.gameTitle->setText(tr("No game loaded"));
        ui.achievementCount->setText("-");
        ui.completionStatus->setText("-");
        ui.pointsEarned->setText("-");
    } else {
        ui.gameTitle->setText(gameInfo);
        onAchievementUnlocked("", ""); // Update achievement stats
    }
}

void AchievementSettingsWidget::onLoginStateChanged()
{
    const auto& auth = m_config->GetAuth();
    const bool loggedIn = !auth.username.empty();

    if (loggedIn) {
        ui.usernameLabel->setText(QString::fromStdString(auth.username));
        ui.pointsLabel->setText(QString::number(auth.points));
        ui.loginButton->setText(tr("Logout"));
        ui.viewProfileButton->setEnabled(true);
    } else {
        ui.usernameLabel->setText(tr("Not logged in"));
        ui.pointsLabel->setText("0");
        ui.lastLoginLabel->clear();
        ui.loginButton->setText(tr("Login"));
        ui.viewProfileButton->setEnabled(false);
    }

    updateUIState();
}