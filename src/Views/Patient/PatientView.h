#ifndef PATIENTVIEW_H
#define PATIENTVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QShowEvent>
#include <string>

class MainWindow;

class PatientView : public QWidget {
    Q_OBJECT

public:
    explicit PatientView(MainWindow *mainWindow, QWidget *parent = nullptr);
    void refresh();
    void resetToFirstPage();

protected:
    void showEvent(QShowEvent *event) override;

private:
    MainWindow *mainWindow = nullptr;
    QLineEdit *search = nullptr;
    QPushButton *addBtn = nullptr;
    QTableWidget *table = nullptr;

    QLabel *totalLabel = nullptr;
    QPushButton *prevBtn = nullptr;
    QPushButton *nextBtn = nullptr;
    QLabel *pageStatus = nullptr;
    int currentPage = 0;
    int sortColumn = -1;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    static const int PAGE_SIZE = 50;

    void setupUI();
    void addPatientDialog();
    void editPatientDialog(const std::string &id);
};

#endif // PATIENTVIEW_H
