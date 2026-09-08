#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("PBL_2");
    window.resize(600, 400);

    QVBoxLayout *layout = new QVBoxLayout(&window);
    QLabel *label = new QLabel("PBL_2 build OK!", &window);
    layout->addWidget(label);

    window.show();

    return app.exec();
}