#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QDir>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QCoreApplication::setApplicationName("Phòng khám Đa khoa An Tâm");
    QApplication app(argc, argv);

    // Đảm bảo thư mục làm việc chuẩn nếu chạy từ thư mục bin/
    QDir appDir(QCoreApplication::applicationDirPath());
    if (appDir.dirName().compare("bin", Qt::CaseInsensitive) == 0) {
        appDir.cdUp();
        QDir::setCurrent(appDir.absolutePath());
    }

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

    QFont appFont("Be Vietnam Pro", 10);
    app.setFont(appFont);

    // Khởi tạo cửa sổ chính và thực thi ứng dụng
    MainWindow window;
    if (!window.isInitialized()) {
        return 1;
    }
    window.showMaximized();

    return app.exec();
}
