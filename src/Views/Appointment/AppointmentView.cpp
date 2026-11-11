#include "AppointmentView.h"
#include "libPBL2.h"
#include "MainWindow.h"
#include "AppointmentController.h"
#include "DoctorController.h"
#include "PatientController.h"
#include "StatisticController.h"
#include "UIHelper.h"

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
#include <QDateTimeEdit>
#include <QDateEdit>
#include <QCalendarWidget>
#include <QTextCharFormat>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QCheckBox>
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
        if (event->type() == QEvent::Show) {
            if (onShow) onShow();
        }
        return QObject::eventFilter(obj, event);
    }
};

QString getAppointmentStatusText(const Appointment &a) {
    if (a.getState() == 1) return QStringLiteral("Đã khám");
    if (a.getState() == -1) return QStringLiteral("Trễ hẹn");
    Date ad = a.getDateGet();
    QDateTime now = QDateTime::currentDateTime();
    QDateTime apptDt(QDate(ad.getYear(), ad.getMonth(), ad.getDay()), QTime(ad.getHour(), ad.getMinute()));
    if (apptDt < now) {
        return QStringLiteral("Trễ hẹn");
    }
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

    // Xóa màu đỏ của Thứ 7 & Chủ Nhật trên tiêu đề tuần (bỏ hoàn toàn màu đỏ)
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

    // Lưới hiển thị của QCalendarWidget gồm 6 tuần = 42 ô
    QDate firstDay(year, month, 1);
    QDate gridStart = firstDay.addDays(-14);
    QDate gridEnd = firstDay.addDays(45);

    for (QDate d = gridStart; d <= gridEnd; d = d.addDays(1)) {
        QTextCharFormat fmt;
        if (d.month() != month) {
            // Ngày không thuộc tháng hiện tại: nền cell màu xám
            fmt.setBackground(QColor("#e2e8f0"));
            fmt.setForeground(QColor("#64748b"));
            fmt.setFontWeight(QFont::Normal);
        } else {
            // Ngày trong tháng
            bool isHol = mainWindow->isHoliday(d);
            if (isHol) {
                // Ngày nghỉ lễ (5 ngày Tết và các ngày lễ quy định)
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

class FormNoWheelFilter : public QObject {
public:
    static FormNoWheelFilter* instance() {
        static FormNoWheelFilter inst;
        return &inst;
    }
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Wheel) {
            auto *cb = qobject_cast<QComboBox*>(obj);
            if (cb) {
                event->ignore();
                return true;
            }
            auto *dt = qobject_cast<QDateTimeEdit*>(obj);
            if (dt) {
                event->ignore();
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

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

class ClickableLabel : public QLabel {
public:
    ToggleSwitch *targetSwitch = nullptr;
    explicit ClickableLabel(const QString &text, QWidget *parent = nullptr)
        : QLabel(text, parent) {}
protected:
    void mouseReleaseEvent(QMouseEvent *e) override {
        if (targetSwitch && e->button() == Qt::LeftButton) {
            targetSwitch->toggle();
        }
        QLabel::mouseReleaseEvent(e);
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
        installEventFilter(FormNoWheelFilter::instance());
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
    bool layoutConfigured = false;

public:
    explicit SearchablePatientComboBox(QWidget *parent = nullptr) : NoWheelComboBox(parent) {
        auto *lv = qobject_cast<QListView*>(this->view());
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

        searchEdit = new SearchLineEdit;
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

    SearchLineEdit* getSearchEdit() const { return searchEdit; }

    void showPopup() override {
        if (view()) {
            view()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        NoWheelComboBox::showPopup();
        QWidget *popup = this->view() ? this->view()->parentWidget() : nullptr;
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
            if (!layoutConfigured) {
                if (auto *boxLay = qobject_cast<QBoxLayout*>(popup->layout())) {
                    boxLay->setContentsMargins(4, 4, 4, 6);
                    boxLay->setSpacing(2);
                    boxLay->insertWidget(0, searchEdit);
                    layoutConfigured = true;
                }
            }
        }
        if (searchEdit) {
            searchEdit->clear();
            filterItems("");
            QTimer::singleShot(20, searchEdit, [this]() {
                searchEdit->setFocus();
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
    QCalendarWidget *calendar = nullptr;
    QLabel *dateHeaderLabel = nullptr;
    QWidget *slotsContainer = nullptr;
    QVBoxLayout *slotsLayout = nullptr;
    QList<QPushButton*> slotButtons;

    QDate initialDate;
    QDate selectedDate;
    std::string currentDoctorId;
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

    void configure(const std::string &doctorId, const QDate &date = QDate(), const std::string &excludeApptId = "", const QString &initSlot = "", const QString &curSelect = "") {
        currentDoctorId = doctorId;
        initialDate = date;
        selectedDate = date.isValid() ? date : QDate::currentDate();
        currentExcludeApptId = excludeApptId;
        initialSlot = initSlot;
        selectedSlot = curSelect.isEmpty() ? initSlot : curSelect;

        if (calendar) {
            calendar->blockSignals(true);
            calendar->setSelectedDate(selectedDate);
            calendar->blockSignals(false);
        }
        refreshSlots();
    }

    void resetToDefault() {
        initialDate = QDate();
        selectedDate = QDate();
        selectedSlot.clear();
        initialSlot.clear();
        currentDoctorId.clear();
        currentExcludeApptId.clear();
        if (calendar) {
            calendar->blockSignals(true);
            calendar->setSelectedDate(QDate::currentDate());
            calendar->blockSignals(false);
        }
        refreshSlots();
    }

    QDate getSelectedDate() const { return selectedDate; }
    QString getSelectedSlot() const { return selectedSlot; }

private:
    void setupUI() {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(10, 10, 10, 10);
        root->setSpacing(8);

        // Header
        auto *headerBox = new QHBoxLayout;
        headerBox->setSpacing(8);
        auto *iconLbl = new QLabel("📅");
        iconLbl->setStyleSheet("font-size: 16px;");
        auto *titleBox = new QVBoxLayout;
        titleBox->setSpacing(1);
        auto *titleLabel = new QLabel("Chọn lịch & Giờ khám");
        titleLabel->setStyleSheet("font-size: 13px; font-weight: 750; color: #0f172a;");
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

        // Scroll Area chứa Calendar và Slots
        auto *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setStyleSheet("background: transparent; border: none;");

        auto *scrollContent = new QWidget;
        scrollContent->setStyleSheet("background: transparent;");
        auto *contentLay = new QVBoxLayout(scrollContent);
        contentLay->setContentsMargins(0, 0, 0, 0);
        contentLay->setSpacing(8);

        // 1. Calendar
        calendar = new QCalendarWidget(scrollContent);
        UIHelper::applyCalendarStyle(calendar);
        calendar->setMinimumDate(QDate::currentDate());
        contentLay->addWidget(calendar);

        connect(calendar, &QCalendarWidget::clicked, this, [this](const QDate &d) {
            if (mainWindow && mainWindow->isHoliday(d)) {
                UIHelper::showWarning(this, "Ngày nghỉ lễ", QStringLiteral("Phòng khám nghỉ lễ vào ngày %1 (%2). Vui lòng chọn ngày khác.").arg(d.toString("dd/MM/yyyy"), mainWindow->holidayNote(d)));
                return;
            }
            selectedDate = d;
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

        // 2. Slots Area
        dateHeaderLabel = new QLabel("⏱ Khung giờ khả dụng");
        dateHeaderLabel->setStyleSheet("font-size: 12px; font-weight: 750; color: #0d9488;");
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

        // Morning
        auto *mornLbl = new QLabel("☀️ Buổi sáng (07:00 – 11:30)");
        mornLbl->setStyleSheet("font-size: 11px; font-weight: 750; color: #475569; padding-top: 2px;");
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

        // Afternoon
        auto *aftLbl = new QLabel("🌙 Buổi chiều (13:00 – 16:45)");
        aftLbl->setStyleSheet("font-size: 11px; font-weight: 750; color: #475569; padding-top: 6px;");
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
        if (!selectedDate.isValid()) {
            btn->setText(QStringLiteral("[ %1 ]").arg(slotStr));
            btn->setEnabled(false);
            btn->setCursor(Qt::ArrowCursor);
            btn->setStyleSheet("QPushButton { background-color: #f1f5f9; border: 1.5px solid #cbd5e1; color: #94a3b8; font-weight: 600; font-size: 11px; border-radius: 6px; }");
            btn->setToolTip("Vui lòng chọn ngày trên lịch trước");
            return;
        }

        QTime slotTime = QTime::fromString(slotStr, "HH:mm");
        QDateTime slotDT(selectedDate, slotTime);
        QDateTime nowDT = QDateTime::currentDateTime();

        bool isPast = (slotDT <= nowDT);

        Date checkDate = Date::loadFromString(UIHelper::ss(slotDT.toString("yyyy-MM-dd HH:mm")));
        bool isBooked = (!currentDoctorId.empty() && mainWindow &&
                         AppointmentController::isSlotBooked(mainWindow->getAppointments(), currentDoctorId, checkDate, currentExcludeApptId));

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
                btn->setStyleSheet("QPushButton { background-color: #0d9488; border: 1.5px solid #0f766e; color: #ffffff; font-weight: 750; font-size: 11px; border-radius: 6px; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #f0fdf4; border: 1.5px solid #86efac; color: #15803d; font-weight: 750; font-size: 11px; border-radius: 6px; } QPushButton:hover { background-color: #dcfce7; border-color: #4ade80; }");
            }
        } else if (isPast || isBooked) {
            btn->setEnabled(false);
            btn->setCursor(Qt::ArrowCursor);
            btn->setStyleSheet("QPushButton { background-color: #f1f5f9; border: 1.5px solid #cbd5e1; color: #94a3b8; font-weight: 600; font-size: 11.5px; border-radius: 6px; }");
            btn->setToolTip(isPast ? "Khung giờ này đã trôi qua" : "Bác sĩ đã có lịch hẹn khác vào khung giờ này");
        } else {
            btn->setEnabled(true);
            btn->setCursor(Qt::PointingHandCursor);
            if (isSelected) {
                btn->setStyleSheet("QPushButton { background-color: #0d9488; border: 1.5px solid #0f766e; color: #ffffff; font-weight: 750; font-size: 11.5px; border-radius: 6px; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #f0fdf4; border: 1.5px solid #86efac; color: #15803d; font-weight: 700; font-size: 11.5px; border-radius: 6px; } QPushButton:hover { background-color: #dcfce7; border-color: #4ade80; }");
            }
        }
    }
};

class BottomCalendarTimeWidget : public QWidget {
private:
    MainWindow *mainWindow = nullptr;
    QCalendarWidget *calendar = nullptr;
    QLabel *dateHeaderLabel = nullptr;
    QWidget *slotsContainer = nullptr;
    QVBoxLayout *slotsLayout = nullptr;
    QList<QPushButton*> slotButtons;

    QDate initialDate;
    QDate selectedDate;
    std::string currentDoctorId;
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

    void configure(const std::string &doctorId, const QDate &date = QDate(), const std::string &excludeApptId = "", const QString &initSlot = "", const QString &curSelect = "") {
        currentDoctorId = doctorId;
        initialDate = date;
        selectedDate = date;
        currentExcludeApptId = excludeApptId;
        initialSlot = initSlot;
        selectedSlot = curSelect.isEmpty() ? initSlot : curSelect;

        if (calendar && selectedDate.isValid()) {
            calendar->blockSignals(true);
            calendar->setSelectedDate(selectedDate);
            calendar->blockSignals(false);
        }
        refreshSlots();
    }

    QDate getSelectedDate() const { return selectedDate; }
    QString getSelectedSlot() const { return selectedSlot; }

private:
    void setupUI() {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        auto *root = new QHBoxLayout(this);
        root->setContentsMargins(12, 12, 12, 12);
        root->setSpacing(14);

        // Cột bên trái: Lịch khám
        auto *calBox = new QVBoxLayout;
        calBox->setSpacing(6);
        auto *calTitle = new QLabel("📅 Chọn ngày khám");
        calTitle->setStyleSheet("font-size: 12px; font-weight: 750; color: #0f172a;");
        calBox->addWidget(calTitle);

        calendar = new QCalendarWidget(this);
        UIHelper::applyCalendarStyle(calendar);
        calendar->setMinimumDate(QDate::currentDate());
        calendar->setFixedWidth(275);
        calendar->setFixedHeight(225);
        calBox->addWidget(calendar);
        calBox->addStretch();
        root->addLayout(calBox);

        connect(calendar, &QCalendarWidget::clicked, this, [this](const QDate &d) {
            if (mainWindow && mainWindow->isHoliday(d)) {
                UIHelper::showWarning(this, "Ngày nghỉ lễ", QStringLiteral("Phòng khám nghỉ lễ vào ngày %1 (%2). Vui lòng chọn ngày khác.").arg(d.toString("dd/MM/yyyy"), mainWindow->holidayNote(d)));
                if (calendar) {
                    calendar->blockSignals(true);
                    calendar->setSelectedDate(selectedDate.isValid() ? selectedDate : QDate::currentDate());
                    calendar->blockSignals(false);
                }
                return;
            }
            selectedDate = d;
            selectedSlot = "";
            refreshSlots();
            if (onDateTimeSelected) {
                onDateTimeSelected(selectedDate, selectedSlot);
            }
        });

        // Đường kẻ dọc phân cách
        auto *vSep = new QFrame;
        vSep->setFrameShape(QFrame::VLine);
        vSep->setFixedWidth(1);
        vSep->setStyleSheet("background-color: #e2e8f0; border: none; margin: 4px 0;");
        root->addWidget(vSep);

        // Cột bên phải: Ca khám
        auto *rightBox = new QVBoxLayout;
        rightBox->setSpacing(6);

        dateHeaderLabel = new QLabel("⏱ Khung giờ khả dụng");
        dateHeaderLabel->setStyleSheet("font-size: 12px; font-weight: 750; color: #0d9488;");
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

        // Morning
        auto *mornLbl = new QLabel("☀️ Ca sáng (07:00 – 11:30)");
        mornLbl->setStyleSheet("font-size: 11px; font-weight: 750; color: #475569; padding-top: 1px;");
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

        // Afternoon
        auto *aftLbl = new QLabel("🌙 Ca chiều (13:00 – 16:45)");
        aftLbl->setStyleSheet("font-size: 11px; font-weight: 750; color: #475569; padding-top: 4px;");
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
        if (!selectedDate.isValid()) {
            btn->setText(QStringLiteral("[ %1 ]").arg(slotStr));
            btn->setEnabled(false);
            btn->setCursor(Qt::ArrowCursor);
            btn->setStyleSheet("QPushButton { background-color: #f1f5f9; border: 1.5px solid #cbd5e1; color: #94a3b8; font-weight: 600; font-size: 11px; border-radius: 6px; }");
            btn->setToolTip("Vui lòng chọn ngày trên lịch trước");
            return;
        }

        QTime slotTime = QTime::fromString(slotStr, "HH:mm");
        QDateTime slotDT(selectedDate, slotTime);
        QDateTime nowDT = QDateTime::currentDateTime();

        bool isPast = (slotDT <= nowDT);

        Date checkDate = Date::loadFromString(UIHelper::ss(slotDT.toString("yyyy-MM-dd HH:mm")));
        bool isBooked = (!currentDoctorId.empty() && mainWindow &&
                         AppointmentController::isSlotBooked(mainWindow->getAppointments(), currentDoctorId, checkDate, currentExcludeApptId));

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
                btn->setStyleSheet("QPushButton { background-color: #0d9488; border: 1.5px solid #0f766e; color: #ffffff; font-weight: 750; font-size: 11px; border-radius: 6px; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #f0fdf4; border: 1.5px solid #86efac; color: #15803d; font-weight: 750; font-size: 11px; border-radius: 6px; } QPushButton:hover { background-color: #dcfce7; border-color: #4ade80; }");
            }
        } else if (isPast || isBooked) {
            btn->setEnabled(false);
            btn->setCursor(Qt::ArrowCursor);
            btn->setStyleSheet("QPushButton { background-color: #f1f5f9; border: 1.5px solid #cbd5e1; color: #94a3b8; font-weight: 600; font-size: 11px; border-radius: 6px; }");
            btn->setToolTip(isPast ? "Khung giờ này đã trôi qua" : "Bác sĩ đã có lịch hẹn khác vào khung giờ này");
        } else {
            btn->setEnabled(true);
            btn->setCursor(Qt::PointingHandCursor);
            if (isSelected) {
                btn->setStyleSheet("QPushButton { background-color: #0d9488; border: 1.5px solid #0f766e; color: #ffffff; font-weight: 750; font-size: 11px; border-radius: 6px; }");
            } else {
                btn->setStyleSheet("QPushButton { background-color: #f0fdf4; border: 1.5px solid #86efac; color: #15803d; font-weight: 700; font-size: 11px; border-radius: 6px; } QPushButton:hover { background-color: #dcfce7; border-color: #4ade80; }");
            }
        }
    }
};

} // namespace

bool AppointmentView::showBookingDialog(const QDate &defaultDate) {
    Q_UNUSED(defaultDate);
    if (!mainWindow) return false;

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
        QComboBox::drop-down { border: none; width: 28px; }
        QComboBox::down-arrow {
            image: url(:/assets/textures/arrow_down_dark.png);
            width: 12px; height: 12px; margin-right: 8px;
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
    sidePanel->setFixedWidth(270);
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
    dlgTitle->setStyleSheet("font-size: 17px; font-weight: 750; color: #0f172a;");
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

    // 1. Patient
    auto *patient = new SearchablePatientComboBox;
    patient->addItem("Chọn hồ sơ bệnh nhân", "");
    auto *patients = mainWindow->getPatients();
    if (patients) {
        auto *p = patients->getHeader()->next;
        while (p && p->next) {
            patient->addItem(UIHelper::qs(p->info.getfullName()), UIHelper::qs(p->info.getID()));
            p = p->next;
        }
    }
    form->addRow(makeLabel("Bệnh nhân:"), patient);

    // 2. Symptoms: 1 to 5 dynamic dropdowns
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

    // 3. Specialty
    auto *specialtyValue = new QLabel("—");
    specialtyValue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    specialtyValue->setStyleSheet("background-color: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 8px 12px; font-size: 13.5px; color: #0d9488; font-weight: 750; min-height: 22px;");
    form->addRow(makeLabel("Chuyên khoa:"), specialtyValue);

    // 4. Doctor
    auto *doctor = new NoWheelComboBox;
    doctor->addItem("Chọn bác sĩ phụ trách", "");
    form->addRow(makeLabel("Bác sĩ khám:"), doctor);

    // 5. Fee row
    auto *feeValue = new QLabel("—");
    feeValue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    feeValue->setStyleSheet("background-color: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 8px 12px; font-size: 14.5px; color: #0d9488; font-weight: 800; min-height: 22px;");
    form->addRow(makeLabel("Phí khám:"), feeValue);

    // 6. Date button: Thay mũi tên bằng icon lịch 📅, mặc định hiển thị trống
    QDate selectedDate;
    QString selectedSlot;

    auto *dateBtn = new QPushButton;
    dateBtn->setCursor(Qt::PointingHandCursor);
    dateBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    dateBtn->setEnabled(false);
    dateBtn->setText("Chọn ngày khám");
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

    // Bấm vào dateBtn hoặc icon lịch: Mở/đóng Side panel mở rộng 50%
    connect(dateBtn, &QPushButton::clicked, &dialog, [dateBtn, sidePanel, doctor, &dialog]() {
        if (doctor->currentData().toString().isEmpty()) {
            UIHelper::showWarning(&dialog, "Thiếu thông tin", "Vui lòng chọn bác sĩ phụ trách trước khi chọn lịch khám.");
            return;
        }
        if (sidePanel->isVisible()) {
            sidePanel->setVisible(false);
            dialog.setFixedWidth(540);
        } else {
            std::string dId = UIHelper::ss(doctor->currentData().toString());
            sidePanel->configure(dId, QDate::currentDate());
            sidePanel->setVisible(true);
            dialog.setFixedWidth(826);
        }
        dialog.adjustSize();
    });

    // Callback khi chọn ngày hoặc giờ trên Side panel
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

    // Dynamic symptom handling & specialty score calculation
    auto updateSpecialtyAndDoctors = [this, &dialog, symptomCombos, form, specialtyValue, doctor, feeValue, dateBtn, sidePanel, &selectedDate, &selectedSlot]() {
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
                        if (j != i && selectedIds[j] == sid) {
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

        for (int i = 0; i < 4; ++i) {
            bool currentSelected = (symptomCombos[i]->currentIndex() > 0);
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

        QString bestSpId;
        int maxScore = 0;
        for (auto it = spScores.begin(); it != spScores.end(); ++it) {
            if (it.value() > maxScore) {
                maxScore = it.value();
                bestSpId = it.key();
            }
        }

        if (!bestSpId.isEmpty()) {
            specialtyValue->setText(mainWindow->specialty(bestSpId.toStdString()));
        } else {
            specialtyValue->setText("—");
        }

        QString prevDocId = doctor->currentData().toString();
        doctor->clear();
        doctor->addItem("Chọn bác sĩ phụ trách", "");
        feeValue->setText("—");

        if (!bestSpId.isEmpty()) {
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
            if (restoredIdx >= 0) {
                doctor->setCurrentIndex(restoredIdx);
            }
        }

        if (doctor->currentIndex() <= 0) {
            dateBtn->setEnabled(false);
            dateBtn->setText("Chọn ngày khám");
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
            selectedDate = QDate();
            selectedSlot.clear();
            sidePanel->resetToDefault();
            sidePanel->setVisible(false);
            dialog.setFixedWidth(540);
        }
        dialog.adjustSize();
    };

    for (int i = 0; i < 5; ++i) {
        connect(symptomCombos[i], qOverload<int>(&QComboBox::currentIndexChanged), &dialog, updateSpecialtyAndDoctors);
    }

    // Khi chọn / đổi Bác sĩ: nếu chọn lại bác sĩ thì lịch về default là chưa có lịch, đóng side panel
    connect(doctor, qOverload<int>(&QComboBox::currentIndexChanged), &dialog, [this, doctor, feeValue, dateBtn, sidePanel, &selectedDate, &selectedSlot, &dialog]() {
        Doctor *selected = mainWindow->findDoctor(UIHelper::ss(doctor->currentData().toString()));
        if (selected) {
            feeValue->setText(QLocale(QLocale::Vietnamese).toString(selected->getFee(), 'f', 0) + " ₫");
            dateBtn->setEnabled(true);
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
        } else {
            feeValue->setText("—");
            dateBtn->setEnabled(false);
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

        // Đổi bác sĩ -> Lịch về default là chưa có lịch, đóng side panel
        selectedDate = QDate();
        selectedSlot.clear();
        dateBtn->setText("Chọn ngày khám");
        sidePanel->resetToDefault();
        sidePanel->setVisible(false);
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
        if (bookingDT <= QDateTime::currentDateTime()) {
            UIHelper::showWarning(&dialog, "Thời gian không hợp lệ", "Vui lòng chọn thời gian khám trong tương lai.");
            return;
        }

        if (mainWindow->isHoliday(selectedDate)) {
            UIHelper::showWarning(&dialog, "Ngày nghỉ lễ", QStringLiteral("Phòng khám nghỉ lễ vào ngày %1 (%2). Vui lòng chọn ngày khác.").arg(selectedDate.toString("dd/MM/yyyy"), mainWindow->holidayNote(selectedDate)));
            return;
        }

        std::string pId = UIHelper::ss(patient->currentData().toString());
        std::string dId = UIHelper::ss(doctor->currentData().toString());
        Date targetTime = Date::loadFromString(UIHelper::ss(bookingDT.toString("yyyy-MM-dd HH:mm")));
        if (AppointmentController::isSlotBooked(mainWindow->getAppointments(), dId, targetTime)) {
            UIHelper::showWarning(&dialog, "Trùng giờ khám", "Bác sĩ đã có lịch hẹn khác vào khung giờ này. Vui lòng chọn khung giờ hoặc bác sĩ khác.");
            return;
        }

        Doctor *doc = mainWindow->findDoctor(dId);
        std::string specId = doc ? doc->getSpId() : "";

        auto *appointments = mainWindow->getAppointments();
        auto *symptomPatients = mainWindow->getSymptomPatients();
        auto *symptomPatientDetails = mainWindow->getSymptomPatientDetails();
        std::string nextSpId = SymptomPatient::generateNextId(symptomPatients);

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
            appointments->insert(Appointment(AppointmentController::generateNextId(appointments), nextSpId, dId, Date::now(), targetTime, 0));
        }

        mainWindow->updatePatientSymptomStatus(pId);
        mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal);
        UIHelper::showSuccess(&dialog, "Thành công", "Lịch khám đã được ghi nhận và lưu thành công.");
        mainWindow->refreshPage(4);
        mainWindow->refreshPage(0);
        dialog.accept();
    });

    btnBox->addWidget(cancelBtn);
    btnBox->addWidget(confirmBtn);
    leftLayout->addLayout(btnBox);

    dialog.setFocusPolicy(Qt::StrongFocus);
    dialog.setFocus();
    QTimer::singleShot(0, [&dialog]() { dialog.setFocus(); });

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

    void showPopup() override {
        if (stepperBox && !comboStyle) {
            comboStyle = new StepperComboStyle(stepperBox, style());
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
            step = 25;  // Lướt siêu tốc: 25 năm / nấc
        } else if (dt < 70) {
            step = 10;  // Lướt nhanh: 10 năm / nấc
        } else if (dt < 130) {
            step = 3;   // Lướt vừa: 3 năm / nấc
        } else {
            step = 1;   // Lăn chậm: 1 năm / nấc
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
    AppointmentRowClickFilter(Appointment *a, AppointmentView *v, std::function<void()> callback, QObject *parent = nullptr)
        : QObject(parent), appointment(a), view(v), onClosed(std::move(callback)) {}
    AppointmentRowClickFilter(Appointment *a, AppointmentView *v, QObject *parent = nullptr)
        : QObject(parent), appointment(a), view(v), onClosed(nullptr) {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonDblClick) {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && view && appointment) {
                view->showAppointmentDetailDialog(appointment);
                if (onClosed) onClosed();
                return true;
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

} // namespace

QWidget *AppointmentView::createDashboardWidget(QWidget *parent) {
    if (!mainWindow) return new QWidget(parent);

    auto *w = new QWidget(parent);
    w->setCursor(Qt::ArrowCursor);
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(28, 20, 28, 20);
    layout->setSpacing(16);

    // Tiêu đề & Lời chào theo thời gian
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

    // 4 Thẻ chỉ số nổi bật
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

    // Banner đặt lịch nhanh
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

    // Bảng danh sách lịch hẹn sắp tới
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

    // Thanh phân trang
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

    // Trạng thái phân trang
    struct DashState {
        int pageNumber = 0;
        int sortCol = -1;
        bool sortAsc = true;
        enum { PAGE_SIZE = 8 };
    };
    auto *state = new DashState;

    auto renderRowsPtr = std::make_shared<std::function<void(bool)>>();
    auto renderRows = [renderRowsPtr](bool preserveScroll = true) { if (*renderRowsPtr) (*renderRowsPtr)(preserveScroll); };
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

            int st = a->getState();
            if (st == 0) {
                Date adg = a->getDateGet();
                QDateTime now = QDateTime::currentDateTime();
                QDateTime apptDt(QDate(adg.getYear(), adg.getMonth(), adg.getDay()), QTime(adg.getHour(), adg.getMinute()));
                if (apptDt < now) {
                    st = -1;
                }
            }
            QString badgeText = "● Chưa khám";
            QString badgeStyle = "QLabel { background: #fffbeb; color: #d97706; border: 1px solid #fde68a; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px; }";
            if (st == 1) {
                badgeText = "● Đã khám";
                badgeStyle = "QLabel { background: #ecfdf5; color: #059669; border: 1px solid #a7f3d0; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px; }";
            } else if (st == -1) {
                badgeText = "● Trễ hẹn";
                badgeStyle = "QLabel { background: #fff7ed; color: #ea580c; border: 1px solid #ffedd5; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px; }";
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

            auto makeRowClickable = [&](QWidget *widget, Appointment *appt) {
                widget->installEventFilter(new AppointmentRowClickFilter(appt, this, renderRows, widget));
                const auto children = widget->findChildren<QWidget*>();
                for (auto *c : children) {
                    c->installEventFilter(new AppointmentRowClickFilter(appt, this, renderRows, c));
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

    // Header layout
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

    // Bảng lịch tháng
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

    // Chọn tháng
    auto *monthGroup = makeStepperGroup();
    auto *monthGroupLayout = new QHBoxLayout(monthGroup);
    monthGroupLayout->setContentsMargins(4, 2, 4, 2);
    monthGroupLayout->setSpacing(2);
    auto *prevMonthBtn = makeStepperBtn("‹");
    auto *monthCombo = new StepperComboBox(monthGroup);
    monthCombo->stepperBox = monthGroup;
    monthCombo->comboStyle = new StepperComboStyle(monthGroup, monthCombo->style());
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

    // Chọn năm (Không hiển thị danh sách thả xuống, lăn chuột mượt mà về 0 hoặc vô cùng, đoạn lăn bên phải để trống)
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

    // Bắt sự kiện lăn chuột trên toàn bộ khung chứa năm & tháng
    yearGroup->installEventFilter(new YearWheelFilter(yearSpin, yearGroup));
    prevYearBtn->installEventFilter(new YearWheelFilter(yearSpin, prevYearBtn));
    nextYearBtn->installEventFilter(new YearWheelFilter(yearSpin, nextYearBtn));

    monthGroup->installEventFilter(new MonthWheelFilter(monthCombo, monthGroup));
    prevMonthBtn->installEventFilter(new MonthWheelFilter(monthCombo, prevMonthBtn));
    nextMonthBtn->installEventFilter(new MonthWheelFilter(monthCombo, nextMonthBtn));

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

    // Bỏ màu đỏ của Thứ 7 và Chủ Nhật trên tiêu đề
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

    // Phần dưới: Chi tiết danh sách bệnh nhân
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
    dayTitle->setStyleSheet("font-size: 17px; font-weight: 750; color: #102f43;");
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

    // Thanh phân trang
    auto *pagerRow = new QHBoxLayout;
    pagerRow->addStretch();
    auto *prevBtn = UIHelper::button("‹  Trang trước", "secondary");
    auto *nextBtn = UIHelper::button("Trang sau  ›", "secondary");
    auto *pageStatus = new QLabel("Trang 1 / 1");
    pageStatus->setStyleSheet("font-size: 13px; font-weight: 750; color: #0f172a; padding: 0 12px;");
    pagerRow->addWidget(prevBtn);
    pagerRow->addWidget(pageStatus);
    pagerRow->addWidget(nextBtn);
    dayCardLayout->addLayout(pagerRow);

    layout->addWidget(dayCard, 1);
    scroll->setWidget(w);

    // Trạng thái cục bộ
    struct BookingPageState {
        QDate selectedDate;
        QList<Appointment*> appointments;
        int pageNumber = 0;
        int sortCol = -1;
        bool sortAsc = true;
        enum { PAGE_SIZE = 8 };
    };
    auto *state = new BookingPageState;
    state->selectedDate = QDate::currentDate();

    auto reloadDayViewPtr = std::make_shared<std::function<void(bool)>>();
    auto reloadDayView = [reloadDayViewPtr](bool preserveScroll = true) { if (*reloadDayViewPtr) (*reloadDayViewPtr)(preserveScroll); };
    *reloadDayViewPtr = [=, this](bool preserveScroll) {
        highlightCalendarAppointments(cal, state->selectedDate.year(), state->selectedDate.month());
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

        bookBtn->setEnabled(!isPast && !isHol);
        if (isPast || isHol) {
            bookBtn->setCursor(Qt::ForbiddenCursor);
        } else {
            bookBtn->setCursor(Qt::PointingHandCursor);
        }

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
            pagerRow->parentWidget(); // keep
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

            int st = a->getState();
            if (st == 0) {
                Date adg = a->getDateGet();
                QDateTime now = QDateTime::currentDateTime();
                QDateTime apptDt(QDate(adg.getYear(), adg.getMonth(), adg.getDay()), QTime(adg.getHour(), adg.getMinute()));
                if (apptDt < now) {
                    st = -1;
                }
            }
            QString badgeText = "● Chưa khám";
            QString badgeStyle = "background: #fffbeb; color: #d97706; border: 1px solid #fde68a; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;";
            if (st == 1) {
                badgeText = "● Đã khám";
                badgeStyle = "background: #ecfdf5; color: #059669; border: 1px solid #a7f3d0; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;";
            } else if (st == -1) {
                badgeText = "● Trễ hẹn";
                badgeStyle = "background: #fff7ed; color: #ea580c; border: 1px solid #ffedd5; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;";
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

            auto makeRowClickable = [&](QWidget *widget, Appointment *appt) {
                widget->installEventFilter(new AppointmentRowClickFilter(appt, this, reloadDayView, widget));
                const auto children = widget->findChildren<QWidget*>();
                for (auto *c : children) {
                    c->installEventFilter(new AppointmentRowClickFilter(appt, this, reloadDayView, c));
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

    connect(t, &QTableWidget::cellDoubleClicked, w, [=, this](int r, int) {
        int idx = state->pageNumber * BookingPageState::PAGE_SIZE + r;
        if (idx >= 0 && idx < static_cast<int>(state->appointments.size())) {
            showAppointmentDetailDialog(state->appointments[idx]);
            reloadDayView(true);
        }
    });

    connect(holidayBtn, &QPushButton::clicked, w, [=, this]() {
        showHolidayManagementDialog(w);
        highlightCalendarAppointments(cal, yearSpin->value(), monthCombo->currentData().toInt());
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
    highlightCalendarAppointments(cal, cur.year(), cur.month());
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
        setComboYear(y);
        monthCombo->setCurrentIndex(m - 1);
    });
    connect(nextMonthBtn, &QPushButton::clicked, w, [=]() {
        int m = monthCombo->currentData().toInt();
        int y = yearSpin->value();
        if (m == 12) { m = 1; y++; } else { m++; }
        setComboYear(y);
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
        yearSpin->blockSignals(true);
        setComboYear(today.year());
        yearSpin->blockSignals(false);
        monthCombo->blockSignals(true);
        monthCombo->setCurrentIndex(today.month() - 1);
        monthCombo->blockSignals(false);
        cal->setCurrentPage(today.year(), today.month());
        cal->setSelectedDate(today);
        state->selectedDate = today;
        state->pageNumber = 0;
        highlightCalendarAppointments(cal, today.year(), today.month());
        reloadDayView(false);
    });

    connect(cal, &QCalendarWidget::currentPageChanged, w, [=, this](int year, int month) {
        yearSpin->blockSignals(true);
        setComboYear(year);
        yearSpin->blockSignals(false);
        monthCombo->blockSignals(true);
        monthCombo->setCurrentIndex(month - 1);
        monthCombo->blockSignals(false);
        highlightCalendarAppointments(cal, year, month);
    });

    auto openBooking = [=, this]() {
        if (state->selectedDate < QDate::currentDate()) {
            UIHelper::showWarning(w, "Không thể đặt lịch", "Không thể đặt lịch khám cho ngày trước thời gian hiện tại.");
            return;
        }
        if (mainWindow->isHoliday(state->selectedDate)) {
            UIHelper::showWarning(w, "Ngày nghỉ lễ", "Không thể đặt lịch khám vào ngày nghỉ lễ.");
            return;
        }
        if (showBookingDialog(state->selectedDate)) {
            highlightCalendarAppointments(cal, yearSpin->value(), monthCombo->currentData().toInt());
            reloadDayView(false);
        }
    };
    connect(bookBtn, &QPushButton::clicked, w, openBooking);
    connect(quickBookBtn, &QPushButton::clicked, w, openBooking);

    return scroll;
}

QWidget *AppointmentView::createAppointmentListWidget(QWidget *parent) {
    listSortColumn = -1;
    listSortAscending = false;
    listCurrentPage = 0;
    if (mainWindow && mainWindow->getAppointments()) {
        libPBL2::sortAppointments(mainWindow->getAppointments(), libPBL2::AppointmentSortBy::ID, false);
    }

    QWidget *w = UIHelper::page("Quản lý lịch khám", "Theo dõi lịch theo bác sĩ, ngày hẹn và trạng thái khám.", parent);
    auto *layout = qobject_cast<QVBoxLayout*>(w->layout());

    // Filter controls inside a neat card
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
        QComboBox, QDateEdit {
            background: #f8fafc;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 5px 20px 5px 8px;
            font-size: 13px;
            color: #0f172a;
            min-height: 22px;
        }
        QComboBox:disabled, QDateEdit:disabled {
            background: #f1f5f9;
            color: #94a3b8;
            border-color: #e2e8f0;
        }
        QComboBox:focus, QDateEdit:focus {
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

    // 1. Cột Bác sĩ
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
            doctor->addItem(QStringLiteral("BS. %1").arg(UIHelper::qs(d->info.getfullName())), UIHelper::qs(d->info.getID()));
            d = d->next;
        }
    }
    doctor->setMinimumWidth(160);
    doctor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    docCol->addWidget(lblDoctor);
    docCol->addWidget(doctor);

    // 2. Cột Ngày khám
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
    date->setFixedWidth(125);
    UIHelper::setupCalendarPopup(date);

    auto *allDates = new ToggleSwitch;
    allDates->setChecked(true);
    allDates->setToolTip(QString::fromUtf8("Tất cả các ngày (Bật để xem mọi ngày, Tắt để chọn ngày khám)"));
    date->setEnabled(false);

    dateRow->addWidget(date);
    dateRow->addWidget(allDates);
    dateCol->addLayout(dateRow);

    // 3. Cột Trạng thái
    auto *statusCol = new QVBoxLayout;
    statusCol->setSpacing(3);
    auto *lblStatus = new QLabel("Trạng thái:");
    lblStatus->setStyleSheet("font-size: 11.5px; font-weight: 700; color: #475569;");
    auto *status = new QComboBox;
    status->view()->setTextElideMode(Qt::ElideRight);
    status->addItems({"Tất cả trạng thái", "Chưa khám", "Đã khám", "Trễ hẹn"});
    status->setMinimumWidth(150);
    status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    statusCol->addWidget(lblStatus);
    statusCol->addWidget(status);

    filters->addLayout(docCol, 4);
    filters->addSpacing(16);
    filters->addLayout(dateCol, 0);
    filters->addSpacing(16);
    filters->addLayout(statusCol, 2);
    layout->addWidget(filterCard);

    // Table: Bỏ mã lịch, Bỏ triệu chứng, Tách Bác sĩ & Chuyên khoa, Đổi Ngày giờ hẹn thành Ngày khám, có cột hủy lịch
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

    // Tự động scale các cột sao cho tổng chiều rộng các cột luôn bằng chính xác chiều rộng bảng
    UIHelper::setupTableScaling(t, {0.23, 0.22, 0.20, 0.20, 0.15}, 56);

    // Khung thẻ bo tròn hoàn thiện 4 góc, chống tràn viền
    auto *tableCard = UIHelper::createTableCard(t);
    layout->addWidget(tableCard, 1);

    // Pagination: 50 stack 1 trang
    const int PAGE_SIZE = 50;
    auto currentPage = std::make_shared<int>(listCurrentPage);
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

    auto computeApptStatus = [](const Appointment &a, Doctor *d = nullptr) -> QString {
        (void)d;
        return getAppointmentStatusText(a);
    };

    auto fillPtr = std::make_shared<std::function<void(bool)>>();
    auto fill = [fillPtr](bool preserveScroll = true) { if (*fillPtr) (*fillPtr)(preserveScroll); };
    *fillPtr = [this, w, t, doctor, date, allDates, status, totalLabel, prevBtn, nextBtn, pageStatus, currentPage, PAGE_SIZE, computeApptStatus, sortColumn, sortAscending, fill](bool preserveScroll) {
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
            return;
        }

        QList<Appointment*> ordered;
        auto *n = appointments->getHeader()->next;
        while (n && n->next) {
            if (!n->info.isDefault() && n->info.getState() != -2) ordered.append(&n->info);
            n = n->next;
        }

        int col = *sortColumn;
        bool asc = *sortAscending;

        if (col >= 0) {
            libPBL2::qsort(ordered, [this, col, asc, &computeApptStatus](const Appointment *a, const Appointment *b) -> bool {
            if (!a || !b) return false;
            int cmp = 0;
            switch (col) {
                case 0: { // BỆNH NHÂN
                    Patient *pa = mainWindow ? mainWindow->findPatientForAppointment(*a) : nullptr;
                    Patient *pb = mainWindow ? mainWindow->findPatientForAppointment(*b) : nullptr;
                    std::string na = pa ? pa->getfullName() : "";
                    std::string nb = pb ? pb->getfullName() : "";
                    cmp = libPBL2::compareVietnameseNames(na, nb);
                    break;
                }
                case 1: { // BÁC SĨ
                    Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                    Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                    std::string na = da ? da->getfullName() : "";
                    std::string nb = db ? db->getfullName() : "";
                    cmp = libPBL2::compareVietnameseNames(na, nb);
                    break;
                }
                case 2: { // CHUYÊN KHOA
                    Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                    Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                    QString sa = (da && mainWindow) ? mainWindow->specialty(da->getSpId()) : "";
                    QString sb = (db && mainWindow) ? mainWindow->specialty(db->getSpId()) : "";
                    cmp = QString::compare(sa, sb, Qt::CaseInsensitive);
                    break;
                }
                case 3: { // NGÀY KHÁM
                    cmp = libPBL2::compareDates(a->getDateGet(), b->getDateGet());
                    break;
                }
                case 4: { // TRẠNG THÁI
                    Doctor *da = mainWindow ? mainWindow->findDoctor(a->getDoctorId()) : nullptr;
                    Doctor *db = mainWindow ? mainWindow->findDoctor(b->getDoctorId()) : nullptr;
                    QString sa = computeApptStatus(*a, da);
                    QString sb = computeApptStatus(*b, db);
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
            libPBL2::qsort(ordered, [](const Appointment *a, const Appointment *b) -> bool {
                if (!a || !b) return false;
                return libPBL2::compareNatural(a->getId(), b->getId()) > 0;
            });
        }

        QList<Appointment*> filtered;
        for (auto *item : ordered) {
            auto &a = *item;
            Doctor *d = mainWindow->findDoctor(a.getDoctorId());
            Date ad = a.getDateGet();

            bool matchesDoctor = doctor->currentData().toString().isEmpty() || UIHelper::qs(a.getDoctorId()) == doctor->currentData().toString();
            bool matchesDate = allDates->isChecked() || (ad.getYear() == date->date().year() && ad.getMonth() == date->date().month() && ad.getDay() == date->date().day());
            QString sText = computeApptStatus(a, d);
            bool matchesStatus = (status->currentIndex() == 0) || (status->currentText() == sText);

            if (matchesDoctor && matchesDate && matchesStatus) {
                filtered.append(item);
            }
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
            QString sText = computeApptStatus(a, d);

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
            }
            QFont f = statusItem->font();
            f.setBold(true);
            statusItem->setFont(f);
            t->setItem(r, 4, statusItem);

            // Cột 5: Nút hủy lịch khám (icon thùng rác đỏ)
            auto action = UIHelper::createActionCell(UIHelper::trashIcon(), "Hủy lịch khám");
            if (mainWindow && !mainWindow->hasPermission("Per_4")) {
                action.second->setToolTip("Bạn không có quyền hủy lịch khám (Yêu cầu quyền Per_4)");
            }
            std::string aid = a.getId();
            connect(action.second, &QPushButton::clicked, w, [this, aid, &fill] {
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

                QString confirmMsg = QStringLiteral("Bạn có chắc chắn muốn hủy lịch khám lúc %1 của bệnh nhân \"%2\"?\nLịch hẹn sau khi hủy sẽ không còn hiển thị trong danh sách chờ.")
                    .arg(dateStr, patientName);
                if (!UIHelper::confirmCancel(mainWindow, "Hủy lịch khám", confirmMsg, "Hủy lịch", "Không hủy")) {
                    return;
                }

                auto *cur = appointments->getHeader()->next;
                while (cur && cur->next) {
                    if (cur->info.getId() == aid) {
                        appointments->remove(cur);
                        break;
                    }
                    cur = cur->next;
                }

                mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal);
                fill(true);
                mainWindow->refreshPage(0);
                mainWindow->refreshPage(2);
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
        listCurrentPage = 0;
        fill(false);
    });

    connect(doctor, qOverload<int>(&QComboBox::currentIndexChanged), w, [this, fill, currentPage] { *currentPage = 0; listCurrentPage = 0; fill(false); });
    connect(date, &QDateEdit::dateChanged, w, [this, fill, currentPage] { *currentPage = 0; listCurrentPage = 0; fill(false); });
    connect(allDates, &QAbstractButton::toggled, w, [this, fill, currentPage, date](bool c) {
        *currentPage = 0;
        listCurrentPage = 0;
        date->setEnabled(!c);
        fill(false);
    });
    connect(status, qOverload<int>(&QComboBox::currentIndexChanged), w, [this, fill, currentPage] { *currentPage = 0; listCurrentPage = 0; fill(false); });

    connect(prevBtn, &QPushButton::clicked, w, [this, fill, currentPage] {
        if (*currentPage > 0) {
            (*currentPage)--;
            listCurrentPage = *currentPage;
            fill(false);
        }
    });

    connect(nextBtn, &QPushButton::clicked, w, [this, fill, currentPage] {
        (*currentPage)++;
        listCurrentPage = *currentPage;
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
        listCurrentPage = 0;
        if (mainWindow && mainWindow->getAppointments()) {
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
    Date sd = a->getDateSet();

    QDialog dialog(mainWindow);
    dialog.setWindowTitle("Chi tiết lịch khám bệnh");
    dialog.setFixedWidth(610);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #f8fafc;
            font-family: 'Segoe UI', sans-serif;
        }
        QLineEdit, QComboBox, QDateTimeEdit {
            background: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
            color: #0f172a;
            min-height: 20px;
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

    // 1. Header Banner
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
        ? "background: #eff6ff; color: #2563eb; border: 1px solid #bfdbfe; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;"
        : "background: #f1f5f9; color: #475569; border: 1px solid #cbd5e1; border-radius: 12px; font-size: 11.5px; font-weight: 700; padding: 4px 10px;");

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
        h->setStyleSheet("font-size: 12.5px; font-weight: 750; color: #0f766e; letter-spacing: 0.5px; text-transform: uppercase; font-family: 'Segoe UI', sans-serif;");
        fl->addWidget(h);
        return qMakePair(f, fl);
    };

    auto makeField = [](const QString &label, const QString &val, bool isBold = false) {
        auto *col = new QVBoxLayout;
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(3);
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748b; font-family: 'Segoe UI', sans-serif;");
        auto *v = new QLabel(val);
        v->setStyleSheet(isBold
            ? "font-size: 13.5px; font-weight: 700; color: #0f172a; font-family: 'Segoe UI', sans-serif;"
            : "font-size: 13.5px; font-weight: 600; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
        col->addWidget(lbl);
        col->addWidget(v);
        return col;
    };

    auto wrapField = [](const QString &label, QLabel *valWidget) {
        auto *col = new QVBoxLayout;
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(3);
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748b; font-family: 'Segoe UI', sans-serif;");
        col->addWidget(lbl);
        col->addWidget(valWidget);
        return col;
    };

    // 2. Card Thông tin bệnh nhân
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

    pGrid->addLayout(makeField("Họ và tên", pName, true), 0, 0);
    pGrid->addLayout(makeField("Tuổi & Giới tính", pGenderAge), 0, 1);
    pGrid->addLayout(makeField("Số điện thoại", pPhone), 1, 0);
    pGrid->addLayout(makeField("Số CCCD", pCccd), 1, 1);
    pCardPair.second->addLayout(pGrid);
    root->addWidget(pCardPair.first);

    // 3. Card Triệu chứng lâm sàng
    auto sCardPair = makeSectionCard("🩺  Triệu chứng lâm sàng");
    QString apptSymptomName = mainWindow->getSymptomNamesForAppointment(*a);
    if (apptSymptomName.isEmpty()) {
        apptSymptomName = "Chưa ghi nhận";
    }
    auto *allSymRow = new QHBoxLayout;
    auto *allSymLbl = new QLabel("Các triệu chứng đã ghi nhận:");
    allSymLbl->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748b; font-family: 'Segoe UI', sans-serif;");
    auto *allSymVal = new QLabel(apptSymptomName);
    allSymVal->setStyleSheet("font-size: 13.5px; font-weight: 600; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    allSymVal->setWordWrap(true);
    allSymRow->addWidget(allSymLbl);
    allSymRow->addWidget(allSymVal, 1);
    sCardPair.second->addLayout(allSymRow);
    root->addWidget(sCardPair.first);

    // 4. Card Bác sĩ phụ trách (HOÀN TOÀN DẠNG TEXT TĨNH READ-ONLY, KHÔNG DÙNG BOX INPUT)
    auto dCardPair = makeSectionCard("👨‍⚕️  Bác sĩ phụ trách");
    auto *dGrid = new QGridLayout;
    dGrid->setHorizontalSpacing(20);
    dGrid->setVerticalSpacing(8);
    dGrid->setColumnStretch(0, 1);
    dGrid->setColumnStretch(1, 1);

    QString dName = d ? QStringLiteral("BS. %1").arg(UIHelper::qs(d->getfullName())) : UIHelper::qs(a->getDoctorId());
    dGrid->addLayout(makeField("Bác sĩ nhận ca", dName, true), 0, 0);

    QLabel *dSpecVal = new QLabel(d ? mainWindow->specialty(d->getSpId()) : "—");
    dSpecVal->setStyleSheet("font-size: 13.5px; font-weight: 600; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    QLabel *dRoomVal = new QLabel(d ? UIHelper::formatRoom(UIHelper::qs(d->getRoomId())) : "Chưa xác định");
    dRoomVal->setStyleSheet("font-size: 13.5px; font-weight: 600; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    QLabel *dPhoneVal = new QLabel((d && !d->getPhone().empty()) ? UIHelper::qs(d->getPhone()) : "Chưa cập nhật");
    dPhoneVal->setStyleSheet("font-size: 13.5px; font-weight: 600; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    QLabel *dRatingVal = new QLabel(d ? QStringLiteral("⭐ %1 / 5.0").arg(QString::number(d->getRating(), 'f', 1)) : "—");
    dRatingVal->setStyleSheet("font-size: 13.5px; font-weight: 600; color: #1e293b; font-family: 'Segoe UI', sans-serif;");
    QLabel *dFeeVal = new QLabel(d ? (QLocale(QLocale::Vietnamese).toString(d->getFee(), 'f', 0) + " đ") : "—");
    dFeeVal->setStyleSheet("font-size: 13.5px; font-weight: 600; color: #1e293b; font-family: 'Segoe UI', sans-serif;");

    dGrid->addLayout(wrapField("Chuyên khoa", dSpecVal), 0, 1);
    dGrid->addLayout(wrapField("Phòng khám chỉ định", dRoomVal), 1, 0);
    dGrid->addLayout(wrapField("Số điện thoại bác sĩ", dPhoneVal), 1, 1);
    dGrid->addLayout(wrapField("Đánh giá chuyên môn", dRatingVal), 2, 0);
    dGrid->addLayout(wrapField("Phí khám tham chiếu", dFeeVal), 2, 1);
    dCardPair.second->addLayout(dGrid);
    root->addWidget(dCardPair.first);

    // 5. Card Thời gian & Trạng thái (HOÀN TOÀN DẠNG TEXT TĨNH READ-ONLY, KHÔNG DÙNG BOX INPUT)
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

    // Xử lý huy hiệu Trạng thái
    QDateTime now = QDateTime::currentDateTime();
    QDateTime apptDt(QDate(ad.getYear(), ad.getMonth(), ad.getDay()), QTime(ad.getHour(), ad.getMinute()));

    auto *statCol = new QVBoxLayout;
    statCol->setContentsMargins(0, 0, 0, 0);
    statCol->setSpacing(3);
    auto *statLbl = new QLabel("Trạng thái ca khám");
    statLbl->setStyleSheet("font-size: 12px; font-weight: 600; color: #64748b; font-family: 'Segoe UI', sans-serif;");

    auto *statusBadge = new QLabel;
    auto updateStatusBadge = [statusBadge, a, now, apptDt](bool isWaiting) {
        if (a->getState() == 1) {
            statusBadge->setText("● Đã khám");
            statusBadge->setStyleSheet("background: #ecfdf5; color: #059669; border: 1px solid #a7f3d0; border-radius: 10px; font-size: 12px; font-weight: 750; padding: 4px 12px; font-family: 'Segoe UI', sans-serif;");
        } else if (a->getState() == -2) {
            statusBadge->setText("● Dời lịch");
            statusBadge->setStyleSheet("background: #f5f3ff; color: #7c3aed; border: 1px solid #ddd6fe; border-radius: 10px; font-size: 12px; font-weight: 750; padding: 4px 12px; font-family: 'Segoe UI', sans-serif;");
        } else if (isWaiting) {
            statusBadge->setText("● Đang chờ khám");
            statusBadge->setStyleSheet("background: #eff6ff; color: #2563eb; border: 1px solid #bfdbfe; border-radius: 10px; font-size: 12px; font-weight: 750; padding: 4px 12px; font-family: 'Segoe UI', sans-serif;");
        } else if (a->getState() == -1 || now > apptDt.addSecs(20 * 60)) {
            statusBadge->setText("● Trễ hẹn");
            statusBadge->setStyleSheet("background: #fff7ed; color: #ea580c; border: 1px solid #ffedd5; border-radius: 10px; font-size: 12px; font-weight: 750; padding: 4px 12px; font-family: 'Segoe UI', sans-serif;");
        } else {
            statusBadge->setText("● Chưa khám");
            statusBadge->setStyleSheet("background: #fffbeb; color: #d97706; border: 1px solid #fde68a; border-radius: 10px; font-size: 12px; font-weight: 750; padding: 4px 12px; font-family: 'Segoe UI', sans-serif;");
        }
    };
    bool isWaitingState = false;
    updateStatusBadge(isWaitingState);

    auto *badgeWrap = new QWidget;
    auto *bwLay = new QHBoxLayout(badgeWrap);
    bwLay->setContentsMargins(0, 0, 0, 0);
    bwLay->addWidget(statusBadge);
    bwLay->addStretch();

    statCol->addWidget(statLbl);
    statCol->addWidget(badgeWrap);
    aGrid->addLayout(statCol, 1, 0);

    aCardPair.second->addLayout(aGrid);
    root->addWidget(aCardPair.first);

    // 6. CỤM 3 NÚT HÀNH ĐỘNG NGHIỆP VỤ Ở ĐÁY HỘP THOẠI
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
        // --- NÚT 1: MỜI VÀO PHÒNG (Màu xanh lá, hiệu lực trong 20s đầu của ca khám hoặc khi đã chờ khám) ---
        auto *inviteBtn = new QPushButton("Mời vào phòng");
        inviteBtn->setAutoDefault(false);
        inviteBtn->setDefault(false);
        inviteBtn->setFixedHeight(38);

        auto updateInviteBtn = [inviteBtn, a, apptDt](bool isWaiting) {
            QDateTime current = QDateTime::currentDateTime();
            bool canInvite = (a->getState() == 0) && (isWaiting || (current >= apptDt && current <= apptDt.addSecs(20)));
            if (canInvite) {
                inviteBtn->setEnabled(true);
                inviteBtn->setCursor(Qt::PointingHandCursor);
                inviteBtn->setStyleSheet("QPushButton { background: #10b981; color: #ffffff; border: none; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #059669; }");
            } else {
                inviteBtn->setEnabled(false);
                inviteBtn->setCursor(Qt::ArrowCursor);
                inviteBtn->setStyleSheet("QPushButton { background: #e2e8f0; color: #94a3b8; border: none; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 13px; }");
            }
        };
        updateInviteBtn(isWaitingState);

        QObject::connect(inviteBtn, &QPushButton::clicked, &dialog, [this, &dialog, a] {
            a->setState(1);
            std::string patientId = mainWindow->getPatientIdForAppointment(*a);
            if (!patientId.empty()) {
                mainWindow->updatePatientSymptomStatus(patientId);
            }
            mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal);
            UIHelper::showSuccess(&dialog, "Thành công", "Đã xác nhận bệnh nhân vào phòng và hoàn tất ca khám.");
            dialog.accept();
        });

        // --- NÚT 2: CHỜ KHÁM (Màu xanh dương, hiệu lực: 0 <= thời gian hiện tại - thời gian khám <= 20s) ---
        auto *waitBtn = new QPushButton("Chờ khám");
        waitBtn->setAutoDefault(false);
        waitBtn->setDefault(false);
        waitBtn->setFixedHeight(38);

        auto updateWaitBtn = [waitBtn, a, apptDt](bool isWaiting) {
            QDateTime current = QDateTime::currentDateTime();
            bool canWait = !isWaiting && (a->getState() == 0) && (current >= apptDt && current <= apptDt.addSecs(20));
            if (canWait) {
                waitBtn->setEnabled(true);
                waitBtn->setCursor(Qt::PointingHandCursor);
                waitBtn->setStyleSheet("QPushButton { background: #2563eb; color: #ffffff; border: none; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 13px; } QPushButton:hover { background: #1d4ed8; }");
            } else {
                waitBtn->setEnabled(false);
                waitBtn->setCursor(Qt::ArrowCursor);
                waitBtn->setStyleSheet("QPushButton { background: #e2e8f0; color: #94a3b8; border: none; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 13px; }");
            }
        };
        updateWaitBtn(isWaitingState);

        // --- NÚT 3: DỜI LỊCH (Màu vàng, hiệu lực < giờ hẹn + 20 phút và chưa khám/dời) ---
        auto *reschedBtn = new QPushButton("Dời lịch");
        reschedBtn->setAutoDefault(false);
        reschedBtn->setDefault(false);
        reschedBtn->setFixedHeight(38);

        auto updateReschedBtn = [reschedBtn, a, apptDt](bool isWaiting) {
            QDateTime current = QDateTime::currentDateTime();
            bool canResched = (a->getState() == 0 || a->getState() == -1) && (isWaiting || current < apptDt.addSecs(20 * 60));
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
        updateReschedBtn(isWaitingState);

        auto *btnTimer = new QTimer(&dialog);
        QObject::connect(btnTimer, &QTimer::timeout, &dialog, [&isWaitingState, updateInviteBtn, updateWaitBtn, updateReschedBtn]() {
            updateInviteBtn(isWaitingState);
            updateWaitBtn(isWaitingState);
            updateReschedBtn(isWaitingState);
        });
        btnTimer->start(1000);

        // Sự kiện khi bấm "Chờ khám"
        QObject::connect(waitBtn, &QPushButton::clicked, &dialog, [this, &dialog, a, d, apptDt, patientId, &isWaitingState, updateStatusBadge, updateInviteBtn, updateWaitBtn, updateReschedBtn] {
            QDateTime curNow = QDateTime::currentDateTime();
            bool wantSwitch = false;
            {
                QDialog waitDlg(&dialog);
                waitDlg.setWindowTitle("Xác nhận trạng thái chờ khám");
                waitDlg.setFixedSize(460, 210);
                waitDlg.setStyleSheet("QDialog { background-color: #f8fafc; font-family: 'Segoe UI', sans-serif; }");

                auto *wRoot = new QVBoxLayout(&waitDlg);
                wRoot->setContentsMargins(24, 20, 24, 20);
                wRoot->setSpacing(12);

                auto *icoLbl = new QLabel("⏱️");
                icoLbl->setStyleSheet("font-size: 32px; background: transparent;");
                icoLbl->setAlignment(Qt::AlignCenter);
                wRoot->addWidget(icoLbl);

                auto *msgLbl = new QLabel("Bác sĩ phụ trách hiện đang bận / có ca khám chưa hoàn tất.\nBệnh nhân có muốn đổi sang Bác sĩ khác đang rảnh lịch gần nhất để được khám ngay không?");
                msgLbl->setStyleSheet("font-size: 13px; font-weight: 600; color: #334155; line-height: 1.4;");
                msgLbl->setAlignment(Qt::AlignCenter);
                msgLbl->setWordWrap(true);
                wRoot->addWidget(msgLbl);

                auto *s1Btns = new QHBoxLayout;
                s1Btns->setSpacing(10);
                auto *keepWaitBtn = new QPushButton("Không, tiếp tục chờ");
                keepWaitBtn->setCursor(Qt::PointingHandCursor);
                keepWaitBtn->setFixedHeight(36);
                keepWaitBtn->setStyleSheet("QPushButton { background: #ffffff; color: #475569; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 0 16px; font-weight: 700; font-size: 12.5px; } QPushButton:hover { background: #f1f5f9; color: #0f172a; }");

                auto *switchDocBtn = new QPushButton("Có, đổi bác sĩ  ›");
                switchDocBtn->setCursor(Qt::PointingHandCursor);
                switchDocBtn->setFixedHeight(36);
                switchDocBtn->setStyleSheet("QPushButton { background: #2563eb; color: #ffffff; border: none; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 12.5px; } QPushButton:hover { background: #1d4ed8; }");

                s1Btns->addWidget(keepWaitBtn);
                s1Btns->addWidget(switchDocBtn);
                wRoot->addLayout(s1Btns);

                QObject::connect(keepWaitBtn, &QPushButton::clicked, &waitDlg, [&waitDlg, &dialog, &isWaitingState, updateStatusBadge, updateInviteBtn, updateWaitBtn, updateReschedBtn] {
                    isWaitingState = true;
                    updateStatusBadge(true);
                    updateInviteBtn(true);
                    updateWaitBtn(true);
                    updateReschedBtn(true);
                    waitDlg.accept();
                    UIHelper::showSuccess(&dialog, "Đã ghi nhận", "Bệnh nhân tiếp tục chờ. Nút 'Mời vào phòng' đã được kích hoạt sẵn sàng.");
                });

                QObject::connect(switchDocBtn, &QPushButton::clicked, &waitDlg, [&waitDlg, &wantSwitch] {
                    wantSwitch = true;
                    waitDlg.accept();
                });

                waitDlg.exec();
            }

            if (!wantSwitch) return;

            // Mở dialog riêng biệt hiển thị danh sách bác sĩ gợi ý
            QDialog listDlg(&dialog);
            listDlg.setWindowTitle("Khung giờ");
            listDlg.setFixedSize(720, 500);
            listDlg.setStyleSheet(R"(
                QDialog { background-color: #f8fafc; font-family: 'Segoe UI', sans-serif; }
                QTableWidget { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 8px; outline: none; }
                QTableWidget::item { padding: 4px 8px; color: #0f172a; outline: none; border: none; }
                QHeaderView::section { background: #f8fafc; color: #475569; font-size: 11.5px; font-weight: 750; border: none; border-bottom: 1.5px solid #cbd5e1; padding: 8px 6px; font-family: 'Segoe UI', sans-serif; }
                QScrollBar:vertical { width: 6px; background: transparent; border: none; margin: 0px; }
                QScrollBar::handle:vertical { background: #cbd5e1; border-radius: 3px; min-height: 24px; }
                QScrollBar::handle:vertical:hover { background: #94a3b8; }
                QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
            )");

            auto *lRoot = new QVBoxLayout(&listDlg);
            lRoot->setContentsMargins(16, 16, 16, 14);
            lRoot->setSpacing(10);

            auto *docTable = new QTableWidget;
            docTable->setColumnCount(4);
            docTable->setHorizontalHeaderLabels({"BÁC SĨ", "CHUYÊN KHOA & PHÒNG", "KHUNG GIỜ KHẢ DỤNG", ""});
            docTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
            docTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            docTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
            docTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
            docTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
            docTable->setColumnWidth(3, 58);
            docTable->verticalHeader()->setVisible(false);
            docTable->setFocusPolicy(Qt::NoFocus);
            docTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
            docTable->setSelectionMode(QAbstractItemView::NoSelection);
            docTable->setShowGrid(false);
            docTable->setAlternatingRowColors(true);
            lRoot->addWidget(docTable, 1);

            // Quét danh sách Bác sĩ
            std::string mainSpId = d ? d->getSpId() : "";
            QSet<std::string> symSpIds;
            std::string curSpId = a->getSymptomPatientId();
            if (!curSpId.empty() && mainWindow->getSymptomPatientDetails()) {
                auto *dtl = mainWindow->getSymptomPatientDetails()->getHeader()->next;
                while (dtl && dtl->next) {
                    if (dtl->info.getSP_id() == curSpId) {
                        std::string symId = dtl->info.getS_id();
                        if (mainWindow->getSymptoms()) {
                            auto *sn = mainWindow->getSymptoms()->getHeader()->next;
                            while (sn && sn->next) {
                                if (sn->info.getS_id() == symId) {
                                    symSpIds.insert(sn->info.getSp_id());
                                    break;
                                }
                                sn = sn->next;
                            }
                        }
                    }
                    dtl = dtl->next;
                }
            }

            struct DocCandidate {
                Doctor *doc = nullptr;
                int priority = 2; // 0: khoa chính, 1: khoa liên quan, 2: khác
                bool isFreeNow = false;
                Date freeSlot;
                QDateTime freeSlotDT;
                QString freeSlotText;
            };
            QList<DocCandidate> candidates;

            const QStringList stdSlots = {
                "07:00", "07:45", "08:30", "09:15", "10:00", "10:45",
                "13:00", "13:45", "14:30", "15:15", "16:00"
            };

            QDateTime startDT = (apptDt.isValid() && apptDt.addSecs(20) >= curNow) ? apptDt : curNow;
            QDate startDate = startDT.date();

            if (mainWindow->getDoctors()) {
                auto *dn = mainWindow->getDoctors()->getHeader()->next;
                while (dn && dn->next) {
                    if (dn->info.getID() != a->getDoctorId() && dn->info.getState() == 1) {
                        DocCandidate cand;
                        cand.doc = &dn->info;
                        if (cand.doc->getSpId() == mainSpId) cand.priority = 0;
                        else if (symSpIds.contains(cand.doc->getSpId())) cand.priority = 1;
                        else cand.priority = 2;

                        bool foundSlot = false;
                        for (int dayOffset = 0; dayOffset <= 14 && !foundSlot; ++dayOffset) {
                            QDate curDate = startDate.addDays(dayOffset);
                            if (mainWindow && mainWindow->isHoliday(curDate)) {
                                continue;
                            }

                            for (const QString &slotStr : stdSlots) {
                                QTime slotTime = QTime::fromString(slotStr, "HH:mm");
                                QDateTime slotDT(curDate, slotTime);

                                if (slotDT.addSecs(20) < curNow) {
                                    continue;
                                }

                                Date checkDate(slotDT.date().day(), slotDT.date().month(), slotDT.date().year(), slotDT.time().hour(), slotDT.time().minute());
                                bool booked = AppointmentController::isSlotBooked(mainWindow->getAppointments(), cand.doc->getID(), checkDate, "");
                                if (!booked) {
                                    cand.freeSlot = checkDate;
                                    cand.freeSlotDT = slotDT;
                                    cand.freeSlotText = slotDT.toString("dd/MM/yyyy HH:mm");
                                    cand.isFreeNow = (dayOffset == 0 && slotDT.addSecs(20) >= curNow && slotDT <= curNow.addSecs(45 * 60));
                                    foundSlot = true;
                                    break;
                                }
                            }
                        }

                        if (!foundSlot) {
                            cand.freeSlotText = "—";
                            cand.isFreeNow = false;
                        }

                        candidates.append(cand);
                    }
                    dn = dn->next;
                }
            }

            libPBL2::qsort(candidates, [](const DocCandidate &c1, const DocCandidate &c2) {
                if (c1.priority != c2.priority) return c1.priority < c2.priority;
                if (c1.isFreeNow != c2.isFreeNow) return c1.isFreeNow;
                if (c1.freeSlotDT.isValid() && c2.freeSlotDT.isValid() && c1.freeSlotDT != c2.freeSlotDT) {
                    return c1.freeSlotDT < c2.freeSlotDT;
                }
                return false;
            });

            docTable->setRowCount(0);
            for (int i = 0; i < candidates.size(); ++i) {
                const auto &cand = candidates[i];
                int r = docTable->rowCount();
                docTable->insertRow(r);
                docTable->setRowHeight(r, 50);

                double curFee = d ? d->getFee() : 0.0;
                double candFee = cand.doc ? cand.doc->getFee() : 0.0;
                double feeDiff = candFee - curFee;

                QString diffText;
                QString diffColor;
                if (feeDiff > 0) {
                    diffText = QStringLiteral("Chênh lệch phí: +%1 đ").arg(QLocale(QLocale::Vietnamese).toString(feeDiff, 'f', 0));
                    diffColor = "#ea580c"; // đắt hơn (dương)
                } else if (feeDiff < 0) {
                    diffText = QStringLiteral("Chênh lệch phí: -%1 đ").arg(QLocale(QLocale::Vietnamese).toString(-feeDiff, 'f', 0));
                    diffColor = "#16a34a"; // rẻ hơn (âm)
                } else {
                    diffText = "Chênh lệch phí: 0 đ (bằng phí)";
                    diffColor = "#64748b";
                }

                auto *docWrap = new QWidget;
                auto *dwLay = new QVBoxLayout(docWrap);
                dwLay->setContentsMargins(6, 4, 6, 4);
                dwLay->setSpacing(2);

                auto *nameLbl = new QLabel(QStringLiteral("BS. %1").arg(UIHelper::qs(cand.doc->getfullName())));
                nameLbl->setStyleSheet("font-size: 13px; font-weight: 700; color: #0f172a; font-family: 'Segoe UI', sans-serif;");

                auto *diffLbl = new QLabel(diffText);
                diffLbl->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 600; color: %1; font-family: 'Segoe UI', sans-serif;").arg(diffColor));

                dwLay->addWidget(nameLbl);
                dwLay->addWidget(diffLbl);
                docTable->setCellWidget(r, 0, docWrap);

                QString spRoom = QStringLiteral("%1 · %2").arg(mainWindow->specialty(cand.doc->getSpId()), UIHelper::formatRoom(UIHelper::qs(cand.doc->getRoomId())));
                auto *spItem = new QTableWidgetItem(spRoom);
                spItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                docTable->setItem(r, 1, spItem);

                auto *slotItem = new QTableWidgetItem(cand.freeSlotText);
                slotItem->setTextAlignment(Qt::AlignCenter);
                docTable->setItem(r, 2, slotItem);

                auto *pickBtn = new QPushButton("✓");
                pickBtn->setFixedSize(30, 26);
                if (cand.freeSlotDT.isValid()) {
                    pickBtn->setCursor(Qt::PointingHandCursor);
                    pickBtn->setToolTip("Chọn khám với bác sĩ này");
                    pickBtn->setStyleSheet("QPushButton { background: #0d9488; color: #ffffff; border: none; border-radius: 6px; font-weight: 800; font-size: 13px; } QPushButton:hover { background: #0f766e; }");
                } else {
                    pickBtn->setEnabled(false);
                    pickBtn->setCursor(Qt::ArrowCursor);
                    pickBtn->setToolTip("Bác sĩ hiện không có ca khám khả dụng");
                    pickBtn->setStyleSheet("QPushButton { background: #e2e8f0; color: #94a3b8; border: none; border-radius: 6px; font-weight: 800; font-size: 13px; }");
                }

                QObject::connect(pickBtn, &QPushButton::clicked, &listDlg, [this, &listDlg, &dialog, a, cand, patientId, feeDiff] {
                    a->setState(-2); // Đánh dấu dời lịch

                    auto *symptomPatients = mainWindow->getSymptomPatients();
                    auto *symptomPatientDetails = mainWindow->getSymptomPatientDetails();
                    std::string nextSpId = SymptomPatient::generateNextId(symptomPatients);
                    std::string oldSpId = a->getSymptomPatientId();

                    if (symptomPatients) {
                        symptomPatients->insert(SymptomPatient(nextSpId, patientId, cand.doc->getSpId(), Date::now()));
                    }
                    if (symptomPatientDetails && !oldSpId.empty()) {
                        auto *dn = symptomPatientDetails->getHeader()->next;
                        while (dn && dn->next) {
                            if (dn->info.getSP_id() == oldSpId) {
                                symptomPatientDetails->insert(SymptomPatientDetail(nextSpId, dn->info.getS_id()));
                            }
                            dn = dn->next;
                        }
                    }

                    std::string newApptId = mainWindow->nextAppointmentId();
                    Date targetDate = cand.freeSlot.isDefault() ? Date::now() : cand.freeSlot;
                    if (mainWindow->getAppointments()) {
                        mainWindow->getAppointments()->insert(Appointment(newApptId, nextSpId, cand.doc->getID(), Date::now(), targetDate, 0));
                    }

                    mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal);

                    QString diffMsg = (feeDiff > 0)
                        ? QStringLiteral(" (Chênh lệch: +%1 đ)").arg(QLocale(QLocale::Vietnamese).toString(feeDiff, 'f', 0))
                        : (feeDiff < 0)
                        ? QStringLiteral(" (Chênh lệch: -%1 đ)").arg(QLocale(QLocale::Vietnamese).toString(-feeDiff, 'f', 0))
                        : QStringLiteral(" (Phí không đổi)");

                    UIHelper::showSuccess(&listDlg, "Thành công", QStringLiteral("Đã chuyển ca khám sang BS. %1 (Phòng %2)%3.").arg(UIHelper::qs(cand.doc->getfullName()), UIHelper::formatRoom(UIHelper::qs(cand.doc->getRoomId())), diffMsg));
                    listDlg.accept();
                    dialog.accept();
                });

                auto *btnWrap = new QWidget;
                auto *bwLay = new QHBoxLayout(btnWrap);
                bwLay->setContentsMargins(0, 0, 0, 0);
                bwLay->setAlignment(Qt::AlignCenter);
                bwLay->addWidget(pickBtn);
                docTable->setCellWidget(r, 3, btnWrap);
            }

            auto *bottomLay = new QHBoxLayout;
            bottomLay->setContentsMargins(0, 0, 0, 0);
            auto *closeListBtn = new QPushButton("Đóng");
            closeListBtn->setCursor(Qt::PointingHandCursor);
            closeListBtn->setFixedHeight(34);
            closeListBtn->setStyleSheet("QPushButton { background: #ffffff; color: #475569; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 0 18px; font-weight: 700; font-size: 12.5px; } QPushButton:hover { background: #f1f5f9; color: #0f172a; }");
            QObject::connect(closeListBtn, &QPushButton::clicked, &listDlg, &QDialog::reject);
            bottomLay->addWidget(closeListBtn, 0, Qt::AlignLeft);
            bottomLay->addStretch();
            lRoot->addLayout(bottomLay);

            listDlg.exec();
        });

        // Sự kiện khi bấm "Dời lịch"
        QObject::connect(reschedBtn, &QPushButton::clicked, &dialog, [this, &dialog, a, patientId] {
            QDialog rDlg(&dialog);
            rDlg.setWindowTitle("Dời lịch khám bệnh");
            rDlg.setFixedWidth(640);
            rDlg.setStyleSheet("QDialog { background-color: #f8fafc; font-family: 'Segoe UI', sans-serif; } QComboBox { background: #ffffff; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 6px 12px; font-size: 13px; color: #0f172a; }");

            auto *rLay = new QVBoxLayout(&rDlg);
            rLay->setContentsMargins(22, 20, 22, 20);
            rLay->setSpacing(14);

            auto *rTitle = new QLabel("Dời lịch & Đổi bác sĩ khám bệnh");
            rTitle->setStyleSheet("font-size: 16px; font-weight: 750; color: #0f172a;");
            rLay->addWidget(rTitle);

            auto *docBox = new QVBoxLayout;
            docBox->setSpacing(5);
            auto *docLbl = new QLabel("Chọn bác sĩ khám:");
            docLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
            auto *docCombo = new QComboBox;

            // 1. Thu thập triệu chứng của cuộc hẹn hiện tại và tính điểm cho từng chuyên khoa
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

            // 2. Thu thập danh sách bác sĩ đang hoạt động và tính điểm phù hợp
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

            // 3. Sắp xếp danh sách bác sĩ: Điểm cao nhất đứng trước -> giảm dần -> các khoa khác (0 điểm)
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

            QDate selD;
            QString selSlot = "";

            auto *dateBox = new QVBoxLayout;
            dateBox->setSpacing(5);
            auto *dateLbl = new QLabel("Chọn ngày & khung giờ mới:");
            dateLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #334155;");
            auto *dateBtn = new QPushButton("Chọn lịch");
            dateBtn->setCursor(Qt::PointingHandCursor);
            dateBtn->setFixedHeight(38);
            dateBtn->setStyleSheet("QPushButton { background-color: #ffffff; border: 1.5px solid #cbd5e1; border-radius: 8px; padding: 6px 14px; font-size: 13px; color: #0f172a; font-weight: 700; text-align: left; } QPushButton:hover { border-color: #0d9488; background-color: #f0fdfa; }");
            dateBox->addWidget(dateLbl);
            dateBox->addWidget(dateBtn);
            rLay->addLayout(dateBox);

            // Phần mở rộng lịch & ca khám phía dưới
            auto *bottomPanel = new BottomCalendarTimeWidget(mainWindow, &rDlg);
            bottomPanel->setVisible(false);
            rLay->addWidget(bottomPanel);

            std::string initDocId = UIHelper::ss(docCombo->currentData().toString());
            bottomPanel->configure(initDocId, QDate(), a->getId(), "", "");

            QObject::connect(dateBtn, &QPushButton::clicked, &rDlg, [&rDlg, bottomPanel, docCombo, a, &selD, &selSlot] {
                bool willShow = !bottomPanel->isVisible();
                if (willShow) {
                    std::string curDocId = UIHelper::ss(docCombo->currentData().toString());
                    bottomPanel->configure(curDocId, selD, a->getId(), "", selSlot);
                }
                bottomPanel->setVisible(willShow);
                rDlg.adjustSize();
            });

            QObject::connect(docCombo, qOverload<int>(&QComboBox::currentIndexChanged), &rDlg, [docCombo, bottomPanel, a, dateBtn, &selD, &selSlot] {
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
                    bottomPanel->configure(curDocId, selD, a->getId(), "", "");
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
                // Kiểm tra cuộc hẹn cũ còn ở trạng thái chờ khám hoặc trễ hẹn không (Rủi ro 5)
                if (a->getState() != 0 && a->getState() != -1) {
                    UIHelper::showWarning(&rDlg, "Không thể dời lịch", "Lịch khám này đã hoàn tất hoặc không còn ở trạng thái cho phép dời lịch.");
                    rDlg.reject();
                    dialog.reject();
                    return;
                }

                // Kiểm tra thời hạn dời lịch (< giờ hẹn + 20 phút)
                Date ad = a->getDateGet();
                QDateTime apptDT(QDate(ad.getYear(), ad.getMonth(), ad.getDay()), QTime(ad.getHour(), ad.getMinute()));
                if (apptDT.isValid() && QDateTime::currentDateTime() >= apptDT.addSecs(20 * 60)) {
                    UIHelper::showWarning(&rDlg, "Hết hạn dời lịch", "Đã quá thời gian cho phép dời lịch khám (chỉ được dời trước giờ hẹn quá 20 phút).");
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
                if (newDT <= QDateTime::currentDateTime()) {
                    UIHelper::showWarning(&rDlg, "Thời gian không hợp lệ", "Thời gian dời lịch phải ở tương lai.");
                    return;
                }
                Date newTargetDate = Date::loadFromString(UIHelper::ss(newDT.toString("yyyy-MM-dd HH:mm")));
                if (AppointmentController::isSlotBooked(mainWindow->getAppointments(), newDocId, newTargetDate, a->getId())) {
                    UIHelper::showWarning(&rDlg, "Trùng giờ khám", "Bác sĩ đã có lịch hẹn khác vào khung giờ này. Vui lòng chọn khung giờ hoặc bác sĩ khác.");
                    return;
                }

                // Nếu cùng bác sĩ và cùng thời gian khám cũ thì vẫn cho thông qua nhưng không tạo symptomPatient và appointment mới
                bool isSameDoctor = (newDocId == a->getDoctorId());
                bool isSameDateTime = (newTargetDate.toString() == a->getDateGet().toString());
                if (isSameDoctor && isSameDateTime) {
                    UIHelper::showSuccess(&rDlg, "Thành công", "Lịch khám và bác sĩ phụ trách được giữ nguyên.");
                    rDlg.accept();
                    dialog.accept();
                    return;
                }

                a->setState(-2); // Đánh dấu dời lịch

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
                    auto *dn = symptomPatientDetails->getHeader()->next;
                    while (dn && dn->next) {
                        if (dn->info.getSP_id() == oldSpId) {
                            symptomPatientDetails->insert(SymptomPatientDetail(nextSpId, dn->info.getS_id()));
                        }
                        dn = dn->next;
                    }
                }

                std::string newApptId = mainWindow->nextAppointmentId();
                if (mainWindow->getAppointments()) {
                    mainWindow->getAppointments()->insert(Appointment(newApptId, nextSpId, newDocId, Date::now(), newTargetDate, 0));
                }

                mainWindow->saveAll(SaveScope::Appointment, SaveAction::Normal);
                UIHelper::showSuccess(&rDlg, "Thành công", "Đã dời lịch khám thành công.");
                rDlg.accept();
                dialog.accept();
            });

            rBtnBox->addWidget(rCancel);
            rBtnBox->addWidget(rConfirm);
            rLay->addLayout(rBtnBox);

            rDlg.exec();
        });

        btnRow->addWidget(inviteBtn);
        btnRow->addWidget(waitBtn);
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

QWidget *AppointmentView::createHolidayWidget(QWidget *parent) {
    QWidget *w = UIHelper::page("Lịch nghỉ", "Quản lý ngày nghỉ lễ áp dụng cho lịch đặt khám.", parent);
    auto *layout = qobject_cast<QVBoxLayout*>(w->layout());

    auto *actions = new QHBoxLayout;
    auto *add = UIHelper::button("＋ Thêm ngày nghỉ");
    auto *edit = UIHelper::button("Sửa lịch nghỉ", "secondary");
    auto *remove = UIHelper::button("Xóa ngày nghỉ", "danger");

    if (mainWindow && !mainWindow->hasPermission("Per_5")) {
        add->hide();
        edit->hide();
        remove->hide();
    }

    actions->addWidget(add);
    actions->addWidget(edit);
    actions->addWidget(remove);
    actions->addStretch();
    layout->addLayout(actions);

    auto *t = UIHelper::table({"NGÀY NGHỈ", "GHI CHÚ"});
    layout->addWidget(t, 1);

    auto fill = [t] {
        t->setRowCount(0);
        if (t->viewport()) {
            t->viewport()->setCursor(Qt::ArrowCursor);
        }
        auto list = AppointmentController::loadHolidays("Config/Lichnghile.txt");
        for (const auto &item : list) {
            int row = t->rowCount();
            t->insertRow(row);
            UIHelper::cell(t, row, 0, item.date, Qt::AlignCenter);
            UIHelper::cell(t, row, 1, item.note);
        }
    };

    auto persist = [t] {
        QList<HolidayEntry> list;
        for (int r = 0; r < t->rowCount(); ++r) {
            list.append({t->item(r, 0)->text(), t->item(r, 1)->text()});
        }
        AppointmentController::saveHolidays("Config/Lichnghile.txt", list);
    };

    auto editHoliday = [this, t, fill, persist](int row, bool isNew) {
        QString oldDate = isNew ? QString() : t->item(row, 0)->text();
        QString oldNote = isNew ? QString() : t->item(row, 1)->text();

        QDialog dialog(mainWindow);
        dialog.setWindowTitle(isNew ? "Thêm ngày nghỉ" : "Chỉnh sửa ngày nghỉ");
        dialog.setStyleSheet(UIHelper::dialogFormStyle());

        auto *v = new QVBoxLayout(&dialog);
        auto *f = new QFormLayout;

        auto *date = new QDateEdit(QDate::currentDate());
        date->setDisplayFormat("d/M");
        UIHelper::setupCalendarPopup(date);

        if (!isNew) {
            auto parts = oldDate.split('/');
            if (parts.size() == 2) {
                date->setDate(QDate(QDate::currentDate().year(), parts[1].toInt(), parts[0].toInt()));
            }
        }
        auto *note = new QLineEdit(oldNote);
        f->addRow("Ngày nghỉ (ngày/tháng)", date);
        f->addRow("Ghi chú", note);
        v->addLayout(f);

        auto *save = UIHelper::button("Lưu");
        save->setAutoDefault(false);
        save->setDefault(false);
        v->addWidget(save);

        connect(save, &QPushButton::clicked, &dialog, [&dialog, t, date, note, row, isNew, fill, persist] {
            if (note->text().trimmed().isEmpty()) {
                UIHelper::showWarning(&dialog, "Thiếu ghi chú", "Vui lòng nhập ghi chú ngày nghỉ.");
                return;
            }
            QString value = date->date().toString("d/M");
            if (isNew) {
                int r = t->rowCount();
                t->insertRow(r);
                auto *item0 = new QTableWidgetItem(value);
                item0->setTextAlignment(Qt::AlignCenter);
                t->setItem(r, 0, item0);
                t->setItem(r, 1, new QTableWidgetItem(note->text().trimmed()));
            } else {
                t->item(row, 0)->setText(value);
                t->item(row, 1)->setText(note->text().trimmed());
            }
            persist();
            fill();
            dialog.accept();
        });
        dialog.exec();
    };

    connect(add, &QPushButton::clicked, w, [editHoliday] { editHoliday(0, true); });
    connect(edit, &QPushButton::clicked, w, [t, editHoliday] {
        int r = t->currentRow();
        if (r >= 0) editHoliday(r, false);
    });
    connect(remove, &QPushButton::clicked, w, [this, t, fill, persist] {
        int r = t->currentRow();
        if (r < 0) return;
        if (!mainWindow || !mainWindow->hasPermission("Per_5")) {
            UIHelper::showWarning(mainWindow, "Không có quyền", "Bạn không có quyền xóa lịch nghỉ.");
            return;
        }
        if (!UIHelper::confirmDelete(mainWindow, "Xóa ngày nghỉ", "Bạn có chắc chắn muốn xóa ngày nghỉ này khỏi lịch không?")) {
            return;
        }
        t->removeRow(r);
        persist();
        fill();
    });

    fill();
    return w;
}


void AppointmentView::showHolidayManagementDialog(QWidget *parent) {
    if (!mainWindow) return;

    QDialog dialog(parent ? parent : mainWindow);
    dialog.setWindowTitle("Quản lý lịch nghỉ lễ");
    dialog.setMinimumWidth(660);
    dialog.setMinimumHeight(460);
    dialog.setStyleSheet(UIHelper::dialogFormStyle() + R"(
        QLabel#dialogTitle { font-size: 18px; font-weight: 750; color: #0f172a; }
        QLabel#dialogSubtitle { font-size: 13px; color: #64748b; }
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

    // Header layout
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

    // Table
    auto *t = UIHelper::table({"NGÀY NGHỈ", "GHI CHÚ / THỜI GIAN", "THAO TÁC"});
    t->setShowGrid(false);
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    t->setColumnWidth(0, 160);
    t->setColumnWidth(2, 140);
    t->verticalHeader()->setDefaultSectionSize(48);
    rootLayout->addWidget(t, 1);

    auto fillPtr = std::make_shared<std::function<void()>>();
    auto fill = [fillPtr]() { if (*fillPtr) (*fillPtr)(); };
    *fillPtr = [this, t, &dialog, fill]() {
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
                displayNote = QStringLiteral("Nghỉ 5 ngày (Mùng 1 Tết: %1)").arg(mung1.toString("dd/MM/yyyy"));
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

            // Action widget
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

            QObject::connect(editBtn, &QPushButton::clicked, &dialog, [this, &dialog, i, item, isTet]() {
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

                QObject::connect(saveB, &QPushButton::clicked, &editDlg, [&editDlg, dateEdit, noteEdit, i]() {
                    auto curList = AppointmentController::loadHolidays("Config/Lichnghile.txt");
                    if (i < curList.size()) {
                        curList[i].date = dateEdit->text().trimmed();
                        curList[i].note = noteEdit->text().trimmed();
                        AppointmentController::saveHolidays("Config/Lichnghile.txt", curList);
                    }
                    editDlg.accept();
                });

                if (editDlg.exec() == QDialog::Accepted) {
                    editDlg.close();
                }
            });

            QObject::connect(delBtn, &QPushButton::clicked, &dialog, [this, &dialog, i, item, fill]() {
                if (!mainWindow || !mainWindow->hasPermission("Per_5")) {
                    UIHelper::showWarning(&dialog, "Không có quyền", "Bạn không có quyền xóa lịch nghỉ.");
                    return;
                }
                if (UIHelper::confirmDelete(&dialog, "Xác nhận xóa ngày nghỉ",
                    QStringLiteral("Bạn có chắc chắn muốn xóa ngày nghỉ '%1' khỏi lịch phòng khám?").arg(item.date), "Xóa", "Không xóa")) {
                    auto curList = AppointmentController::loadHolidays("Config/Lichnghile.txt");
                    if (i < curList.size()) {
                        curList.removeAt(i);
                        AppointmentController::saveHolidays("Config/Lichnghile.txt", curList);
                        fill();
                    }
                }
            });
        }
    };

    if (mainWindow && !mainWindow->hasPermission("Per_5")) {
        addBtn->setToolTip("Bạn không có quyền thêm ngày nghỉ (Yêu cầu quyền Per_5)");
    }

    QObject::connect(addBtn, &QPushButton::clicked, &dialog, [this, &dialog, fill]() {
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

        QObject::connect(saveB, &QPushButton::clicked, &addDlg, [&addDlg, dateEdit, noteEdit]() {
            if (dateEdit->text().trimmed().isEmpty()) {
                UIHelper::showWarning(&addDlg, "Thiếu thông tin", "Vui lòng nhập ngày nghỉ.");
                return;
            }
            auto curList = AppointmentController::loadHolidays("Config/Lichnghile.txt");
            curList.append({dateEdit->text().trimmed(), noteEdit->text().trimmed()});
            AppointmentController::saveHolidays("Config/Lichnghile.txt", curList);
            addDlg.accept();
        });

        if (addDlg.exec() == QDialog::Accepted) {
            fill();
        }
    });

    fill();
    dialog.exec();
}

void AppointmentView::reapplyCurrentSort() {
    if (!mainWindow || !mainWindow->getAppointments()) return;
    if (listSortColumn < 0) {
        libPBL2::sortAppointments(mainWindow->getAppointments(), libPBL2::AppointmentSortBy::ID, false);
        return;
    }
    libPBL2::AppointmentSortBy criteria = libPBL2::AppointmentSortBy::Date;
    switch (listSortColumn) {
        case 1: criteria = libPBL2::AppointmentSortBy::DoctorID; break;
        case 3: criteria = libPBL2::AppointmentSortBy::Date; break;
        case 4: criteria = libPBL2::AppointmentSortBy::State; break;
        default: criteria = libPBL2::AppointmentSortBy::Date; break;
    }
    libPBL2::sortAppointments(mainWindow->getAppointments(), criteria, listSortAscending);
}

void AppointmentView::resetToFirstPage() {
    listSortColumn = -1;
    listSortAscending = false;
    listCurrentPage = 0;
    if (mainWindow && mainWindow->getAppointments()) {
        libPBL2::sortAppointments(mainWindow->getAppointments(), libPBL2::AppointmentSortBy::ID, false);
    }
}
