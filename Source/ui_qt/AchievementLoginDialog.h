#pragma once

#include "uint128.h"

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <memory>

// Forward declarations
class AchievementHTTPDownloader;
class CAchievementsConfig;

class AchievementLoginDialog : public QDialog {
	Q_OBJECT

public:
	explicit AchievementLoginDialog(QWidget* parent = nullptr, bool tokenExpired = false);
	~AchievementLoginDialog();

	private slots:
		void onLoginRequested();
	void onCancelRequested();
	void onInputChanged();
	void processLoginResult(int32 statusCode, const std::string& errorMessage = "");

private:
	void setupUI();
	void connectSignals();
	void updateUI(bool enabled);
	bool validateInput() const;
	void showError(const QString& message);
	void handleSuccessfulLogin(const std::string& token);
	void updateLoginStatus(const QString& message, bool isError = false);

	struct {
		QLineEdit* usernameInput;
		QLineEdit* passwordInput;
		QPushButton* loginButton;
		QPushButton* cancelButton;
		QLabel* statusLabel;
		QLabel* messageLabel;
	} ui{};

	std::unique_ptr<AchievementHTTPDownloader> m_httpDownloader;
	std::unique_ptr<CAchievementsConfig> m_config;
	bool m_isProcessingLogin;
	bool m_tokenExpired;
};