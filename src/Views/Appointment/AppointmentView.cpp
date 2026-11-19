#include "AppointmentView.h"
#include "libPBL2.h"
#include "MainWindow.h"
#include "StatisticController.h"
#include "UIHelper.h"
#include "../Utils/AppExceptions.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QCalendarWidget>
#include <QPainter>
#include <QTextCharFormat>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QAbstractButton>
#include <QLocale>
#include <QTime>
#include <QListView>
#include <QTimer>
#include <QScrollBar>
#include <QDateTime>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QMap>
#include <QStandardItemModel>
#include <QBoxLayout>
#include <QProxyStyle>
#include <QEvent>
#include <QShowEvent>

namespace {
class AppointmentTabShowFilter : public QObject {
public:
    std::function<void()> onShow;
    explicit AppointmentTabShowFilter(std::function<void()> cb, QObject *parent = nullptr)
        : QObject(parent), onShow(std::move(cb)) {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Show && !event->spontaneous()) {
            if (onShow) onShow();
        }
        return QObject::eventFilter(obj, event);
    }
};

QString getAppointmentStatusText(const Appointment &a) {
    if (a.getState() == -2) return QStringLiteral("Đã dời lịch");
    if (a.getState() == 1) return QStringLiteral("Đã khám");
    if (a.getState() == -1) return QStringLiteral("Trễ hẹn");
    Date now = Date::now();
    int st = a.computeEffectiveState(now.getDay(), now.getMonth(), now.getYear(), now.getHour(), now.getMinute());
    if (st == -1) return QStringLiteral("Trễ hẹn");
    return QStringLiteral("Chưa khám");
}
}

AppointmentView::AppointmentView(MainWindow *mainWindow, QObject *parent)
    : QObject(parent), mainWindow(mainWindow) {
}

QList<Appointment*> AppointmentView::getAppointmentsForDate(const QDate &targetDate) const {
    if (!mainWindow || !mainWindow->getAppointments()) return QList<Appointment*>();
    return AppointmentController::getAppointmentsForDate(mainWindow->getAppointments(), targetDate);
}

void AppointmentView::highlightCalendarAppointments(QCalendarWidget *cal, int year, int month) {
    if (!cal || !mainWindow || !mainWindow->getAppointments() || year < 1 || month < 1 || month > 12) return;
    int dayCount[32] = {0};
    AppointmentController::getMonthDayCounts(mainWindow->getAppointments(), year, month, dayCount);

    QTextCharFormat weekdayHeaderFmt;
    weekdayHeaderFmt.setForeground(QColor("#475569"));
    weekdayHeaderFmt.setFontWeight(QFont::DemiBold);
    for (int dow = 1; dow <= 7; ++dow) {
        cal->setWeekdayTextFormat(static_cast<Qt::DayOfWeek>(dow), weekdayHeaderFmt);
    }

    const auto oldDates = cal->dateTextFormat().keys();
    for (const QDate &oldD : oldDates) {
        cal->setDateTextFormat(oldD, QTextCharFormat());
    }

    QDate firstDay(year, month, 1);
    QDate gridStart = firstDay.addDays(-14);
    QDate gridEnd = firstDay.addDays(45);

    for (QDate d = gridStart; d <= gridEnd; d = d.addDays(1)) {
        QTextCharFormat fmt;
        if (d.month() != month) {

            fmt.setBackground(QColor("#e2e8f0"));
            fmt.setForeground(QColor("#64748b"));
            fmt.setFontWeight(QFont::Normal);
        } else {

            bool isHol = mainWindow->isHoliday(d);
            if (isHol) {

                fmt.setBackground(QColor("#fee2e2"));
                fmt.setForeground(QColor("#b91c1c"));
                fmt.setFontWeight(QFont::Bold);
                fmt.setToolTip(mainWindow->holidayNote(d));
            } else if (dayCount[d.day()] > 0) {
                fmt.setBackground(QColor("#ccfbf1"));
                fmt.setForeground(QColor("#0f766e"));
                fmt.setFontWeight(QFont::Bold);
            } else {
                fmt.setForeground(QColor("#0f172a"));
                fmt.setFontWeight(QFont::Normal);
                fmt.clearBackground();
            }
        }
        cal->setDateTextFormat(d, fmt);
    }
}

namespace {

class DialogCalendarWidget : public QCalendarWidget {
private:
    MainWindow *mainWindow = nullptr;
    QDate customSelectedDate;

public:
    explicit DialogCalendarWidget(MainWindow *mw, QWidget *parent = nullptr)
        : QCalendarWidget(parent), mainWindow(mw), customSelectedDate(QDate()) {
        UIHelper::applyCalendarStyle(this);
        setDateRange(QDate(2000, 1, 1), QDate(2099, 12, 31));
        setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
        setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
        setGridVisible(false);
        refreshHolidays();
    }

    void setCustomSelectedDate(const QDate &d) {
        customSelectedDate = d;
        update();
    }

    void clearCustomSelection() {
        customSelectedDate = QDate();
        update();
    }

    void refreshHolidays() {
        if (!mainWindow) return;
        int m = monthShown();
        int y = yearShown();
        if (y < 1 || m < 1 || m > 12) return;

        QDate firstDay(y, m, 1);
        QDate start = firstDay.addDays(-14);
        QDate end = firstDay.addDays(45);

        for (QDate d = start; d <= end; d = d.addDays(1)) {
            if (mainWindow->isHoliday(d)) {
                QTextCharFormat fmt;
                fmt.setToolTip(mainWindow->holidayNote(d));
                setDateTextFormat(d, fmt);
            }
        }
    }

protected:
    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        painter->fillRect(rect, Qt::white);

        QRect cellRect = rect.adjusted(1, 1, -1, -1);
        QDate today = QDate::currentDate();
        bool isSel = (customSelectedDate.isValid() && date == customSelectedDate);

        QColor bgColor = Qt::transparent;
        QColor textColor = QColor("#0f172a");

        if (isSel) {
            bgColor = QColor("#0d9488");
            textColor = QColor("#ffffff");
        } else if (date < today) {
            bgColor = QColor("#fef08a");
            textColor = QColor("#713f12");
        } else if (date.month() != monthShown()) {
            bgColor = QColor("#e2e8f0");
            textColor = QColor("#64748b");
        } else if (mainWindow && mainWindow->isHoliday(date)) {
            bgColor = QColor("#fee2e2");
            textColor = QColor("#b91c1c");
        }

        if (bgColor != Qt::transparent) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);
            painter->drawRoundedRect(cellRect, 5, 5);
        }

        QFont f = font();
        f.setWeight(QFont::Normal);
        f.setBold(false);
        painter->setFont(f);
        painter->setPen(textColor);
        painter->drawText(cellRect, Qt::AlignCenter, QString::number(date.day()));

        painter->restore();
    }
};

class AppointmentListFilterCalendar : public QCalendarWidget {
private:
    MainWindow *mainWindow = nullptr;
    int dayCounts[32] = {0};

public:
    explicit AppointmentListFilterCalendar(MainWindow *mw, QWidget *parent = nullptr)
        : QCalendarWidget(parent), mainWindow(mw) {
        UIHelper::applyCalendarStyle(this);
        setDateRange(QDate(2000, 1, 1), QDate(2099, 12, 31));
        setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
        setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
        setGridVisible(false);

        QTextCharFormat weekdayFmt;
        weekdayFmt.setForeground(QColor("#475569"));
        weekdayFmt.setFontWeight(QFont::DemiBold);
        weekdayFmt.setFontPointSize(9.0);
        for (int dow = 1; dow <= 7; ++dow) {
            setWeekdayTextFormat(static_cast<Qt::DayOfWeek>(dow), weekdayFmt);
        }

        if (auto *view = findChild<QTableView*>("qt_calendar_calendarview")) {
            if (auto *hh = view->horizontalHeader()) {
                hh->setVisible(false);
            }
            if (auto *vh = view->verticalHeader()) {
                vh->setVisible(false);
            }
        }

        setStyleSheet(styleSheet() + R"(
            QCalendarWidget {
                background-color: #ffffff;
                border: 1.5px solid #cbd5e1;
                border-radius: 8px;
                font-family: 'Segoe UI', -apple-system, sans-serif;
                font-size: 11px;
            }
            QCalendarWidget QWidget#qt_calendar_navigationbar {
                background-color: #f8fafc;
                border-top-left-radius: 7px;
                border-top-right-radius: 7px;
                border-bottom: 1px solid #e2e8f0;
                min-height: 30px;
                max-height: 30px;
            }
            QCalendarWidget QToolButton#qt_calendar_prevmonth,
            QCalendarWidget QToolButton#qt_calendar_nextmonth {
                font-size: 16px;
                font-weight: bold;
                color: #0d9488;
                min-width: 24px;
                max-width: 24px;
                min-height: 22px;
                max-height: 22px;
                border-radius: 4px;
            }
            QCalendarWidget QToolButton#qt_calendar_prevmonth:hover,
            QCalendarWidget QToolButton#qt_calendar_nextmonth:hover {
                background-color: #e6fffa;
                color: #0f766e;
            }
            QCalendarWidget QToolButton#qt_calendar_monthbutton,
            QCalendarWidget QToolButton#qt_calendar_yearbutton {
                color: #0f172a;
                font-weight: 700;
                font-size: 12px;
                padding: 0px 3px;
                background: transparent;
                border: none;
            }
            QCalendarWidget QTableView {
                selection-background-color: transparent;
                border-bottom-left-radius: 7px;
                border-bottom-right-radius: 7px;
            }
            QCalendarWidget QTableView::item:selected {
                background-color: transparent;
            }
        )");

        refreshData();

        connect(this, &QCalendarWidget::currentPageChanged, this, [this](int, int) {
            refreshData();
            if (auto *v = findChild<QTableView*>("qt_calendar_calendarview")) {
                v->viewport()->update();
            }
        });
    }

    void refreshData() {
        for (int i = 0; i < 32; ++i) dayCounts[i] = 0;
        if (!mainWindow) return;
        int m = monthShown();
        int y = yearShown();
        if (y < 1 || m < 1 || m > 12) return;

        if (mainWindow->getAppointments()) {
            AppointmentController::getMonthDayCounts(mainWindow->getAppointments(), y, m, dayCounts);
        }

        QDate firstDay(y, m, 1);
        QDate start = firstDay.addDays(-14);
        QDate end = firstDay.addDays(45);

        for (QDate d = start; d <= end; d = d.addDays(1)) {
            QTextCharFormat fmt;
            if (mainWindow->isHoliday(d)) {
                fmt.setToolTip(mainWindow->holidayNote(d));
                setDateTextFormat(d, fmt);
            } else if (d.month() == m && d.day() >= 1 && d.day() <= 31 && dayCounts[d.day()] > 0) {
                fmt.setToolTip(QStringLiteral("%1 lịch hẹn khám").arg(dayCounts[d.day()]));
                setDateTextFormat(d, fmt);
            } else {
                setDateTextFormat(d, QTextCharFormat());
            }
        }
    }

protected:
    void showEvent(QShowEvent *event) override {
        QCalendarWidget::showEvent(event);
        refreshData();
        if (auto *v = findChild<QTableView*>("qt_calendar_calendarview")) {
            v->viewport()->update();
        }
    }

    void paintCell(QPainter *painter, const QRect &rect, QDate date) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        painter->fillRect(rect, Qt::white);

        QRect cellRect = rect.adjusted(1, 1, -1, -1);

        QColor bgColor = Qt::transparent;
        QColor textColor = QColor("#0f172a");

        if (date.month() != monthShown()) {

            bgColor = QColor("#e2e8f0");
            textColor = QColor("#64748b");
        } else if (mainWindow && mainWindow->isHoliday(date)) {

            bgColor = QColor("#fee2e2");
            textColor = QColor("#b91c1c");
        } else if (date.day() >= 1 && date.day() <= 31 && dayCounts[date.day()] > 0) {

            bgColor = QColor("#dcfce7");
            textColor = QColor("#166534");
        } else {

            bgColor = Qt::transparent;
            textColor = QColor("#0f172a");
        }

        if (bgColor != Qt::transparent) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);
            painter->drawRoundedRect(cellRect, 5, 5);
        }

        QFont f = font();
        if (mainWindow && mainWindow->isHoliday(date) && date.month() == monthShown()) {
            f.setWeight(QFont::DemiBold);
        } else if (date.month() == monthShown() && date.day() >= 1 && date.day() <= 31 && dayCounts[date.day()] > 0) {
            f.setWeight(QFont::DemiBold);
        } else {
            f.setWeight(QFont::Normal);
            f.setBold(false);
        }
        painter->setFont(f);
        painter->setPen(textColor);
        painter->drawText(cellRect, Qt::AlignCenter, QString::number(date.day()));

        painter->restore();
    }
};

class FilterCalendarPopupAligner : public QObject {
private:
    QWidget *m_anchor = nullptr;
    QCalendarWidget *m_cal = nullptr;

public:
    FilterCalendarPopupAligner(QWidget *anchor, QCalendarWidget *cal, QObject *parent = nullptr)
        : QObject(parent), m_anchor(anchor), m_cal(cal) {}

    void align() {
        if (!m_cal || !m_anchor) return;
        QWidget *popup = m_cal->window();
        if (!popup) return;

        popup->setAttribute(Qt::WA_TranslucentBackground, true);
        popup->setStyleSheet("background: transparent; border: none;");

        if (popup->layout()) {
            popup->layout()->setContentsMargins(0, 0, 0, 0);
            popup->layout()->setSpacing(0);
        }

        int targetW = 280;
        int targetH = 255;

        m_cal->resize(targetW, targetH);
        m_cal->setFixedWidth(targetW);
        m_cal->setFixedHeight(targetH);
        popup->resize(targetW, targetH);
        popup->setFixedWidth(targetW);
        popup->setFixedHeight(targetH);

        if (auto *view = m_cal->findChild<QTableView*>("qt_calendar_calendarview")) {
            if (auto *hh = view->horizontalHeader()) {
                hh->setVisible(false);
            }
            if (auto *vh = view->verticalHeader()) {
                vh->setVisible(false);
            }
        }

        QPoint anchorGlobal = m_anchor->mapToGlobal(QPoint(0, m_anchor->height() + 2));
        popup->move(anchorGlobal.x(), anchorGlobal.y());
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Show) {
            align();
            QTimer::singleShot(0, this, [this]() { align(); });
        }
        return QObject::eventFilter(obj, event);
    }
};

static QString stripVietnameseAccents(const QString &str) {
    QString res = str.normalized(QString::NormalizationForm_D);
    QString out;
    for (const QChar &ch : res) {
        ushort u = ch.unicode();
        if (u < 0x0300 || u > 0x036F) {
            if (ch == QChar(0x0111)) out.append('d');
            else if (ch == QChar(0x0110)) out.append('D');
            else out.append(ch);
        }
    }
    return out;
}

class ToggleSwitch : public QAbstractButton {
public:
    explicit ToggleSwitch(QWidget *parent = nullptr) : QAbstractButton(parent) {
        setCheckable(true);
        setChecked(false);
        setCursor(Qt::PointingHandCursor);
        setFixedHeight(22);
        setFixedWidth(40);
    }

    QSize sizeHint() const override {
        return QSize(40, 22);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRect trackRect(1, 1, width() - 2, height() - 2);
        QColor trackColor = isChecked() ? QColor("#0d9488") : QColor("#cbd5e1");
        if (underMouse()) {
            trackColor = isChecked() ? QColor("#0f766e") : QColor("#94a3b8");
        }

        p.setPen(Qt::NoPen);
        p.setBrush(trackColor);
        p.drawRoundedRect(trackRect, trackRect.height() / 2.0, trackRect.height() / 2.0);

        int knobDiameter = trackRect.height() - 4;
        int knobX = isChecked() ? (trackRect.right() - knobDiameter - 2) : (trackRect.left() + 2);
        int knobY = trackRect.top() + 2;

        p.setBrush(Qt::white);
        p.drawEllipse(knobX, knobY, knobDiameter, knobDiameter);
    }
};

class NoWheelComboBox : public QComboBox {
public:
    explicit NoWheelComboBox(QWidget *parent = nullptr) : QComboBox(parent) {
        auto *lv = new QListView(this);
        lv->setTextElideMode(Qt::ElideNone);
        lv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        lv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setView(lv);
    }

    void showPopup() override {
        if (view()) {
            view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        QComboBox::showPopup();
    }

protected:
    void wheelEvent(QWheelEvent *e) override {
        e->ignore();
    }
};

class SearchLineEdit : public QLineEdit {
public:
    QListView *listView = nullptr;
    QComboBox *combo = nullptr;

    explicit SearchLineEdit(QWidget *parent = nullptr) : QLineEdit(parent) {}

protected:
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape) {
            if (combo) combo->hidePopup();
            return;
        }
        if (e->key() == Qt::Key_Down || e->key() == Qt::Key_Up) {
            if (listView) {
                listView->setFocus();
                QApplication::sendEvent(listView, e);
                return;
            }
        }
        QLineEdit::keyPressEvent(e);
    }
};

class SearchablePatientComboBox : public NoWheelComboBox {
private:
    SearchLineEdit *searchEdit = nullptr;

public:
    explicit SearchablePatientComboBox(QWidget *parent = nullptr) : NoWheelComboBox(parent) {
        auto *lv = qobject_cast<QListView*>(this->view());
        QWidget *popup = lv ? lv->parentWidget() : nullptr;
        if (lv) {
            lv->setTextElideMode(Qt::ElideNone);
            lv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            lv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            lv->setStyleSheet(R"(
                QListView {
                    border: none;
                    background-color: #ffffff;
                    color: #0f172a;
                    outline: none;
                    padding: 4px;
                    font-size: 13.5px;
                }
                QListView::item {
                    min-height: 32px;
                    padding-left: 10px;
                    border-radius: 6px;
                }
                QListView::item:hover {
                    background-color: #f0fdfa;
                    color: #0d9488;
                }
                QListView::item:selected {
                    background-color: #0d9488;
                    color: #ffffff;
                }
                QScrollBar:vertical {
                    width: 0px;
                    height: 0px;
                    background: transparent;
                }
            )");
        }

        if (popup) {
            popup->setStyleSheet(R"(
                QWidget {
                    background-color: #ffffff;
                    border: 1.5px solid #cbd5e1;
                    border-radius: 10px;
                }
                QScrollBar:vertical {
                    width: 0px;
                    height: 0px;
                    background: transparent;
                }
            )");

            searchEdit = new SearchLineEdit(popup);
            searchEdit->listView = lv;
            searchEdit->combo = this;
            searchEdit->setPlaceholderText(QString::fromUtf8("🔍 Tìm kiếm tên hoặc mã bệnh nhân..."));
            searchEdit->setClearButtonEnabled(true);
            searchEdit->setStyleSheet(R"(
                QLineEdit {
                    background: #f8fafc;
                    border: 1.5px solid #cbd5e1;
                    border-radius: 6px;
                    padding: 7px 10px;
                    font-size: 13px;
                    color: #0f172a;
                    margin: 4px 6px;
                }
                QLineEdit:focus {
                    border: 1.5px solid #0d9488;
                    background: #ffffff;
                }
            )");

            if (auto *boxLay = qobject_cast<QBoxLayout*>(popup->layout())) {
                boxLay->setContentsMargins(4, 4, 4, 6);
                boxLay->setSpacing(2);
                boxLay->insertWidget(0, searchEdit);
            }

            connect(searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
                filterItems(text);
            });

            connect(searchEdit, &QLineEdit::returnPressed, this, [this]() {
                auto *lv = qobject_cast<QListView*>(this->view());
                auto *m = qobject_cast<QStandardItemModel*>(this->model());
                if (!lv || !m) return;
                for (int r = 1; r < m->rowCount(); ++r) {
                    if (!lv->isRowHidden(r)) {
                        setCurrentIndex(r);
                        hidePopup();
                        break;
                    }
                }
            });
        }
    }

    void showPopup() override {
        if (view()) {
            view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        NoWheelComboBox::showPopup();
        if (searchEdit) {
            searchEdit->clear();
            filterItems("");
            QTimer::singleShot(20, this, [this]() {
                if (searchEdit) searchEdit->setFocus();
            });
        }
    }

    void hidePopup() override {
        if (searchEdit) {
            searchEdit->clear();
            filterItems("");
        }
        NoWheelComboBox::hidePopup();
    }

private:
    void filterItems(const QString &keyword) {
        QString kw = stripVietnameseAccents(keyword.trimmed().toLower());
        auto *lv = qobject_cast<QListView*>(this->view());
        if (!lv) return;
        auto *m = qobject_cast<QStandardItemModel*>(this->model());
        if (!m) return;

        lv->setRowHidden(0, true);

        for (int r = 1; r < m->rowCount(); ++r) {
            auto *item = m->item(r);
            if (!item) continue;
            if (kw.isEmpty()) {
                lv->setRowHidden(r, false);
            } else {
                QString textNorm = stripVietnameseAccents(item->text().toLower());
                QString dataNorm = stripVietnameseAccents(item->data(Qt::UserRole).toString().toLower());
                bool match = textNorm.contains(kw) || dataNorm.contains(kw);
                lv->setRowHidden(r, !match);
            }
        }
    }
};

class SideCalendarTimeWidget : public QWidget {
private:
    MainWindow *mainWindow = nullptr;
    DialogCalendarWidget *calendar = nullptr;
    QLabel *dateHeaderLabel = nullptr;
    QWidget *slotsContainer = nullptr;
    QVBoxLayout *slotsLayout = nullptr;
    QList<QPushButton*> slotButtons;

    QDate initialDate;
    QDate selectedDate;
    std::string currentDoctorId;
    std::string currentPatientId;
    std::string currentExcludeApptId;
    QString initialSlot;
    QString selectedSlot;

    std::function<void(const QDate &d, const QString &slot)> onDateTimeSelected;

public:
    explicit SideCalendarTimeWidget(MainWindow *mw, QWidget *parent = nullptr)
        : QWidget(parent), mainWindow(mw) {
        setupUI();
    }

    void setDateTimeSelectedCallback(std::function<void(const QDate &d, const QString &slot)> cb) {
        onDateTimeSelected = cb;
    }

    void setPatientId(const std::string &patientId) {
        if (currentPatientId != patientId) {
            currentPatientId = patientId;
            if (!selectedSlot.isEmpty() && selectedDate.isValid() && !currentPatientId.empty()) {
                QTime slotTime = QTime::fromString(selectedSlot, "HH:mm");
                QDateTime slotDT(selectedDate, slotTime);
                Date checkDate = Date::loadFromString(UIHelper::ss(slotDT.toString("yyyy-MM-dd HH:mm")));
                if (mainWindow && AppointmentController::isPatientSlotBooked(
                        mainWindow->getAppointments(), mainWindow->getSymptomPatients(), currentPatientId, checkDate, currentExcludeApptId)) {
                    selectedSlot.clear();
                    if (onDateTimeSelected) {
                        onDateTimeSelected(selectedDate, selectedSlot);
                    }
                }
            }
            refreshSlots();
        }
    }

    void configure(const std::string &doctorId, const QDate &date = QDate(), const std::string &excludeApptId = "", const QString &initSlot = "", const QString &curSelect = "", const std::string &patientId = "") {
        currentDoctorId = doctorId;
        currentPatientId = patientId;
        initialDate = date;
        selectedDate = date;
        currentExcludeApptId = excludeApptId;
        initialSlot = initSlot;
        selectedSlot = curSelect.isEmpty() ? initSlot : curSelect;

        QDate calDate = date.isValid() ? date : QDate::currentDate();
        if (calendar) {
            calendar->blockSignals(true);
            calendar->setCustomSelectedDate(date);
            calendar->setSelectedDate(calDate);
            calendar->setCurrentPage(calDate.year(), calDate.month());
            calendar->blockSignals(false);
            calendar->refreshHolidays();
            calendar->update();
        }
        refreshSlots();
    }

    void resetToDefault() {
        initialDate = QDate();
        selectedDate = QDate();
        selectedSlot.clear();
        initialSlot.clear();
        currentDoctorId.clear();
        currentPatientId.clear();
        currentExcludeApptId.clear();
        if (calendar) {
            calendar->blockSignals(true);
            calendar->clearCustomSelection();
            calendar->setSelectedDate(QDate::currentDate());
            calendar->setCurrentPage(QDate::currentDate().year(), QDate::currentDate().month());
            calendar->blockSignals(false);
            calendar->refreshHolidays();
            calendar->update();
        }
        refreshSlots();
    }

private:
    void setupUI() {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(12, 12, 12, 12);
        root->setSpacing(8);

        auto *headerBox = new QHBoxLayout;
        headerBox->setSpacing(8);
        auto *iconLbl = new QLabel("📅");
        iconLbl->setStyleSheet("font-size: 16px;");
        auto *titleBox = new QVBoxLayout;
        titleBox->setSpacing(1);
        auto *titleLabel = new QLabel("Chọn lịch & Giờ khám");
        titleLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #0f172a;");
        auto *subLabel = new QLabel("Chọn ngày và ca khám bên dưới");
        subLabel->setStyleSheet("font-size: 11px; color: #64748b; font-weight: 500;");
        titleBox->addWidget(titleLabel);
        titleBox->addWidget(subLabel);
        headerBox->addWidget(iconLbl);
        headerBox->addLayout(titleBox, 1);
        root->addLayout(headerBox);

        auto *divider1 = new QFrame;
        divider1->setFrameShape(QFrame::HLine);
        divider1->setStyleSheet("background-color: #e2e8f0; max-height: 1px; border: none;");
        root->addWidget(divider1);

        auto *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->setStyleSheet("background: transparent; border: none;");

        auto *scrollContent = new QWidget;
        scrollContent->setStyleSheet("background: transparent;");
        auto *contentLay = new QVBoxLayout(scrollContent);
        contentLay->setContentsMargins(0, 0, 0, 0);
        contentLay->setSpacing(8);

        calendar = new DialogCalendarWidget(mainWindow, scrollContent);
        calendar->setFixedWidth(276);
        calendar->setFixedHeight(225);
        contentLay->addWidget(calendar, 0, Qt::AlignHCenter);

        connect(calendar, &QCalendarWidget::currentPageChanged, this, [this](int, int) {
            if (calendar) {
                calendar->refreshHolidays();
                calendar->update();
            }
        });

        connect(calendar, &QCalendarWidget::clicked, this, [this](const QDate &d) {
            if (d < QDate::currentDate()) {
                UIHelper::showWarning(calendar, "Ngày không hợp lệ", "Không thể chọn ngày trong quá khứ. Vui lòng chọn ngày từ hôm nay trở đi.");
                if (calendar) {
                    calendar->blockSignals(true);
                    calendar->setSelectedDate(selectedDate.isValid() ? selectedDate : QDate::currentDate());
                    calendar->blockSignals(false);
                    calendar->update();
                }
                return;
            }
            if (mainWindow && mainWindow->isHoliday(d)) {
                UIHelper::showWarning(calendar, "Ngày nghỉ lễ", QStringLiteral("Phòng khám nghỉ lễ vào ngày %1 (%2). Vui lòng chọn ngày khác.").arg(d.toString("dd/MM/yyyy"), mainWindow->holidayNote(d)));
                if (calendar) {
                    calendar->blockSignals(true);
                    calendar->setSelectedDate(selectedDate.isValid() ? selectedDate : QDate::currentDate());
                    calendar->blockSignals(false);
                    calendar->update();
                }
                return;
            }
            if (d.month() != calendar->monthShown() || d.year() != calendar->yearShown()) {
                calendar->setCurrentPage(d.year(), d.month());
            }
            selectedDate = d;
            calendar->setCustomSelectedDate(d);
            calendar->setSelectedDate(d);
            calendar->update();
            selectedSlot.clear();
            refreshSlots();
            if (onDateTimeSelected) {
                onDateTimeSelected(selectedDate, selectedSlot);
            }
        });

        auto *divider2 = new QFrame;
        divider2->setFrameShape(QFrame::HLine);
        divider2->setStyleSheet("background-color: #e2e8f0; max-height: 1px; border: none; margin: 4px 0;");
        contentLay->addWidget(divider2);

        dateHeaderLabel = new QLabel("⏱ Khung giờ khả dụng");
        dateHeaderLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #0d9488;");
        contentLay->addWidget(dateHeaderLabel);

        slotsContainer = new QWidget(scrollContent);
        slotsContainer->setStyleSheet("background: transparent;");
        slotsLayout = new QVBoxLayout(slotsContainer);
        slotsLayout->setContentsMargins(0, 0, 0, 0);
        slotsLayout->setSpacing(6);
        contentLay->addWidget(slotsContainer);

        contentLay->addStretch(1);
        scroll->setWidget(scrollContent);
        root->addWidget(scroll, 1);

        setStyleSheet(R"(
            SideCalendarTimeWidget {
                background-color: #f8fafc;
                border: 1.5px solid #cbd5e1;
                border-radius: 12px;
            }
            SideCalendarTimeWidget QScrollBar:vertical {
                width: 6px;
                background: transparent;
                margin: 2px 0px;
            }
            SideCalendarTimeWidget QScrollBar::handle:vertical {
                background: #cbd5e1;
                border-radius: 3px;
                min-height: 24px;
            }
            SideCalendarTimeWidget QScrollBar::handle:vertical:hover {
                background: #94a3b8;
            }
            SideCalendarTimeWidget QScrollBar::add-line:vertical, SideCalendarTimeWidget QScrollBar::sub-line:vertical {
                height: 0px;
                background: transparent;
            }
        )");
    }

    void refreshSlots() {
        qDeleteAll(slotButtons);
        slotButtons.clear();

        QLayoutItem *child;
        while ((child = slotsLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }

        if (!selectedDate.isValid()) {
            dateHeaderLabel->setText("⏱ Vui lòng chọn ngày trên lịch");
            return;
        }

        dateHeaderLabel->setText(QStringLiteral("⏱ Khung giờ ngày %1").arg(selectedDate.toString("dd/MM/yyyy")));

        const QStringList morningSlots = {"07:00", "07:45", "08:30", "09:15", "10:00", "10:45"};
        const QStringList afternoonSlots = {"13:00", "13:45", "14:30", "15:15", "16:00"};

        auto *mornLbl = new QLabel("☀️ Buổi sáng (07:00 – 11:30)");
        mornLbl->setStyleSheet("font-size: 11px; font-weight: 700; color: #475569; padding-top: 2px;");
        slotsLayout->addWidget(mornLbl);

        auto *mornGrid = new QGridLayout;
        mornGrid->setHorizontalSpacing(6);
        mornGrid->setVerticalSpacing(6);
        for (int i = 0; i < morningSlots.size(); ++i) {
            auto *btn = createSlotButton(morningSlots[i]);
            slotButtons.append(btn);
            mornGrid->addWidget(btn, i / 2, i % 2);
        }
        slotsLayout->addLayout(mornGrid);

        auto *aftLbl = new QLabel("🌙 Buổi chiều (13:00 – 16:45)");
        aftLbl->setStyleSheet("font-size: 11px; font-weight: 700; color: #475569; padding-top: 6px;");
        slotsLayout->addWidget(aftLbl);

        auto *aftGrid = new QGridLayout;
        aftGrid->setHorizontalSpacing(6);
        aftGrid->setVerticalSpacing(6);
        for (int i = 0; i < afternoonSlots.size(); ++i) {
            auto *btn = createSlotButton(afternoonSlots[i]);
            slotButtons.append(btn);
            aftGrid->addWidget(btn, i / 2, i % 2);
        }
        slotsLayout->addLayout(aftGrid);
    }

    QPushButton* createSlotButton(const QString &slotStr) {
        auto *btn = new QPushButton;
        btn->setProperty("slotStr", slotStr);
        btn->setFixedHeight(34);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        updateButtonStyle(btn, slotStr);

        connect(btn, &QPushButton::clicked, this, [this, slotStr]() {
            selectedSlot = slotStr;
            for (auto *b : slotButtons) {
                updateButtonStyle(b, b->property("slotStr").toString());
            }
            if (onDateTimeSelected) {
                onDateTimeSelected(selectedDate, selectedSlot);
            }
        });

        return btn;
    }

    void updateButtonStyle(QPushButton *btn, const QString &slotStr) {
        QTime slotTime = QTime::fromString(slotStr, "HH:mm");
        QDateTime slotDT(selectedDate, slotTime);
        QDateTime nowDT = QDateTime::currentDateTime();

        bool isPast = (slotDT <= nowDT);

        Date checkDate = Date::loadFromString(UIHelper::ss(slotDT.toString("yyyy-MM-dd HH:mm")));
        bool isDoctorBooked = (!currentDoctorId.empty() && mainWindow &&
                               AppointmentController::isSlotBooked(mainWindow->getAppointments(), currentDoctorId, checkDate, currentExcludeApptId));
        bool isPatientBooked = (!currentPatientId.empty() && mainWindow &&
                                AppointmentController::isPatientSlotBooked(mainWindow->getAppointments(), mainWindow->getSymptomPatients(), currentPatientId, checkDate, currentExcludeApptId));
        bool isBooked = isDoctorBooked || isPatientBooked;

        bool isCurrentSlot = (!initialSlot.isEmpty() && initialSlot == slotStr && initialDate.isValid() && selectedDate == initialDate);
        bool isSelected = (selectedSlot == slotStr);

        if (isCurrentSlot) {
            btn->setText(QStringLiteral("[ %1 - hiện tại ]").arg(slotStr));
        } else {
            btn->setText(QStringLiteral("[ %1 ]").arg(slotStr));
        }

        if (isCurrentSlot) {
            btn->setEnabled(true);
            btn->setCursor(Qt::PointingHandCursor);
            if (isSelected) {
                btn->setStyleSheet("QPushButton { background-color: #0d9488; border: 1.5px solid #0f766e; color: #ffffff; font-weight: 700; font-size: 11px; border-radius: 6px; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #f0fdf4; border: 1.5px solid #86efac; color: #15803d; font-weight: 700; font-size: 11px; border-radius: 6px; } QPushButton:hover { background-color: #dcfce7; border-color: #4ade80; }");
            }
        } else if (isPast || isBooked) {
            btn->setEnabled(false);
            btn->setCursor(Qt::ArrowCursor);
            btn->setStyleSheet("QPushButton { background-color: #f1f5f9; border: 1.5px solid #cbd5e1; color: #94a3b8; font-weight: 600; font-size: 11.5px; border-radius: 6px; }");
            if (isPast) {
                btn->setToolTip("Khung giờ này đã trôi qua");
            } else if (isDoctorBooked) {
                btn->setToolTip("Bác sĩ đã có lịch hẹn khác vào khung giờ này");
            } else {
                btn->setToolTip("Bệnh nhân đã có một lịch khám khác vào khung giờ này");
            }
        } else {
            btn->setEnabled(true);
            btn->setCursor(Qt::PointingHandCursor);
            if (isSelected) {
                btn->setStyleSheet("QPushButton { background-color: #0d9488; border: 1.5px solid #0f766e; color: #ffffff; font-weight: 700; font-size: 11.5px; border-radius: 6px; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #f0fdf4; border: 1.5px solid #86efac; color: #15803d; font-weight: 700; font-size: 11.5px; border-radius: 6px; } QPushButton:hover { background-color: #dcfce7; border-color: #4ade80; }");
            }
        }
    }
};

class BottomCalendarTimeWidget : public QWidget {
private:
    MainWindow *mainWindow = nullptr;
    DialogCalendarWidget *calendar = nullptr;
    QLabel *dateHeaderLabel = nullptr;
    QWidget *slotsContainer = nullptr;
    QVBoxLayout *slotsLayout = nullptr;
    QList<QPushButton*> slotButtons;

    QDate initialDate;
    QDate selectedDate;
    std::string currentDoctorId;
    std::string currentPatientId;
    std::string currentExcludeApptId;
    QString initialSlot;
    QString selectedSlot;

    std::function<void(const QDate &d, const QString &slot)> onDateTimeSelected;

public:
    explicit BottomCalendarTimeWidget(MainWindow *mw, QWidget *parent = nullptr)
        : QWidget(parent), mainWindow(mw) {
        setupUI();
    }

    void setDateTimeSelectedCallback(std::function<void(const QDate &d, const QString &slot)> cb) {
        onDateTimeSelected = cb;
    }

    void configure(const std::string &doctorId, const QDate &date = QDate(), const std::string &excludeApptId = "", const QString &initSlot = "", const QString &curSelect = "", const std::string &patientId = "") {
        currentDoctorId = doctorId;
        currentPatientId = patientId;
        initialDate = date;
        selectedDate = date;
        currentExcludeApptId = excludeApptId;
        initialSlot = initSlot;
        selectedSlot = curSelect.isEmpty() ? initSlot : curSelect;

        QDate calDate = date.isValid() ? date : QDate::currentDate();
        if (calendar) {
            calendar->blockSignals(true);
            calendar->setCustomSelectedDate(date);
            calendar->setSelectedDate(calDate);
            calendar->setCurrentPage(calDate.year(), calDate.month());
            calendar->blockSignals(false);
            calendar->refreshHolidays();
            calendar->update();
        }
        refreshSlots();
    }

private:
    void setupUI() {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        auto *root = new QHBoxLayout(this);
        root->setContentsMargins(12, 12, 12, 12);
        root->setSpacing(14);

        auto *calBox = new QVBoxLayout;
        calBox->setSpacing(6);
        auto *calTitle = new QLabel("📅 Chọn ngày khám");
        calTitle->setStyleSheet("font-size: 12px; font-weight: 700; color: #0f172a;");
        calBox->addWidget(calTitle);

        calendar = new DialogCalendarWidget(mainWindow, this);
        calendar->setFixedWidth(275);
        calendar->setFixedHeight(225);
        calBox->addWidget(calendar);
        calBox->addStretch();
        root->addLayout(calBox);

        connect(calendar, &QCalendarWidget::currentPageChanged, this, [this](int, int) {
            if (calendar) {
                calendar->refreshHolidays();
                calendar->update();
            }
        });

        connect(calendar, &QCalendarWidget::clicked, this, [this](const QDate &d) {
            if (d < QDate::currentDate()) {
                UIHelper::showWarning(calendar, "Ngày không hợp lệ", "Không thể chọn ngày trong quá khứ. Vui lòng chọn ngày từ hôm nay trở đi.");
                if (calendar) {
                    calendar->blockSignals(true);
                    calendar->setSelectedDate(selectedDate.isValid() ? selectedDate : QDate::currentDate());
                    calendar->blockSignals(false);
                    calendar->update();
                }
                return;
            }
            if (mainWindow && mainWindow->isHoliday(d)) {
                UIHelper::showWarning(calendar, "Ngày nghỉ lễ", QStringLiteral("Phòng khám nghỉ lễ vào ngày %1 (%2). Vui lòng chọn ngày khác.").arg(d.toString("dd/MM/yyyy"), mainWindow->holidayNote(d)));
                if (calendar) {
                    calendar->blockSignals(true);
                    calendar->setSelectedDate(selectedDate.isValid() ? selectedDate : QDate::currentDate());
                    calendar->blockSignals(false);
                    calendar->update();
                }
                return;
            }
            if (d.month() != calendar->monthShown() || d.year() != calendar->yearShown()) {
                calendar->setCurrentPage(d.year(), d.month());
            }
            selectedDate = d;
            calendar->setCustomSelectedDate(d);
            calendar->setSelectedDate(d);
            calendar->update();
            selectedSlot = "";
            refreshSlots();
            if (onDateTimeSelected) {
                onDateTimeSelected(selectedDate, selectedSlot);
            }
        });

        auto *vSep = new QFrame;
        vSep->setFrameShape(QFrame::VLine);
        vSep->setFixedWidth(1);
        vSep->setStyleSheet("background-color: #e2e8f0; border: none; margin: 4px 0;");
        root->addWidget(vSep);

        auto *rightBox = new QVBoxLayout;
        rightBox->setSpacing(6);

        dateHeaderLabel = new QLabel("⏱ Khung giờ khả dụng");
        dateHeaderLabel->setStyleSheet("font-size: 12px; font-weight: 700; color: #0d9488;");
        rightBox->addWidget(dateHeaderLabel);

        slotsContainer = new QWidget(this);
        slotsContainer->setStyleSheet("background: transparent;");
        slotsLayout = new QVBoxLayout(slotsContainer);
        slotsLayout->setContentsMargins(0, 0, 0, 0);
        slotsLayout->setSpacing(5);
        rightBox->addWidget(slotsContainer);
        rightBox->addStretch();

        root->addLayout(rightBox, 1);

        setStyleSheet(R"(
            BottomCalendarTimeWidget {
                background-color: #ffffff;
                border: 1.5px solid #cbd5e1;
                border-radius: 12px;
            }
        )");
    }

    void refreshSlots() {
        qDeleteAll(slotButtons);
        slotButtons.clear();

        QLayoutItem *child;
        while ((child = slotsLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }

        if (!selectedDate.isValid()) {
            dateHeaderLabel->setText("⏱ Khung giờ khả dụng");
        } else {
            dateHeaderLabel->setText(QStringLiteral("⏱ Ca khám ngày %1").arg(selectedDate.toString("dd/MM/yyyy")));
        }

        const QStringList morningSlots = {"07:00", "07:45", "08:30", "09:15", "10:00", "10:45"};
        const QStringList afternoonSlots = {"13:00", "13:45", "14:30", "15:15", "16:00"};

        auto *mornLbl = new QLabel("☀️ Ca sáng (07:00 – 11:30)");
        mornLbl->setStyleSheet("font-size: 11px; font-weight: 700; color: #475569; padding-top: 1px;");
        slotsLayout->addWidget(mornLbl);

        auto *mornGrid = new QGridLayout;
        mornGrid->setHorizontalSpacing(6);
        mornGrid->setVerticalSpacing(4);
        for (int i = 0; i < morningSlots.size(); ++i) {
            auto *btn = createSlotButton(morningSlots[i]);
            slotButtons.append(btn);
            mornGrid->addWidget(btn, i / 2, i % 2);
        }
        slotsLayout->addLayout(mornGrid);

        auto *aftLbl = new QLabel("🌙 Ca chiều (13:00 – 16:45)");
        aftLbl->setStyleSheet("font-size: 11px; font-weight: 700; color: #475569; padding-top: 4px;");
        slotsLayout->addWidget(aftLbl);

        auto *aftGrid = new QGridLayout;
        aftGrid->setHorizontalSpacing(6);
        aftGrid->setVerticalSpacing(4);
        for (int i = 0; i < afternoonSlots.size(); ++i) {
            auto *btn = createSlotButton(afternoonSlots[i]);
            slotButtons.append(btn);
            aftGrid->addWidget(btn, i / 2, i % 2);
        }
        slotsLayout->addLayout(aftGrid);
    }

    QPushButton* createSlotButton(const QString &slotStr) {
        auto *btn = new QPushButton;
        btn->setProperty("slotStr", slotStr);
        btn->setFixedHeight(29);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        updateButtonStyle(btn, slotStr);

        connect(btn, &QPushButton::clicked, this, [this, slotStr]() {
            selectedSlot = slotStr;
            for (auto *b : slotButtons) {
                updateButtonStyle(b, b->property("slotStr").toString());
            }
            if (onDateTimeSelected) {
                onDateTimeSelected(selectedDate, selectedSlot);
            }
        });

        return btn;
    }

    void updateButtonStyle(QPushButton *btn, const QString &slotStr) {
        QTime slotTime = QTime::fromString(slotStr, "HH:mm");
        QDateTime slotDT(selectedDate, slotTime);
        QDateTime nowDT = QDateTime::currentDateTime();

        bool isPast = (slotDT <= nowDT);

        Date checkDate = Date::loadFromString(UIHelper::ss(slotDT.toString("yyyy-MM-dd HH:mm")));
        bool isDoctorBooked = (!currentDoctorId.empty() && mainWindow &&
                               AppointmentController::isSlotBooked(mainWindow->getAppointments(), currentDoctorId, checkDate, currentExcludeApptId));
        bool isPatientBooked = (!currentPatientId.empty() && mainWindow &&
                                AppointmentController::isPatientSlotBooked(mainWindow->getAppointments(), mainWindow->getSymptomPatients(), currentPatientId, checkDate, currentExcludeApptId));
        bool isBooked = isDoctorBooked || isPatientBooked;

        bool isCurrentSlot = (!initialSlot.isEmpty() && initialSlot == slotStr && initialDate.isValid() && selectedDate == initialDate);
        bool isSelected = (selectedSlot == slotStr);

        if (isCurrentSlot) {
            btn->setText(QStringLiteral("[ %1 - hiện tại ]").arg(slotStr));
        } else {
            btn->setText(QStringLiteral("[ %1 ]").arg(slotStr));
        }

        if (isCurrentSlot) {
            btn->setEnabled(true);
            btn->setCursor(Qt::PointingHandCursor);
            if (isSelected) {
                btn->setStyleSheet("QPushButton { background-color: #0d9488; border: 1.5px solid #0f766e; color: #ffffff; font-weight: 700; font-size: 11px; border-radius: 6px; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #f0fdf4; border: 1.5px solid #86efac; color: #15803d; font-weight: 700; font-size: 11px; border-radius: 6px; } QPushButton:hover { background-color: #dcfce7; border-color: #4ade80; }");
            }
        } else if (isPast || isBooked) {
            btn->setEnabled(false);
            btn->setCursor(Qt::ArrowCursor);
            btn->setStyleSheet("QPushButton { background-color: #f1f5f9; border: 1.5px solid #cbd5e1; color: #94a3b8; font-weight: 600; font-size: 11px; border-radius: 6px; }");
            if (isPast) {
                btn->setToolTip("Khung giờ này đã trôi qua");
            } else if (isDoctorBooked) {
                btn->setToolTip("Bác sĩ đã có lịch hẹn khác vào khung giờ này");
            } else {
                btn->setToolTip("Bệnh nhân đã có một lịch khám khác vào khung giờ này");
            }
        } else {
            btn->setEnabled(true);
            btn->setCursor(Qt::PointingHandCursor);
            if (isSelected) {
                btn->setStyleSheet("QPushButton { background-color: #0d9488; border: 1.5px solid #0f766e; color: #ffffff; font-weight: 700; font-size: 11px; border-radius: 6px; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #f0fdf4; border: 1.5px solid #86efac; color: #15803d; font-weight: 700; font-size: 11px; border-radius: 6px; } QPushButton:hover { background-color: #dcfce7; border-color: #4ade80; }");
            }
        }
    }
};

}

bool AppointmentView::showBookingDialog(const QDate &defaultDate, const std::string &preselectedPatientId) {
    if (!mainWindow) return false;

    QDate targetedDate = (defaultDate.isValid() && defaultDate >= QDate::currentDate() && !mainWindow->isHoliday(defaultDate)) ? defaultDate : QDate();

    QDialog dialog(mainWindow);
    dialog.setWindowTitle("Đặt lịch khám bệnh");
    dialog.setFixedWidth(540);
    dialog.setStyleSheet(R"(
        QDialog { background-color: #ffffff; border-radius: 12px; }
        QComboBox {
            background-color: #f8fafc;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13.5px;
            color: #0f172a;
            min-height: 22px;
        }
        QComboBox:focus {
            border: 1.5px solid #0d9488;
            background: #ffffff;
        }
        QComboBox:disabled {
            background-color: #e2e8f0;
            color: #475569;
            border: 1.5px solid #cbd5e1;
        }
        QComboBox::drop-down { border: none; width: 28px; }
        QComboBox::drop-down:disabled { width: 0px; border: none; }
        QComboBox::down-arrow {
            image: url(:/assets/textures/arrow_down_dark.png);
            width: 12px; height: 12px; margin-right: 8px;
        }
        QComboBox::down-arrow:disabled {
            image: none;
        }
        QComboBox QAbstractItemView {
            background-color: #ffffff;
            color: #0f172a;
            selection-background-color: #0d9488;
            selection-color: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            outline: none;
            padding: 4px;
            font-size: 13.5px;
        }
        QComboBox QAbstractItemView::item {
            min-height: 32px;
            padding: 4px 10px;
            border-radius: 6px;
            color: #0f172a;
            background-color: #ffffff;
        }
        QComboBox QAbstractItemView::item:hover {
            background-color: #f0fdfa;
            color: #0f766e;
        }
        QComboBox QAbstractItemView::item:selected {
            background-color: #0d9488;
            color: #ffffff;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            width: 0px;
            height: 0px;
            background: transparent;
            border: none;
        }
    )");

    auto *rootLayout = new QHBoxLayout(&dialog);
    rootLayout->setContentsMargins(28, 24, 28, 24);
    rootLayout->setSpacing(16);

    auto *leftContainer = new QWidget(&dialog);
    leftContainer->setFixedWidth(484);
    auto *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(14);

    auto *sidePanel = new SideCalendarTimeWidget(mainWindow, &dialog);
    sidePanel->setFixedWidth(300);
    sidePanel->setVisible(false);

    rootLayout->addWidget(leftContainer);
    rootLayout->addWidget(sidePanel);

    auto *headerBox = new QHBoxLayout;
    headerBox->setSpacing(14);
    auto *iconBadge = new QLabel("📅");
    iconBadge->setFixedSize(44, 44);
    iconBadge->setAlignment(Qt::AlignCenter);
    iconBadge->setStyleSheet("background: #f0fdfa; color: #0d9488; border: 1.5px solid #99f6e4; border-radius: 12px; font-size: 20px;");

    auto *titleBox = new QVBoxLayout;
    titleBox->setSpacing(2);
    auto *dlgTitle = new QLabel("Tạo lịch khám bệnh mới");
    dlgTitle->setStyleSheet("font-size: 17px; font-weight: 700; color: #0f172a;");
    auto *dlgSub = new QLabel("Điền thông tin bệnh nhân, triệu chứng và bác sĩ phụ trách");
    dlgSub->setStyleSheet("font-size: 12.5px; color: #64748b; font-weight: 500;");
    titleBox->addWidget(dlgTitle);
    titleBox->addWidget(dlgSub);
    headerBox->addWidget(iconBadge);
    headerBox->addLayout(titleBox, 1);
    leftLayout->addLayout(headerBox);

    auto *divider = new QFrame;
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("background-color: #f1f5f9; max-height: 1px; border: none;");
    leftLayout->addWidget(divider);

    auto *formWidget = new QWidget;
    auto *form = new QFormLayout(formWidget);
    form->setContentsMargins(0, 0, 0, 0);
    form->setSpacing(11);
    form->setLabelAlignment(Qt::AlignLeft);

    auto makeLabel = [](const QString &t) {
        auto *l = new QLabel(t);
        l->setStyleSheet("font-weight: 600; font-size: 13px; color: #334155;");
        return l;
    };

    auto *patient = new SearchablePatientComboBox;
    patient->addItem("Chọn hồ sơ bệnh nhân", "");
    auto *patients = mainWindow->getPatients();
    if (patients) {
        auto *p = patients->getHeader()->next;
        while (p && p->next) {
            if (!p->info.isDefault() && p->info.getState() != -1) {
                patient->addItem(UIHelper::qs(p->info.getfullName()), UIHelper::qs(p->info.getID()));
            }
            p = p->next;
        }
    }
    if (!preselectedPatientId.empty()) {
        int idx = patient->findData(UIHelper::qs(preselectedPatientId));
        if (idx != -1) {
            patient->setCurrentIndex(idx);
            patient->setEnabled(false);
        }
        sidePanel->setPatientId(preselectedPatientId);
    }

    form->addRow(makeLabel("Bệnh nhân:"), patient);

    NoWheelComboBox *symptomCombos[5];
    QWidget *symptomRowWidgets[5];
    auto *symptoms = mainWindow->getSymptoms();

    for (int i = 0; i < 5; ++i) {
        symptomCombos[i] = new NoWheelComboBox;
        symptomCombos[i]->addItem("Chọn triệu chứng", "");
        if (symptoms) {
            auto *s = symptoms->getHeader()->next;
            while (s && s->next) {
                symptomCombos[i]->addItem(UIHelper::qs(s->info.getSName()), UIHelper::qs(s->info.getS_id()));
                s = s->next;
            }
        }

        symptomRowWidgets[i] = new QWidget;
        auto *rowLay = new QHBoxLayout(symptomRowWidgets[i]);
        rowLay->setContentsMargins(0, 0, 0, 0);
        rowLay->addWidget(symptomCombos[i]);

        QString lblText = (i == 0) ? "Triệu chứng:" : QStringLiteral("Triệu chứng %1:").arg(i + 1);
        form->addRow(makeLabel(lblText), symptomRowWidgets[i]);

        if (i > 0) {
            symptomRowWidgets[i]->hide();
            if (auto *lblItem = form->labelForField(symptomRowWidgets[i])) {
                lblItem->hide();
            }
        }
    }

    auto *specialtyValue = new QLabel("—");
    specialtyValue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    specialtyValue->setStyleSheet("background-color: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 8px 12px; font-size: 13.5px; color: #0d9488; font-weight: 700; min-height: 22px;");
    form->addRow(makeLabel("Chuyên khoa:"), specialtyValue);

    auto *doctor = new NoWheelComboBox;
    doctor->addItem("Chọn bác sĩ phụ trách", "");
    form->addRow(makeLabel("Bác sĩ khám:"), doctor);

    auto *feeValue = new QLabel("—");
    feeValue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    feeValue->setStyleSheet("background-color: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 8px 12px; font-size: 14.5px; color: #0d9488; font-weight: 800; min-height: 22px;");
    form->addRow(makeLabel("Phí khám:"), feeValue);

    QDate selectedDate = targetedDate;
    QString selectedSlot;

    auto *dateBtn = new QPushButton;
    dateBtn->setCursor(Qt::PointingHandCursor);
    dateBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    dateBtn->setEnabled(false);
    dateBtn->setText(targetedDate.isValid() ? targetedDate.toString("dd/MM/yyyy") : "Chọn ngày khám");
    dateBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f1f5f9;
            border: 1.5px solid #e2e8f0;
            border-radius: 8px;
            padding: 8px 36px 8px 12px;
            font-size: 13.5px;
            color: #94a3b8;
            text-align: left;
            min-height: 22px;
        }
    )");

    auto *calIco = new QLabel("📅", dateBtn);
    calIco->setStyleSheet("font-size: 14px; background: transparent;");
    calIco->setAttribute(Qt::WA_TransparentForMouseEvents);
    auto *btnLay = new QHBoxLayout(dateBtn);
    btnLay->setContentsMargins(0, 0, 10, 0);
    btnLay->addStretch();
    btnLay->addWidget(calIco);

    form->addRow(makeLabel("Ngày giờ hẹn:"), dateBtn);
    leftLayout->addWidget(formWidget);

    connect(dateBtn, &QPushButton::clicked, &dialog, [sidePanel, doctor, patient, &selectedDate, &selectedSlot, &dialog]() {
        if (doctor->currentData().toString().isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng chọn bác sĩ phụ trách trước khi chọn lịch khám.");
            return;
        }
        if (sidePanel->isVisible()) {
            sidePanel->setVisible(false);
            dialog.setMinimumHeight(0);
            dialog.setFixedWidth(540);
            dialog.adjustSize();
        } else {
            std::string dId = UIHelper::ss(doctor->currentData().toString());
            std::string pId = UIHelper::ss(patient->currentData().toString());
            sidePanel->configure(dId, selectedDate, "", "", selectedSlot, pId);
            sidePanel->setVisible(true);
            dialog.setFixedWidth(856);
            dialog.setMinimumHeight(630);
            dialog.resize(856, 630);
        }
    });

    sidePanel->setDateTimeSelectedCallback([dateBtn, &selectedDate, &selectedSlot](const QDate &d, const QString &slot) {
        selectedDate = d;
        selectedSlot = slot;
        dateBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #ffffff;
                border: 1.5px solid #0d9488;
                border-radius: 8px;
                padding: 8px 36px 8px 12px;
                font-size: 13.5px;
                color: #0f172a;
                font-weight: 700;
                text-align: left;
                min-height: 22px;
            }
            QPushButton:hover {
                background-color: #ffffff;
                border: 1.5px solid #0d9488;
            }
        )");
        if (slot.isEmpty()) {
            dateBtn->setText(d.toString("dd/MM/yyyy"));
        } else {
            dateBtn->setText(QStringLiteral("%1   [ %2 ]").arg(d.toString("dd/MM/yyyy"), slot));
        }
    });

    connect(patient, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, [this, patient, sidePanel, dateBtn, &selectedDate, &selectedSlot]() {
        std::string pId = UIHelper::ss(patient->currentData().toString());
        sidePanel->setPatientId(pId);
        if (!selectedSlot.isEmpty() && selectedDate.isValid() && !pId.empty()) {
            QTime slotTime = QTime::fromString(selectedSlot, "HH:mm");
            Date targetDate(selectedDate.day(), selectedDate.month(), selectedDate.year(), slotTime.hour(), slotTime.minute());
            if (mainWindow && AppointmentController::isPatientSlotBooked(mainWindow->getAppointments(), mainWindow->getSymptomPatients(), pId, targetDate)) {
                selectedSlot.clear();
                dateBtn->setText(selectedDate.toString("dd/MM/yyyy"));
            }
        }
    });

    auto updateSpecialtyAndDoctors = [this, &dialog, symptomCombos, form, specialtyValue, doctor, feeValue, dateBtn, sidePanel, &selectedDate, &selectedSlot, targetedDate]() {

        for (int i = 0; i < 4; ++i) {
            bool currentSelected = !symptomCombos[i]->currentData().toString().isEmpty();
            if (currentSelected) {
                symptomCombos[i + 1]->parentWidget()->show();
                if (auto *lbl = form->labelForField(symptomCombos[i + 1]->parentWidget())) lbl->show();
            } else {
                for (int j = i + 1; j < 5; ++j) {
                    symptomCombos[j]->blockSignals(true);
                    symptomCombos[j]->setCurrentIndex(0);
                    symptomCombos[j]->blockSignals(false);
                    symptomCombos[j]->parentWidget()->hide();
                    if (auto *lbl = form->labelForField(symptomCombos[j]->parentWidget())) lbl->hide();
                }
                break;
            }
        }

        QString selectedIds[5];
        for (int i = 0; i < 5; ++i) {
            selectedIds[i] = symptomCombos[i]->currentData().toString();
        }

        auto *symptomList = mainWindow->getSymptoms();
        if (symptomList) {
            for (int i = 0; i < 5; ++i) {
                QString myCurrent = selectedIds[i];
                symptomCombos[i]->blockSignals(true);
                symptomCombos[i]->clear();
                symptomCombos[i]->addItem("Chọn triệu chứng", "");

                auto *s = symptomList->getHeader()->next;
                while (s && s->next) {
                    QString sid = UIHelper::qs(s->info.getS_id());
                    bool chosenInOther = false;
                    for (int j = 0; j < 5; ++j) {
                        if (j != i && !selectedIds[j].isEmpty() && selectedIds[j] == sid) {
                            chosenInOther = true;
                            break;
                        }
                    }
                    if (!chosenInOther) {
                        symptomCombos[i]->addItem(UIHelper::qs(s->info.getSName()), sid);
                    }
                    s = s->next;
                }
                int idx = symptomCombos[i]->findData(myCurrent);
                if (idx >= 0) symptomCombos[i]->setCurrentIndex(idx);
                else symptomCombos[i]->setCurrentIndex(0);
                symptomCombos[i]->blockSignals(false);
            }
        }

        QMap<QString, int> spScores;
        if (symptomList) {
            for (int i = 0; i < 5; ++i) {
                QString sid = symptomCombos[i]->currentData().toString();
                if (!sid.isEmpty()) {
                    auto *s = symptomList->getHeader()->next;
                    while (s && s->next) {
                        if (UIHelper::qs(s->info.getS_id()) == sid) {
                            QString spId = UIHelper::qs(s->info.getSp_id());
                            spScores[spId] += s->info.getSScore();
                            break;
                        }
                        s = s->next;
                    }
                }
            }
        }

        auto countActiveDoctors = [this](const QString &spId) -> int {
            int count = 0;
            if (mainWindow && mainWindow->getDoctors()) {
                auto *d = mainWindow->getDoctors()->getHeader()->next;
                while (d && d->next) {
                    if (d->info.getState() == 1 && UIHelper::qs(d->info.getSpId()) == spId) {
                        count++;
                    }
                    d = d->next;
                }
            }
            return count;
        };

        QString bestSpId;
        int bestScore = 0;
        int bestActiveCount = 0;
        for (auto it = spScores.begin(); it != spScores.end(); ++it) {
            QString spId = it.key();
            int score = it.value();
            int activeDocs = countActiveDoctors(spId);

            bool isBetter = false;
            if (bestSpId.isEmpty()) {
                isBetter = true;
            } else if (activeDocs > 0 && bestActiveCount == 0) {
                isBetter = true;
            } else if ((activeDocs > 0) == (bestActiveCount > 0)) {
                if (score > bestScore) {
                    isBetter = true;
                }
            }

            if (isBetter) {
                bestSpId = spId;
                bestScore = score;
                bestActiveCount = activeDocs;
            }
        }

        if (!bestSpId.isEmpty()) {
            if (bestActiveCount > 0) {
                specialtyValue->setText(mainWindow->specialty(bestSpId.toStdString()));
            } else {
                specialtyValue->setText(QStringLiteral("%1 (Tạm ngưng tiếp nhận)").arg(mainWindow->specialty(bestSpId.toStdString())));
            }
        } else {
            specialtyValue->setText("—");
        }

        QString prevDocId = doctor->currentData().toString();
        {
            QSignalBlocker docBlocker(doctor);
            doctor->clear();
            if (!bestSpId.isEmpty() && bestActiveCount == 0) {
                doctor->addItem("Không có bác sĩ đang công tác thuộc chuyên khoa này", "");
            } else {
                doctor->addItem("Chọn bác sĩ phụ trách", "");
            }

            if (!bestSpId.isEmpty() && bestActiveCount > 0) {
                auto *doctors = mainWindow->getDoctors();
                if (doctors) {
                    auto *d = doctors->getHeader()->next;
                    while (d && d->next) {
                        if (d->info.getState() == 1 && UIHelper::qs(d->info.getSpId()) == bestSpId) {
                            QString roomStr = UIHelper::formatRoom(UIHelper::qs(d->info.getRoomId()));
                            doctor->addItem(QStringLiteral("BS. %1 · %2 (%3)")
                                .arg(UIHelper::qs(d->info.getfullName()), mainWindow->specialty(d->info.getSpId()), roomStr),
                                UIHelper::qs(d->info.getID()));
                        }
                        d = d->next;
                    }
                }
                int restoredIdx = doctor->findData(prevDocId);
                if (restoredIdx > 0) {
                    doctor->setCurrentIndex(restoredIdx);
                }
            }
        }

        QString newDocId = doctor->currentData().toString();
        if (newDocId.isEmpty()) {
            feeValue->setText("—");
        } else if (Doctor *selDoc = mainWindow->findDoctor(UIHelper::ss(newDocId))) {
            feeValue->setText(QLocale(QLocale::Vietnamese).toString(selDoc->getFee(), 'f', 0) + " ₫");
        }

        if (doctor->currentIndex() <= 0) {
            dateBtn->setEnabled(false);
            if (targetedDate.isValid()) {
                selectedDate = targetedDate;
                dateBtn->setText(targetedDate.toString("dd/MM/yyyy"));
            } else {
                selectedDate = QDate();
                dateBtn->setText("Chọn ngày khám");
            }
            dateBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: #f1f5f9;
                    border: 1.5px solid #e2e8f0;
                    border-radius: 8px;
                    padding: 8px 36px 8px 12px;
                    font-size: 13.5px;
                    color: #94a3b8;
                    text-align: left;
                    min-height: 22px;
                }
            )");
            selectedSlot.clear();
            sidePanel->resetToDefault();
            sidePanel->setVisible(false);
            dialog.setMinimumHeight(0);
            dialog.setFixedWidth(540);
        }
        dialog.adjustSize();
    };

    for (int i = 0; i < 5; ++i) {
        connect(symptomCombos[i], qOverload<int>(&QComboBox::currentIndexChanged), &dialog, updateSpecialtyAndDoctors);
    }

    connect(doctor, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, [this, doctor, feeValue, dateBtn, sidePanel, &selectedDate, &selectedSlot, &dialog, targetedDate]() {
        Doctor *selected = mainWindow->findDoctor(UIHelper::ss(doctor->currentData().toString()));
        if (selected) {
            feeValue->setText(QLocale(QLocale::Vietnamese).toString(selected->getFee(), 'f', 0) + " ₫");
            dateBtn->setEnabled(true);
            if (targetedDate.isValid()) {
                selectedDate = targetedDate;
                selectedSlot.clear();
                dateBtn->setText(targetedDate.toString("dd/MM/yyyy"));
                dateBtn->setStyleSheet(R"(
                    QPushButton {
                        background-color: #ffffff;
                        border: 1.5px solid #0d9488;
                        border-radius: 8px;
                        padding: 8px 36px 8px 12px;
                        font-size: 13.5px;
                        color: #0f172a;
                        font-weight: 700;
                        text-align: left;
                        min-height: 22px;
                    }
                    QPushButton:hover {
                        background-color: #ffffff;
                        border: 1.5px solid #0d9488;
                    }
                )");
            } else {
                selectedDate = QDate();
                selectedSlot.clear();
                dateBtn->setText("Chọn ngày khám");
                dateBtn->setStyleSheet(R"(
                    QPushButton {
                        background-color: #f8fafc;
                        border: 1.5px solid #cbd5e1;
                        border-radius: 8px;
                        padding: 8px 36px 8px 12px;
                        font-size: 13.5px;
                        color: #94a3b8;
                        text-align: left;
                        min-height: 22px;
                    }
                    QPushButton:hover:enabled {
                        border: 1.5px solid #0d9488;
                        background-color: #ffffff;
                    }
                )");
            }
        } else {
            feeValue->setText("—");
            dateBtn->setEnabled(false);
            selectedDate = targetedDate.isValid() ? targetedDate : QDate();
            selectedSlot.clear();
            dateBtn->setText(targetedDate.isValid() ? targetedDate.toString("dd/MM/yyyy") : "Chọn ngày khám");
            dateBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: #f1f5f9;
                    border: 1.5px solid #e2e8f0;
                    border-radius: 8px;
                    padding: 8px 36px 8px 12px;
                    font-size: 13.5px;
                    color: #94a3b8;
                    text-align: left;
                    min-height: 22px;
                }
            )");
        }

        sidePanel->resetToDefault();
        sidePanel->setVisible(false);
        dialog.setMinimumHeight(0);
        dialog.setFixedWidth(540);
        dialog.adjustSize();
    });

    auto *btnBox = new QHBoxLayout;
    btnBox->setContentsMargins(0, 10, 0, 0);
    btnBox->setSpacing(10);
    btnBox->addStretch();

    auto *cancelBtn = new QPushButton("Hủy bỏ");
    cancelBtn->setAutoDefault(false);
    cancelBtn->setDefault(false);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet("QPushButton { background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 8px; padding: 9px 20px; font-size: 13px; font-weight: 600; color: #475569; } QPushButton:hover { background: #e2e8f0; color: #1e293b; }");
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    auto *confirmBtn = new QPushButton("＋  Xác nhận đặt lịch");
    confirmBtn->setAutoDefault(false);
    confirmBtn->setDefault(false);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    confirmBtn->setStyleSheet("QPushButton { background: #0d9488; border: none; border-radius: 8px; padding: 9px 22px; font-size: 13px; font-weight: 700; color: white; } QPushButton:hover { background: #0f766e; }");

    connect(confirmBtn, &QPushButton::clicked, &dialog, [this, &dialog, patient, symptomCombos, doctor, &selectedDate, &selectedSlot]() {
        if (patient->currentData().toString().isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng chọn hồ sơ bệnh nhân.");
            return;
        }

        QStringList symList;
        for (int i = 0; i < 5; ++i) {
            QString sid = symptomCombos[i]->currentData().toString();
            if (!sid.isEmpty() && !symList.contains(sid)) {
                symList.append(sid);
            }
        }

        if (symList.isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng chọn ít nhất một triệu chứng khám.");
            return;
        }

        if (doctor->currentData().toString().isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng chọn bác sĩ phụ trách.");
            return;
        }

        if (!selectedDate.isValid() || selectedSlot.isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng bấm vào icon lịch để chọn ngày và khung giờ khám khả dụng.");
            return;
        }

        QTime slotTime = QTime::fromString(selectedSlot, "HH:mm");
        QDateTime bookingDT(selectedDate, slotTime);
        std::string pId = UIHelper::ss(patient->currentData().toString());
        std::string dId = UIHelper::ss(doctor->currentData().toString());

        try {
            AppointmentController::validateBooking(
                mainWindow->getAppointments(),
                mainWindow->getSymptomPatients(),
                pId,
                dId,
                bookingDT,
                "",
                true,
                "Config/Lichnghile.txt",
                mainWindow->getDoctors(),
                mainWindow->getPatients()
            );
        } catch (const ValidationException &e) {
            UIHelper::showWarning(&dialog, "Không thể đặt lịch", QString::fromUtf8(e.what()));
            return;
        }

        Date targetTime = Date::loadFromString(UIHelper::ss(bookingDT.toString("yyyy-MM-dd HH:mm")));
        Doctor *doc = mainWindow->findDoctor(dId);
        std::string specId = doc ? doc->getSpId() : "";

        auto *appointments = mainWindow->getAppointments();
        auto *symptomPatients = mainWindow->getSymptomPatients();
        auto *symptomPatientDetails = mainWindow->getSymptomPatientDetails();
        std::string nextSpId = SymptomPatient::generateNextId(symptomPatients);
        std::string newApptId = AppointmentController::generateNextId(appointments);

        if (symptomPatients) {
            symptomPatients->insert(SymptomPatient(nextSpId, pId, specId, Date::now()));
        }
        if (symptomPatientDetails) {
            for (const QString &s : symList) {
                std::string sId = UIHelper::ss(s.trimmed());
                if (!sId.empty()) {
                    symptomPatientDetails->insert(SymptomPatientDetail(nextSpId, sId));
                }
            }
        }
        if (appointments) {
            appointments->insert(Appointment(newApptId, nextSpId, dId, targetTime, 0, -1.0));
        }

        if (!mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal)) {
            AppointmentController::rollbackInsertedAppointments(
                appointments, symptomPatients, symptomPatientDetails,
                {newApptId}, {nextSpId}, {}
            );
            return;
        }
        UIHelper::showSuccess(&dialog, "Thành công", "Lịch khám đã được ghi nhận và lưu thành công.");
        mainWindow->refreshPage(4);
        mainWindow->refreshPage(0);
        mainWindow->refreshPage(1);
        mainWindow->refreshPage(5);
        dialog.accept();
    });

    btnBox->addWidget(cancelBtn);
    btnBox->addWidget(confirmBtn);
    leftLayout->addStretch(1);
    leftLayout->addLayout(btnBox);

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, &dialog, [&dialog]() { dialog.setFocus(); });

    return (dialog.exec() == QDialog::Accepted);
}

namespace {

class StepperComboStyle : public QProxyStyle {
public:
    QWidget *stepperBox = nullptr;
    explicit StepperComboStyle(QWidget *sb, QStyle *baseStyle = nullptr)
        : QProxyStyle(baseStyle), stepperBox(sb) {}

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                         SubControl sc, const QWidget *widget) const override {
        if (cc == CC_ComboBox && sc == SC_ComboBoxListBoxPopup && stepperBox && widget) {
            QRect baseRect = QProxyStyle::subControlRect(cc, opt, sc, widget);
            QPoint sbGlobal = stepperBox->mapToGlobal(QPoint(0, 0));
            QPoint wGlobal = widget->mapToGlobal(QPoint(0, 0));
            int xOffset = sbGlobal.x() - wGlobal.x();
            int targetWidth = stepperBox->width();
            if (targetWidth < 120) targetWidth = 140;

            int stepperBottom = sbGlobal.y() + stepperBox->height();
            int widgetBottom = wGlobal.y() + widget->height();
            int dy = (stepperBottom - widgetBottom) + 2;

            return QRect(xOffset, baseRect.y() + dy, targetWidth, baseRect.height());
        }
        return QProxyStyle::subControlRect(cc, opt, sc, widget);
    }
};

class StepperComboBox : public QComboBox {
public:
    QWidget *stepperBox = nullptr;
    StepperComboStyle *comboStyle = nullptr;

    explicit StepperComboBox(QWidget *parent = nullptr) : QComboBox(parent) {
        setView(new QListView(this));
        if (view()) {
            view()->setTextElideMode(Qt::ElideNone);
            view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    ~StepperComboBox() override = default;

    void showPopup() override {
        if (stepperBox && !comboStyle) {
            comboStyle = new StepperComboStyle(stepperBox);
            comboStyle->setParent(this);
            setStyle(comboStyle);
        } else if (comboStyle) {
            comboStyle->stepperBox = stepperBox;
        }
        QComboBox::showPopup();

        if (view()) {
            view()->scrollToTop();
            if (auto *sb = view()->verticalScrollBar()) {
                sb->setValue(0);
            }
        }
    }

    void applyWheelRoll(int delta) {
        int m = currentData().toInt();
        if (delta > 0) {
            m = (m == 12) ? 1 : (m + 1);
        } else {
            m = (m == 1) ? 12 : (m - 1);
        }
        int idx = findData(m);
        if (idx != -1) setCurrentIndex(idx);
    }

    void wheelEvent(QWheelEvent *event) override {
        int delta = event->angleDelta().y();
        if (delta != 0) {
            applyWheelRoll(delta);
            event->accept();
        } else {
            QComboBox::wheelEvent(event);
        }
    }
};

class YearSpinBox : public QSpinBox {
public:
    qint64 lastWheelTime = 0;

    explicit YearSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {
        setRange(1, 9999);
        setButtonSymbols(QAbstractSpinBox::NoButtons);
        setAlignment(Qt::AlignCenter);
        setPrefix("Năm ");
        setFocusPolicy(Qt::StrongFocus);
        setFixedWidth(98);
        setStyleSheet(R"(
            QSpinBox {
                background: transparent;
                border: none;
                font-weight: 700;
                font-size: 13.5px;
                color: #0f172a;
                padding: 2px 4px;
            }
            QSpinBox::up-button, QSpinBox::down-button {
                width: 0px;
                height: 0px;
                border: none;
            }
        )");
    }

    void applyWheelRoll(int delta, Qt::KeyboardModifiers mods) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 dt = now - lastWheelTime;
        lastWheelTime = now;

        int step = 1;
        if (dt < 35) {
            step = 25;
        } else if (dt < 70) {
            step = 10;
        } else if (dt < 130) {
            step = 3;
        } else {
            step = 1;
        }

        if (mods & Qt::ShiftModifier) {
            step = qMax(step, 10);
        }
        if (mods & Qt::ControlModifier) {
            step = qMax(step, 100);
        }

        if (delta > 0) {
            setValue(value() + step);
        } else {
            setValue(qMax(1, value() - step));
        }
    }

    void wheelEvent(QWheelEvent *event) override {
        int delta = event->angleDelta().y();
        if (delta != 0) {
            applyWheelRoll(delta, event->modifiers());
            event->accept();
        } else {
            QSpinBox::wheelEvent(event);
        }
    }
};

class YearWheelFilter : public QObject {
public:
    YearSpinBox *target = nullptr;
    YearWheelFilter(YearSpinBox *t, QObject *parent = nullptr) : QObject(parent), target(t) {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Wheel && target) {
            auto *we = static_cast<QWheelEvent*>(event);
            target->applyWheelRoll(we->angleDelta().y(), we->modifiers());
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
};

class MonthWheelFilter : public QObject {
public:
    StepperComboBox *target = nullptr;
    MonthWheelFilter(StepperComboBox *t, QObject *parent = nullptr) : QObject(parent), target(t) {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Wheel && target) {
            auto *we = static_cast<QWheelEvent*>(event);
            target->applyWheelRoll(we->angleDelta().y());
            return true;
        }
        return QObject::eventFilter(obj, event);
    }
};

class AppointmentRowClickFilter : public QObject {
public:
    Appointment *appointment = nullptr;
    AppointmentView *view = nullptr;
    std::function<void()> onClosed = nullptr;
    AppointmentRowClickFilter(Appointment *a, AppointmentView *v, std::function<void()> callback = nullptr, QObject *parent = nullptr)
        : QObject(parent), appointment(a), view(v), onClosed(std::move(callback)) {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && view && appointment) {
                auto *v = view;
                auto *a = appointment;
                auto cb = onClosed;
                QWidget *ctx = qobject_cast<QWidget*>(obj) ? qobject_cast<QWidget*>(obj)->window() : nullptr;
                v->showAppointmentDetailDialog(a);
                if (cb) {
                    if (ctx) {
                        QTimer::singleShot(0, ctx, [cb]() { cb(); });
                    } else {
                        cb();
                    }
                }
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

}

QWidget *AppointmentView::createDashboardWidget(QWidget *parent) {
    if (!mainWindow) return new QWidget(parent);

    auto *w = new QWidget(parent);
    w->setCursor(Qt::ArrowCursor);
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(28, 20, 28, 20);
    layout->setSpacing(16);

    auto *h = new QVBoxLayout;
    h->setSpacing(4);
    int hour = QTime::currentTime().hour();
    QString timeGreeting = (hour >= 5 && hour < 12) ? "Chào buổi sáng" : (hour >= 12 && hour < 18) ? "Chào buổi chiều" : "Chào buổi tối";
    auto *mgr = mainWindow->getCurrentManager();
    QString managerName = mgr ? UIHelper::qs(mgr->getfullName()) : "";
    QString gTitle = managerName.isEmpty() ? QStringLiteral("%1 👋").arg(timeGreeting)
                                          : QStringLiteral("%1, %2 👋").arg(timeGreeting, managerName);
    QString gSub = QStringLiteral("Hôm nay là %1 · Chúc bạn một ngày làm việc hiệu quả tại Phòng khám An Tâm.")
                      .arg(QDate::currentDate().toString("dd/MM/yyyy"));

    auto *greetingTitle = new QLabel(gTitle);
    greetingTitle->setObjectName("pageTitle");
    auto *greetingSub = new QLabel(gSub);
    greetingSub->setObjectName("pageSubtitle");
    h->addWidget(greetingTitle);
    h->addWidget(greetingSub);
    layout->addLayout(h);

    auto *metrics = new QGridLayout;
    metrics->setSpacing(16);
    const QString labels[] = {"BỆNH NHÂN", "BÁC SĨ", "LỊCH KHÁM", "CHƯA KHÁM"};
    const QString colors[] = {"#0d9488", "#0284c7", "#6366f1", "#ea580c"};
    int counts[4] = {
        StatisticController::patientCount(mainWindow->getPatients()),
        StatisticController::doctorCount(mainWindow->getDoctors()),
        StatisticController::appointmentCount(mainWindow->getAppointments()),
        StatisticController::pendingAppointmentCount(mainWindow->getAppointments())
    };

    QLabel *metricValLabels[4] = {nullptr};
    for (int i = 0; i < 4; ++i) {
        auto *c = UIHelper::panel();
        c->setStyleSheet(QStringLiteral(
            "QFrame#panel { "
            "    background: #ffffff; "
            "    border: 1px solid #e2e8f0; "
            "    border-left: 5px solid %1; "
            "    border-radius: 12px; "
            "}").arg(colors[i]));

        auto *v = new QVBoxLayout(c);
        v->setContentsMargins(20, 16, 20, 16);
        v->setSpacing(6);

        auto *val = new QLabel(QString::number(counts[i]));
        metricValLabels[i] = val;
        val->setStyleSheet(QStringLiteral("font-size: 32px; font-weight: 800; color: %1;").arg(colors[i]));

        auto *l = new QLabel(labels[i]);
        l->setStyleSheet("font-size: 11px; font-weight: 700; color: #475569; letter-spacing: 0.8px;");

        v->addWidget(val);
        v->addWidget(l);
        metrics->addWidget(c, 0, i);
    }
    layout->addLayout(metrics);

    auto *banner = UIHelper::panel();
    banner->setStyleSheet(R"(
        QFrame#panel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #f0fdfa, stop:1 #e6fffa);
            border: 1px solid #99f6e4;
            border-radius: 14px;
        }
    )");
    auto *bLayout = new QHBoxLayout(banner);
    bLayout->setContentsMargins(24, 18, 24, 18);

    auto *bText = new QVBoxLayout;
    bText->setSpacing(4);
    auto *bTitle = new QLabel("Chăm sóc chu đáo, bắt đầu từ một lịch hẹn tốt.");
    bTitle->setStyleSheet("font-size: 15px; font-weight: 700; color: #134e4a;");
    auto *bSub = new QLabel("Tạo lịch khám mới và chọn bác sĩ phù hợp với triệu chứng của bệnh nhân.");
    bSub->setStyleSheet("font-size: 12.5px; color: #0f766e;");
    bText->addWidget(bTitle);
    bText->addWidget(bSub);
    bLayout->addLayout(bText, 1);

    auto *bookQuick = UIHelper::button("Đặt lịch khám  →", "primary");
    connect(bookQuick, &QPushButton::clicked, w, [this] {
        if (mainWindow) {
            mainWindow->refreshPage(1);
            mainWindow->setPage(1);
        }
    });
    bLayout->addWidget(bookQuick);
    layout->addWidget(banner);

    auto *card = UIHelper::panel();
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 18, 20, 18);
    cardLayout->setSpacing(14);

    auto *tHeader = new QHBoxLayout;
    auto *tTitle = new QLabel("Lịch hẹn sắp tới");
    tTitle->setStyleSheet("font-size: 15px; font-weight: 700; color: #0f172a;");
    tHeader->addWidget(tTitle);
    tHeader->addStretch();
    cardLayout->addLayout(tHeader);

    auto *table = UIHelper::table({"BỆNH NHÂN", "BÁC SĨ", "THỜI GIAN", "TRẠNG THÁI"});
    table->setShowGrid(false);
    table->setStyleSheet(R"(
        QTableWidget {
            background-color: transparent;
            border: none;
            gridline-color: transparent;
        }
        QHeaderView {
            border: none;
            background: transparent;
        }
        QHeaderView::section {
            background: #f8fafc;
            color: #475569;
            font-weight: 700;
            font-size: 11.5px;
            padding: 10px 14px;
            border: none;
            border-top: 1px solid #e2e8f0;
            border-bottom: 2px solid #e2e8f0;
            border-left: none;
            border-right: none;
        }
        QTableWidget::item {
            background: #ffffff;
            border: none;
            border-bottom: 1px solid #f1f5f9;
            border-left: none;
            border-right: none;
        }
        QTableWidget::item:hover { background: #f0fdfa; }
        QScrollBar:vertical, QScrollBar:horizontal {
            width: 0px;
            height: 0px;
            background: transparent;
            border: none;
        }
    )");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    table->setColumnWidth(2, 145);
    table->setColumnWidth(3, 145);
    table->verticalHeader()->setDefaultSectionSize(54);
    if (table->horizontalHeaderItem(0)) table->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(1)) table->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (table->horizontalHeaderItem(2)) table->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignCenter);
    if (table->horizontalHeaderItem(3)) table->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignCenter);
    cardLayout->addWidget(table, 1);

    auto *pagination = new QHBoxLayout;
    pagination->addStretch();
    auto *prevBtn = UIHelper::button("‹  Trang trước", "secondary");
    auto *nextBtn = UIHelper::button("Trang sau  ›", "secondary");
    auto *pageStatus = new QLabel("Trang 1 / 1");
    pageStatus->setStyleSheet("font-size: 12.5px; font-weight: 700; color: #0f172a; padding: 0 10px;");
    pagination->addWidget(prevBtn);
    pagination->addWidget(pageStatus);
    pagination->addWidget(nextBtn);
    cardLayout->addLayout(pagination);

    layout->addWidget(card, 1);

    struct DashState {
        int pageNumber = 0;
        int sortCol = -1;
        bool sortAsc = true;
        enum { PAGE_SIZE = 8 };
    };
    auto state = std::make_shared<DashState>();

    auto renderRowsPtr = std::make_shared<std::function<void(bool)>>();
    std::weak_ptr<std::function<void(bool)>> weakRender = renderRowsPtr;
    *renderRowsPtr = [=, this](bool preserveScroll) {
        int freshCounts[4] = {
            StatisticController::patientCount(mainWindow ? mainWindow->getPatients() : nullptr),
            StatisticController::doctorCount(mainWindow ? mainWindow->getDoctors() : nullptr),
            StatisticController::appointmentCount(mainWindow ? mainWindow->getAppointments() : nullptr),
            StatisticController::pendingAppointmentCount(mainWindow ? mainWindow->getAppointments() : nullptr)
        };
        for (int i = 0; i < 4; ++i) {
            if (metricValLabels[i]) metricValLabels[i]->setText(QString::number(freshCounts[i]));
        }
        QDate today = QDate::currentDate();
        QList<Appointment*> all = AppointmentController::getUpcomingAppointments(mainWindow->getAppointments(), today);
        if (state->sortCol >= 0) {
            int col = state->sortCol;
            bool asc = state->sortAsc;
            libPBL2::qsort(all, [this, col, asc](const Appointment *a, const Appointment *b) -> bool {
                if (!a || !b) return false;
                int cmp = 0;
                switch (col) {
                    case 0: {
                        Patient *pa = mainWindow ? mainWindow->findPatientForAppointment(*a) : nullptr;
                        Patient *pb = mainWindow ? mainWindow->findPatientForAppointment(*b) : nullptr;
                        cmp = libPBL2::compareVietnameseNames(pa ? pa->getfullName() : "", pb ? pb->getfullName() : "");
                        break;
                    }
                    case 1: {
                        Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                        Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                        cmp = libPBL2::compareVietnameseNames(da ? da->getfullName() : "", db ? db->getfullName() : "");
                        break;
                    }
                    case 2:
                        cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
                        break;
                    case 3: {
                        QString sa = getAppointmentStatusText(*a);
                        QString sb = getAppointmentStatusText(*b);
                        cmp = QString::compare(sa, sb, Qt::CaseInsensitive);
                        break;
                    }
                    default: break;
                }
                if (cmp == 0) cmp = libPBL2::compareNatural(a->getId(), b->getId());
                return asc ? (cmp < 0) : (cmp > 0);
            });
        }
        int vScroll = (preserveScroll && table && table->verticalScrollBar()) ? table->verticalScrollBar()->value() : 0;
        int curRow = preserveScroll ? table->currentRow() : -1;
        if (table) table->setUpdatesEnabled(false);
        table->setRowCount(0);
        if (table->viewport()) {
            table->viewport()->setCursor(Qt::ArrowCursor);
        }
        int total = all.size();
        int totalPages = total == 0 ? 1 : (total + DashState::PAGE_SIZE - 1) / DashState::PAGE_SIZE;
        state->pageNumber = qBound(0, state->pageNumber, totalPages - 1);
        pageStatus->setText(QStringLiteral("Trang %1 / %2").arg(state->pageNumber + 1).arg(totalPages));
        prevBtn->setEnabled(state->pageNumber > 0);
        nextBtn->setEnabled(state->pageNumber < totalPages - 1);

        int start = state->pageNumber * DashState::PAGE_SIZE;
        int end = qMin(start + DashState::PAGE_SIZE, total);

        for (int i = start; i < end; ++i) {
            auto *a = all[i];
            int r = table->rowCount();
            table->insertRow(r);
            table->setRowHeight(r, 54);

            Patient *p = mainWindow->findPatientForAppointment(*a);
            Doctor *d = mainWindow->findDoctor(a->getDoctorId());
            Date ad = a->getDateGet();
            QDate appDate(ad.getYear(), ad.getMonth(), ad.getDay());

            auto *pw = new QWidget;
            auto *pwl = new QVBoxLayout(pw);
            pwl->setContentsMargins(14, 4, 14, 4);
            pwl->setSpacing(2);
            auto *nl = new QLabel(p ? UIHelper::qs(p->getfullName()) : "—");
            nl->setStyleSheet("font-weight: 700; font-size: 13px; color: #0f172a;");
            nl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            QString pSub = p ? QStringLiteral("%1 tuổi (%2)").arg(p->getAge()).arg(p->getGender() ? "Nam" : "Nữ") : "";
            auto *sl = new QLabel(pSub);
            sl->setStyleSheet("font-size: 11.5px; color: #64748b;");
            sl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            pwl->addWidget(nl);
            pwl->addWidget(sl);
            table->setCellWidget(r, 0, pw);

            auto *dw = new QWidget;
            auto *dwl = new QVBoxLayout(dw);
            dwl->setContentsMargins(14, 4, 14, 4);
            dwl->setSpacing(2);
            auto *dnl = new QLabel(d ? QStringLiteral("BS. %1").arg(UIHelper::qs(d->getfullName())) : UIHelper::qs(a->getDoctorId()));
            dnl->setStyleSheet("font-weight: 700; font-size: 13px; color: #0f172a;");
            dnl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            QString dSub = d ? QStringLiteral("%1 · %2").arg(mainWindow->specialty(d->getSpId()), UIHelper::formatRoom(UIHelper::qs(d->getRoomId()))) : "";
            auto *docSub = new QLabel(dSub);
            docSub->setStyleSheet("font-size: 11.5px; color: #0284c7; font-weight: 600;");
            docSub->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            dwl->addWidget(dnl);
            dwl->addWidget(docSub);
            table->setCellWidget(r, 1, dw);

            auto *tw = new QWidget;
            auto *twl = new QVBoxLayout(tw);
            twl->setContentsMargins(6, 4, 6, 4);
            twl->setSpacing(2);
            auto *hourLabel = new QLabel(QString::asprintf("%02d:%02d", ad.getHour(), ad.getMinute()));
            hourLabel->setStyleSheet("font-weight: 700; font-size: 13px; color: #0f172a;");
            hourLabel->setAlignment(Qt::AlignCenter);

            QString dayText;
            if (appDate == today) {
                dayText = QStringLiteral("Hôm nay, %1/%2").arg(ad.getDay()).arg(ad.getMonth());
            } else if (appDate == today.addDays(1)) {
                dayText = QStringLiteral("Ngày mai, %1/%2").arg(ad.getDay()).arg(ad.getMonth());
            } else {
                dayText = QString::asprintf("%02d/%02d/%04d", ad.getDay(), ad.getMonth(), ad.getYear());
            }
            auto *dayLabel = new QLabel(dayText);
            dayLabel->setStyleSheet("font-size: 11.5px; color: #64748b;");
            dayLabel->setAlignment(Qt::AlignCenter);
            twl->addWidget(hourLabel);
            twl->addWidget(dayLabel);
            table->setCellWidget(r, 2, tw);

            Date now = Date::now();
            int st = a->computeEffectiveState(now.getDay(), now.getMonth(), now.getYear(), now.getHour(), now.getMinute());
            QString badgeText = "● Chưa khám";
            QString badgeStyle = "QLabel { background: #fffbeb; color: #d97706; border: 1px solid #fde68a; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px; }";
            if (st == 1) {
                badgeText = "● Đã khám";
                badgeStyle = "QLabel { background: #ecfdf5; color: #059669; border: 1px solid #a7f3d0; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px; }";
            } else if (st == -1) {
                badgeText = "● Trễ hẹn";
                badgeStyle = "QLabel { background: #fff7ed; color: #ea580c; border: 1px solid #ffedd5; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px; }";
            } else if (st == -2) {
                badgeText = "● Đã dời lịch";
                badgeStyle = "QLabel { background: #f1f5f9; color: #64748b; border: 1px solid #e2e8f0; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px; }";
            }
            auto *badge = new QLabel(badgeText);
            badge->setStyleSheet(badgeStyle);
            badge->setAlignment(Qt::AlignCenter);
            badge->setFixedHeight(26);

            auto *bb = new QWidget;
            auto *bbl = new QHBoxLayout(bb);
            bbl->setContentsMargins(6, 0, 6, 0);
            bbl->setAlignment(Qt::AlignCenter);
            bbl->addWidget(badge);
            table->setCellWidget(r, 3, bb);

            auto safeRender = [weakRender]() { if (auto sp = weakRender.lock()) (*sp)(true); };
            auto makeRowClickable = [&](QWidget *widget, Appointment *appt) {
                widget->installEventFilter(new AppointmentRowClickFilter(appt, this, safeRender, widget));
                const auto children = widget->findChildren<QWidget*>();
                for (auto *c : children) {
                    c->installEventFilter(new AppointmentRowClickFilter(appt, this, safeRender, c));
                }
            };
            makeRowClickable(pw, a);
            makeRowClickable(dw, a);
            makeRowClickable(tw, a);
            makeRowClickable(bb, a);
        }
        if (table) table->setUpdatesEnabled(true);
        if (preserveScroll) {
            if (table && table->verticalScrollBar()) table->verticalScrollBar()->setValue(vScroll);
            if (table && curRow >= 0 && curRow < table->rowCount()) table->setCurrentCell(curRow, 0);
            QTimer::singleShot(0, table, [table, vScroll, curRow]() {
                if (table && table->verticalScrollBar()) table->verticalScrollBar()->setValue(qBound(0, vScroll, table->verticalScrollBar()->maximum()));
                if (table && curRow >= 0 && curRow < table->rowCount()) table->setCurrentCell(curRow, 0);
            });
        }
    };
    auto renderRows = [renderRowsPtr](bool preserveScroll = true) { if (*renderRowsPtr) (*renderRowsPtr)(preserveScroll); };

    connect(table, &QTableWidget::cellDoubleClicked, w, [=, this](int r, int) {
        QDate today = QDate::currentDate();
        QList<Appointment*> all = AppointmentController::getUpcomingAppointments(mainWindow->getAppointments(), today);
        if (state->sortCol >= 0) {
            int col = state->sortCol;
            bool asc = state->sortAsc;
            libPBL2::qsort(all, [this, col, asc](const Appointment *a, const Appointment *b) -> bool {
                if (!a || !b) return false;
                int cmp = 0;
                switch (col) {
                    case 0: {
                        Patient *pa = mainWindow ? mainWindow->findPatientForAppointment(*a) : nullptr;
                        Patient *pb = mainWindow ? mainWindow->findPatientForAppointment(*b) : nullptr;
                        cmp = libPBL2::compareVietnameseNames(pa ? pa->getfullName() : "", pb ? pb->getfullName() : "");
                        break;
                    }
                    case 1: {
                        Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                        Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                        cmp = libPBL2::compareVietnameseNames(da ? da->getfullName() : "", db ? db->getfullName() : "");
                        break;
                    }
                    case 2:
                        cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
                        break;
                    case 3: {
                        QString sa = getAppointmentStatusText(*a);
                        QString sb = getAppointmentStatusText(*b);
                        cmp = QString::compare(sa, sb, Qt::CaseInsensitive);
                        break;
                    }
                    default: break;
                }
                if (cmp == 0) cmp = libPBL2::compareNatural(a->getId(), b->getId());
                return asc ? (cmp < 0) : (cmp > 0);
            });
        }
        int idx = state->pageNumber * DashState::PAGE_SIZE + r;
        if (idx >= 0 && idx < static_cast<int>(all.size())) {
            showAppointmentDetailDialog(all[idx]);
            renderRows(true);
        }
    });

    connect(table->horizontalHeader(), &QHeaderView::sectionClicked, w, [state, renderRows](int logicalIndex) {
        if (logicalIndex < 0 || logicalIndex > 3) return;
        if (state->sortCol == logicalIndex) {
            state->sortAsc = !state->sortAsc;
        } else {
            state->sortCol = logicalIndex;
            state->sortAsc = true;
        }
        state->pageNumber = 0;
        renderRows(false);
    });

    connect(prevBtn, &QPushButton::clicked, w, [=]() {
        if (state->pageNumber > 0) {
            state->pageNumber--;
            renderRows(false);
        }
    });
    connect(nextBtn, &QPushButton::clicked, w, [=]() {
        state->pageNumber++;
        renderRows(false);
    });

    auto *refreshTimer = new QTimer(w);
    connect(refreshTimer, &QTimer::timeout, w, [renderRows]() { renderRows(true); });
    refreshTimer->start(30000);

    renderRows(false);
    return w;
}

QWidget *AppointmentView::createBookingWidget(QWidget *parent) {
    if (!mainWindow) return new QWidget(parent);
    auto *scroll = new QScrollArea(parent);
    scroll->setCursor(Qt::ArrowCursor);
    if (scroll->viewport()) {
        scroll->viewport()->setCursor(Qt::ArrowCursor);
    }
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollBar:vertical, QScrollBar:horizontal { width: 0px; height: 0px; background: transparent; border: none; }");

    auto *w = new QWidget;
    w->setCursor(Qt::ArrowCursor);
    w->setStyleSheet("background: transparent;");
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(24, 14, 24, 14);
    layout->setSpacing(10);

    auto *headerLayout = new QHBoxLayout;
    auto *headerTextLayout = new QVBoxLayout;
    headerTextLayout->setSpacing(2);
    auto *heading = new QLabel("Lịch khám bệnh");
    heading->setObjectName("pageTitle");
    auto *subtitle = new QLabel("Theo dõi lịch khám của bệnh nhân theo từng ngày và quản lý đặt lịch hẹn.");
    subtitle->setObjectName("pageSubtitle");
    headerTextLayout->addWidget(heading);
    headerTextLayout->addWidget(subtitle);

    auto *holidayBtn = new QPushButton("🗓   Quản lý lịch nghỉ");
    holidayBtn->setCursor(Qt::PointingHandCursor);
    holidayBtn->setStyleSheet("QPushButton { background-color: #ffffff; color: #0f766e; border: 1.5px solid #0d9488; border-radius: 9px; padding: 8px 18px; font-size: 13px; font-weight: 700; } QPushButton:hover { background-color: #f0fdfa; border-color: #0f766e; }");

    auto *bookBtn = UIHelper::button("＋   Đặt lịch", "primary");
    bookBtn->setStyleSheet(R"(
        QPushButton#primary {
            background-color: #0d9488;
            color: white;
            border: none;
            border-radius: 9px;
            padding: 9px 20px;
            font-size: 13.5px;
            font-weight: 700;
        }
        QPushButton#primary:hover {
            background-color: #0f766e;
        }
        QPushButton#primary:disabled {
            background-color: #cbd5e1;
            color: #94a3b8;
        }
    )");

    headerLayout->addLayout(headerTextLayout, 1);
    headerLayout->addWidget(holidayBtn, 0, Qt::AlignVCenter);
    headerLayout->addWidget(bookBtn, 0, Qt::AlignVCenter);
    layout->addLayout(headerLayout);

    auto *calCard = UIHelper::panel();
    calCard->setFixedHeight(290);
    auto *calCardLayout = new QVBoxLayout(calCard);
    calCardLayout->setContentsMargins(18, 12, 18, 12);
    calCardLayout->setSpacing(8);

    auto *calTopBar = new QHBoxLayout;
    calTopBar->setSpacing(12);

    auto makeStepperGroup = []() {
        auto *g = new QFrame;
        g->setObjectName("stepperGroup");
        g->setFixedHeight(34);
        g->setStyleSheet("QFrame#stepperGroup { background: #ffffff; border: 1.5px solid #cbd5e1; border-radius: 8px; } QFrame#stepperGroup:hover { border-color: #94a3b8; }");
        return g;
    };

    auto makeStepperBtn = [](const QString &text) {
        auto *b = new QPushButton(text);
        b->setFixedSize(26, 26);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(R"(
            QPushButton {
                background: transparent;
                border: none;
                border-radius: 6px;
                font-weight: 800;
                font-size: 15px;
                color: #475569;
            }
            QPushButton:hover {
                background: #f1f5f9;
                color: #0d9488;
            }
            QPushButton:pressed {
                background: #e2e8f0;
            }
        )");
        return b;
    };

    auto *monthGroup = makeStepperGroup();
    auto *monthGroupLayout = new QHBoxLayout(monthGroup);
    monthGroupLayout->setContentsMargins(4, 2, 4, 2);
    monthGroupLayout->setSpacing(2);
    auto *prevMonthBtn = makeStepperBtn("‹");
    auto *monthCombo = new StepperComboBox(monthGroup);
    monthCombo->stepperBox = monthGroup;
    monthCombo->comboStyle = new StepperComboStyle(monthGroup);
    monthCombo->comboStyle->setParent(monthCombo);
    monthCombo->setStyle(monthCombo->comboStyle);
    monthCombo->setFixedWidth(92);
    monthCombo->setMaxVisibleItems(12);
    for (int m = 1; m <= 12; ++m) {
        monthCombo->addItem(QStringLiteral("Tháng %1").arg(m), m);
    }
    monthCombo->setStyleSheet(R"(
        QComboBox {
            combobox-popup: 0;
            background: transparent;
            border: none;
            padding: 2px 4px;
            font-weight: 700;
            font-size: 13.5px;
            color: #0f172a;
        }
        QComboBox::drop-down {
            border: none;
            width: 0px;
        }
        QComboBox QAbstractItemView {
            background-color: #ffffff;
            color: #0f172a;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 4px;
            selection-background-color: #0d9488;
            selection-color: #ffffff;
            outline: none;
        }
        QComboBox QAbstractItemView::item {
            min-height: 22px;
            padding: 3px 8px;
            border-radius: 5px;
            color: #0f172a;
            font-weight: 600;
            font-size: 13px;
        }
        QComboBox QAbstractItemView::item:hover {
            background-color: #f0fdfa;
            color: #0f766e;
        }
        QComboBox QAbstractItemView::item:selected {
            background-color: #0d9488;
            color: #ffffff;
        }
        QScrollBar:vertical {
            width: 5px;
            background: transparent;
            margin: 2px 0px;
        }
        QScrollBar::handle:vertical {
            background: #cbd5e1;
            border-radius: 2px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #94a3b8;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: transparent;
            border: none;
            height: 0px;
        }
    )");
    auto *nextMonthBtn = makeStepperBtn("›");
    monthGroupLayout->addWidget(prevMonthBtn);
    monthGroupLayout->addWidget(monthCombo);
    monthGroupLayout->addWidget(nextMonthBtn);

    auto *yearGroup = makeStepperGroup();
    auto *yearGroupLayout = new QHBoxLayout(yearGroup);
    yearGroupLayout->setContentsMargins(4, 2, 4, 2);
    yearGroupLayout->setSpacing(2);
    auto *prevYearBtn = makeStepperBtn("‹");
    prevYearBtn->setAutoRepeat(true);
    prevYearBtn->setAutoRepeatDelay(250);
    prevYearBtn->setAutoRepeatInterval(50);

    auto *yearSpin = new YearSpinBox(yearGroup);

    auto *nextYearBtn = makeStepperBtn("›");
    nextYearBtn->setAutoRepeat(true);
    nextYearBtn->setAutoRepeatDelay(250);
    nextYearBtn->setAutoRepeatInterval(50);

    yearGroupLayout->addWidget(prevYearBtn);
    yearGroupLayout->addWidget(yearSpin);
    yearGroupLayout->addWidget(nextYearBtn);

    auto *yearWheel = new YearWheelFilter(yearSpin, yearGroup);
    yearGroup->installEventFilter(yearWheel);
    prevYearBtn->installEventFilter(yearWheel);
    nextYearBtn->installEventFilter(yearWheel);

    auto *monthWheel = new MonthWheelFilter(monthCombo, monthGroup);
    monthGroup->installEventFilter(monthWheel);
    prevMonthBtn->installEventFilter(monthWheel);
    nextMonthBtn->installEventFilter(monthWheel);

    auto setComboYear = [yearSpin](int y) {
        yearSpin->setValue(qMax(1, y));
    };

    auto *todayBtn = new QPushButton("Hôm nay");
    todayBtn->setCursor(Qt::PointingHandCursor);
    todayBtn->setFixedHeight(34);
    todayBtn->setStyleSheet("QPushButton { background: #f0fdfa; border: 1.5px solid #99f6e4; border-radius: 8px; color: #0f766e; font-weight: 700; font-size: 12.5px; padding: 4px 14px; } QPushButton:hover { background: #0d9488; color: white; border-color: #0d9488; }");

    calTopBar->addWidget(monthGroup);
    calTopBar->addWidget(yearGroup);
    calTopBar->addWidget(todayBtn);
    calTopBar->addStretch();
    calCardLayout->addLayout(calTopBar);

    auto *cal = new QCalendarWidget;
    cal->setGridVisible(false);
    cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    cal->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    cal->setNavigationBarVisible(false);
    cal->setFirstDayOfWeek(Qt::Monday);
    cal->setDateRange(QDate(1, 1, 1), QDate(9999, 12, 31));

    QTextCharFormat weekdayHeaderFmt;
    weekdayHeaderFmt.setForeground(QColor("#475569"));
    weekdayHeaderFmt.setFontWeight(QFont::DemiBold);
    for (int dow = 1; dow <= 7; ++dow) {
        cal->setWeekdayTextFormat(static_cast<Qt::DayOfWeek>(dow), weekdayHeaderFmt);
    }

    cal->setStyleSheet(R"(
        QCalendarWidget QWidget#qt_calendar_calendarview {
            background-color: #ffffff;
            selection-background-color: #0d9488;
            selection-color: white;
            font-size: 13px;
            font-weight: 500;
            outline: none;
        }
        QCalendarWidget QTableView {
            alternate-background-color: #ffffff;
            border: none;
        }
        QCalendarWidget QTableView QHeaderView::section {
            background-color: #f8fafc;
            color: #475569;
            font-weight: 700;
            font-size: 11.5px;
            border: none;
            border-bottom: 1.5px solid #e2e8f0;
            padding: 4px 0px;
        }
    )");
    calCardLayout->addWidget(cal);
    layout->addWidget(calCard);

    auto *dayCard = UIHelper::panel();
    dayCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    dayCard->setMinimumHeight(240);
    dayCard->setStyleSheet("QFrame#panel { background: #ffffff; border: 1.5px solid #cbd5e1; border-radius: 12px; }");
    auto *dayCardLayout = new QVBoxLayout(dayCard);
    dayCardLayout->setContentsMargins(18, 14, 18, 14);
    dayCardLayout->setSpacing(8);

    auto *dayHeaderLayout = new QHBoxLayout;
    auto *dayTitleCol = new QVBoxLayout;
    dayTitleCol->setSpacing(2);
    auto *dayTitle = new QLabel;
    dayTitle->setStyleSheet("font-size: 17px; font-weight: 700; color: #102f43;");
    dayTitleCol->addWidget(dayTitle);

    auto *dayCountBadge = new QLabel;
    dayCountBadge->setStyleSheet("background: #f0fdf4; color: #15803d; border: 1px solid #bbf7d0; font-size: 12px; font-weight: 700; border-radius: 12px; padding: 3px 12px;");
    dayHeaderLayout->addLayout(dayTitleCol, 1);
    dayHeaderLayout->addWidget(dayCountBadge, 0, Qt::AlignVCenter);
    dayCardLayout->addLayout(dayHeaderLayout);

    auto *t = UIHelper::table({"BỆNH NHÂN", "BÁC SĨ", "THỜI GIAN", "TRẠNG THÁI"});
    t->setObjectName("dayPatientTable");
    t->setShowGrid(false);
    t->setStyleSheet(R"(
        #dayPatientTable {
            background-color: transparent;
            border: none;
            gridline-color: transparent;
        }
        #dayPatientTable QHeaderView {
            border: none;
            background: transparent;
        }
        #dayPatientTable QHeaderView::section {
            background: #f8fafc;
            color: #475569;
            font-weight: 700;
            font-size: 11.5px;
            padding: 10px 14px;
            border: none;
            border-top: 1px solid #e2e8f0;
            border-bottom: 2px solid #e2e8f0;
            border-left: none;
            border-right: none;
        }
        #dayPatientTable::item {
            background: #ffffff;
            border: none;
            border-bottom: 1px solid #f1f5f9;
            border-left: none;
            border-right: none;
        }
        #dayPatientTable::item:hover { background: #f0fdfa; }
    )");
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    t->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    t->setColumnWidth(2, 145);
    t->setColumnWidth(3, 145);
    t->verticalHeader()->setDefaultSectionSize(54);
    if (t->horizontalHeaderItem(0)) t->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (t->horizontalHeaderItem(1)) t->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (t->horizontalHeaderItem(2)) t->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignCenter);
    if (t->horizontalHeaderItem(3)) t->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignCenter);
    dayCardLayout->addWidget(t, 1);

    auto *emptyWidget = new QWidget;
    emptyWidget->setObjectName("emptyCard");
    emptyWidget->setStyleSheet("#emptyCard { background-color: #f8fafc; border: 1.5px dashed #cbd5e1; border-radius: 10px; }");
    auto *emptyLayout = new QVBoxLayout(emptyWidget);
    emptyLayout->setContentsMargins(20, 30, 20, 30);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(10);
    auto *emptyIcon = new QLabel("📋");
    emptyIcon->setStyleSheet("font-size: 38px; color: #94a3b8;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    auto *emptyTitle = new QLabel("Chưa có lịch khám nào trong ngày này");
    emptyTitle->setStyleSheet("font-size: 15px; font-weight: 700; color: #475569;");
    emptyTitle->setAlignment(Qt::AlignCenter);
    auto *emptySub = new QLabel("Bấm nút 'Đặt lịch' ở góc trên để tạo lịch hẹn cho bệnh nhân.");
    emptySub->setStyleSheet("font-size: 12.5px; color: #94a3b8;");
    emptySub->setAlignment(Qt::AlignCenter);
    auto *quickBookBtn = new QPushButton("＋  Đặt lịch khám cho ngày này");
    quickBookBtn->setCursor(Qt::PointingHandCursor);
    quickBookBtn->setStyleSheet("QPushButton { background-color: #0d9488; color: white; border: none; border-radius: 8px; padding: 9px 20px; font-weight: 700; font-size: 13px; } QPushButton:hover { background-color: #0f766e; }");
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptySub);
    emptyLayout->addWidget(quickBookBtn, 0, Qt::AlignCenter);
    dayCardLayout->addWidget(emptyWidget, 1);

    auto *pagerRow = new QHBoxLayout;
    pagerRow->addStretch();
    auto *prevBtn = UIHelper::button("‹  Trang trước", "secondary");
    auto *nextBtn = UIHelper::button("Trang sau  ›", "secondary");
    auto *pageStatus = new QLabel("Trang 1 / 1");
    pageStatus->setStyleSheet("font-size: 13px; font-weight: 700; color: #0f172a; padding: 0 12px;");
    pagerRow->addWidget(prevBtn);
    pagerRow->addWidget(pageStatus);
    pagerRow->addWidget(nextBtn);
    dayCardLayout->addLayout(pagerRow);

    layout->addWidget(dayCard, 1);
    scroll->setWidget(w);

    struct BookingPageState {
        QDate selectedDate;
        QList<Appointment*> appointments;
        int pageNumber = 0;
        int sortCol = -1;
        bool sortAsc = true;
        enum { PAGE_SIZE = 8 };
    };
    auto state = std::make_shared<BookingPageState>();
    state->selectedDate = QDate::currentDate();

    auto reloadDayViewPtr = std::make_shared<std::function<void(bool)>>();
    std::weak_ptr<std::function<void(bool)>> weakReload = reloadDayViewPtr;
    *reloadDayViewPtr = [=, this](bool preserveScroll) {
        highlightCalendarAppointments(cal, cal->yearShown(), cal->monthShown());
        state->appointments = getAppointmentsForDate(state->selectedDate);
        if (state->sortCol >= 0) {
            int col = state->sortCol;
            bool asc = state->sortAsc;
            libPBL2::qsort(state->appointments, [this, col, asc](const Appointment *a, const Appointment *b) -> bool {
                if (!a || !b) return false;
                int cmp = 0;
                switch (col) {
                    case 0: {
                        Patient *pa = mainWindow ? mainWindow->findPatientForAppointment(*a) : nullptr;
                        Patient *pb = mainWindow ? mainWindow->findPatientForAppointment(*b) : nullptr;
                        cmp = libPBL2::compareVietnameseNames(pa ? pa->getfullName() : "", pb ? pb->getfullName() : "");
                        break;
                    }
                    case 1: {
                        Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                        Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                        cmp = libPBL2::compareVietnameseNames(da ? da->getfullName() : "", db ? db->getfullName() : "");
                        break;
                    }
                    case 2:
                        cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
                        break;
                    case 3: {
                        QString sa = getAppointmentStatusText(*a);
                        QString sb = getAppointmentStatusText(*b);
                        cmp = QString::compare(sa, sb, Qt::CaseInsensitive);
                        break;
                    }
                    default: break;
                }
                if (cmp == 0) cmp = libPBL2::compareNatural(a->getId(), b->getId());
                return asc ? (cmp < 0) : (cmp > 0);
            });
        }
        int total = state->appointments.size();

        static const char *weekdayNames[] = {"", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy", "Chủ Nhật"};
        int dow = state->selectedDate.dayOfWeek();
        QString dayName = (dow >= 1 && dow <= 7) ? weekdayNames[dow] : "";

        bool isHol = mainWindow->isHoliday(state->selectedDate);
        bool isPast = (state->selectedDate < QDate::currentDate());

        bookBtn->setEnabled(true);
        bookBtn->setCursor(Qt::PointingHandCursor);

        if (isHol) {
            QString holNote = mainWindow->holidayNote(state->selectedDate);
            dayTitle->setText(QStringLiteral("Lịch khám %1, ngày %2 - Nghỉ lễ: %3").arg(dayName, state->selectedDate.toString("dd/MM/yyyy"), holNote));
            dayCountBadge->setText(QStringLiteral("🗓 Nghỉ lễ: %1").arg(holNote));
            dayCountBadge->setStyleSheet("background: #fee2e2; color: #b91c1c; border: 1px solid #fca5a5; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;");
        } else {
            dayTitle->setText(QStringLiteral("Lịch khám %1, ngày %2").arg(dayName, state->selectedDate.toString("dd/MM/yyyy")));
            dayCountBadge->setText(QStringLiteral("%1 bệnh nhân").arg(total));
            dayCountBadge->setStyleSheet("background: #f1f5f9; color: #475569; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;");
        }

        if (total == 0) {
            t->hide();
            prevBtn->hide();
            nextBtn->hide();
            pageStatus->hide();
            emptyWidget->show();
            if (isHol) {
                emptyIcon->setText("🗓");
                emptyTitle->setText(QStringLiteral("Phòng khám nghỉ lễ vào ngày này (%1)").arg(mainWindow->holidayNote(state->selectedDate)));
                emptySub->setText("Phòng khám đóng cửa trong các ngày nghỉ lễ. Vui lòng chọn ngày làm việc khác.");
                quickBookBtn->hide();
            } else if (isPast) {
                emptyIcon->setText("⏳");
                emptyTitle->setText("Không thể đặt lịch cho ngày trước thời gian hiện tại");
                emptySub->setText("Thời gian đã qua, phòng khám không tiếp nhận đặt lịch mới cho ngày này.");
                quickBookBtn->hide();
            } else {
                emptyIcon->setText("📋");
                emptyTitle->setText("Chưa có lịch khám nào trong ngày này");
                emptySub->setText("Bấm nút 'Đặt lịch' ở góc trên để tạo lịch hẹn cho bệnh nhân.");
                quickBookBtn->show();
            }
            return;
        }

        emptyWidget->hide();
        t->show();
        prevBtn->show();
        nextBtn->show();
        pageStatus->show();

        int totalPages = (total + BookingPageState::PAGE_SIZE - 1) / BookingPageState::PAGE_SIZE;
        state->pageNumber = qBound(0, state->pageNumber, totalPages - 1);
        pageStatus->setText(QStringLiteral("Trang %1 / %2").arg(state->pageNumber + 1).arg(totalPages));
        prevBtn->setEnabled(state->pageNumber > 0);
        nextBtn->setEnabled(state->pageNumber < totalPages - 1);

        int pageScrollV = (preserveScroll && scroll && scroll->verticalScrollBar()) ? scroll->verticalScrollBar()->value() : 0;
        int tableScrollV = (preserveScroll && t && t->verticalScrollBar()) ? t->verticalScrollBar()->value() : 0;
        int curRow = preserveScroll ? t->currentRow() : -1;
        if (t) t->setUpdatesEnabled(false);
        if (scroll) scroll->setUpdatesEnabled(false);
        t->setRowCount(0);
        if (t->viewport()) {
            t->viewport()->setCursor(Qt::ArrowCursor);
        }
        int start = state->pageNumber * BookingPageState::PAGE_SIZE;
        int end = qMin(start + BookingPageState::PAGE_SIZE, total);

        for (int i = start; i < end; ++i) {
            auto *a = state->appointments[i];
            int r = t->rowCount();
            t->insertRow(r);
            t->setRowHeight(r, 54);

            Patient *p = mainWindow->findPatientForAppointment(*a);
            Doctor *d = mainWindow->findDoctor(a->getDoctorId());
            Date ad = a->getDateGet();

            auto *pw = new QWidget;
            auto *pwl = new QVBoxLayout(pw);
            pwl->setContentsMargins(14, 4, 14, 4);
            pwl->setSpacing(2);
            auto *nl = new QLabel(p ? UIHelper::qs(p->getfullName()) : "—");
            nl->setStyleSheet("font-weight: 700; font-size: 13px; color: #0f172a;");
            nl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            QString pSub = p ? QStringLiteral("%1 tuổi (%2)").arg(p->getAge()).arg(p->getGender() ? "Nam" : "Nữ") : "";
            auto *sl = new QLabel(pSub);
            sl->setStyleSheet("font-size: 11.5px; color: #64748b;");
            sl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            pwl->addWidget(nl);
            pwl->addWidget(sl);
            t->setCellWidget(r, 0, pw);

            auto *dw = new QWidget;
            auto *dwl = new QVBoxLayout(dw);
            dwl->setContentsMargins(14, 4, 14, 4);
            dwl->setSpacing(2);
            auto *dnl = new QLabel(d ? QStringLiteral("BS. %1").arg(UIHelper::qs(d->getfullName())) : UIHelper::qs(a->getDoctorId()));
            dnl->setStyleSheet("font-weight: 700; font-size: 13px; color: #0f172a;");
            dnl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            QString dSub = d ? QStringLiteral("%1 · %2").arg(mainWindow->specialty(d->getSpId()), UIHelper::formatRoom(UIHelper::qs(d->getRoomId()))) : "";
            auto *docSub = new QLabel(dSub);
            docSub->setStyleSheet("font-size: 11.5px; color: #0284c7; font-weight: 600;");
            docSub->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            dwl->addWidget(dnl);
            dwl->addWidget(docSub);
            t->setCellWidget(r, 1, dw);

            auto *tw = new QWidget;
            auto *twl = new QVBoxLayout(tw);
            twl->setContentsMargins(6, 4, 6, 4);
            twl->setSpacing(2);
            auto *hourLabel = new QLabel(QString::asprintf("%02d:%02d", ad.getHour(), ad.getMinute()));
            hourLabel->setStyleSheet("font-weight: 700; font-size: 13px; color: #0f172a;");
            hourLabel->setAlignment(Qt::AlignCenter);
            auto *dayLabel = new QLabel(state->selectedDate == QDate::currentDate() ? QStringLiteral("Hôm nay, %1/%2").arg(ad.getDay()).arg(ad.getMonth()) : state->selectedDate.toString("dd/MM/yyyy"));
            dayLabel->setStyleSheet("font-size: 11.5px; color: #64748b;");
            dayLabel->setAlignment(Qt::AlignCenter);
            twl->addWidget(hourLabel);
            twl->addWidget(dayLabel);
            t->setCellWidget(r, 2, tw);

            Date now = Date::now();
            int st = a->computeEffectiveState(now.getDay(), now.getMonth(), now.getYear(), now.getHour(), now.getMinute());
            QString badgeText = "● Chưa khám";
            QString badgeStyle = "background: #fffbeb; color: #d97706; border: 1px solid #fde68a; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;";
            if (st == 1) {
                badgeText = "● Đã khám";
                badgeStyle = "background: #ecfdf5; color: #059669; border: 1px solid #a7f3d0; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;";
            } else if (st == -1) {
                badgeText = "● Trễ hẹn";
                badgeStyle = "background: #fff7ed; color: #ea580c; border: 1px solid #ffedd5; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;";
            } else if (st == -2) {
                badgeText = "● Đã dời lịch";
                badgeStyle = "background: #f1f5f9; color: #64748b; border: 1px solid #e2e8f0; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;";
            }
            auto *badge = new QLabel(badgeText);
            badge->setStyleSheet(badgeStyle);
            badge->setAlignment(Qt::AlignCenter);
            badge->setFixedHeight(26);

            auto *bb = new QWidget;
            auto *bbl = new QHBoxLayout(bb);
            bbl->setContentsMargins(6, 0, 6, 0);
            bbl->setAlignment(Qt::AlignCenter);
            bbl->addWidget(badge);
            t->setCellWidget(r, 3, bb);

            auto safeReload = [weakReload]() { if (auto sp = weakReload.lock()) (*sp)(true); };
            auto makeRowClickable = [&](QWidget *widget, Appointment *appt) {
                widget->installEventFilter(new AppointmentRowClickFilter(appt, this, safeReload, widget));
                const auto children = widget->findChildren<QWidget*>();
                for (auto *c : children) {
                    c->installEventFilter(new AppointmentRowClickFilter(appt, this, safeReload, c));
                }
            };
            makeRowClickable(pw, a);
            makeRowClickable(dw, a);
            makeRowClickable(tw, a);
            makeRowClickable(bb, a);
        }
        if (t) t->setUpdatesEnabled(true);
        if (scroll) scroll->setUpdatesEnabled(true);
        if (preserveScroll) {
            if (scroll && scroll->verticalScrollBar()) scroll->verticalScrollBar()->setValue(pageScrollV);
            if (t && t->verticalScrollBar()) t->verticalScrollBar()->setValue(tableScrollV);
            if (curRow >= 0 && curRow < t->rowCount()) t->setCurrentCell(curRow, 0);
            QTimer::singleShot(0, scroll, [scroll, t, pageScrollV, tableScrollV, curRow]() {
                if (scroll && scroll->verticalScrollBar()) scroll->verticalScrollBar()->setValue(qBound(0, pageScrollV, scroll->verticalScrollBar()->maximum()));
                if (t && t->verticalScrollBar()) t->verticalScrollBar()->setValue(qBound(0, tableScrollV, t->verticalScrollBar()->maximum()));
                if (t && curRow >= 0 && curRow < t->rowCount()) t->setCurrentCell(curRow, 0);
            });
        }
    };
    auto reloadDayView = [reloadDayViewPtr](bool preserveScroll = true) { if (*reloadDayViewPtr) (*reloadDayViewPtr)(preserveScroll); };

    connect(t, &QTableWidget::cellDoubleClicked, w, [=, this](int r, int) {
        int idx = state->pageNumber * BookingPageState::PAGE_SIZE + r;
        if (idx >= 0 && idx < static_cast<int>(state->appointments.size())) {
            showAppointmentDetailDialog(state->appointments[idx]);
            reloadDayView(true);
        }
    });

    connect(holidayBtn, &QPushButton::clicked, w, [=, this]() {
        showHolidayManagementDialog(w);
        reloadDayView(true);
    });

    connect(cal, &QCalendarWidget::clicked, w, [=](const QDate &d) {
        state->selectedDate = d;
        state->pageNumber = 0;
        reloadDayView(false);
    });

    QDate cur = QDate::currentDate();
    monthCombo->setCurrentIndex(cur.month() - 1);
    setComboYear(cur.year());
    cal->setSelectedDate(cur);
    reloadDayView(false);

    connect(t->horizontalHeader(), &QHeaderView::sectionClicked, w, [state, reloadDayView](int logicalIndex) {
        if (logicalIndex < 0 || logicalIndex > 3) return;
        if (state->sortCol == logicalIndex) {
            state->sortAsc = !state->sortAsc;
        } else {
            state->sortCol = logicalIndex;
            state->sortAsc = true;
        }
        state->pageNumber = 0;
        reloadDayView(false);
    });

    connect(prevBtn, &QPushButton::clicked, w, [=]() {
        if (state->pageNumber > 0) {
            state->pageNumber--;
            reloadDayView(false);
        }
    });
    connect(nextBtn, &QPushButton::clicked, w, [=]() {
        int totalPages = state->appointments.empty() ? 1 : (static_cast<int>(state->appointments.size()) + 7) / 8;
        if (state->pageNumber < totalPages - 1) {
            state->pageNumber++;
            reloadDayView(false);
        }
    });

    auto onPickerChanged = [=, this]() {
        int m = monthCombo->currentData().toInt();
        int y = yearSpin->value();
        cal->blockSignals(true);
        cal->setCurrentPage(qMax(1, y), m);
        cal->blockSignals(false);
        highlightCalendarAppointments(cal, y, m);
    };
    connect(monthCombo, qOverload<int>(&QComboBox::currentIndexChanged), w, onPickerChanged);
    connect(yearSpin, qOverload<int>(&QSpinBox::valueChanged), w, onPickerChanged);

    connect(prevMonthBtn, &QPushButton::clicked, w, [=]() {
        int m = monthCombo->currentData().toInt();
        int y = yearSpin->value();
        if (m == 1) {
            if (y > 1) {
                m = 12;
                y--;
            }
        } else {
            m--;
        }
        {
            QSignalBlocker blocker(yearSpin);
            setComboYear(y);
        }
        monthCombo->setCurrentIndex(m - 1);
    });
    connect(nextMonthBtn, &QPushButton::clicked, w, [=]() {
        int m = monthCombo->currentData().toInt();
        int y = yearSpin->value();
        if (m == 12) {
            if (y < 9999) {
                m = 1;
                y++;
            }
        } else {
            m++;
        }
        {
            QSignalBlocker blocker(yearSpin);
            setComboYear(y);
        }
        monthCombo->setCurrentIndex(m - 1);
    });

    connect(prevYearBtn, &QPushButton::clicked, w, [=]() {
        yearSpin->setValue(qMax(1, yearSpin->value() - 1));
    });
    connect(nextYearBtn, &QPushButton::clicked, w, [=]() {
        yearSpin->setValue(yearSpin->value() + 1);
    });

    connect(todayBtn, &QPushButton::clicked, w, [=, this]() {
        QDate today = QDate::currentDate();
        {
            QSignalBlocker b1(yearSpin);
            QSignalBlocker b2(monthCombo);
            setComboYear(today.year());
            monthCombo->setCurrentIndex(today.month() - 1);
        }
        {
            QSignalBlocker b3(cal);
            cal->setCurrentPage(today.year(), today.month());
            cal->setSelectedDate(today);
        }
        state->selectedDate = today;
        state->pageNumber = 0;
        reloadDayView(false);
    });

    connect(cal, &QCalendarWidget::currentPageChanged, w, [=, this](int year, int month) {
        {
            QSignalBlocker b1(yearSpin);
            QSignalBlocker b2(monthCombo);
            setComboYear(year);
            monthCombo->setCurrentIndex(month - 1);
        }
        highlightCalendarAppointments(cal, year, month);
    });

    connect(bookBtn, &QPushButton::clicked, w, [=, this]() {
        if (showBookingDialog(QDate())) {
            reloadDayView(false);
        }
    });

    connect(quickBookBtn, &QPushButton::clicked, w, [=, this]() {
        if (state->selectedDate < QDate::currentDate()) {
            UIHelper::showWarning(w, "Không thể đặt lịch", "Không thể đặt lịch khám cho ngày trước thời gian hiện tại.");
            return;
        }
        if (mainWindow->isHoliday(state->selectedDate)) {
            UIHelper::showWarning(w, "Ngày nghỉ lễ", "Không thể đặt lịch khám vào ngày nghỉ lễ.");
            return;
        }
        if (showBookingDialog(state->selectedDate)) {
            reloadDayView(false);
        }
    });

    return scroll;
}

QWidget *AppointmentView::createAppointmentListWidget(QWidget *parent) {
    listSortColumn = -1;
    listSortAscending = false;
    if (mainWindow && mainWindow->getAppointments()) {
        libPBL2::sortAppointments(mainWindow->getAppointments(), libPBL2::AppointmentSortBy::ID, false);
    }

    QWidget *w = UIHelper::page("Quản lý lịch khám", "Theo dõi lịch theo bác sĩ, ngày hẹn và trạng thái khám.", parent);
    auto *layout = qobject_cast<QVBoxLayout*>(w->layout());

    auto *filterCard = new QFrame;
    filterCard->setObjectName("filterCard");
    filterCard->setStyleSheet(R"(
        QFrame#filterCard {
            background-color: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 10px;
        }
        QLabel {
            font-size: 12px;
            font-weight: 700;
            color: #475569;
        }
        QComboBox, QDateEdit, QLineEdit {
            background: #f8fafc;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 5px 10px;
            font-size: 13px;
            color: #0f172a;
            min-height: 22px;
        }
        QComboBox, QDateEdit {
            padding: 5px 24px 5px 8px;
        }
        QComboBox:disabled, QDateEdit:disabled, QLineEdit:disabled {
            background: #e2e8f0;
            color: #475569;
            border: 1.5px solid #cbd5e1;
        }
        QComboBox:focus, QDateEdit:focus, QLineEdit:focus {
            border-color: #0d9488;
            background: #ffffff;
        }
        QComboBox::drop-down, QDateEdit::drop-down {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 24px;
            border: none;
            background: transparent;
        }
        QComboBox::drop-down:disabled, QDateEdit::drop-down:disabled {
            width: 0px;
        }
        QComboBox::down-arrow, QDateEdit::down-arrow {
            image: url(:/assets/textures/arrow_down_dark.png);
            width: 10px;
            height: 10px;
            margin-right: 6px;
        }
        QComboBox::down-arrow:disabled, QDateEdit::down-arrow:disabled {
            image: none;
        }
        QComboBox QAbstractItemView {
            background-color: #ffffff;
            color: #0f172a;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 4px;
            selection-background-color: #0d9488;
            selection-color: #ffffff;
            outline: none;
        }
        QComboBox QAbstractItemView::item {
            min-height: 28px;
            padding: 4px 10px;
            border-radius: 6px;
        }
        QComboBox QAbstractItemView::item:hover {
            background-color: #f0fdfa;
            color: #0f766e;
        }
        QComboBox QAbstractItemView::item:selected {
            background-color: #0d9488;
            color: #ffffff;
        }
    )");

    auto *filters = new QHBoxLayout(filterCard);
    filters->setContentsMargins(14, 8, 14, 8);
    filters->setSpacing(12);

    auto *searchCol = new QVBoxLayout;
    searchCol->setSpacing(3);
    auto *lblSearch = new QLabel("Tìm kiếm:");
    lblSearch->setStyleSheet("font-size: 11.5px; font-weight: 700; color: #475569;");
    auto *searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText("🔍 Tìm theo tên bệnh nhân, mã ca khám, SĐT, bác sĩ...");
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    searchCol->addWidget(lblSearch);
    searchCol->addWidget(searchEdit);

    auto *docCol = new QVBoxLayout;
    docCol->setSpacing(3);
    auto *lblDoctor = new QLabel("Bác sĩ:");
    lblDoctor->setStyleSheet("font-size: 11.5px; font-weight: 700; color: #475569;");
    auto *doctor = new QComboBox;
    doctor->view()->setTextElideMode(Qt::ElideRight);
    doctor->addItem("Tất cả bác sĩ", "");
    auto *doctors = mainWindow ? mainWindow->getDoctors() : nullptr;
    if (doctors) {
        auto *d = doctors->getHeader()->next;
        while (d && d->next) {
            if (d->info.getState() != -1) {
                doctor->addItem(QStringLiteral("BS. %1").arg(UIHelper::qs(d->info.getfullName())), UIHelper::qs(d->info.getID()));
            }
            d = d->next;
        }
    }
    doctor->setMinimumWidth(180);
    doctor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    docCol->addWidget(lblDoctor);
    docCol->addWidget(doctor);

    auto *dateCol = new QVBoxLayout;
    dateCol->setSpacing(3);
    auto *lblDate = new QLabel("Ngày khám:");
    lblDate->setStyleSheet("font-size: 11.5px; font-weight: 700; color: #475569;");
    dateCol->addWidget(lblDate);

    auto *dateRow = new QHBoxLayout;
    dateRow->setSpacing(8);
    dateRow->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *date = new QDateEdit(QDate::currentDate());
    date->setDisplayFormat("dd/MM/yyyy");
    date->setMinimumWidth(145);
    date->setMaximumWidth(165);
    date->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    date->setCalendarPopup(true);
    auto *filterCal = new AppointmentListFilterCalendar(mainWindow, date);
    date->setCalendarWidget(filterCal);
    auto *calAligner = new FilterCalendarPopupAligner(date, filterCal, date);
    if (auto *pop = filterCal->window(); pop && pop != filterCal) {
        pop->installEventFilter(calAligner);
    } else {
        filterCal->installEventFilter(calAligner);
    }

    auto *allDates = new ToggleSwitch;
    allDates->setChecked(true);
    allDates->setToolTip(QString::fromUtf8("Tất cả các ngày (Bật để xem mọi ngày, Tắt để chọn ngày khám)"));
    date->setEnabled(false);

    auto handleDateSelect = [date, filterCal](const QDate &d) {
        date->setDate(d);
        if (auto *pop = filterCal->window()) {
            pop->hide();
        }
    };
    QObject::connect(filterCal, &QCalendarWidget::clicked, date, handleDateSelect);
    QObject::connect(filterCal, &QCalendarWidget::activated, date, handleDateSelect);

    dateRow->addWidget(date);
    dateRow->addWidget(allDates);
    dateCol->addLayout(dateRow);

    auto *statusCol = new QVBoxLayout;
    statusCol->setSpacing(3);
    auto *lblStatus = new QLabel("Trạng thái:");
    lblStatus->setStyleSheet("font-size: 11.5px; font-weight: 700; color: #475569;");
    auto *status = new QComboBox;
    status->view()->setTextElideMode(Qt::ElideRight);
    status->addItems({"Tất cả trạng thái", "Chưa khám", "Đã khám", "Trễ hẹn", "Đã dời lịch"});
    status->setMinimumWidth(185);
    status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    statusCol->addWidget(lblStatus);
    statusCol->addWidget(status);

    filters->addLayout(searchCol, 4);
    filters->addSpacing(14);
    filters->addLayout(docCol, 2);
    filters->addSpacing(14);
    filters->addLayout(dateCol, 0);
    filters->addSpacing(14);
    filters->addLayout(statusCol, 2);
    layout->addWidget(filterCard);

    auto *t = UIHelper::table({"BỆNH NHÂN", "BÁC SĨ", "CHUYÊN KHOA", "NGÀY KHÁM", "TRẠNG THÁI", ""});
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setFocusPolicy(Qt::StrongFocus);
    t->verticalHeader()->setDefaultSectionSize(48);

    if (t->horizontalHeaderItem(0)) t->horizontalHeaderItem(0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (t->horizontalHeaderItem(1)) t->horizontalHeaderItem(1)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (t->horizontalHeaderItem(2)) t->horizontalHeaderItem(2)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (t->horizontalHeaderItem(3)) t->horizontalHeaderItem(3)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (t->horizontalHeaderItem(4)) t->horizontalHeaderItem(4)->setTextAlignment(Qt::AlignCenter);
    if (t->horizontalHeaderItem(5)) t->horizontalHeaderItem(5)->setTextAlignment(Qt::AlignCenter);

    UIHelper::setupTableScaling(t, {0.23, 0.22, 0.20, 0.20, 0.15}, 56);

    auto *tableCard = UIHelper::createTableCard(t);
    layout->addWidget(tableCard, 1);

    const int PAGE_SIZE = 50;
    auto currentPage = std::make_shared<int>(0);
    auto sortColumn = std::make_shared<int>(listSortColumn);
    auto sortAscending = std::make_shared<bool>(listSortAscending);

    auto *pagination = new QHBoxLayout;
    pagination->setContentsMargins(4, 4, 4, 0);

    auto *totalLabel = new QLabel("Tổng số: 0 lịch hẹn");
    totalLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #475569;");
    pagination->addWidget(totalLabel);

    pagination->addStretch();

    auto *prevBtn = UIHelper::button("‹  Trang trước", "secondary");
    auto *nextBtn = UIHelper::button("Trang sau  ›", "secondary");
    auto *pageStatus = new QLabel("Trang 1 / 1");
    pageStatus->setStyleSheet("font-size: 12.5px; font-weight: 700; color: #0f172a; padding: 0 12px;");

    pagination->addWidget(prevBtn);
    pagination->addWidget(pageStatus);
    pagination->addWidget(nextBtn);
    layout->addLayout(pagination);

    auto computeApptStatus = [](const Appointment &a) -> QString {
        return getAppointmentStatusText(a);
    };

    auto fillPtr = std::make_shared<std::function<void(bool)>>();
    std::weak_ptr<std::function<void(bool)>> weakFill = fillPtr;
    *fillPtr = [this, w, t, searchEdit, doctor, date, allDates, status, totalLabel, prevBtn, nextBtn, pageStatus, currentPage, PAGE_SIZE, computeApptStatus, sortColumn, sortAscending, weakFill](bool preserveScroll) {
        int vScroll = (preserveScroll && t && t->verticalScrollBar()) ? t->verticalScrollBar()->value() : 0;
        int hScroll = (preserveScroll && t && t->horizontalScrollBar()) ? t->horizontalScrollBar()->value() : 0;
        int curRow = preserveScroll ? t->currentRow() : -1;
        if (t) t->setUpdatesEnabled(false);
        t->setRowCount(0);
        if (t->viewport()) {
            t->viewport()->setCursor(Qt::ArrowCursor);
        }
        auto *appointments = mainWindow ? mainWindow->getAppointments() : nullptr;
        if (!appointments) {
            totalLabel->setText("Tổng số: 0 lịch hẹn");
            pageStatus->setText("Trang 1 / 1");
            prevBtn->setEnabled(false);
            nextBtn->setEnabled(false);
            if (t) t->setUpdatesEnabled(true);
            return;
        }

        QString q = searchEdit ? searchEdit->text().trimmed().toLower() : "";

        QList<Appointment*> filtered;
        auto *n = appointments->getHeader()->next;
        while (n && n->next) {
            if (!n->info.isDefault()) {
                auto &a = n->info;
                Doctor *d = mainWindow->findDoctor(a.getDoctorId());
                Patient *p = mainWindow->findPatientForAppointment(a);
                Date ad = a.getDateGet();

                bool matchesDoctor = doctor->currentData().toString().isEmpty() || UIHelper::qs(a.getDoctorId()) == doctor->currentData().toString();
                bool matchesDate = allDates->isChecked() || (ad.getYear() == date->date().year() && ad.getMonth() == date->date().month() && ad.getDay() == date->date().day());
                QString sText = computeApptStatus(a);
                bool matchesStatus = (status->currentIndex() == 0) || (status->currentText() == sText);

                bool matchesSearch = true;
                if (!q.isEmpty()) {
                    QString patName = p ? UIHelper::qs(p->getfullName()).toLower() : "";
                    QString patId = p ? UIHelper::qs(p->getID()).toLower() : "";
                    QString patPhone = p ? UIHelper::qs(p->getPhone()).toLower() : "";
                    QString docName = d ? UIHelper::qs(d->getfullName()).toLower() : "";
                    QString specName = d ? mainWindow->specialty(d->getSpId()).toLower() : "";
                    QString apptId = UIHelper::qs(a.getId()).toLower();
                    QString room = d ? UIHelper::qs(d->getRoomId()).toLower() : "";

                    QString fullSearch = apptId + " " + patName + " " + patId + " " + patPhone + " " + docName + " " + specName + " " + room;
                    if (!fullSearch.contains(q)) {
                        matchesSearch = false;
                    }
                }

                if (matchesDoctor && matchesDate && matchesStatus && matchesSearch) {
                    filtered.append(&n->info);
                }
            }
            n = n->next;
        }

        int col = *sortColumn;
        bool asc = *sortAscending;

        if (col >= 0) {
            libPBL2::qsort(filtered, [this, col, asc, &computeApptStatus](const Appointment *a, const Appointment *b) -> bool {
            if (!a || !b) return false;
            int cmp = 0;
            switch (col) {
                case 0: {
                    Patient *pa = mainWindow ? mainWindow->findPatientForAppointment(*a) : nullptr;
                    Patient *pb = mainWindow ? mainWindow->findPatientForAppointment(*b) : nullptr;
                    std::string na = pa ? pa->getfullName() : "";
                    std::string nb = pb ? pb->getfullName() : "";
                    cmp = libPBL2::compareVietnameseNames(na, nb);
                    break;
                }
                case 1: {
                    Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                    Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                    std::string na = da ? da->getfullName() : "";
                    std::string nb = db ? db->getfullName() : "";
                    cmp = libPBL2::compareVietnameseNames(na, nb);
                    break;
                }
                case 2: {
                    Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                    Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                    QString sa = (da && mainWindow) ? mainWindow->specialty(da->getSpId()) : "";
                    QString sb = (db && mainWindow) ? mainWindow->specialty(db->getSpId()) : "";
                    cmp = QString::compare(sa, sb, Qt::CaseInsensitive);
                    break;
                }
                case 3: {
                    cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
                    break;
                }
                case 4: {
                    QString sa = computeApptStatus(*a);
                    QString sb = computeApptStatus(*b);
                    cmp = QString::compare(sa, sb, Qt::CaseInsensitive);
                    break;
                }
                default:
                    cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
                    break;
            }
            if (cmp == 0) {
                cmp = libPBL2::compareNatural(a->getId(), b->getId());
            }
            return asc ? (cmp < 0) : (cmp > 0);
        });
        } else {
            libPBL2::qsort(filtered, [](const Appointment *a, const Appointment *b) -> bool {
                if (!a || !b) return false;
                return libPBL2::compareNatural(a->getId(), b->getId()) > 0;
            });
        }

        totalLabel->setText(QStringLiteral("Tổng số: %1 lịch hẹn").arg(filtered.size()));

        int totalPages = (filtered.size() + PAGE_SIZE - 1) / PAGE_SIZE;
        if (totalPages < 1) totalPages = 1;
        if (*currentPage >= totalPages) *currentPage = totalPages - 1;
        if (*currentPage < 0) *currentPage = 0;

        pageStatus->setText(QStringLiteral("Trang %1 / %2").arg(*currentPage + 1).arg(totalPages));
        prevBtn->setEnabled(*currentPage > 0);
        nextBtn->setEnabled(*currentPage + 1 < totalPages);

        int startIdx = *currentPage * PAGE_SIZE;
        int endIdx = qMin(startIdx + PAGE_SIZE, filtered.size());

        for (int i = startIdx; i < endIdx; ++i) {
            auto &a = *filtered[i];
            Doctor *d = mainWindow->findDoctor(a.getDoctorId());
            Patient *p = mainWindow->findPatientForAppointment(a);
            Date ad = a.getDateGet();
            QString sText = computeApptStatus(a);

            int r = t->rowCount();
            t->insertRow(r);

            auto *pItem = new QTableWidgetItem(p ? UIHelper::qs(p->getfullName()) : "—");
            pItem->setData(Qt::UserRole, UIHelper::qs(a.getId()));
            pItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            t->setItem(r, 0, pItem);

            UIHelper::cell(t, r, 1, d ? QStringLiteral("BS. %1").arg(UIHelper::qs(d->getfullName())) : UIHelper::qs(a.getDoctorId()), Qt::AlignLeft | Qt::AlignVCenter);
            UIHelper::cell(t, r, 2, d ? mainWindow->specialty(d->getSpId()) : "—", Qt::AlignLeft | Qt::AlignVCenter);

            QString dateStr = QString::asprintf("%02d:%02d · %02d/%02d/%04d", ad.getHour(), ad.getMinute(), ad.getDay(), ad.getMonth(), ad.getYear());
            UIHelper::cell(t, r, 3, dateStr, Qt::AlignLeft | Qt::AlignVCenter);

            auto *statusItem = new QTableWidgetItem(sText);
            statusItem->setTextAlignment(Qt::AlignCenter);
            if (sText == "Đã khám") {
                statusItem->setForeground(QColor("#059669"));
            } else if (sText == "Chưa khám") {
                statusItem->setForeground(QColor("#d97706"));
            } else if (sText == "Trễ hẹn") {
                statusItem->setForeground(QColor("#ea580c"));
            } else if (sText == "Đã dời lịch") {
                statusItem->setForeground(QColor("#64748b"));
            }
            QFont f = statusItem->font();
            f.setBold(true);
            statusItem->setFont(f);
            t->setItem(r, 4, statusItem);

            auto action = UIHelper::createActionCell(UIHelper::trashIcon(), "Hủy lịch khám");
            if (mainWindow && !mainWindow->hasPermission("Per_4")) {
                action.second->setToolTip("Bạn không có quyền hủy lịch khám (Yêu cầu quyền Per_4)");
            }
            std::string aid = a.getId();
            connect(action.second, &QPushButton::clicked, w, [this, w, aid, weakFill] {
                if (!mainWindow) return;
                if (!mainWindow->hasPermission("Per_4")) {
                    UIHelper::showWarning(mainWindow, "Không có quyền", "Bạn không có quyền hủy lịch khám.");
                    return;
                }
                auto *appointments = mainWindow->getAppointments();
                if (!appointments) return;

                Appointment *targetAppt = nullptr;
                auto *n = appointments->getHeader()->next;
                while (n && n->next) {
                    if (n->info.getId() == aid) {
                        targetAppt = &n->info;
                        break;
                    }
                    n = n->next;
                }
                if (!targetAppt) return;

                if (targetAppt->getState() == 1) {
                    UIHelper::showInfo(mainWindow, "Lịch đã hoàn tất", "Lịch khám này đã hoàn tất (Đã khám), không thể thực hiện hủy.");
                    return;
                }

                Patient *p = mainWindow->findPatientForAppointment(*targetAppt);
                Date ad = targetAppt->getDateGet();
                QString patientName = p ? UIHelper::qs(p->getfullName()) : "bệnh nhân";
                QString dateStr = QString::asprintf("%02d:%02d · %02d/%02d/%04d", ad.getHour(), ad.getMinute(), ad.getDay(), ad.getMonth(), ad.getYear());

                QString confirmMsg = QStringLiteral("Bạn có chắc chắn muốn xóa lịch khám lúc %1 của bệnh nhân \"%2\"?\nLịch hẹn sau khi xóa sẽ không còn hiển thị trong danh sách chờ.")
                    .arg(dateStr, patientName);
                if (!UIHelper::confirmDelete(mainWindow, "Xác nhận xóa lịch khám", confirmMsg, "Xóa lịch khám", "Hủy bỏ")) {
                    return;
                }

                std::string spId = targetAppt->getSymptomPatientId();
                Appointment backupAppt = *targetAppt;
                bool hasBackupSp = false;
                SymptomPatient backupSp;
                QList<SymptomPatientDetail> backupSpds;

                auto *cur = appointments->getHeader()->next;
                while (cur && cur->next) {
                    if (cur->info.getId() == aid) {
                        appointments->remove(cur);
                        break;
                    }
                    cur = cur->next;
                }

                auto *spList = mainWindow->getSymptomPatients();
                auto *spdList = mainWindow->getSymptomPatientDetails();
                if (!spId.empty()) {
                    if (spList && spList->getHeader()) {
                        auto *spNode = spList->getHeader()->next;
                        while (spNode && spNode->next) {
                            if (spNode->info.getSP_id() == spId) {
                                backupSp = spNode->info;
                                hasBackupSp = true;
                                spList->remove(spNode);
                                break;
                            }
                            spNode = spNode->next;
                        }
                    }

                    if (spdList && spdList->getHeader()) {
                        auto *spdNode = spdList->getHeader()->next;
                        while (spdNode && spdNode->next) {
                            if (spdNode->info.getSP_id() == spId) {
                                backupSpds.append(spdNode->info);
                                auto *toDel = spdNode;
                                spdNode = spdNode->next;
                                spdList->remove(toDel);
                            } else {
                                spdNode = spdNode->next;
                            }
                        }
                    }
                }

                if (!mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal)) {
                    appointments->insert(backupAppt);
                    if (hasBackupSp && spList) {
                        spList->insert(backupSp);
                    }
                    if (spdList) {
                        for (const auto &spd : backupSpds) {
                            spdList->insert(spd);
                        }
                    }
                    return;
                }
                if (auto sp = weakFill.lock()) (*sp)(true);
                mainWindow->refreshPage(0);
                mainWindow->refreshPage(1);
                mainWindow->refreshPage(2);
                mainWindow->refreshPage(5);
            });

            t->setCellWidget(r, 5, action.first);
        }
        if (t) t->setUpdatesEnabled(true);
        if (preserveScroll) {
            if (t && t->verticalScrollBar()) t->verticalScrollBar()->setValue(vScroll);
            if (t && t->horizontalScrollBar()) t->horizontalScrollBar()->setValue(hScroll);
            if (curRow >= 0 && curRow < t->rowCount()) t->setCurrentCell(curRow, 0);
            QTimer::singleShot(0, t, [t, vScroll, hScroll, curRow]() {
                if (t && t->verticalScrollBar()) t->verticalScrollBar()->setValue(qBound(0, vScroll, t->verticalScrollBar()->maximum()));
                if (t && t->horizontalScrollBar()) t->horizontalScrollBar()->setValue(qBound(0, hScroll, t->horizontalScrollBar()->maximum()));
                if (t && curRow >= 0 && curRow < t->rowCount()) t->setCurrentCell(curRow, 0);
            });
        }
    };
    auto fill = [fillPtr](bool preserveScroll = true) { if (*fillPtr) (*fillPtr)(preserveScroll); };

    connect(t->horizontalHeader(), &QHeaderView::sectionClicked, w, [this, fill, sortColumn, sortAscending, currentPage](int logicalIndex) {
        if (logicalIndex < 0 || logicalIndex >= 5) return;
        if (*sortColumn == logicalIndex) {
            *sortAscending = !*sortAscending;
        } else {
            *sortColumn = logicalIndex;
            *sortAscending = true;
        }
        listSortColumn = *sortColumn;
        listSortAscending = *sortAscending;
        *currentPage = 0;
        fill(false);
    });

    connect(searchEdit, &QLineEdit::textChanged, w, [fill, currentPage] { *currentPage = 0; fill(false); });
    connect(doctor, qOverload<int>(&QComboBox::currentIndexChanged), w, [fill, currentPage] { *currentPage = 0; fill(false); });
    connect(date, &QDateEdit::dateChanged, w, [fill, currentPage] { *currentPage = 0; fill(false); });
    connect(allDates, &QAbstractButton::toggled, w, [fill, currentPage, date](bool c) {
        *currentPage = 0;
        date->setEnabled(!c);
        fill(false);
    });
    connect(status, qOverload<int>(&QComboBox::currentIndexChanged), w, [fill, currentPage] { *currentPage = 0; fill(false); });

    connect(prevBtn, &QPushButton::clicked, w, [fill, currentPage] {
        if (*currentPage > 0) {
            (*currentPage)--;
            fill(false);
        }
    });

    connect(nextBtn, &QPushButton::clicked, w, [fill, currentPage] {
        (*currentPage)++;
        fill(false);
    });

    connect(t, &QTableWidget::cellDoubleClicked, w, [this, t, fill](int row, int col) {
        if (col == 5) return;
        if (row >= 0 && t->item(row, 0)) {
            std::string id = UIHelper::ss(t->item(row, 0)->data(Qt::UserRole).toString());
            auto *appointments = mainWindow ? mainWindow->getAppointments() : nullptr;
            if (appointments) {
                auto *n = appointments->getHeader()->next;
                while (n && n->next) {
                    if (n->info.getId() == id) {
                        showAppointmentDetailDialog(&n->info);
                        fill(true);
                        return;
                    }
                    n = n->next;
                }
            }
        }
    });

    auto *filter = new AppointmentTabShowFilter([this, fill, sortColumn, sortAscending, currentPage]() {
        *sortColumn = -1;
        *sortAscending = false;
        *currentPage = 0;
        listSortColumn = -1;
        listSortAscending = false;
        if (mainWindow && mainWindow->getAppointments()) {
            AppointmentController::syncExpiredAppointments(mainWindow->getAppointments());
            libPBL2::sortAppointments(mainWindow->getAppointments(), libPBL2::AppointmentSortBy::ID, false);
        }
        fill(false);
    }, w);
    w->installEventFilter(filter);

    fill(false);
    return w;
}

void AppointmentView::showAppointmentDetailDialog(Appointment *a) {
    if (!mainWindow || !a) return;

    struct DialogGuard {
        bool &flag;
        DialogGuard(bool &f) : flag(f) { flag = true; }
        ~DialogGuard() { flag = false; }
    };
    static bool isShowingDialog = false;
    if (isShowingDialog) return;
    DialogGuard guard(isShowingDialog);

    bool canEdit = mainWindow->hasPermission("Per_4");

    Patient *p = mainWindow->findPatientForAppointment(*a);
    std::string patientId = mainWindow->getPatientIdForAppointment(*a);
    Doctor *d = mainWindow->findDoctor(a->getDoctorId());
    Date ad = a->getDateGet();
    Date sd = mainWindow->getDateSetForAppointment(*a);

    QDialog dialog(mainWindow);
    dialog.setWindowTitle("Chi tiết lịch khám bệnh");
    dialog.setFixedWidth(610);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #f8fafc;
            font-family: 'Segoe UI', sans-serif;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            width: 0px;
            height: 0px;
            background: transparent;
            border: none;
        }
    )");

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    auto *headerBox = new QHBoxLayout;
    headerBox->setSpacing(12);

    auto *icon = new QLabel("📋");
    icon->setFixedSize(46, 46);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("background: #f0fdfa; border: 1.5px solid #99f6e4; border-radius: 12px; font-size: 22px;");

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    auto *title = new QLabel("Chi tiết lịch khám bệnh");
    title->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a;");
    titleCol->addWidget(title);

    auto *roleBadge = new QLabel(canEdit ? "● Có quyền xử lý" : "● Chỉ xem");
    roleBadge->setStyleSheet(canEdit
        ? "background: #eff6ff; color: #2563eb; border: 1px solid #bfdbfe; border-radius: 12px; font-size: 11.5px; font-weight: 500; font-style: normal; padding: 4px 10px; font-family: 'Segoe UI', sans-serif;"
        : "background: #f1f5f9; color: #475569; border: 1px solid #cbd5e1; border-radius: 12px; font-size: 11.5px; font-weight: 500; font-style: normal; padding: 4px 10px; font-family: 'Segoe UI', sans-serif;");

    headerBox->addWidget(icon);
    headerBox->addLayout(titleCol, 1);
    headerBox->addWidget(roleBadge, 0, Qt::AlignVCenter);
    root->addLayout(headerBox);

    auto makeSectionCard = [](const QString &iconTitle) {
        auto *f = new QFrame;
        f->setStyleSheet("QFrame#card { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px; }");
        f->setObjectName("card");
        auto *fl = new QVBoxLayout(f);
        fl->setContentsMargins(16, 12, 16, 12);
        fl->setSpacing(8);

        auto *h = new QLabel(iconTitle);
        h->setStyleSheet("font-size: 12.5px; font-weight: 800; font-style: normal; color: #0f766e; letter-spacing: 0.5px; text-transform: uppercase; font-family: 'Segoe UI', sans-serif;");
        fl->addWidget(h);
        return qMakePair(f, fl);
    };

    auto makeField = [](const QString &label, const QString &val) {
        auto *col = new QVBoxLayout;
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(3);
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 12px; font-weight: 400; font-style: normal; color: #64748b; font-family: 'Segoe UI', sans-serif;");
        auto *v = new QLabel(val);
        v->setStyleSheet("font-size: 13.5px; font-weight: 400; font-style: normal; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
        col->addWidget(lbl);
        col->addWidget(v);
        return col;
    };

    auto wrapField = [](const QString &label, QLabel *valWidget) {
        auto *col = new QVBoxLayout;
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(3);
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 12px; font-weight: 400; font-style: normal; color: #64748b; font-family: 'Segoe UI', sans-serif;");
        col->addWidget(lbl);
        col->addWidget(valWidget);
        return col;
    };

    auto pCardPair = makeSectionCard("👤  Thông tin bệnh nhân");
    auto *pGrid = new QGridLayout;
    pGrid->setHorizontalSpacing(20);
    pGrid->setVerticalSpacing(8);
    pGrid->setColumnStretch(0, 1);
    pGrid->setColumnStretch(1, 1);
    QString pName = p ? UIHelper::qs(p->getfullName()) : UIHelper::qs(mainWindow->getPatientIdForAppointment(*a));
    QString pGenderAge = p ? QStringLiteral("%1 tuổi · %2").arg(p->getAge()).arg(p->getGender() ? "Nam" : "Nữ") : "—";
    QString pPhone = (p && !p->getPhone().empty()) ? UIHelper::qs(p->getPhone()) : "Chưa cập nhật";
    QString pCccd = (p && !p->getCCCD().empty()) ? UIHelper::qs(p->getCCCD()) : "Chưa cập nhật";

    pGrid->addLayout(makeField("Họ và tên", pName), 0, 0);
    pGrid->addLayout(makeField("Tuổi & Giới tính", pGenderAge), 0, 1);
    pGrid->addLayout(makeField("Số điện thoại", pPhone), 1, 0);
    pGrid->addLayout(makeField("Số CCCD", pCccd), 1, 1);
    pCardPair.second->addLayout(pGrid);
    root->addWidget(pCardPair.first);

    auto sCardPair = makeSectionCard("🩺  Triệu chứng lâm sàng");
    QString apptSymptomName = mainWindow->getSymptomNamesForAppointment(*a);
    if (apptSymptomName.isEmpty()) {
        apptSymptomName = "Chưa ghi nhận";
    }
    auto *allSymRow = new QHBoxLayout;
    auto *allSymLbl = new QLabel("Các triệu chứng đã ghi nhận:");
    allSymLbl->setStyleSheet("font-size: 12px; font-weight: 400; font-style: normal; color: #64748b; font-family: 'Segoe UI', sans-serif;");
    auto *allSymVal = new QLabel(apptSymptomName);
    allSymVal->setStyleSheet("font-size: 13.5px; font-weight: 400; font-style: normal; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    allSymVal->setWordWrap(true);
    allSymRow->addWidget(allSymLbl);
    allSymRow->addWidget(allSymVal, 1);
    sCardPair.second->addLayout(allSymRow);
    root->addWidget(sCardPair.first);

    auto dCardPair = makeSectionCard("👨‍⚕️  Bác sĩ phụ trách");
    auto *dGrid = new QGridLayout;
    dGrid->setHorizontalSpacing(20);
    dGrid->setVerticalSpacing(8);
    dGrid->setColumnStretch(0, 1);
    dGrid->setColumnStretch(1, 1);

    QString dName = d ? QStringLiteral("BS. %1").arg(UIHelper::qs(d->getfullName())) : UIHelper::qs(a->getDoctorId());
    dGrid->addLayout(makeField("Bác sĩ nhận ca", dName), 0, 0);

    QLabel *dSpecVal = new QLabel(d ? mainWindow->specialty(d->getSpId()) : "—");
    dSpecVal->setStyleSheet("font-size: 13.5px; font-weight: 400; font-style: normal; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    QLabel *dRoomVal = new QLabel(d ? UIHelper::formatRoom(UIHelper::qs(d->getRoomId())) : "Chưa xác định");
    dRoomVal->setStyleSheet("font-size: 13.5px; font-weight: 400; font-style: normal; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    QLabel *dPhoneVal = new QLabel((d && !d->getPhone().empty()) ? UIHelper::qs(d->getPhone()) : "Chưa cập nhật");
    dPhoneVal->setStyleSheet("font-size: 13.5px; font-weight: 400; font-style: normal; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    QLabel *dRatingVal = new QLabel(d ? QStringLiteral("⭐ %1 / 5.0").arg(QString::number(d->getRating(), 'f', 1)) : "—");
    dRatingVal->setStyleSheet("font-size: 13.5px; font-weight: 400; font-style: normal; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    QLabel *dFeeVal = new QLabel(d ? (QLocale(QLocale::Vietnamese).toString(d->getFee(), 'f', 0) + " đ") : "—");
    dFeeVal->setStyleSheet("font-size: 13.5px; font-weight: 400; font-style: normal; color: #1e293b; font-family: 'Segoe UI', sans-serif;");

    dGrid->addLayout(wrapField("Chuyên khoa", dSpecVal), 0, 1);
    dGrid->addLayout(wrapField("Phòng khám chỉ định", dRoomVal), 1, 0);
    dGrid->addLayout(wrapField("Số điện thoại bác sĩ", dPhoneVal), 1, 1);
    dGrid->addLayout(wrapField("Đánh giá chuyên môn", dRatingVal), 2, 0);
    dGrid->addLayout(wrapField("Phí khám tham chiếu", dFeeVal), 2, 1);
    dCardPair.second->addLayout(dGrid);
    root->addWidget(dCardPair.first);

    auto aCardPair = makeSectionCard("📅  Thời gian & Trạng thái lịch khám");
    auto *aGrid = new QGridLayout;
    aGrid->setHorizontalSpacing(20);
    aGrid->setVerticalSpacing(8);
    aGrid->setColumnStretch(0, 1);
    aGrid->setColumnStretch(1, 1);

    QString setDateStr = QString::asprintf("%02d/%02d/%04d %02d:%02d", sd.getDay(), sd.getMonth(), sd.getYear(), sd.getHour(), sd.getMinute());
    aGrid->addLayout(makeField("Ngày lên lịch hẹn", setDateStr), 0, 0);

    QString apptDateStr = QString::asprintf("%02d/%02d/%04d %02d:%02d", ad.getDay(), ad.getMonth(), ad.getYear(), ad.getHour(), ad.getMinute());
    aGrid->addLayout(makeField("Ngày đi khám", apptDateStr), 0, 1);

    QDateTime apptDt(QDate(ad.getYear(), ad.getMonth(), ad.getDay()), QTime(ad.getHour(), ad.getMinute()));

    auto *statCol = new QVBoxLayout;
    statCol->setContentsMargins(0, 0, 0, 0);
    statCol->setSpacing(3);
    auto *statLbl = new QLabel("Trạng thái ca khám");
    statLbl->setStyleSheet("font-size: 12px; font-weight: 400; font-style: normal; color: #64748b; font-family: 'Segoe UI', sans-serif;");

    auto *statusBadge = new QLabel;
    auto updateStatusBadge = [statusBadge, a, apptDt]() {
        Date curNowDate = Date::now();
        int eff = a->computeEffectiveState(curNowDate.getDay(), curNowDate.getMonth(), curNowDate.getYear(), curNowDate.getHour(), curNowDate.getMinute());
        if (eff == 0 && apptDt.isValid() && QDateTime::currentDateTime() > apptDt.addSecs(20 * 60)) {
            eff = -1;
        }
        if (eff == 1) {
            statusBadge->setText("● Đã khám");
            statusBadge->setStyleSheet("background: #ecfdf5; color: #059669; border: 1px solid #a7f3d0; border-radius: 10px; font-size: 12px; font-weight: 500; font-style: normal; padding: 4px 12px; font-family: 'Segoe UI', sans-serif;");
        } else if (eff == -2) {
            statusBadge->setText("● Dời lịch");
            statusBadge->setStyleSheet("background: #f5f3ff; color: #7c3aed; border: 1px solid #ddd6fe; border-radius: 10px; font-size: 12px; font-weight: 500; font-style: normal; padding: 4px 12px; font-family: 'Segoe UI', sans-serif;");
        } else if (eff == -1) {
            statusBadge->setText("● Trễ hẹn");
            statusBadge->setStyleSheet("background: #fff7ed; color: #ea580c; border: 1px solid #ffedd5; border-radius: 10px; font-size: 12px; font-weight: 500; font-style: normal; padding: 4px 12px; font-family: 'Segoe UI', sans-serif;");
        } else {
            statusBadge->setText("● Chưa khám");
            statusBadge->setStyleSheet("background: #fffbeb; color: #d97706; border: 1px solid #fde68a; border-radius: 10px; font-size: 12px; font-weight: 500; font-style: normal; padding: 4px 12px; font-family: 'Segoe UI', sans-serif;");
        }
    };
    updateStatusBadge();

    auto *badgeWrap = new QWidget;
    auto *bwLay = new QHBoxLayout(badgeWrap);
    bwLay->setContentsMargins(0, 0, 0, 0);
    bwLay->addWidget(statusBadge);
    bwLay->addStretch();

    statCol->addWidget(statLbl);
    statCol->addWidget(badgeWrap);
    aGrid->addLayout(statCol, 1, 0);

    QString paidFeeStr = (a->getState() == 1 && a->hasPaidFee())
        ? (QLocale(QLocale::Vietnamese).toString(a->getPaidFee(), 'f', 0) + " đ")
        : QStringLiteral("-");
    aGrid->addLayout(makeField("Chi phí thanh toán", paidFeeStr), 1, 1);

    aCardPair.second->addLayout(aGrid);
    root->addWidget(aCardPair.first);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);

    auto *closeBtn = new QPushButton("Đóng");
    closeBtn->setAutoDefault(false);
    closeBtn->setDefault(false);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedHeight(38);
    closeBtn->setStyleSheet("QPushButton { background: #ffffff; color: #475569; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 0 20px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #f1f5f9; color: #0f172a; }");
    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    btnRow->addWidget(closeBtn);
    btnRow->addStretch();

    if (canEdit) {

        auto *confirmEnterBtn = new QPushButton("✓  Xác nhận vào khám");
        confirmEnterBtn->setAutoDefault(false);
        confirmEnterBtn->setDefault(false);
        confirmEnterBtn->setFixedHeight(38);

        auto updateConfirmEnterBtn = [confirmEnterBtn, a, apptDt]() {
            QDateTime current = QDateTime::currentDateTime();
            bool canConfirm = (a->getState() == 0) && apptDt.isValid() &&
                              (current >= apptDt && current <= apptDt.addSecs(20 * 60));
            if (canConfirm) {
                confirmEnterBtn->setEnabled(true);
                confirmEnterBtn->setCursor(Qt::PointingHandCursor);
                confirmEnterBtn->setStyleSheet("QPushButton { background: #2563eb; color: #ffffff; border: none; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #1d4ed8; }");
            } else {
                confirmEnterBtn->setEnabled(false);
                confirmEnterBtn->setCursor(Qt::ArrowCursor);
                confirmEnterBtn->setStyleSheet("QPushButton { background: #e2e8f0; color: #94a3b8; border: none; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 13px; }");
            }
        };
        updateConfirmEnterBtn();

        QObject::connect(confirmEnterBtn, &QPushButton::clicked, &dialog, [this, &dialog, a, d, apptDt] {
            QDateTime current = QDateTime::currentDateTime();
            if (a->getState() != 0 || !apptDt.isValid() || current < apptDt || current > apptDt.addSecs(20 * 60)) {
                if (a->getState() == 0 && apptDt.isValid() && current > apptDt.addSecs(20 * 60)) {
                    a->setState(-1);
                    if (mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal)) {
                        mainWindow->refreshPage(0);
                        mainWindow->refreshPage(1);
                        mainWindow->refreshPage(4);
                        mainWindow->refreshPage(5);
                    } else {
                        a->setState(0);
                    }
                }
                UIHelper::showWarning(&dialog, "Không thể xác nhận", "Chỉ có thể xác nhận vào khám trong khoảng từ giờ hẹn đến tối đa 20 phút sau giờ hẹn.");
                dialog.reject();
                return;
            }
            int oldState = a->getState();
            double oldFee = a->getPaidFee();
            a->setState(1);
            a->setPaidFee(d ? d->getFee() : 0.0);
            if (!mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal)) {
                a->setState(oldState);
                a->setPaidFee(oldFee);
                return;
            }
            mainWindow->refreshPage(0);
            mainWindow->refreshPage(1);
            mainWindow->refreshPage(4);
            mainWindow->refreshPage(5);
            UIHelper::showSuccess(&dialog, "Thành công", "Đã xác nhận bệnh nhân vào khám và ghi nhận chi phí hoàn tất.");
            dialog.accept();
        });

        auto *reschedBtn = new QPushButton("Dời lịch");
        reschedBtn->setAutoDefault(false);
        reschedBtn->setDefault(false);
        reschedBtn->setFixedHeight(38);

        auto updateReschedBtn = [reschedBtn, a, apptDt]() {
            QDateTime current = QDateTime::currentDateTime();
            bool canResched = (a->getState() == 0) && apptDt.isValid() && (current <= apptDt.addSecs(20 * 60));
            if (canResched) {
                reschedBtn->setEnabled(true);
                reschedBtn->setCursor(Qt::PointingHandCursor);
                reschedBtn->setStyleSheet("QPushButton { background: #f59e0b; color: #ffffff; border: none; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #d97706; }");
            } else {
                reschedBtn->setEnabled(false);
                reschedBtn->setCursor(Qt::ArrowCursor);
                reschedBtn->setStyleSheet("QPushButton { background: #e2e8f0; color: #94a3b8; border: none; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 13px; }");
            }
        };
        updateReschedBtn();

        auto *btnTimer = new QTimer(&dialog);
        QObject::connect(btnTimer, &QTimer::timeout, &dialog, [updateStatusBadge, updateConfirmEnterBtn, updateReschedBtn]() {
            updateStatusBadge();
            updateConfirmEnterBtn();
            updateReschedBtn();
        });
        btnTimer->start(1000);

        QObject::connect(reschedBtn, &QPushButton::clicked, &dialog, [this, &dialog, a, patientId] {
            QDialog rDlg(&dialog);
            rDlg.setWindowTitle("Dời lịch khám bệnh");
            rDlg.setFixedWidth(640);
            rDlg.setStyleSheet("QDialog { background-color: #f8fafc; font-family: 'Segoe UI', sans-serif; } QComboBox { background: #ffffff; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 6px 12px; font-size: 13px; color: #0f172a; }");

            auto *rLay = new QVBoxLayout(&rDlg);
            rLay->setContentsMargins(22, 20, 22, 20);
            rLay->setSpacing(14);

            auto *rTitle = new QLabel("Dời lịch & Đổi bác sĩ khám bệnh");
            rTitle->setStyleSheet("font-size: 16px; font-weight: 700; color: #0f172a;");
            rLay->addWidget(rTitle);

            auto *docBox = new QVBoxLayout;
            docBox->setSpacing(5);
            auto *docLbl = new QLabel("Chọn bác sĩ khám:");
            docLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
            auto *docCombo = new QComboBox;

            std::string oldSpId = a->getSymptomPatientId();
            QMap<QString, int> spScores;

            auto *symptomPatientDetails = mainWindow->getSymptomPatientDetails();
            auto *symptomList = mainWindow->getSymptoms();

            if (symptomPatientDetails && symptomList && !oldSpId.empty()) {
                auto *dn = symptomPatientDetails->getHeader()->next;
                while (dn && dn->next) {
                    if (dn->info.getSP_id() == oldSpId) {
                        std::string sid = dn->info.getS_id();
                        auto *s = symptomList->getHeader()->next;
                        while (s && s->next) {
                            if (s->info.getS_id() == sid) {
                                QString spId = UIHelper::qs(s->info.getSp_id());
                                spScores[spId] += s->info.getSScore();
                                break;
                            }
                            s = s->next;
                        }
                    }
                    dn = dn->next;
                }
            }

            struct ReschedDocItem {
                QString id;
                QString name;
                std::string spId;
                int score;
            };
            QList<ReschedDocItem> docList;

            if (mainWindow->getDoctors()) {
                auto *dn = mainWindow->getDoctors()->getHeader()->next;
                while (dn && dn->next) {
                    if (dn->info.getState() == 1) {
                        std::string sp = dn->info.getSpId();
                        int score = spScores.value(UIHelper::qs(sp), 0);
                        docList.append({UIHelper::qs(dn->info.getID()), UIHelper::qs(dn->info.getfullName()), sp, score});
                    }
                    dn = dn->next;
                }
            }

            libPBL2::qsort(docList, [](const ReschedDocItem &d1, const ReschedDocItem &d2) -> bool {
                if (d1.score != d2.score) {
                    return d1.score > d2.score;
                }
                if (d1.spId != d2.spId) {
                    return d1.spId < d2.spId;
                }
                return d1.name < d2.name;
            });

            for (const auto &item : docList) {
                QString label = item.name.startsWith("BS.") ? item.name : QStringLiteral("BS. %1").arg(item.name);
                docCombo->addItem(label, item.id);
            }

            int curIdx = docCombo->findData(UIHelper::qs(a->getDoctorId()));
            if (curIdx >= 0) docCombo->setCurrentIndex(curIdx);
            docBox->addWidget(docLbl);
            docBox->addWidget(docCombo);
            rLay->addLayout(docBox);

            Date curAd = a->getDateGet();
            QDate selD(curAd.getYear(), curAd.getMonth(), curAd.getDay());
            QString selSlot = QString::asprintf("%02d:%02d", curAd.getHour(), curAd.getMinute());

            auto *dateBox = new QVBoxLayout;
            dateBox->setSpacing(5);
            auto *dateLbl = new QLabel("Chọn ngày & khung giờ mới:");
            dateLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
            auto *dateBtn = new QPushButton(QStringLiteral("%1   [ %2 ]").arg(selD.toString("dd/MM/yyyy"), selSlot));
            dateBtn->setCursor(Qt::PointingHandCursor);
            dateBtn->setFixedHeight(38);
            dateBtn->setStyleSheet("QPushButton { background-color: #ffffff; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 6px 14px; font-size: 13px; color: #0f172a; font-weight: 700; text-align: left; } QPushButton:hover { border-color: #0d9488; background-color: #f0fdfa; }");
            dateBox->addWidget(dateLbl);
            dateBox->addWidget(dateBtn);
            rLay->addLayout(dateBox);

            auto *bottomPanel = new BottomCalendarTimeWidget(mainWindow, &rDlg);
            bottomPanel->setVisible(false);
            rLay->addWidget(bottomPanel);

            std::string origDocId = a->getDoctorId();
            QDate origDate = selD;
            QString origSlot = selSlot;
            std::string initDocId = UIHelper::ss(docCombo->currentData().toString());
            QString initOrigSlot = (initDocId == origDocId && selD == origDate) ? origSlot : "";
            bottomPanel->configure(initDocId, selD, a->getId(), initOrigSlot, (initDocId == origDocId ? selSlot : ""), patientId);

            QObject::connect(dateBtn, &QPushButton::clicked, &rDlg, [&rDlg, bottomPanel, docCombo, a, patientId, origDocId, origDate, origSlot, &selD, &selSlot] {
                bool willShow = !bottomPanel->isVisible();
                if (willShow) {
                    std::string curDocId = UIHelper::ss(docCombo->currentData().toString());
                    QString curOrigSlot = (curDocId == origDocId && selD == origDate) ? origSlot : "";
                    bottomPanel->configure(curDocId, selD, a->getId(), curOrigSlot, selSlot, patientId);
                }
                bottomPanel->setVisible(willShow);
                rDlg.adjustSize();
            });

            QObject::connect(docCombo, qOverload<int>(&QComboBox::currentIndexChanged), &rDlg, [docCombo, bottomPanel, a, patientId, dateBtn, &selD, &selSlot] {
                selSlot = "";
                if (dateBtn) {
                    if (!selD.isValid()) {
                        dateBtn->setText("Chọn lịch");
                    } else {
                        dateBtn->setText(selD.toString("dd/MM/yyyy"));
                    }
                }
                std::string curDocId = UIHelper::ss(docCombo->currentData().toString());
                if (bottomPanel) {
                    bottomPanel->configure(curDocId, selD, a->getId(), "", "", patientId);
                }
            });

            bottomPanel->setDateTimeSelectedCallback([dateBtn, &selD, &selSlot](const QDate &d, const QString &slot) {
                selD = d;
                selSlot = slot;
                if (dateBtn) {
                    if (!selD.isValid()) {
                        dateBtn->setText("Chọn lịch");
                    } else if (slot.isEmpty()) {
                        dateBtn->setText(d.toString("dd/MM/yyyy"));
                    } else {
                        dateBtn->setText(QStringLiteral("%1 - [ %2 ]").arg(d.toString("dd/MM/yyyy"), slot));
                    }
                }
            });

            auto *rBtnBox = new QHBoxLayout;
            rBtnBox->setSpacing(10);
            rBtnBox->addStretch();
            auto *rCancel = new QPushButton("Hủy");
            rCancel->setCursor(Qt::PointingHandCursor);
            rCancel->setFixedHeight(36);
            rCancel->setStyleSheet("QPushButton { background: #ffffff; color: #475569; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 0 16px; font-weight: 700; font-size: 12.5px; }");
            QObject::connect(rCancel, &QPushButton::clicked, &rDlg, &QDialog::reject);

            auto *rConfirm = new QPushButton("✓  Xác nhận dời lịch");
            rConfirm->setCursor(Qt::PointingHandCursor);
            rConfirm->setFixedHeight(36);
            rConfirm->setStyleSheet("QPushButton { background: #f59e0b; color: #ffffff; border: none; border-radius: 8px; padding: 0 20px; font-weight: 700; font-size: 12.5px; } QPushButton:hover { background: #d97706; }");

            QObject::connect(rConfirm, &QPushButton::clicked, &rDlg, [this, &rDlg, &dialog, a, patientId, docCombo, &selD, &selSlot] {
                if (a->getState() != 0) {
                    UIHelper::showWarning(&rDlg, "Không thể dời lịch", "Lịch khám này đã hoàn tất, đã trễ hoặc không còn ở trạng thái cho phép dời lịch.");
                    rDlg.reject();
                    dialog.reject();
                    return;
                }

                Date ad = a->getDateGet();
                QDateTime apptDT(QDate(ad.getYear(), ad.getMonth(), ad.getDay()), QTime(ad.getHour(), ad.getMinute()));
                if (apptDT.isValid() && QDateTime::currentDateTime() > apptDT.addSecs(20 * 60)) {
                    a->setState(-1);
                    if (mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal)) {
                        mainWindow->refreshPage(0);
                        mainWindow->refreshPage(1);
                        mainWindow->refreshPage(4);
                        mainWindow->refreshPage(5);
                    } else {
                        a->setState(0);
                    }
                    UIHelper::showWarning(&rDlg, "Hết hạn dời lịch", "Đã quá thời gian cho phép dời lịch khám (chỉ được dời trước hoặc trong vòng 20 phút sau giờ hẹn).");
                    rDlg.reject();
                    dialog.reject();
                    return;
                }

                std::string newDocId = UIHelper::ss(docCombo->currentData().toString());
                if (newDocId.empty()) {
                    UIHelper::showWarning(&rDlg, "Thiếu thông tin", "Vui lòng chọn bác sĩ khám.");
                    return;
                }
                if (!selD.isValid() || selSlot.isEmpty()) {
                    UIHelper::showWarning(&rDlg, "Thiếu thông tin", "Vui lòng chọn ngày và khung giờ khám hợp lệ.");
                    return;
                }
                QTime slotT = QTime::fromString(selSlot, "HH:mm");
                QDateTime newDT(selD, slotT);
                Date newTargetDate = Date::loadFromString(UIHelper::ss(newDT.toString("yyyy-MM-dd HH:mm")));

                bool isSameDoctor = (newDocId == a->getDoctorId());
                bool isSameDateTime = (newTargetDate.toString() == a->getDateGet().toString());
                if (isSameDoctor && isSameDateTime) {
                    UIHelper::showSuccess(&rDlg, "Thành công", "Lịch khám và bác sĩ phụ trách được giữ nguyên.");
                    rDlg.accept();
                    dialog.accept();
                    return;
                }

                try {
                    AppointmentController::validateBooking(
                        mainWindow->getAppointments(),
                        mainWindow->getSymptomPatients(),
                        patientId,
                        newDocId,
                        newDT,
                        a->getId(),
                        true,
                        "Config/Lichnghile.txt",
                        mainWindow->getDoctors(),
                        mainWindow->getPatients()
                    );
                } catch (const ValidationException &e) {
                    UIHelper::showWarning(&rDlg, "Không thể dời lịch", QString::fromUtf8(e.what()));
                    return;
                }

                a->setState(-2);

                auto *symptomPatients = mainWindow->getSymptomPatients();
                auto *symptomPatientDetails = mainWindow->getSymptomPatientDetails();
                std::string nextSpId = SymptomPatient::generateNextId(symptomPatients);
                std::string oldSpId = a->getSymptomPatientId();

                Doctor *newD = mainWindow->findDoctor(newDocId);
                std::string newSpSpec = newD ? newD->getSpId() : "";

                if (symptomPatients) {
                    symptomPatients->insert(SymptomPatient(nextSpId, patientId, newSpSpec, Date::now()));
                }
                if (symptomPatientDetails && !oldSpId.empty()) {
                    QList<std::string> sIds;
                    auto *dn = symptomPatientDetails->getHeader()->next;
                    while (dn && dn->next) {
                        if (dn->info.getSP_id() == oldSpId) {
                            sIds.append(dn->info.getS_id());
                        }
                        dn = dn->next;
                    }
                    for (const auto &sid : sIds) {
                        symptomPatientDetails->insert(SymptomPatientDetail(nextSpId, sid));
                    }
                }

                std::string newApptId = mainWindow->nextAppointmentId();
                if (mainWindow->getAppointments()) {
                    mainWindow->getAppointments()->insert(Appointment(newApptId, nextSpId, newDocId, newTargetDate, 0, -1.0));
                }

                if (!mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal)) {
                    AppointmentController::rollbackInsertedAppointments(
                        mainWindow->getAppointments(), symptomPatients, symptomPatientDetails,
                        {newApptId}, {nextSpId}, {a}
                    );
                    return;
                }
                mainWindow->refreshPage(0);
                mainWindow->refreshPage(1);
                mainWindow->refreshPage(4);
                mainWindow->refreshPage(5);
                UIHelper::showSuccess(&rDlg, "Thành công", "Đã dời lịch khám thành công.");
                rDlg.accept();
                dialog.accept();
            });

            rBtnBox->addWidget(rCancel);
            rBtnBox->addWidget(rConfirm);
            rLay->addLayout(rBtnBox);

            rDlg.exec();
        });

        btnRow->addWidget(confirmEnterBtn);
        btnRow->addWidget(reschedBtn);
    }

    root->addLayout(btnRow);
    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    dialog.exec();
}

void AppointmentView::editAppointmentDialog(const std::string &id) {
    if (!mainWindow || !mainWindow->getAppointments()) return;
    auto *n = mainWindow->getAppointments()->getHeader()->next;
    while (n && n->next) {
        if (n->info.getId() == id) {
            showAppointmentDetailDialog(&n->info);
            return;
        }
        n = n->next;
    }
}

void AppointmentView::showRescheduledSummaryDialog(
    QWidget *parent,
    const QList<RescheduledResultItem> &items,
    int totalAffected,
    const QString &customTitle
) {
    QDialog dialog(parent);
    dialog.setWindowTitle(customTitle.isEmpty() ? "Kết quả điều phối lịch khám" : customTitle);
    dialog.setMinimumWidth(700);
    dialog.setStyleSheet(UIHelper::dialogFormStyle() + R"(
        QLabel#reschedTitle { font-size: 17px; font-weight: 700; color: #0f172a; }
        QLabel#reschedSub { font-size: 13px; color: #64748b; }
        QScrollArea {
            background-color: #f8fafc;
            border: 1.5px solid #cbd5e1;
            border-radius: 10px;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            width: 0px;
            height: 0px;
            background: transparent;
            border: none;
        }
    )");

    auto *root = new QVBoxLayout(&dialog);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    auto *headerBox = new QVBoxLayout;
    headerBox->setSpacing(4);

    auto *topRow = new QHBoxLayout;
    auto *badge = new QLabel("✓ ĐIỀU PHỐI THÀNH CÔNG", &dialog);
    badge->setStyleSheet("background-color: #ccfbf1; color: #0f766e; font-weight: 800; font-size: 11px; padding: 4px 10px; border-radius: 6px;");
    topRow->addWidget(badge, 0, Qt::AlignLeft);
    topRow->addStretch();
    headerBox->addLayout(topRow);

    auto *title = new QLabel(customTitle.isEmpty() ? "Danh sách ca khám đã được tự động dời lịch" : customTitle, &dialog);
    title->setObjectName("reschedTitle");
    headerBox->addWidget(title);

    auto *subtitle = new QLabel(
        QStringLiteral("Hệ thống đã tự động điều phối thành công %1/%2 cuộc hẹn sang bác sĩ / ca khám khả dụng phù hợp.")
            .arg(items.size()).arg(totalAffected),
        &dialog
    );
    subtitle->setObjectName("reschedSub");
    headerBox->addWidget(subtitle);

    root->addLayout(headerBox);

    auto *scroll = new QScrollArea(&dialog);
    scroll->setWidgetResizable(true);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *container = new QWidget(scroll);
    container->setStyleSheet("background: transparent;");
    auto *stackLayout = new QVBoxLayout(container);
    stackLayout->setContentsMargins(10, 10, 10, 10);
    stackLayout->setSpacing(6);
    scroll->setWidget(container);

    const int PAGE_SIZE = 15;
    int totalItems = static_cast<int>(items.size());
    int totalPages = totalItems == 0 ? 1 : (totalItems + PAGE_SIZE - 1) / PAGE_SIZE;
    auto currentPage = std::make_shared<int>(0);

    auto *paginationContainer = new QWidget(&dialog);
    auto *pageLayout = new QHBoxLayout(paginationContainer);
    pageLayout->setContentsMargins(0, 4, 0, 4);
    pageLayout->setSpacing(10);

    auto *pageStatus = new QLabel(paginationContainer);
    pageStatus->setStyleSheet("font-size: 12px; font-weight: 700; color: #475569;");

    auto *prevBtn = new QPushButton("‹  Trang trước", paginationContainer);
    prevBtn->setCursor(Qt::PointingHandCursor);
    prevBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f1f5f9;
            color: #0f172a;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            padding: 5px 14px;
            font-weight: 700;
            font-size: 12px;
        }
        QPushButton:hover { background-color: #e2e8f0; border-color: #94a3b8; }
        QPushButton:disabled { background-color: #f8fafc; color: #cbd5e1; border-color: #e2e8f0; }
    )");

    auto *nextBtn = new QPushButton("Trang sau  ›", paginationContainer);
    nextBtn->setCursor(Qt::PointingHandCursor);
    nextBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f1f5f9;
            color: #0f172a;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            padding: 5px 14px;
            font-weight: 700;
            font-size: 12px;
        }
        QPushButton:hover { background-color: #e2e8f0; border-color: #94a3b8; }
        QPushButton:disabled { background-color: #f8fafc; color: #cbd5e1; border-color: #e2e8f0; }
    )");

    pageLayout->addWidget(pageStatus);
    pageLayout->addStretch();
    pageLayout->addWidget(prevBtn);
    pageLayout->addWidget(nextBtn);

    paginationContainer->setVisible(totalItems > PAGE_SIZE);

    auto renderPagePtr = std::make_shared<std::function<void(int)>>();
    auto renderPage = [renderPagePtr](int page) { if (*renderPagePtr) (*renderPagePtr)(page); };

    *renderPagePtr = [&dialog, container, stackLayout, scroll, &items, currentPage, totalPages, prevBtn, nextBtn, pageStatus, PAGE_SIZE](int page) {
        *currentPage = qBound(0, page, totalPages - 1);

        QLayoutItem *child;
        while ((child = stackLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                delete child->widget();
            }
            delete child;
        }

        int startIdx = *currentPage * PAGE_SIZE;
        int endIdx = qMin(startIdx + PAGE_SIZE, static_cast<int>(items.size()));

        for (int i = startIdx; i < endIdx; ++i) {
            const auto &it = items[i];

            auto *card = new QWidget(container);
            card->setFixedHeight(48);
            card->setObjectName("reschedCard");
            card->setStyleSheet(R"(
                QWidget#reschedCard {
                    background-color: #ffffff;
                    border: 1px solid #e2e8f0;
                    border-radius: 8px;
                }
                QWidget#reschedCard:hover {
                    border-color: #0d9488;
                    background-color: #f0fdfa;
                }
            )");

            auto *cardLayout = new QHBoxLayout(card);
            cardLayout->setContentsMargins(14, 4, 14, 4);
            cardLayout->setSpacing(12);

            auto *numLbl = new QLabel(QString::number(i + 1), card);
            numLbl->setFixedWidth(28);
            numLbl->setStyleSheet("color: #94a3b8; font-weight: 700; font-size: 12px;");
            cardLayout->addWidget(numLbl);

            auto *infoCol = new QVBoxLayout;
            infoCol->setSpacing(1);

            auto *patLbl = new QLabel(QStringLiteral("<b>%1</b>").arg(it.patientName), card);
            patLbl->setStyleSheet("font-size: 13px; color: #0f172a;");
            infoCol->addWidget(patLbl);

            QString docText = !it.oldDoctorName.isEmpty()
                ? QStringLiteral("BS: %1  ➔  <b>%2</b>").arg(it.oldDoctorName, it.doctorName)
                : QStringLiteral("BS phụ trách: <b>%1</b>").arg(it.doctorName);
            auto *docLbl = new QLabel(docText, card);
            docLbl->setStyleSheet("font-size: 11.5px; color: #64748b; font-weight: 500;");
            infoCol->addWidget(docLbl);

            cardLayout->addLayout(infoCol, 1);

            auto *timeCol = new QHBoxLayout;
            timeCol->setSpacing(8);
            timeCol->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

            if (it.oldDateTime == it.newDateTime) {
                auto *sameBadge = new QLabel(QStringLiteral("%1 (Giữ nguyên giờ)").arg(it.newDateTime), card);
                sameBadge->setStyleSheet("background: #dcfce7; color: #15803d; font-size: 11.5px; font-weight: 800; padding: 4px 10px; border-radius: 5px;");
                timeCol->addWidget(sameBadge);
            } else {
                auto *oldBadge = new QLabel(it.oldDateTime, card);
                oldBadge->setStyleSheet("background: #fee2e2; color: #b91c1c; font-size: 11px; font-weight: 700; padding: 4px 8px; border-radius: 5px; text-decoration: line-through;");
                timeCol->addWidget(oldBadge);

                auto *arrowLbl = new QLabel("➔", card);
                arrowLbl->setStyleSheet("color: #0d9488; font-weight: 900; font-size: 13px;");
                timeCol->addWidget(arrowLbl);

                auto *newBadge = new QLabel(it.newDateTime, card);
                newBadge->setStyleSheet("background: #dcfce7; color: #15803d; font-size: 11.5px; font-weight: 800; padding: 4px 10px; border-radius: 5px;");
                timeCol->addWidget(newBadge);
            }

            cardLayout->addLayout(timeCol);

            stackLayout->addWidget(card);
        }

        stackLayout->addStretch();

        pageStatus->setText(QStringLiteral("Trang %1 / %2 (Tổng %3 ca)").arg(*currentPage + 1).arg(totalPages).arg(items.size()));
        prevBtn->setEnabled(*currentPage > 0);
        nextBtn->setEnabled(*currentPage < totalPages - 1);

        if (scroll->verticalScrollBar()) {
            scroll->verticalScrollBar()->setValue(0);
        }
    };

    QObject::connect(prevBtn, &QPushButton::clicked, &dialog, [renderPage, currentPage]() {
        if (*currentPage > 0) {
            renderPage(*currentPage - 1);
        }
    });

    QObject::connect(nextBtn, &QPushButton::clicked, &dialog, [renderPage, currentPage, totalPages]() {
        if (*currentPage < totalPages - 1) {
            renderPage(*currentPage + 1);
        }
    });

    renderPage(0);

    int itemsOnFirstPage = std::min(totalItems, PAGE_SIZE);
    int calculatedHeight = std::max(120, itemsOnFirstPage * 54 + 20);
    if (calculatedHeight > 540) calculatedHeight = 540;
    scroll->setFixedHeight(calculatedHeight);

    root->addWidget(scroll, 1);
    root->addWidget(paginationContainer);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();

    auto *closeBtn = new QPushButton("✓  Đã hiểu và hoàn tất", &dialog);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedHeight(38);
    closeBtn->setStyleSheet("QPushButton { background-color: #0d9488; color: white; border: none; border-radius: 8px; padding: 0 24px; font-weight: 700; font-size: 13px; } QPushButton:hover { background-color: #0f766e; }");
    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    btnRow->addWidget(closeBtn);
    root->addLayout(btnRow);

    dialog.exec();
}

bool AppointmentView::processHolidayAdditionWithReschedule(
    QWidget *parentDialog,
    const QList<HolidayEntry> &curList,
    const QList<HolidayEntry> &prospectiveList,
    const std::string &configPath
) {
    if (!mainWindow || !mainWindow->getAppointments()) {
        if (!AppointmentController::saveHolidays(configPath, prospectiveList)) {
            UIHelper::showWarning(parentDialog, "Lỗi lưu cấu hình", "Không thể lưu danh sách ngày nghỉ lễ.");
            return false;
        }
        AppointmentController::clearHolidayCache();
        return true;
    }

    auto affectedList = AppointmentController::getAffectedAppointmentsForHoliday(
        mainWindow->getAppointments(), curList, prospectiveList
    );

    if (!affectedList.isEmpty()) {
        QString warnMsg = QString(
            "Phát hiện có %1 lịch khám đang chờ trong ngày nghỉ lễ này.\n\n"
            "Hệ thống sẽ tự động dời các lịch khám sang các ca khám trống sớm nhất tiếp theo của từng bác sĩ phụ trách.\n\n"
            "Bạn có muốn tiếp tục thêm ngày nghỉ và tự động dời lịch khám không?"
        ).arg(affectedList.size());

        if (!UIHelper::confirmWarning(parentDialog, "Xác nhận dời lịch khám", warnMsg, "Đồng ý dời lịch", "Hủy bỏ")) {
            return false;
        }

        if (!AppointmentController::saveHolidays(configPath, prospectiveList)) {
            UIHelper::showWarning(parentDialog, "Lỗi lưu cấu hình", "Không thể lưu danh sách ngày nghỉ lễ. Hủy thao tác dời lịch.");
            return false;
        }
        AppointmentController::clearHolidayCache();

        QList<RescheduledResultItem> resultItems;
        QList<std::string> insertedApptIds;
        QList<std::string> insertedSpIds;
        QList<Appointment*> modifiedOldAppts;
        int successCount = AppointmentController::rescheduleAppointmentsLinear(
            mainWindow->getAppointments(),
            mainWindow->getSymptomPatients(),
            mainWindow->getSymptomPatientDetails(),
            mainWindow->getDoctors(),
            mainWindow->getPatients(),
            affectedList,
            prospectiveList,
            &resultItems,
            mainWindow->getSymptoms(),
            &insertedApptIds,
            &insertedSpIds,
            &modifiedOldAppts
        );

        if (successCount < affectedList.size()) {
            AppointmentController::rollbackInsertedAppointments(
                mainWindow->getAppointments(),
                mainWindow->getSymptomPatients(),
                mainWindow->getSymptomPatientDetails(),
                insertedApptIds, insertedSpIds, modifiedOldAppts
            );
            AppointmentController::saveHolidays(configPath, curList);
            AppointmentController::clearHolidayCache();
            UIHelper::showWarning(parentDialog, "Dời lịch không hoàn tất",
                QStringLiteral("Không thể tìm thấy khung giờ khám phù hợp trong vòng 365 ngày cho %1 ca khám bị ảnh hưởng. Thao tác thêm ngày nghỉ lễ đã bị hủy bỏ để bảo toàn dữ liệu.")
                    .arg(affectedList.size() - successCount));
            return false;
        }

        if (!mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal)) {
            AppointmentController::rollbackInsertedAppointments(
                mainWindow->getAppointments(),
                mainWindow->getSymptomPatients(),
                mainWindow->getSymptomPatientDetails(),
                insertedApptIds, insertedSpIds, modifiedOldAppts
            );
            AppointmentController::saveHolidays(configPath, curList);
            AppointmentController::clearHolidayCache();
            return false;
        }

        mainWindow->refreshPage(0);
        mainWindow->refreshPage(4);
        mainWindow->refreshPage(5);

        showRescheduledSummaryDialog(parentDialog, resultItems, affectedList.size());
    } else {

        if (!AppointmentController::saveHolidays(configPath, prospectiveList)) {
            UIHelper::showWarning(parentDialog, "Lỗi lưu cấu hình", "Không thể lưu danh sách ngày nghỉ lễ.");
            return false;
        }
        AppointmentController::clearHolidayCache();
    }

    return true;
}

void AppointmentView::showHolidayManagementDialog(QWidget *parent) {
    if (!mainWindow) return;

    QDialog dialog(mainWindow ? static_cast<QWidget*>(mainWindow) : parent);
    dialog.setWindowTitle("Quản lý lịch nghỉ lễ");
    dialog.setMinimumWidth(660);
    dialog.setMinimumHeight(460);
    dialog.setStyleSheet(UIHelper::dialogFormStyle() + R"(
        QLabel#dialogTitle { font-size: 18px; font-weight: 700; color: #0f172a; }
        QTableWidget {
            background-color: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 10px;
            gridline-color: transparent;
        }
        QHeaderView { border: none; background: transparent; }
        QHeaderView::section {
            background: #f8fafc;
            color: #475569;
            font-weight: 700;
            font-size: 11.5px;
            padding: 10px 14px;
            border: none;
            border-bottom: 2px solid #e2e8f0;
            border-left: none;
            border-right: none;
        }
        QTableWidget::item {
            background: #ffffff;
            border: none;
            border-bottom: 1px solid #f1f5f9;
            border-left: none;
            border-right: none;
        }
        QTableWidget::item:hover { background: #f0fdfa; }
        QScrollBar:vertical, QScrollBar:horizontal {
            width: 0px;
            height: 0px;
            background: transparent;
            border: none;
        }
    )");

    auto *rootLayout = new QVBoxLayout(&dialog);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(14);

    auto *headerLayout = new QHBoxLayout;
    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(3);
    auto *title = new QLabel("Quản lý lịch nghỉ lễ", &dialog);
    title->setObjectName("dialogTitle");
    titleCol->addWidget(title);
    headerLayout->addLayout(titleCol, 1);

    auto *addBtn = UIHelper::button("＋  Thêm ngày nghỉ", "primary");
    addBtn->setStyleSheet("QPushButton { background-color: #0d9488; color: white; border: none; border-radius: 8px; padding: 8px 16px; font-weight: 700; font-size: 13px; } QPushButton:hover { background-color: #0f766e; }");
    headerLayout->addWidget(addBtn, 0, Qt::AlignVCenter);
    rootLayout->addLayout(headerLayout);

    auto *t = UIHelper::table({"NGÀY NGHỈ", "GHI CHÚ / THỜI GIAN", "THAO TÁC"});
    t->setShowGrid(false);
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    t->setColumnWidth(0, 160);
    t->setColumnWidth(2, 140);
    t->verticalHeader()->setDefaultSectionSize(48);
    rootLayout->addWidget(t, 1);

    auto validateHolidayInput = [](const QString &dateStr, const QString &noteStr, const QList<HolidayEntry> &existing, int ignoreIdx, QString &errMsg) -> bool {
        if (dateStr.isEmpty()) {
            errMsg = "Vui lòng nhập ngày nghỉ.";
            return false;
        }
        if (dateStr.contains('-') || noteStr.contains('-')) {
            errMsg = "Ngày nghỉ và ghi chú không được chứa dấu gạch ngang '-'.";
            return false;
        }
        bool isTet = (dateStr.compare("Tet", Qt::CaseInsensitive) == 0);
        if (isTet) {
            if (!noteStr.isEmpty()) {
                bool ok = false;
                int days = noteStr.toInt(&ok);
                if (!ok || days < 1 || days > 30) {
                    errMsg = "Số ngày nghỉ Tết phải là số nguyên từ 1 đến 30.";
                    return false;
                }
            }
        } else {
            QStringList parts = dateStr.split('/');
            if (parts.size() != 2 && parts.size() != 3) {
                errMsg = "Ngày nghỉ phải có định dạng dd/MM hoặc dd/MM/yyyy (Ví dụ: 30/4 hoặc 2/9/2026).";
                return false;
            }
            bool okD = false, okM = false, okY = true;
            int d = parts[0].toInt(&okD);
            int m = parts[1].toInt(&okM);
            int y = (parts.size() == 3) ? parts[2].toInt(&okY) : 2024;
            if (!okD || !okM || !okY || !QDate::isValid(y, m, d)) {
                errMsg = "Ngày/tháng không hợp lệ. Vui lòng nhập đúng định dạng dd/MM hoặc dd/MM/yyyy.";
                return false;
            }
        }
        for (int k = 0; k < existing.size(); ++k) {
            if (k == ignoreIdx) continue;
            if (existing[k].date.trimmed().compare(dateStr, Qt::CaseInsensitive) == 0) {
                errMsg = QStringLiteral("Ngày nghỉ '%1' đã tồn tại trong danh sách.").arg(dateStr);
                return false;
            }
        }
        return true;
    };

    auto fillPtr = std::make_shared<std::function<void(bool, int)>>();
    std::weak_ptr<std::function<void(bool, int)>> weakFill = fillPtr;
    *fillPtr = [this, t, &dialog, weakFill, validateHolidayInput](bool preserveScroll, int preferredRow) {
        int vScroll = (preserveScroll && t->verticalScrollBar()) ? t->verticalScrollBar()->value() : 0;
        int curRow = preserveScroll ? (preferredRow >= 0 ? preferredRow : t->currentRow()) : -1;

        t->setUpdatesEnabled(false);
        t->setRowCount(0);
        if (t->viewport()) {
            t->viewport()->setCursor(Qt::ArrowCursor);
        }
        auto list = AppointmentController::loadHolidays("Config/Lichnghile.txt");
        for (int i = 0; i < list.size(); ++i) {
            const auto &item = list[i];
            int r = t->rowCount();
            t->insertRow(r);

            QString displayDate = item.date;
            QString displayNote = item.note;

            bool isTet = item.date.compare("Tet", Qt::CaseInsensitive) == 0;
            if (isTet) {
                displayDate = "Tết Nguyên Đán";
                int curYear = QDate::currentDate().year();
                QDate mung1 = AppointmentController::getLunarNewYear(curYear);
                bool okDays = false;
                int tetDays = item.note.trimmed().toInt(&okDays);
                if (!okDays || tetDays < 1) tetDays = 5;
                displayNote = QStringLiteral("Nghỉ %1 ngày (Mùng 1 Tết: %2)").arg(tetDays).arg(mung1.toString("dd/MM/yyyy"));
            }

            auto *dateItem = new QTableWidgetItem(displayDate);
            dateItem->setTextAlignment(Qt::AlignCenter);
            if (isTet) {
                dateItem->setForeground(QColor("#b91c1c"));
                QFont font = dateItem->font();
                font.setBold(true);
                dateItem->setFont(font);
            }
            t->setItem(r, 0, dateItem);

            auto *noteItem = new QTableWidgetItem(displayNote);
            if (isTet) {
                noteItem->setForeground(QColor("#b91c1c"));
                QFont font = noteItem->font();
                font.setBold(true);
                noteItem->setFont(font);
            }
            t->setItem(r, 1, noteItem);

            auto *aw = new QWidget;
            auto *al = new QHBoxLayout(aw);
            al->setContentsMargins(4, 4, 4, 4);
            al->setSpacing(6);
            al->setAlignment(Qt::AlignCenter);

            auto *editBtn = new QPushButton("Sửa");
            editBtn->setCursor(Qt::PointingHandCursor);
            editBtn->setStyleSheet("QPushButton { background: #f1f5f9; color: #0d9488; border: 1px solid #cbd5e1; border-radius: 6px; padding: 4px 10px; font-weight: 700; font-size: 12px; } QPushButton:hover { background: #e2e8f0; }");

            auto *delBtn = new QPushButton("Xóa");
            delBtn->setCursor(Qt::PointingHandCursor);
            delBtn->setStyleSheet("QPushButton { background: #fee2e2; color: #dc2626; border: 1px solid #fca5a5; border-radius: 6px; padding: 4px 10px; font-weight: 700; font-size: 12px; } QPushButton:hover { background: #fecaca; }");

            bool canEditHoliday = mainWindow && mainWindow->hasPermission("Per_5");
            if (!canEditHoliday) {
                editBtn->setToolTip("Bạn không có quyền chỉnh sửa ngày nghỉ (Yêu cầu quyền Per_5)");
                delBtn->setToolTip("Bạn không có quyền xóa ngày nghỉ (Yêu cầu quyền Per_5)");
            }

            al->addWidget(editBtn);
            al->addWidget(delBtn);
            t->setCellWidget(r, 2, aw);

            QObject::connect(editBtn, &QPushButton::clicked, &dialog, [this, &dialog, i, item, isTet, weakFill, validateHolidayInput]() {
                if (!mainWindow || !mainWindow->hasPermission("Per_5")) {
                    UIHelper::showWarning(&dialog, "Không có quyền", "Bạn không có quyền chỉnh sửa lịch nghỉ.");
                    return;
                }
                QDialog editDlg(&dialog);
                editDlg.setWindowTitle("Chỉnh sửa ngày nghỉ");
                editDlg.setMinimumWidth(380);
                editDlg.setStyleSheet(UIHelper::dialogFormStyle());
                auto *ev = new QVBoxLayout(&editDlg);
                ev->setContentsMargins(20, 20, 20, 20);
                ev->setSpacing(14);

                auto *ef = new QFormLayout;
                auto *dateEdit = new QLineEdit(item.date);
                auto *noteEdit = new QLineEdit(item.note);
                if (isTet) {
                    dateEdit->setEnabled(false);
                    noteEdit->setPlaceholderText("Số ngày nghỉ Tết (mặc định 5)");
                }
                ef->addRow("Mã / Ngày nghỉ:", dateEdit);
                ef->addRow("Ghi chú / Số ngày:", noteEdit);
                ev->addLayout(ef);

                auto *btnBox = new QHBoxLayout;
                btnBox->addStretch();
                auto *saveB = new QPushButton("Lưu");
                saveB->setAutoDefault(false);
                saveB->setDefault(false);
                saveB->setCursor(Qt::PointingHandCursor);
                saveB->setStyleSheet("QPushButton { background-color: #0d9488; color: white; border: none; border-radius: 7px; padding: 8px 18px; font-weight: 700; } QPushButton:hover { background-color: #0f766e; }");
                btnBox->addWidget(saveB);
                ev->addLayout(btnBox);

                QObject::connect(saveB, &QPushButton::clicked, &editDlg, [this, &editDlg, dateEdit, noteEdit, item, validateHolidayInput]() {
                    auto curList = AppointmentController::loadHolidays("Config/Lichnghile.txt");
                    QString newDate = dateEdit->text().trimmed();
                    QString newNote = noteEdit->text().trimmed();
                    QString errMsg;
                    int curIdx = -1;
                    for (int k = 0; k < curList.size(); ++k) {
                        if (curList[k].date.trimmed().compare(item.date.trimmed(), Qt::CaseInsensitive) == 0) {
                            curIdx = k;
                            break;
                        }
                    }
                    if (!validateHolidayInput(newDate, newNote, curList, curIdx, errMsg)) {
                        UIHelper::showWarning(&editDlg, "Dữ liệu không hợp lệ", errMsg);
                        return;
                    }
                    auto prospectiveList = curList;
                    if (curIdx >= 0 && curIdx < prospectiveList.size()) {
                        prospectiveList[curIdx].date = newDate;
                        prospectiveList[curIdx].note = newNote;
                    }
                    if (!processHolidayAdditionWithReschedule(&editDlg, curList, prospectiveList, "Config/Lichnghile.txt")) {
                        return;
                    }
                    editDlg.accept();
                });

                if (editDlg.exec() == QDialog::Accepted) {
                    if (auto sp = weakFill.lock()) (*sp)(true, -1);
                }
            });

            QObject::connect(delBtn, &QPushButton::clicked, &dialog, [this, &dialog, item, weakFill]() {
                if (!mainWindow || !mainWindow->hasPermission("Per_5")) {
                    UIHelper::showWarning(&dialog, "Không có quyền", "Bạn không có quyền xóa lịch nghỉ.");
                    return;
                }
                if (UIHelper::confirmDelete(&dialog, "Xác nhận xóa ngày nghỉ",
                    QStringLiteral("Bạn có chắc chắn muốn xóa ngày nghỉ '%1' khỏi lịch phòng khám?").arg(item.date), "Xóa ngày nghỉ", "Hủy bỏ")) {
                    auto curList = AppointmentController::loadHolidays("Config/Lichnghile.txt");
                    int removeIdx = -1;
                    for (int k = 0; k < curList.size(); ++k) {
                        if (curList[k].date.trimmed().compare(item.date.trimmed(), Qt::CaseInsensitive) == 0) {
                            removeIdx = k;
                            break;
                        }
                    }
                    if (removeIdx != -1) {
                        curList.removeAt(removeIdx);
                        if (!AppointmentController::saveHolidays("Config/Lichnghile.txt", curList)) {
                            UIHelper::showWarning(&dialog, "Lỗi lưu cấu hình", "Không thể lưu thay đổi lịch nghỉ lễ.");
                            return;
                        }
                        AppointmentController::clearHolidayCache();
                        if (auto sp = weakFill.lock()) (*sp)(true, removeIdx);
                    }
                }
            });
        }
        t->setUpdatesEnabled(true);

        if (preserveScroll) {
            int targetRow = curRow;
            if (targetRow >= t->rowCount()) {
                targetRow = t->rowCount() - 1;
            }
            if (t->verticalScrollBar()) {
                t->verticalScrollBar()->setValue(vScroll);
            }
            if (targetRow >= 0 && targetRow < t->rowCount()) {
                t->setCurrentCell(targetRow, 0);
            }
            QTimer::singleShot(0, t, [t, vScroll, targetRow]() {
                if (t && t->verticalScrollBar()) {
                    t->verticalScrollBar()->setValue(vScroll);
                }
                if (t && targetRow >= 0 && targetRow < t->rowCount()) {
                    t->setCurrentCell(targetRow, 0);
                }
            });
        }
    };
    auto fill = [fillPtr](bool preserveScroll = true, int preferredRow = -1) {
        if (*fillPtr) (*fillPtr)(preserveScroll, preferredRow);
    };

    if (mainWindow && !mainWindow->hasPermission("Per_5")) {
        addBtn->setToolTip("Bạn không có quyền thêm ngày nghỉ (Yêu cầu quyền Per_5)");
    }

    QObject::connect(addBtn, &QPushButton::clicked, &dialog, [this, &dialog, fill, t, validateHolidayInput]() {
        if (!mainWindow || !mainWindow->hasPermission("Per_5")) {
            UIHelper::showWarning(&dialog, "Không có quyền", "Bạn không có quyền thêm ngày nghỉ.");
            return;
        }
        QDialog addDlg(&dialog);
        addDlg.setWindowTitle("Thêm ngày nghỉ mới");
        addDlg.setMinimumWidth(380);
        addDlg.setStyleSheet(UIHelper::dialogFormStyle());
        auto *ev = new QVBoxLayout(&addDlg);
        ev->setContentsMargins(20, 20, 20, 20);
        ev->setSpacing(14);

        auto *ef = new QFormLayout;
        auto *dateType = new QComboBox;
        dateType->setView(new QListView(dateType));
        dateType->addItem("Ngày lễ cố định (ngày/tháng)");
        dateType->addItem("Tết Nguyên Đán (Âm lịch - 5 ngày)");

        auto *dateEdit = new QLineEdit;
        dateEdit->setPlaceholderText("Ví dụ: 2/9, 30/4, 1/1...");
        auto *noteEdit = new QLineEdit;
        noteEdit->setPlaceholderText("Ghi chú ngày nghỉ lễ");

        ef->addRow("Loại ngày nghỉ:", dateType);
        ef->addRow("Ngày nghỉ:", dateEdit);
        ef->addRow("Ghi chú:", noteEdit);
        ev->addLayout(ef);

        QObject::connect(dateType, qOverload<int>(&QComboBox::currentIndexChanged), &addDlg, [dateEdit, noteEdit](int idx) {
            if (idx == 1) {
                dateEdit->setText("Tet");
                dateEdit->setEnabled(false);
                noteEdit->setText("5");
            } else {
                dateEdit->clear();
                dateEdit->setEnabled(true);
                noteEdit->clear();
            }
        });

        auto *btnBox = new QHBoxLayout;
        btnBox->addStretch();
        auto *saveB = new QPushButton("Thêm");
        saveB->setAutoDefault(false);
        saveB->setDefault(false);
        saveB->setCursor(Qt::PointingHandCursor);
        saveB->setStyleSheet("QPushButton { background-color: #0d9488; color: white; border: none; border-radius: 7px; padding: 8px 18px; font-weight: 700; } QPushButton:hover { background-color: #0f766e; }");
        btnBox->addWidget(saveB);
        ev->addLayout(btnBox);

        QObject::connect(saveB, &QPushButton::clicked, &addDlg, [this, &addDlg, dateEdit, noteEdit, validateHolidayInput]() {
            auto curList = AppointmentController::loadHolidays("Config/Lichnghile.txt");
            QString newDate = dateEdit->text().trimmed();
            QString newNote = noteEdit->text().trimmed();
            QString errMsg;
            if (!validateHolidayInput(newDate, newNote, curList, -1, errMsg)) {
                UIHelper::showWarning(&addDlg, "Dữ liệu không hợp lệ", errMsg);
                return;
            }
            auto prospectiveList = curList;
            prospectiveList.append({newDate, newNote});
            if (!processHolidayAdditionWithReschedule(&addDlg, curList, prospectiveList, "Config/Lichnghile.txt")) {
                return;
            }
            addDlg.accept();
        });

        if (addDlg.exec() == QDialog::Accepted) {
            fill(true, -1);
            QTimer::singleShot(50, t, [t]() {
                if (t && t->rowCount() > 0) {
                    int lastRow = t->rowCount() - 1;
                    t->setCurrentCell(lastRow, 0);
                    if (t->verticalScrollBar()) {
                        t->verticalScrollBar()->setValue(t->verticalScrollBar()->maximum());
                    }
                }
            });
        }
    });

    fill(false);
    dialog.exec();
}

void AppointmentView::reapplyCurrentSort() {
    if (!mainWindow || !mainWindow->getAppointments()) return;
    if (listSortColumn < 0) {
        libPBL2::sortAppointments(mainWindow->getAppointments(), libPBL2::AppointmentSortBy::ID, false);
        return;
    }
    int col = listSortColumn;
    bool asc = listSortAscending;
    libPBL2::qsort(mainWindow->getAppointments(), [this, col, asc](const Appointment &a, const Appointment &b) -> bool {
        int cmp = 0;
        switch (col) {
            case 0: {
                Patient *pa = mainWindow ? mainWindow->findPatientForAppointment(a) : nullptr;
                Patient *pb = mainWindow ? mainWindow->findPatientForAppointment(b) : nullptr;
                std::string na = pa ? pa->getfullName() : "";
                std::string nb = pb ? pb->getfullName() : "";
                cmp = libPBL2::compareVietnameseNames(na, nb);
                break;
            }
            case 1: {
                Doctor *da = mainWindow ? mainWindow->findDoctor(a.getDoctorId()) : nullptr;
                Doctor *db = mainWindow ? mainWindow->findDoctor(b.getDoctorId()) : nullptr;
                std::string na = da ? da->getfullName() : "";
                std::string nb = db ? db->getfullName() : "";
                cmp = libPBL2::compareVietnameseNames(na, nb);
                break;
            }
            case 2: {
                Doctor *da = mainWindow ? mainWindow->findDoctor(a.getDoctorId()) : nullptr;
                Doctor *db = mainWindow ? mainWindow->findDoctor(b.getDoctorId()) : nullptr;
                QString sa = (da && mainWindow) ? mainWindow->specialty(da->getSpId()) : "";
                QString sb = (db && mainWindow) ? mainWindow->specialty(db->getSpId()) : "";
                cmp = QString::compare(sa, sb, Qt::CaseInsensitive);
                break;
            }
            case 3: {
                cmp = libPBL2::compareDates(a.getDateGet(), b.getDateGet());
                break;
            }
            case 4: {
                QString sa = getAppointmentStatusText(a);
                QString sb = getAppointmentStatusText(b);
                cmp = QString::compare(sa, sb, Qt::CaseInsensitive);
                break;
            }
            default:
                cmp = libPBL2::compareDates(a.getDateGet(), b.getDateGet());
                break;
        }
        if (cmp == 0) {
            cmp = libPBL2::compareNatural(a.getId(), b.getId());
        }
        return asc ? (cmp < 0) : (cmp > 0);
    });
}

void AppointmentView::resetToFirstPage() {
    listSortColumn = -1;
    listSortAscending = false;
    if (mainWindow && mainWindow->getAppointments()) {
        libPBL2::sortAppointments(mainWindow->getAppointments(), libPBL2::AppointmentSortBy::ID, false);
    }
}
