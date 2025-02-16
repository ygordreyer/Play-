#ifndef ACHIEVEMENTLOGINDIALOG_H
#define ACHIEVEMENTLOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

namespace Ui
{
    class AchievementLoginDialog;
}

class CAchievementLoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CAchievementLoginDialog(QWidget* parent = nullptr);
    ~CAchievementLoginDialog();

    QString GetUsername() const;
    QString GetPassword() const;

private slots:
    void OnLoginClicked();
    void OnCancelClicked();
    void ValidateInput();

private:
    void CreateLayout();
    void ConnectSignals();

    QLineEdit* m_usernameInput;
    QLineEdit* m_passwordInput;
    QPushButton* m_loginButton;
    QPushButton* m_cancelButton;
    QLabel* m_statusLabel;

    enum class LoginState
    {
        Idle,
        Authenticating,
        Success,
        Failed
    };

    LoginState m_loginState;
};

#endif // ACHIEVEMENTLOGINDIALOG_H