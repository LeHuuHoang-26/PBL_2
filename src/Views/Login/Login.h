#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QString>
#include <QPixmap>

class QLineEdit;
class QPushButton;
class QLabel;
class QAction;

class Login : public QWidget {
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login() override = default;

    QString getUsername() const;
    QString getPassword() const;
    void clearInputs();
    void showErrorMessage(const QString &message);
    void clearMessage();

signals:
    void loginClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onTogglePasswordVisibility();

private:
    QWidget *m_cardWidget;
    QLabel *m_lblIllustration;
    QLineEdit *m_txtUsername;
    QLineEdit *m_txtPassword;
    QPushButton *m_btnLogin;
    QLabel *m_lblMessage;
    QAction *m_actionTogglePassword;
    QPixmap m_originalIllustration;
    QSize m_lastIllustrationPhysSize;
    bool m_isPasswordVisible;

    void setupUI();
    void setupConnections();
    void updateIllustrationScale();
};

#endif
