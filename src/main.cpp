#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QDir>
#include <QFileInfo>
#include "Views/MainWindow.h"

int main(int argc, char *argv[]) {
    // Đảm bảo thư mục làm việc chuẩn nếu chạy từ bin/
    QCoreApplication::setApplicationName("Phòng khám Đa khoa An Tâm");
    QString appDir = QFileInfo(QString::fromLocal8Bit(argv[0])).dir().absolutePath();
    if (appDir.endsWith("/bin") || appDir.endsWith("\\bin")) {
        QDir::setCurrent(appDir + "/..");
    }

    QApplication app(argc, argv);

    // Cài đặt icon ứng dụng
    QIcon appIcon(":/assets/textures/icon_app.ico");
    if (appIcon.isNull()) appIcon = QIcon("assets/textures/icon_app.ico");
    app.setWindowIcon(appIcon);

    // Tải bộ font ứng dụng từ Resource
    if (QFontDatabase::addApplicationFont(":/assets/fonts/Be_Vietnam_Pro/BeVietnamPro-Regular.ttf") < 0)
        QFontDatabase::addApplicationFont("assets/fonts/Be_Vietnam_Pro/BeVietnamPro-Regular.ttf");
    if (QFontDatabase::addApplicationFont(":/assets/fonts/Be_Vietnam_Pro/BeVietnamPro-Bold.ttf") < 0)
        QFontDatabase::addApplicationFont("assets/fonts/Be_Vietnam_Pro/BeVietnamPro-Bold.ttf");
    if (QFontDatabase::addApplicationFont(":/assets/fonts/Be_Vietnam_Pro/BeVietnamPro-SemiBold.ttf") < 0)
        QFontDatabase::addApplicationFont("assets/fonts/Be_Vietnam_Pro/BeVietnamPro-SemiBold.ttf");
    if (QFontDatabase::addApplicationFont(":/assets/fonts/PlayFair_Display/PlayfairDisplay-Regular.ttf") < 0)
        QFontDatabase::addApplicationFont("assets/fonts/PlayFair_Display/PlayfairDisplay-Regular.ttf");
    if (QFontDatabase::addApplicationFont(":/assets/fonts/PlayFair_Display/PlayfairDisplay-Bold.ttf") < 0)
        QFontDatabase::addApplicationFont("assets/fonts/PlayFair_Display/PlayfairDisplay-Bold.ttf");

    QFont appFont("Be Vietnam Pro", 10);
    app.setFont(appFont);

    // Khởi tạo cửa sổ chính và thực thi ứng dụng
    MainWindow window;
    window.showMaximized();

    return app.exec();
}
