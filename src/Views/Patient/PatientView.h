#ifndef PATIENTVIEW_H
#define PATIENTVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <string>
#include "PatientController.h"

class PatientView : public QWidget {
    Q_OBJECT
private:
    PatientController controller; // Tầng Controller xử lý nghiệp vụ bệnh nhân
    QLineEdit *searchEdit;
    QPushButton *btnSearch;
    QPushButton *btnRefresh;
    QTableWidget *tablePatients;
    QLabel *statusLabel;

public:
    explicit PatientView(QWidget *parent = nullptr);
    void loadData(const std::string &keyword = "");

private slots:
    void onSearchClicked();
    void onRefreshClicked();
};

#endif // PATIENTVIEW_H
