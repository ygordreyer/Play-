#pragma once

#include "uint128.h"

#include <QWidget>
#include <memory>

class QCheckBox;
class QSlider;
class QPushButton;
class QLabel;
class QGroupBox;
class CAchievementsConfig;
class CAchievementSystem;

class AchievementSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AchievementSettingsWidget(QWidget* parent = nullptr);
    ~AchievementSettingsWidget();

private slots:
    void onLoginStateChanged();
    void onSettingChanged();
    void onHardcoreModeToggled(bool enabled);
    void onLoginButtonClicked();
    void onViewProfileClicked();
    void onNotificationDurationChanged(int value);
    void onLeaderboardDurationChanged(int value);
    void onAchievementUnlocked(const std::string& id, const std::string& title);
    void onGameInfoUpdated(uint32 gameId, const QString& gameInfo);

private:
    void setupUI();
    void connectSignals();
    void loadSettings();
    void saveSettings();
    void updateUIState();
    void createTooltips();

    // UI Groups
    QGroupBox* createCoreSettingsGroup();
    QGroupBox* createNotificationSettingsGroup();
    QGroupBox* createProfileGroup();
    QGroupBox* createGameInfoGroup();

    struct {
        // Core Settings
        QCheckBox* enableAchievements;
        QCheckBox* hardcoreMode;
        QCheckBox* encoreMode;
        QCheckBox* spectatorMode;
        QCheckBox* unofficialMode;

        // Notification Settings
        QCheckBox* showNotifications;
        QCheckBox* showLeaderboardNotifications;
        QCheckBox* enableSoundEffects;
        QCheckBox* showOverlays;
        QSlider* notificationDuration;
        QSlider* leaderboardDuration;
        QLabel* notificationDurationLabel;
        QLabel* leaderboardDurationLabel;

        // Profile
        QPushButton* loginButton;
        QPushButton* viewProfileButton;
        QLabel* usernameLabel;
        QLabel* pointsLabel;
        QLabel* lastLoginLabel;

        // Game Info
        QLabel* gameTitle;
        QLabel* achievementCount;
        QLabel* completionStatus;
        QLabel* pointsEarned;
    } ui;

    std::unique_ptr<CAchievementsConfig> m_config;
    CAchievementSystem* m_achievementSystem;
    bool m_blockSettingsUpdate;
};