#include "UIHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QMessageBox>
#include <QLineEdit>
#include <QTableView>
#include <QHeaderView>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QGraphicsDropShadowEffect>
#include <QPainterPath>
#include <QCalendarWidget>
#include <QLocale>
#include <QToolButton>
#include <QTextCharFormat>
#include <QMenu>
#include <QSpinBox>
#include <QTimer>
#include <QEvent>
#include <QKeyEvent>

namespace {

class TableColumnScaler : public QObject {
public:
    TableColumnScaler(QTableWidget *table, const QVector<double> &ratios, int fixedLastColWidth, QObject *parent = nullptr)
        : QObject(parent), m_table(table), m_ratios(ratios), m_fixedLast(fixedLastColWidth) {
        if (m_table) {
            m_table->installEventFilter(this);
            if (auto *vp = m_table->viewport()) {
                vp->installEventFilter(this);
            }
            if (auto *header = m_table->horizontalHeader()) {
                header->installEventFilter(this);
            }
        }
    }

    void scale() {
        if (!m_table || m_isScaling) return;
        m_isScaling = true;

        int totalW = m_table->viewport()->width();
        if (totalW <= 0) totalW = m_table->width();
        if (totalW <= 50) {
            m_isScaling = false;
            return;
        }

        int remainW = totalW - m_fixedLast;
        if (remainW <= 0) {
            m_isScaling = false;
            return;
        }

        int allocated = 0;
        int n = m_ratios.size();
        for (int i = 0; i < n; ++i) {
            int w = 0;
            if (i == n - 1) {
                w = remainW - allocated;
            } else {
                w = static_cast<int>(remainW * m_ratios[i]);
                allocated += w;
            }
            if (w < 20) w = 20;
            m_table->setColumnWidth(i, w);
        }
        if (m_fixedLast > 0 && n < m_table->columnCount()) {
            m_table->setColumnWidth(n, m_fixedLast);
        }
        m_isScaling = false;
    }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show) {
            scale();
        }
        return QObject::eventFilter(obj, event);
    }

private:
    QTableWidget *m_table = nullptr;
    QVector<double> m_ratios;
    int m_fixedLast = 0;
    bool m_isScaling = false;
};

class DigitsOnlyFilter : public QObject {
public:
    explicit DigitsOnlyFilter(int maxLen, QObject *parent = nullptr)
        : QObject(parent), m_maxLen(maxLen) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::InputMethod) {
            return true;
        } else if (event->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent*>(event);

            if (ke->key() == Qt::Key_Backspace || ke->key() == Qt::Key_Delete ||
                ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right ||
                ke->key() == Qt::Key_Home || ke->key() == Qt::Key_End ||
                ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Return ||
                ke->key() == Qt::Key_Enter ||
                (ke->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
                return false;
            }

            QString txt = ke->text();
            if (!txt.isEmpty()) {
                if (!txt[0].isDigit()) {
                    return true;
                }
                auto *le = qobject_cast<QLineEdit*>(obj);
                if (le && le->text().length() >= m_maxLen && !le->hasSelectedText()) {
                    return true;
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }

private:
    int m_maxLen = 10;
};

class CalendarNoTriggerFilter : public QObject {
public:
    using QObject::QObject;
protected:
    bool eventFilter(QObject *obj, QEvent *ev) override {
        if (ev->type() == QEvent::MouseButtonPress ||
            ev->type() == QEvent::MouseButtonRelease ||
            ev->type() == QEvent::MouseButtonDblClick ||
            ev->type() == QEvent::ContextMenu) {
            return true;
        }
        return QObject::eventFilter(obj, ev);
    }
};

}

QWidget *UIHelper::page(const QString &title, const QString &subtitle, QWidget *parent) {
    auto *w = new QWidget(parent);
    w->setCursor(Qt::ArrowCursor);
    auto *layout = new QVBoxLayout(w);
    layout->setContentsMargins(36, 32, 36, 32);
    layout->setSpacing(20);

    auto *h = new QVBoxLayout;
    h->setSpacing(4);
    auto *t = new QLabel(title);
    t->setObjectName("pageTitle");
    auto *s = new QLabel(subtitle);
    s->setObjectName("pageSubtitle");
    h->addWidget(t);
    h->addWidget(s);
    layout->addLayout(h);
    return w;
}

QFrame *UIHelper::panel() {
    auto *f = new QFrame;
    f->setObjectName("panel");
    auto *shadow = new QGraphicsDropShadowEffect(f);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(20, 48, 65, 12));
    f->setGraphicsEffect(shadow);
    return f;
}

QPushButton *UIHelper::button(const QString &text, const QString &kind) {
    auto *b = new QPushButton(text);
    b->setObjectName(kind);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QTableWidget *UIHelper::table(const QStringList &headers) {
    auto *t = new QTableWidget;
    t->setColumnCount(headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->verticalHeader()->setVisible(false);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setShowGrid(false);
    t->setAlternatingRowColors(false);
    t->horizontalHeader()->setStretchLastSection(false);
    t->horizontalHeader()->setSectionsClickable(true);
    t->horizontalHeader()->setSortIndicatorShown(false);
    t->horizontalHeader()->setHighlightSections(false);
    t->horizontalHeader()->setCursor(Qt::ArrowCursor);
    t->setCursor(Qt::ArrowCursor);
    if (t->viewport()) {
        t->viewport()->setCursor(Qt::ArrowCursor);
    }
    t->setFocusPolicy(Qt::NoFocus);
    t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    if (t->viewport()) {
        t->viewport()->setAutoFillBackground(false);
        t->viewport()->setStyleSheet("background: transparent;");
    }
    t->setStyleSheet(R"(
        QTableWidget {
            background-color: transparent;
            border: none;
            gridline-color: transparent;
            outline: none;
        }
        QHeaderView {
            border: none;
            background: transparent;
            border-top-left-radius: 9px;
            border-top-right-radius: 9px;
        }
        QHeaderView::section {
            background: #f8fafc;
            color: #475569;
            font-weight: 700;
            font-size: 11.5px;
            padding: 11px 14px;
            border: none;
            border-bottom: 2px solid #e2e8f0;
            border-left: none;
            border-right: none;
        }
        QHeaderView::section:first {
            border-top-left-radius: 9px;
        }
        QHeaderView::section:last {
            border-top-right-radius: 9px;
        }
        QHeaderView::up-arrow, QHeaderView::down-arrow {
            image: none;
            width: 0px;
            height: 0px;
        }
        QTableWidget::item {
            background: transparent;
            border: none;
            border-bottom: 1px solid #f1f5f9;
            border-left: none;
            border-right: none;
            padding-left: 14px;
            padding-right: 14px;
            color: #1e293b;
            font-size: 13px;
        }
        QTableWidget::item:hover {
            background: #f0fdfa;
        }
        QTableWidget::item:selected {
            background-color: #ccfbf1;
            color: #0f766e;
            font-weight: 700;
        }
        QTableWidget QScrollBar:vertical,
        QTableWidget QScrollBar:horizontal {
            width: 0px;
            height: 0px;
            border: none;
            background: transparent;
        }
    )");
    return t;
}

QFrame *UIHelper::createTableCard(QTableWidget *table, QWidget *parent) {
    auto *card = new QFrame(parent);
    card->setObjectName("tableCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setCursor(Qt::ArrowCursor);
    card->setStyleSheet(R"(
        QFrame#tableCard {
            background-color: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 10px;
        }
    )");
    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(0, 0, 0, 4);
    lay->setSpacing(0);
    if (table) {
        lay->addWidget(table);
    }
    return card;
}

void UIHelper::setupTableScaling(QTableWidget *table, const QVector<double> &ratios, int fixedLastColWidth) {
    if (!table) return;
    table->horizontalHeader()->setStretchLastSection(false);
    for (int i = 0; i < table->columnCount(); ++i) {
        table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
    }
    auto *scaler = new TableColumnScaler(table, ratios, fixedLastColWidth, table);
    QTimer::singleShot(0, table, [scaler]() { scaler->scale(); });
}

void UIHelper::cell(QTableWidget *t, int row, int column, const QString &text, Qt::Alignment align) {
    if (!t) return;
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(align);
    t->setItem(row, column, item);
}

QString UIHelper::formatRoom(const QString &rawRoom) {
    QString r = rawRoom.trimmed();
    if (r.isEmpty()) {
        return "";
    }
    if (r.startsWith("Room_", Qt::CaseInsensitive)) {
        return "Room " + r.mid(5).trimmed();
    }
    if (r.startsWith("Room ", Qt::CaseInsensitive)) {
        return r;
    }
    if (r.startsWith("Room", Qt::CaseInsensitive)) {
        return "Room " + r.mid(4).trimmed();
    }
    if (r.startsWith("R_", Qt::CaseInsensitive)) {
        return "Room " + r.mid(2).trimmed();
    }
    if (r.startsWith("R", Qt::CaseInsensitive) && r.length() > 1 && r[1].isDigit()) {
        return "Room " + r.mid(1).trimmed();
    }
    return "Room " + r;
}

QPixmap UIHelper::createAvatar(const QString &fullName, int size, const QColor &bgColor, const QColor &textColor) {
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setBrush(QBrush(bgColor));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, size, size);

    QString initial = "U";
    QStringList parts = fullName.trimmed().split(' ', Qt::SkipEmptyParts);
    if (!parts.isEmpty()) {
        initial = parts.last().left(1).toUpper();
    }
    p.setPen(textColor);
    QFont f("Be Vietnam Pro");
    f.setPixelSize(qMax(10, static_cast<int>(size / 2.2)));
    f.setBold(true);
    p.setFont(f);
    p.drawText(QRect(0, 0, size, size), Qt::AlignCenter, initial);
    return pix;
}

QIcon UIHelper::createEyeIcon(bool isVisible) {
    int size = 56;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.scale(2.0, 2.0);

        if (!isVisible) {
            QColor color("#94A3B8");
            QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);

            QPainterPath eyePath;
            eyePath.moveTo(3.5, 14.0);
            eyePath.quadTo(14.0, 6.5, 24.5, 14.0);
            eyePath.quadTo(14.0, 21.5, 3.5, 14.0);
            p.drawPath(eyePath);

            p.drawEllipse(QPointF(14.0, 14.0), 3.0, 3.0);
            p.drawLine(QPointF(5.0, 5.0), QPointF(23.0, 23.0));
        } else {
            QColor color("#0D9488");
            QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);

            QPainterPath eyePath;
            eyePath.moveTo(3.5, 14.0);
            eyePath.quadTo(14.0, 6.5, 24.5, 14.0);
            eyePath.quadTo(14.0, 21.5, 3.5, 14.0);
            p.drawPath(eyePath);

            p.setBrush(color);
            p.drawEllipse(QPointF(14.0, 14.0), 3.0, 3.0);
        }
    }
    pix.setDevicePixelRatio(2.0);
    return QIcon(pix);
}

QIcon UIHelper::trashIcon(const QColor &color) {
    int size = 16;
    int scaleFactor = 2;
    auto drawTrash = [&](const QColor &c) {
        QPixmap pixmap(size * scaleFactor, size * scaleFactor);
        pixmap.fill(Qt::transparent);
        {
            QPainter p(&pixmap);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.scale(scaleFactor, scaleFactor);

            QPen pen(c, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);

            p.drawRoundedRect(QRectF(size * 0.35, size * 0.08, size * 0.30, size * 0.12), 0.8, 0.8);
            p.drawLine(QPointF(size * 0.15, size * 0.24), QPointF(size * 0.85, size * 0.24));

            QPainterPath bodyPath;
            bodyPath.moveTo(size * 0.24, size * 0.30);
            bodyPath.lineTo(size * 0.29, size * 0.85);
            bodyPath.quadTo(size * 0.30, size * 0.92, size * 0.38, size * 0.92);
            bodyPath.lineTo(size * 0.62, size * 0.92);
            bodyPath.quadTo(size * 0.70, size * 0.92, size * 0.71, size * 0.85);
            bodyPath.lineTo(size * 0.76, size * 0.30);
            p.drawPath(bodyPath);

            p.drawLine(QPointF(size * 0.42, size * 0.42), QPointF(size * 0.43, size * 0.78));
            p.drawLine(QPointF(size * 0.58, size * 0.42), QPointF(size * 0.57, size * 0.78));
        }
        pixmap.setDevicePixelRatio(scaleFactor);
        return pixmap;
    };

    QIcon icon;
    icon.addPixmap(drawTrash(color), QIcon::Normal);
    icon.addPixmap(drawTrash(QColor("#b91c1c")), QIcon::Active);
    return icon;
}

QPair<QWidget*, QPushButton*> UIHelper::createActionCell(const QIcon &icon, const QString &tooltip) {
    auto *widget = new QWidget;
    widget->setCursor(Qt::ArrowCursor);
    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    auto *btn = new QPushButton;
    btn->setIcon(icon);
    btn->setIconSize(QSize(16, 16));
    btn->setCursor(Qt::PointingHandCursor);
    btn->setToolTip(tooltip);
    btn->setFixedSize(26, 26);
    btn->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: 1px solid transparent;
            border-radius: 5px;
        }
        QPushButton:hover {
            background-color: #fee2e2;
            border-color: #fca5a5;
        }
        QPushButton:pressed {
            background-color: #fecaca;
        }
    )");

    layout->addWidget(btn);
    return qMakePair(widget, btn);
}

QString UIHelper::dialogFormStyle() {
    return QStringLiteral(R"(
        QDialog {
            background-color: #f8fafc;
            font-family: 'Segoe UI', sans-serif;
        }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
            background: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            color: #0f172a;
            min-height: 20px;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
            border: 1.5px solid #0d9488;
            background: #ffffff;
        }
        QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {
            background: #e2e8f0;
            color: #475569;
            border: 1.5px solid #cbd5e1;
        }
        QSpinBox, QDoubleSpinBox {
            padding-right: 30px;
        }
        QSpinBox:disabled, QDoubleSpinBox:disabled {
            padding-right: 12px;
        }
        QSpinBox::up-button, QDoubleSpinBox::up-button {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 25px;
            border-left: 1.5px solid #94a3b8;
            border-bottom: 1px solid #94a3b8;
            border-top-right-radius: 7px;
            background: #e2e8f0;
        }
        QSpinBox::up-button:disabled, QDoubleSpinBox::up-button:disabled {
            width: 0px;
            border: none;
            background: transparent;
        }
        QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover {
            background: #0d9488;
        }
        QSpinBox::up-button:pressed, QDoubleSpinBox::up-button:pressed {
            background: #0f766e;
        }
        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
            image: url(:/assets/textures/arrow_up_dark.png);
            width: 9px;
            height: 9px;
        }
        QSpinBox::up-arrow:disabled, QDoubleSpinBox::up-arrow:disabled {
            image: none;
        }
        QSpinBox::up-arrow:hover, QDoubleSpinBox::up-arrow:hover {
            image: url(:/assets/textures/arrow_up_white.png);
        }
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            width: 25px;
            border-left: 1.5px solid #94a3b8;
            border-bottom-right-radius: 7px;
            background: #e2e8f0;
        }
        QSpinBox::down-button:disabled, QDoubleSpinBox::down-button:disabled {
            width: 0px;
            border: none;
            background: transparent;
        }
        QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
            background: #0d9488;
        }
        QSpinBox::down-button:pressed, QDoubleSpinBox::down-button:pressed {
            background: #0f766e;
        }
        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
            image: url(:/assets/textures/arrow_down_dark.png);
            width: 9px;
            height: 9px;
        }
        QSpinBox::down-arrow:disabled, QDoubleSpinBox::down-arrow:disabled {
            image: none;
        }
        QSpinBox::down-arrow:hover, QDoubleSpinBox::down-arrow:hover {
            image: url(:/assets/textures/arrow_down_white.png);
        }
        QComboBox {
            padding-right: 4px;
        }
        QComboBox:disabled {
            padding-right: 12px;
        }
        QComboBox::drop-down {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 24px;
            border: none;
            background: transparent;
        }
        QComboBox::drop-down:disabled {
            width: 0px;
            border: none;
        }
        QComboBox::down-arrow {
            image: url(:/assets/textures/arrow_down_dark.png);
            width: 11px;
            height: 11px;
            margin-right: 6px;
        }
        QComboBox::down-arrow:disabled {
            image: none;
        }
        QComboBox QAbstractItemView,
        QListView {
            background-color: #ffffff;
            color: #0f172a;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            selection-background-color: #0d9488;
            selection-color: #ffffff;
            outline: none;
            padding: 4px;
        }
        QComboBox QAbstractItemView::item, QListView::item {
            min-height: 28px;
            padding: 4px 10px;
            border-radius: 6px;
            color: #0f172a;
            background-color: #ffffff;
        }
        QComboBox QAbstractItemView::item:hover, QListView::item:hover {
            background-color: #f0fdfa;
            color: #0f766e;
        }
        QComboBox QAbstractItemView::item:selected, QListView::item:selected {
            background-color: #0d9488;
            color: #ffffff;
        }
        QScrollBar:vertical, QScrollBar:horizontal {
            width: 0px;
            height: 0px;
            background: transparent;
            border: none;
        }
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: transparent;
            border: none;
        }
        QScrollBar::add-line, QScrollBar::sub-line,
        QScrollBar::add-page, QScrollBar::sub-page {
            background: transparent;
            border: none;
            width: 0px;
            height: 0px;
        }
        QMessageBox {
            background-color: #ffffff;
            border: 1px solid #cbd5e1;
            border-radius: 12px;
        }
        QMessageBox QLabel {
            color: #0f172a;
            font-size: 13.5px;
            background: transparent;
        }
        QMessageBox QPushButton {
            background-color: #f1f5f9;
            color: #334155;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            padding: 8px 18px;
            font-size: 13px;
            font-weight: 600;
            min-width: 80px;
            min-height: 22px;
        }
        QMessageBox QPushButton:hover {
            background-color: #e2e8f0;
            color: #0f172a;
            border-color: #94a3b8;
        }
        QMessageBox QPushButton:pressed {
            background-color: #cbd5e1;
        }
        QMessageBox QPushButton:default {
            background-color: #0d9488;
            color: #ffffff;
            border: none;
            font-weight: 700;
        }
        QMessageBox QPushButton:default:hover {
            background-color: #0f766e;
        }
    )");
}

void UIHelper::applyCalendarStyle(QCalendarWidget *cal) {
    if (!cal) return;
    cal->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    cal->setFirstDayOfWeek(Qt::Sunday);
    cal->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    if (auto *navBar = cal->findChild<QWidget*>("qt_calendar_navigationbar")) {
        if (auto *lay = qobject_cast<QHBoxLayout*>(navBar->layout())) {
            if (lay->count() == 7) {
                auto *midSpacer = lay->itemAt(3);
                if (midSpacer && midSpacer->spacerItem()) {
                    lay->removeItem(midSpacer);
                    delete midSpacer;
                }
            }
            lay->setContentsMargins(6, 2, 6, 2);
            lay->setSpacing(4);
        }
    }

    if (auto *prevBtn = cal->findChild<QToolButton*>("qt_calendar_prevmonth")) {
        prevBtn->setIcon(QIcon());
        prevBtn->setText(QString::fromUtf8("‹"));
        prevBtn->setCursor(Qt::PointingHandCursor);
    }
    if (auto *nextBtn = cal->findChild<QToolButton*>("qt_calendar_nextmonth")) {
        nextBtn->setIcon(QIcon());
        nextBtn->setText(QString::fromUtf8("›"));
        nextBtn->setCursor(Qt::PointingHandCursor);
    }

    if (auto *monthBtn = cal->findChild<QToolButton*>("qt_calendar_monthbutton")) {
        monthBtn->setMenu(nullptr);
        monthBtn->setPopupMode(QToolButton::DelayedPopup);
        monthBtn->setCursor(Qt::ArrowCursor);
        monthBtn->setFocusPolicy(Qt::NoFocus);
        monthBtn->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        monthBtn->installEventFilter(new CalendarNoTriggerFilter(monthBtn));
    }
    if (auto *monthMenu = cal->findChild<QMenu*>("qt_calendar_monthmenu")) {
        monthMenu->clear();
        monthMenu->setEnabled(false);
        monthMenu->hide();
    }
    if (auto *yearBtn = cal->findChild<QToolButton*>("qt_calendar_yearbutton")) {
        yearBtn->disconnect(SIGNAL(clicked()));
        yearBtn->setCursor(Qt::ArrowCursor);
        yearBtn->setFocusPolicy(Qt::NoFocus);
        yearBtn->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        yearBtn->installEventFilter(new CalendarNoTriggerFilter(yearBtn));
    }
    if (auto *yearEdit = cal->findChild<QSpinBox*>("qt_calendar_yearedit")) {
        yearEdit->hide();
        yearEdit->setEnabled(false);
    }

    QTextCharFormat weekdayFmt;
    weekdayFmt.setForeground(QColor("#475569"));
    weekdayFmt.setFontWeight(QFont::Normal);
    weekdayFmt.setFontPointSize(8.5);
    for (int dow = 1; dow <= 7; ++dow) {
        cal->setWeekdayTextFormat(static_cast<Qt::DayOfWeek>(dow), weekdayFmt);
    }

    if (auto *view = cal->findChild<QTableView*>("qt_calendar_calendarview")) {
        if (view->horizontalHeader()) {
            view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        }
        if (view->verticalHeader()) {
            view->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        }
    }

    cal->setStyleSheet(R"(
        QCalendarWidget {
            background-color: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            font-family: 'Segoe UI', -apple-system, sans-serif;
            font-size: 11px;
        }
        QCalendarWidget QWidget {
            alternate-background-color: #ffffff;
        }
        QCalendarWidget QWidget#qt_calendar_navigationbar {
            background-color: #f8fafc;
            border-top-left-radius: 7px;
            border-top-right-radius: 7px;
            border-bottom: 1px solid #e2e8f0;
            min-height: 30px;
            max-height: 30px;
        }
        QCalendarWidget QToolButton {
            color: #0f172a;
            font-weight: 700;
            font-size: 11px;
            background: transparent;
            border: none;
            padding: 2px 4px;
            margin: 0px;
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
        QCalendarWidget QToolButton#qt_calendar_monthbutton:hover,
        QCalendarWidget QToolButton#qt_calendar_yearbutton:hover {
            background: transparent;
            color: #0f172a;
        }
        QCalendarWidget QToolButton::menu-indicator,
        QCalendarWidget QToolButton#qt_calendar_monthbutton::menu-indicator {
            image: none;
            width: 0px;
            height: 0px;
        }
        QCalendarWidget QSpinBox#qt_calendar_yearedit {
            width: 0px;
            height: 0px;
            border: none;
        }
        QCalendarWidget QTableView {
            background-color: #ffffff;
            selection-background-color: transparent;
            selection-color: #ffffff;
            color: #1e293b;
            font-size: 11px;
            font-weight: normal;
            outline: none;
            border: none;
            gridline-color: transparent;
        }
        QCalendarWidget QTableView QHeaderView::section {
            background-color: #f8fafc;
            color: #475569;
            font-weight: normal;
            font-size: 9.5px;
            border: none;
            border-bottom: 1px solid #e2e8f0;
            padding: 2px 0px;
            margin: 0px;
        }
        QCalendarWidget QTableView::item {
            border-radius: 4px;
            padding: 1px;
            margin: 1px;
            background-color: transparent;
            font-weight: normal;
        }
        QCalendarWidget QTableView::item:hover {
            background-color: #ccfbf1;
            color: #0f766e;
            font-weight: normal;
        }
        QCalendarWidget QTableView::item:selected {
            background-color: #0d9488;
            color: #ffffff;
            font-weight: normal;
        }
        QCalendarWidget QTableView::item:disabled {
            color: #94a3b8;
            background-color: transparent;
            font-weight: normal;
        }
        QCalendarWidget QWidget#qt_calendar_calendarview {
            background-color: #ffffff;
            border-bottom-left-radius: 7px;
            border-bottom-right-radius: 7px;
        }
    )");
}

namespace {

void setupDigitsInput(QLineEdit *lineEdit, int maxLen) {
    if (!lineEdit) return;
    lineEdit->setMaxLength(maxLen);
    lineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);
    lineEdit->setInputMethodHints(Qt::ImhDigitsOnly | Qt::ImhNoPredictiveText);
    lineEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("^\\d{0,%1}$").arg(maxLen)), lineEdit));
    lineEdit->installEventFilter(new DigitsOnlyFilter(maxLen, lineEdit));

    QObject::connect(lineEdit, &QLineEdit::textChanged, lineEdit, [lineEdit, maxLen](const QString &text) {
        QString digits;
        for (const QChar &ch : text) {
            if (ch.isDigit()) {
                digits.append(ch);
                if (digits.length() == maxLen) break;
            }
        }
        if (digits != text) {
            int pos = lineEdit->cursorPosition();
            lineEdit->setText(digits);
            lineEdit->setCursorPosition(qMin(pos, digits.length()));
        }
    });
}

bool isValidDigitsStartingWithZero(const QString &value, int expectedLen) {
    QString s = value.trimmed();
    if (s.length() != expectedLen || !s.startsWith('0')) return false;
    for (const QChar &c : s) {
        if (!c.isDigit()) return false;
    }
    return true;
}

void showStandardMessageBox(QMessageBox::Icon icon, QWidget *parent, const QString &title, const QString &message) {
    QMessageBox box(icon, title, message, QMessageBox::Ok, parent);
    box.setStyleSheet(R"(
        QMessageBox {
            background-color: #ffffff;
        }
        QLabel {
            color: #0f172a;
            font-size: 13.5px;
            background: transparent;
        }
        QPushButton {
            background-color: #0d9488;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            padding: 6px 18px;
            font-size: 13px;
            font-weight: 700;
            min-width: 75px;
            min-height: 22px;
        }
        QPushButton:hover {
            background-color: #0f766e;
        }
    )");
    box.setWindowModality(Qt::ApplicationModal);
    box.raise();
    box.activateWindow();
    box.exec();
}

}

void UIHelper::setupPhoneInput(QLineEdit *lineEdit) {
    setupDigitsInput(lineEdit, 10);
}

void UIHelper::setupCccdInput(QLineEdit *lineEdit) {
    setupDigitsInput(lineEdit, 12);
}

bool UIHelper::isValidPhone(const QString &phone) {
    return isValidDigitsStartingWithZero(phone, 10);
}

bool UIHelper::isValidCccd(const QString &cccd) {
    return isValidDigitsStartingWithZero(cccd, 12);
}

bool UIHelper::confirmDelete(QWidget *parent, const QString &title, const QString &message,
                             const QString &confirmText, const QString &cancelText) {
    QMessageBox box(QMessageBox::Question, title, message, QMessageBox::NoButton, parent);
    QPushButton *confirmBtn = box.addButton(confirmText, QMessageBox::AcceptRole);
    QPushButton *cancelBtn = box.addButton(cancelText, QMessageBox::RejectRole);
    box.setDefaultButton(cancelBtn);

    confirmBtn->setObjectName("confirmBtnNeutral");
    cancelBtn->setObjectName("cancelBtnRed");

    box.setStyleSheet(R"(
        QMessageBox {
            background-color: #ffffff;
        }
        QLabel {
            color: #0f172a;
            font-size: 13.5px;
            background: transparent;
        }
        QPushButton {
            border-radius: 6px;
            padding: 6px 18px;
            font-size: 13px;
            font-weight: 700;
            min-width: 80px;
            min-height: 24px;
        }
        QPushButton#cancelBtnRed {
            background-color: #dc2626;
            color: #ffffff;
            border: none;
        }
        QPushButton#cancelBtnRed:hover {
            background-color: #b91c1c;
        }
        QPushButton#confirmBtnNeutral {
            background-color: #f1f5f9;
            color: #475569;
            border: 1px solid #cbd5e1;
        }
        QPushButton#confirmBtnNeutral:hover {
            background-color: #e2e8f0;
            color: #1e293b;
        }
    )");
    box.setWindowModality(Qt::ApplicationModal);
    box.raise();
    box.activateWindow();
    box.exec();
    return box.clickedButton() == confirmBtn;
}

bool UIHelper::confirmWarning(QWidget *parent, const QString &title, const QString &message,
                              const QString &confirmText, const QString &cancelText) {
    QMessageBox box(QMessageBox::Warning, title, message, QMessageBox::NoButton, parent);
    QPushButton *confirmBtn = box.addButton(confirmText, QMessageBox::AcceptRole);
    QPushButton *cancelBtn = box.addButton(cancelText, QMessageBox::RejectRole);
    box.setDefaultButton(cancelBtn);
    confirmBtn->setObjectName("confirmBtnAmber");
    cancelBtn->setObjectName("cancelBtnNeutral");
    box.setStyleSheet(R"(
        QMessageBox {
            background-color: #ffffff;
        }
        QLabel {
            color: #0f172a;
            font-size: 13.5px;
            background: transparent;
        }
        QPushButton {
            border-radius: 6px;
            padding: 6px 18px;
            font-size: 13px;
            font-weight: 700;
            min-width: 75px;
            min-height: 22px;
        }
        QPushButton#confirmBtnAmber {
            background-color: #d97706;
            color: #ffffff;
            border: none;
        }
        QPushButton#confirmBtnAmber:hover {
            background-color: #b45309;
        }
        QPushButton#cancelBtnNeutral {
            background-color: #f1f5f9;
            color: #475569;
            border: 1px solid #cbd5e1;
        }
        QPushButton#cancelBtnNeutral:hover {
            background-color: #e2e8f0;
        }
    )");
    box.setWindowModality(Qt::ApplicationModal);
    box.raise();
    box.activateWindow();
    box.exec();
    return box.clickedButton() == confirmBtn;
}

void UIHelper::showWarning(QWidget *parent, const QString &title, const QString &message) {
    showStandardMessageBox(QMessageBox::Warning, parent, title, message);
}

void UIHelper::showInfo(QWidget *parent, const QString &title, const QString &message) {
    showStandardMessageBox(QMessageBox::Information, parent, title, message);
}

void UIHelper::showSuccess(QWidget *parent, const QString &title, const QString &message) {
    showInfo(parent, title, message);
}
