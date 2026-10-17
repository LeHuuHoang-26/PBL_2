#ifndef DOCTORVIEW_H
#define DOCTORVIEW_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <string>
#include "DoctorController.h"

class DoctorView : public QWidget {
    Q_OBJECT
private:
    DoctorController controller; // Tầng Controller xử lý nghiệp vụ
    QLineEdit *searchEdit;
    QPushButton *btnSearch;
    QPushButton *btnRefresh;
    QTableWidget *tableDoctors;
    QLabel *statusLabel;

public:
    explicit DoctorView(QWidget *parent = nullptr);
    void loadData(const std::string &keyword = "");

private slots:
    void onSearchClicked();
    void onRefreshClicked();
};

#endif // DOCTORVIEW_H
