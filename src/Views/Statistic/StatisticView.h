#ifndef STATISTICVIEW_H
#define STATISTICVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QProgressBar>

class MainWindow;

class StatisticView : public QWidget {
    Q_OBJECT
public:
    explicit StatisticView(MainWindow *mainWindow, QWidget *parent = nullptr);
    void refresh();

private:
    MainWindow *mainWindow = nullptr;
    QScrollArea *scrollArea = nullptr;
    QWidget *contentWidget = nullptr;
    QVBoxLayout *contentLayout = nullptr;

    void setupUI();
    QGridLayout *buildKpiGrid(double totalRev, double todayRev, double compRate,
                              int compDone, int compTotalUntilNow, int totalToday,
                              int completedToday, int pendingToday, int pending);
    QHBoxLayout *buildStatusAndDemographicsRow(int completed, int pending, int delayed,
                                              int totalAppts, int male, int female,
                                              int totalPatients, int under18,
                                              int from18to45, int from46to60, int over60);
    QHBoxLayout *buildSpecialtyAndLeaderboardRow(int totalAppts);
    QFrame *buildWeeklyTrendCard(const QDate &curDate, int curDay, int curMonth, int curYear);
};

#endif // STATISTICVIEW_H
