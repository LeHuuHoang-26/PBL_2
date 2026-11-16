#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QString>

class QLineEdit;
class QPushButton;
class QLabel;
class QAction;

class Login : public QWidget {
    Q_OBJECT

private:
    // Các phần tử giao diện người dùng
    QWidget *m_cardWidget;
    QLabel *m_lblIllustration;
    QLineEdit *m_txtUsername;
    QLineEdit *m_txtPassword;
    QPushButton *m_btnLogin;
    QLabel *m_lblMessage;
    QAction *m_actionTogglePassword;

    bool m_isPasswordVisible;

    void setupUI();
    void setupConnections();
    void updateIllustrationScale();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login() override = default;

    // Các phương thức cung cấp dữ liệu cho MainWindow / Controller
    QString getUsername() const;
    QString getPassword() const;
    void clearInputs();

    // Các phương thức phản hồi hiển thị
    void showErrorMessage(const QString &message);
    void showSuccessMessage(const QString &message);
    void clearMessage();

signals:
    // Tín hiệu tương tác gửi về cho MainWindow (đầu não)
    void loginClicked();

private slots:
    void onTogglePasswordVisibility();
};

#endif // LOGIN_H
