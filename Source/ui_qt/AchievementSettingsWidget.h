#ifndef ACHIEVEMENTSETTINGSWIDGET_H
#define ACHIEVEMENTSETTINGSWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>
#include <QLabel>

namespace Ui
{
    class AchievementSettingsWidget;
}

class CAchievementSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CAchievementSettingsWidget(QWidget* parent = nullptr);
    ~CAchievementSettingsWidget();

    void LoadSettings();
    void SaveSettings();

private slots:
    void OnLoginClicked();
    void OnViewProfileClicked();
    void OnHardcoreModeChanged(bool checked);
    void OnSettingChanged();

private:
    void CreateLayout();
    void ConnectSignals();
    void UpdateLoginState();
    void UpdateProfileDisplay();

    // Core settings
    QCheckBox* m_enableAchievements;
    QCheckBox* m_hardcoreMode;
    QCheckBox* m_encoreMode;
    QCheckBox* m_spectatorMode;
    QCheckBox* m_unofficialMode;

    // Notification settings
    QCheckBox* m_enableNotifications;
    QCheckBox* m_enableLeaderboardNotifications;
    QCheckBox* m_enableSoundEffects;
    QCheckBox* m_enableOverlays;
    QSlider* m_notificationDuration;
    QSlider* m_leaderboardDuration;

    // Profile
    QPushButton* m_loginButton;
    QPushButton* m_viewProfile;
    QLabel* m_usernameLabel;
    QLabel* m_pointsLabel;
    QLabel* m_unreadLabel;

    bool m_isLoggedIn;
};

#endif // ACHIEVEMENTSETTINGSWIDGET_H