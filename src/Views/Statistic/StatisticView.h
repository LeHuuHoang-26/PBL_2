#ifndef STATISTICVIEW_H
#define STATISTICVIEW_H

#include <QWidget>
#include <QDate>

class MainWindow;
class QStackedWidget;
class QPushButton;
class QLabel;

class StatisticView : public QWidget {
    Q_OBJECT
public:
    explicit StatisticView(MainWindow *mainWindow, QWidget *parent = nullptr);
    void refresh();

protected:
    void showEvent(QShowEvent *event) override;

private:
    MainWindow *mainWindow = nullptr;

    QPushButton *btnTab1 = nullptr;
    QPushButton *btnTab2 = nullptr;
    QPushButton *btnPeriods[4] = {nullptr};
    QPushButton *btnStartDate = nullptr;
    QPushButton *btnEndDate = nullptr;

    QStackedWidget *tabStack = nullptr;
    QWidget *pageTab1 = nullptr;
    QWidget *pageTab2 = nullptr;

    QDate filterStartDate;
    QDate filterEndDate;
    int currentPeriodIndex = 0;

    void setupUI();
    void updateTabStyles(int activeIndex);
    void updatePeriodButtonStyles(int selectedIndex);
    void updateDateButtonStyles();
    void openPeriodSelectionDialog(int periodTypeIndex);
    void openDateDialog(bool isStartDate);
    void refreshTab1();
    void refreshTab2();
};

#endif
