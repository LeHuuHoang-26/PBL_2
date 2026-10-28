#include "UIHelper.h"
#include <QDialog>
#include <QLineEdit>
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
#include <QFocusEvent>
#ifdef _WIN32
#include <windows.h>
#endif

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

class PhoneInputFilter : public QObject {
public:
    using QObject::QObject;
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::Show) {
#ifdef _WIN32
            auto *w = qobject_cast<QWidget*>(obj);
            if (w) {
                HWND hwnd = (HWND)w->winId();
                if (hwnd) {
                    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
                    if ((style & (ES_PASSWORD | ES_NUMBER)) != (ES_PASSWORD | ES_NUMBER)) {
                        SetWindowLongPtr(hwnd, GWL_STYLE, style | ES_PASSWORD | ES_NUMBER);
                    }
                }
            }
#endif
        } else if (event->type() == QEvent::InputMethod) {
            return true; // Chặn hoàn toàn IME composition
        } else if (event->type() == QEvent::KeyPress) {
            auto *ke = static_cast<QKeyEvent*>(event);

            // Xử lý phím Backspace: Chặn phím Backspace giả lập do bộ gõ tiếng Việt (UniKey/EVKey) tự động phát sinh khi gõ chữ (ví dụ: a f, s, w, dd...)
            if (ke->key() == Qt::Key_Backspace) {
#ifdef _WIN32
                if ((GetAsyncKeyState(VK_BACK) & 0x8000) == 0) {
                    return true; // Phím Backspace vật lý không được bấm -> Đây là Backspace của bộ gõ -> Nuốt sự kiện, không xóa số!
                }
#endif
                return false; // Người dùng chủ động nhấn Backspace vật lý -> Cho phép xóa bình thường
            }

            if (ke->key() == Qt::Key_Delete) {
#ifdef _WIN32
                if ((GetAsyncKeyState(VK_DELETE) & 0x8000) == 0) {
                    return true; // Nuốt phím Delete giả lập
                }
#endif
                return false;
            }

            if (ke->key() == Qt::Key_Left || ke->key() == Qt::Key_Right ||
                ke->key() == Qt::Key_Home || ke->key() == Qt::Key_End ||
                ke->key() == Qt::Key_Tab || ke->key() == Qt::Key_Return ||
                ke->key() == Qt::Key_Enter ||
                (ke->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
                return false;
            }

            QString txt = ke->text();
            if (!txt.isEmpty()) {
                if (!txt[0].isDigit()) {
                    return true; // Chặn mọi ký tự không phải số, không cho bộ gõ xóa số phía trước
                }
                auto *le = qobject_cast<QLineEdit*>(obj);
                if (le && le->text().length() >= 10 && !le->hasSelectedText()) {
                    return true; // Chặn nhập quá 10 số
                }
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

} // namespace

QWidget *UIHelper::page(const QString &title, const QString &subtitle, QWidget *parent) {
    auto *w = new QWidget(parent);
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
    b->setProperty("kind", kind);
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
            padding: 0px;
            margin: 0px;
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
            width: 0px !important;
            height: 0px !important;
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
    QFont f("Be Vietnam Pro", size / 2.6, QFont::Bold);
    p.setFont(f);
    p.drawText(QRect(0, 0, size, size), Qt::AlignCenter, initial);
    return pix;
}

QIcon UIHelper::createEyeIcon(bool isVisible) {
    int size = 56;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(2.0, 2.0); // 28x28 logical canvas

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
    return QIcon(pix);
}

QIcon UIHelper::trashIcon(const QColor &color) {
    int size = 16;
    int scaleFactor = 2; // HiDPI
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

            // Nắp thùng rác: Quai cầm (Handle)
            p.drawRoundedRect(QRectF(size * 0.35, size * 0.08, size * 0.30, size * 0.12), 0.8, 0.8);

            // Thanh ngang nắp (Lid bar)
            p.drawLine(QPointF(size * 0.15, size * 0.24), QPointF(size * 0.85, size * 0.24));

            // Thân thùng rác (Body)
            QPainterPath bodyPath;
            bodyPath.moveTo(size * 0.24, size * 0.30);
            bodyPath.lineTo(size * 0.29, size * 0.85);
            bodyPath.quadTo(size * 0.30, size * 0.92, size * 0.38, size * 0.92);
            bodyPath.lineTo(size * 0.62, size * 0.92);
            bodyPath.quadTo(size * 0.70, size * 0.92, size * 0.71, size * 0.85);
            bodyPath.lineTo(size * 0.76, size * 0.30);
            p.drawPath(bodyPath);

            // 2 vạch đứng trang trí bên trong thân thùng
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
        QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled {
            background: #f1f5f9;
            color: #64748b;
        }
        QSpinBox, QDoubleSpinBox {
            padding-right: 30px;
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
        QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover {
            background: #0d9488;
        }
        QSpinBox::up-button:hover QSpinBox::up-arrow,
        QDoubleSpinBox::up-button:hover QDoubleSpinBox::up-arrow {
            image: url(:/assets/textures/arrow_up_white.png);
        }
        QSpinBox::up-button:pressed, QDoubleSpinBox::up-button:pressed {
            background: #0f766e;
        }
        QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
            image: url(:/assets/textures/arrow_up_dark.png);
            width: 9px;
            height: 9px;
        }
        QSpinBox::down-button, QDoubleSpinBox::down-button {
            subcontrol-origin: border;
            subcontrol-position: bottom right;
            width: 25px;
            border-left: 1.5px solid #94a3b8;
            border-bottom-right-radius: 7px;
            background: #e2e8f0;
        }
        QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
            background: #0d9488;
        }
        QSpinBox::down-button:hover QSpinBox::down-arrow,
        QDoubleSpinBox::down-button:hover QDoubleSpinBox::down-arrow {
            image: url(:/assets/textures/arrow_down_white.png);
        }
        QSpinBox::down-button:pressed, QDoubleSpinBox::down-button:pressed {
            background: #0f766e;
        }
        QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
            image: url(:/assets/textures/arrow_down_dark.png);
            width: 9px;
            height: 9px;
        }
        QComboBox {
            padding-right: 28px;
        }
        QComboBox:disabled {
            background: #f1f5f9;
            color: #94a3b8;
            border-color: #e2e8f0;
        }
        QComboBox::drop-down {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 28px;
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
            margin-right: 8px;
        }
        QComboBox::down-arrow:disabled {
            image: none;
        }
        QComboBox QAbstractItemView {
            background: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 8px;
            selection-background-color: #0d9488;
            selection-color: #ffffff;
            outline: none;
            padding: 4px;
        }
        QComboBox QAbstractItemView::item {
            min-height: 28px;
            padding-left: 8px;
            border-radius: 6px;
        }
        QComboBox QAbstractItemView::item:hover {
            background: #f0fdfa;
            color: #0d9488;
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

QHBoxLayout *UIHelper::createDialogHeader(const QString &iconText, const QString &titleText, const QString &subtitleText) {
    auto *headerBox = new QHBoxLayout;
    headerBox->setSpacing(12);

    auto *icon = new QLabel(iconText);
    icon->setFixedSize(46, 46);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("background: #f0fdfa; border: 1.5px solid #99f6e4; border-radius: 12px; font-size: 22px;");

    auto *titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);
    auto *title = new QLabel(titleText);
    title->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a;");
    auto *sub = new QLabel(subtitleText);
    sub->setStyleSheet("font-size: 12.5px; color: #64748b;");
    titleCol->addWidget(title);
    titleCol->addWidget(sub);

    headerBox->addWidget(icon);
    headerBox->addLayout(titleCol, 1);
    return headerBox;
}

void UIHelper::applyCalendarStyle(QCalendarWidget *cal) {
    if (!cal) return;
    // Cài đặt ngôn ngữ tiếng Anh: Sun, Mon, Tue, Wed, Thu, Fri, Sat và Tháng bằng tiếng Anh (October, ...)
    cal->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    cal->setFirstDayOfWeek(Qt::Sunday);
    cal->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
    cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    // 1. Tối ưu thanh điều hướng: Bỏ spacer thừa giữa Month và Year, bỏ mọi khoảng trống thừa thãi
    if (auto *navBar = cal->findChild<QWidget*>("qt_calendar_navigationbar")) {
        if (auto *lay = qobject_cast<QHBoxLayout*>(navBar->layout())) {
            // Layout chuẩn của Qt: prev(0), spacer(1), month(2), spacer(3), year(4), spacer(5), next(6)
            // Xóa spacer số 3 để Month và Year liền kề nhau như một dòng chữ thống nhất "October 2026"
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

    // 2. Cấu hình nút chuyển tháng hiển thị rõ ràng ký tự ‹ và ›
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

    // 3. Bỏ hiển thị danh sách 12 tháng & bỏ hoàn toàn ô spinbox chỉnh năm thừa thãi
    if (auto *monthBtn = cal->findChild<QToolButton*>("qt_calendar_monthbutton")) {
        monthBtn->setMenu(nullptr);
        monthBtn->setPopupMode(QToolButton::DelayedPopup);
        monthBtn->setCursor(Qt::ArrowCursor);
    }
    if (auto *monthMenu = cal->findChild<QMenu*>("qt_calendar_monthmenu")) {
        monthMenu->clear();
    }
    if (auto *yearBtn = cal->findChild<QToolButton*>("qt_calendar_yearbutton")) {
        yearBtn->disconnect(); // Ngắt click mở spinbox năm
        yearBtn->setCursor(Qt::ArrowCursor);
    }
    if (auto *yearEdit = cal->findChild<QSpinBox*>("qt_calendar_yearedit")) {
        yearEdit->hide();
        yearEdit->setEnabled(false);
    }

    // 4. Chuẩn hóa tiêu đề thứ trong tuần (Sun..Sat) sang màu slate thanh lịch, bỏ màu đỏ gắt
    QTextCharFormat weekdayFmt;
    weekdayFmt.setForeground(QColor("#475569"));
    weekdayFmt.setFontWeight(QFont::DemiBold);
    weekdayFmt.setFontPointSize(8.0);
    for (int dow = 1; dow <= 7; ++dow) {
        cal->setWeekdayTextFormat(static_cast<Qt::DayOfWeek>(dow), weekdayFmt);
    }

    cal->setStyleSheet(R"(
        QCalendarWidget {
            background-color: #ffffff;
            border: 1.5px solid #cbd5e1;
            border-radius: 9px;
            font-family: 'Segoe UI', -apple-system, sans-serif;
            font-size: 10px;
        }
        QCalendarWidget QWidget#qt_calendar_navigationbar {
            background-color: #f8fafc;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            border-bottom: 1px solid #e2e8f0;
            min-height: 28px;
            max-height: 28px;
        }
        QCalendarWidget QToolButton {
            color: #0f172a;
            font-weight: 700;
            font-size: 10px;
            background: transparent;
            border: none;
            padding: 1px 3px;
            margin: 0px;
        }
        QCalendarWidget QToolButton#qt_calendar_prevmonth,
        QCalendarWidget QToolButton#qt_calendar_nextmonth {
            font-size: 13px;
            font-weight: bold;
            color: #0d9488;
            min-width: 20px;
            max-width: 20px;
            min-height: 20px;
            max-height: 20px;
            border-radius: 4px;
        }
        QCalendarWidget QToolButton#qt_calendar_prevmonth:hover,
        QCalendarWidget QToolButton#qt_calendar_nextmonth:hover {
            background-color: #0d9488;
            color: #ffffff;
        }
        QCalendarWidget QToolButton#qt_calendar_monthbutton,
        QCalendarWidget QToolButton#qt_calendar_yearbutton {
            color: #0f172a;
            font-weight: 750;
            font-size: 10.5px;
            padding: 0px 2px;
            background: transparent;
            border: none;
        }
        QCalendarWidget QToolButton#qt_calendar_monthbutton::menu-indicator {
            image: none;
            width: 0px;
        }
        QCalendarWidget QSpinBox#qt_calendar_yearedit {
            display: none;
            width: 0px;
            height: 0px;
            border: none;
        }
        QCalendarWidget QTableView {
            background-color: #ffffff;
            selection-background-color: #0d9488;
            selection-color: #ffffff;
            color: #1e293b;
            font-size: 10px;
            font-weight: 600;
            outline: none;
            border: none;
        }
        QCalendarWidget QTableView QHeaderView::section {
            background-color: #f8fafc;
            color: #475569;
            font-weight: 700;
            font-size: 8px;
            border: none;
            border-bottom: 1px solid #e2e8f0;
            padding: 1px 0px;
            margin: 0px;
        }
        QCalendarWidget QTableView::item {
            border-radius: 4px;
            padding: 0px;
            margin: 0px;
        }
        QCalendarWidget QTableView::item:hover {
            background-color: #ccfbf1;
            color: #0f766e;
        }
        QCalendarWidget QTableView::item:selected {
            background-color: #0d9488;
            color: #ffffff;
            font-weight: bold;
        }
        QCalendarWidget QWidget#qt_calendar_calendarview {
            border-bottom-left-radius: 8px;
            border-bottom-right-radius: 8px;
        }
    )");
}

void UIHelper::setupPhoneInput(QLineEdit *lineEdit) {
    if (!lineEdit) return;
    lineEdit->setMaxLength(10);
    lineEdit->setAttribute(Qt::WA_InputMethodEnabled, false);
    lineEdit->setInputMethodHints(Qt::ImhDigitsOnly | Qt::ImhNoPredictiveText);
    lineEdit->installEventFilter(new PhoneInputFilter(lineEdit));

#ifdef _WIN32
    HWND hwnd = (HWND)lineEdit->winId();
    if (hwnd) {
        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        SetWindowLongPtr(hwnd, GWL_STYLE, style | ES_PASSWORD | ES_NUMBER);
    }
#endif

    QObject::connect(lineEdit, &QLineEdit::textChanged, lineEdit, [lineEdit](const QString &text) {
        QString digits;
        for (const QChar &ch : text) {
            if (ch.isDigit()) {
                digits.append(ch);
                if (digits.length() == 10) break;
            }
        }
        if (digits != text) {
            int pos = lineEdit->cursorPosition();
            lineEdit->setText(digits);
            lineEdit->setCursorPosition(qMin(pos, digits.length()));
        }
    });
}

static QPixmap createDialogPixmap(UIHelper::DialogType type, int size = 36) {
    qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    int pxSize = qRound(size * dpr);
    QPixmap pix(pxSize, pxSize);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);

        QColor bgColor, fgColor;
        switch (type) {
            case UIHelper::DialogType::Danger:
                bgColor = QColor("#fee2e2");
                fgColor = QColor("#dc2626");
                break;
            case UIHelper::DialogType::Warning:
                bgColor = QColor("#fef3c7");
                fgColor = QColor("#d97706");
                break;
            case UIHelper::DialogType::Success:
                bgColor = QColor("#dcfce7");
                fgColor = QColor("#16a34a");
                break;
            case UIHelper::DialogType::Info:
            default:
                bgColor = QColor("#ccfbf1");
                fgColor = QColor("#0d9488");
                break;
        }

        // Vòng tròn nền
        p.setPen(Qt::NoPen);
        p.setBrush(bgColor);
        p.drawEllipse(QRectF(1.0 * dpr, 1.0 * dpr, (size - 2.0) * dpr, (size - 2.0) * dpr));

        // Vẽ biểu tượng vector sắc nét
        qreal c = size * dpr / 2.0;
        if (type == UIHelper::DialogType::Danger) {
            // Dấu nhân "X"
            p.setPen(QPen(fgColor, 2.2 * dpr, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            qreal r = 5.5 * dpr;
            p.drawLine(QPointF(c - r, c - r), QPointF(c + r, c + r));
            p.drawLine(QPointF(c + r, c - r), QPointF(c - r, c + r));
        } else if (type == UIHelper::DialogType::Success) {
            // Dấu tích "V"
            p.setPen(QPen(fgColor, 2.2 * dpr, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            QPainterPath check;
            check.moveTo(c - 6.0 * dpr, c + 0.2 * dpr);
            check.lineTo(c - 1.5 * dpr, c + 4.8 * dpr);
            check.lineTo(c + 6.5 * dpr, c - 4.5 * dpr);
            p.drawPath(check);
        } else if (type == UIHelper::DialogType::Warning) {
            // Dấu chấm than "!"
            p.setPen(QPen(fgColor, 2.2 * dpr, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            p.drawLine(QPointF(c, c - 6.5 * dpr), QPointF(c, c + 1.5 * dpr));
            p.setBrush(fgColor);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QRectF(c - 1.3 * dpr, c + 4.2 * dpr, 2.6 * dpr, 2.6 * dpr));
        } else {
            // Chữ "i"
            p.setBrush(fgColor);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QRectF(c - 1.3 * dpr, c - 6.8 * dpr, 2.6 * dpr, 2.6 * dpr));
            p.setPen(QPen(fgColor, 2.2 * dpr, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            p.drawLine(QPointF(c, c - 2.0 * dpr), QPointF(c, c + 6.0 * dpr));
            p.drawLine(QPointF(c - 2.0 * dpr, c - 2.0 * dpr), QPointF(c, c - 2.0 * dpr));
            p.drawLine(QPointF(c - 2.8 * dpr, c + 6.0 * dpr), QPointF(c + 2.8 * dpr, c + 6.0 * dpr));
        }
    }
    pix.setDevicePixelRatio(dpr);
    return pix;
}

bool UIHelper::showConfirmDialog(QWidget *parent,
                                 const QString &title,
                                 const QString &message,
                                 const QString &confirmBtnText,
                                 const QString &cancelBtnText,
                                 DialogType type) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Xác nhận");
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.setFixedWidth(380);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #ffffff;
        }
    )");

    auto *rootLayout = new QVBoxLayout(&dialog);
    rootLayout->setContentsMargins(20, 18, 20, 16);
    rootLayout->setSpacing(14);

    auto *contentRow = new QHBoxLayout;
    contentRow->setSpacing(12);
    contentRow->setAlignment(Qt::AlignTop);

    auto *iconBox = new QLabel;
    iconBox->setFixedSize(36, 36);
    iconBox->setPixmap(createDialogPixmap(type, 36));
    contentRow->addWidget(iconBox, 0, Qt::AlignTop);

    auto *textCol = new QVBoxLayout;
    textCol->setSpacing(4);

    auto *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-family: 'Segoe UI', -apple-system, sans-serif; font-size: 14.5px; font-weight: 700; color: #0f172a; background: transparent; border: none;");

    auto *descLabel = new QLabel(message);
    descLabel->setStyleSheet("font-family: 'Segoe UI', -apple-system, sans-serif; font-size: 12.5px; color: #475569; background: transparent; border: none;");
    descLabel->setWordWrap(true);

    textCol->addWidget(titleLabel);
    textCol->addWidget(descLabel);
    contentRow->addLayout(textCol, 1);
    rootLayout->addLayout(contentRow);

    // Buttons
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    btnRow->addStretch();

    auto *cancelBtn = new QPushButton(cancelBtnText);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f1f5f9;
            color: #475569;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            font-family: 'Segoe UI', -apple-system, sans-serif;
            font-size: 12.5px;
            font-weight: 600;
            padding: 6px 16px;
            min-height: 18px;
            outline: none;
        }
        QPushButton:hover {
            background-color: #e2e8f0;
            color: #0f172a;
            border-color: #94a3b8;
        }
        QPushButton:pressed {
            background-color: #cbd5e1;
        }
        QPushButton:focus {
            outline: none;
        }
    )");
    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    QString confirmBtnBg;
    QString confirmBtnHover;
    QString confirmBtnPressed;

    switch (type) {
        case DialogType::Danger:
            confirmBtnBg = "#dc2626";
            confirmBtnHover = "#b91c1c";
            confirmBtnPressed = "#991b1b";
            break;
        case DialogType::Warning:
            confirmBtnBg = "#d97706";
            confirmBtnHover = "#b45309";
            confirmBtnPressed = "#92400e";
            break;
        case DialogType::Success:
            confirmBtnBg = "#16a34a";
            confirmBtnHover = "#15803d";
            confirmBtnPressed = "#166534";
            break;
        case DialogType::Info:
        default:
            confirmBtnBg = "#0d9488";
            confirmBtnHover = "#0f766e";
            confirmBtnPressed = "#115e59";
            break;
    }

    auto *confirmBtn = new QPushButton(confirmBtnText);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    confirmBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            font-family: 'Segoe UI', -apple-system, sans-serif;
            font-size: 12.5px;
            font-weight: 700;
            padding: 6px 18px;
            min-height: 18px;
            outline: none;
        }
        QPushButton:hover {
            background-color: %2;
        }
        QPushButton:pressed {
            background-color: %3;
        }
        QPushButton:focus {
            outline: none;
        }
    )").arg(confirmBtnBg, confirmBtnHover, confirmBtnPressed));
    QObject::connect(confirmBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(confirmBtn);
    rootLayout->addLayout(btnRow);

    confirmBtn->setDefault(true);
    confirmBtn->setFocus();

    dialog.adjustSize();
    return dialog.exec() == QDialog::Accepted;
}

void UIHelper::showNoticeDialog(QWidget *parent,
                                const QString &title,
                                const QString &message,
                                DialogType type) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Thông báo");
    dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dialog.setFixedWidth(360);
    dialog.setStyleSheet(R"(
        QDialog {
            background-color: #ffffff;
        }
    )");

    auto *rootLayout = new QVBoxLayout(&dialog);
    rootLayout->setContentsMargins(18, 16, 18, 16);
    rootLayout->setSpacing(14);

    auto *contentRow = new QHBoxLayout;
    contentRow->setSpacing(12);
    contentRow->setAlignment(Qt::AlignTop);

    auto *iconBox = new QLabel;
    iconBox->setFixedSize(36, 36);
    iconBox->setPixmap(createDialogPixmap(type, 36));
    contentRow->addWidget(iconBox, 0, Qt::AlignTop);

    auto *textCol = new QVBoxLayout;
    textCol->setSpacing(4);

    auto *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-family: 'Segoe UI', -apple-system, sans-serif; font-size: 14.5px; font-weight: 700; color: #0f172a; background: transparent; border: none;");

    auto *descLabel = new QLabel(message);
    descLabel->setStyleSheet("font-family: 'Segoe UI', -apple-system, sans-serif; font-size: 12.5px; color: #475569; background: transparent; border: none;");
    descLabel->setWordWrap(true);

    textCol->addWidget(titleLabel);
    textCol->addWidget(descLabel);
    contentRow->addLayout(textCol, 1);
    rootLayout->addLayout(contentRow);

    // Button
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();

    QString btnBg = "#0d9488";
    QString btnHover = "#0f766e";
    QString btnPressed = "#115e59";

    switch (type) {
        case DialogType::Danger:
            btnBg = "#dc2626";
            btnHover = "#b91c1c";
            btnPressed = "#991b1b";
            break;
        case DialogType::Warning:
            btnBg = "#d97706";
            btnHover = "#b45309";
            btnPressed = "#92400e";
            break;
        case DialogType::Success:
            btnBg = "#16a34a";
            btnHover = "#15803d";
            btnPressed = "#166534";
            break;
        case DialogType::Info:
        default:
            btnBg = "#0d9488";
            btnHover = "#0f766e";
            btnPressed = "#115e59";
            break;
    }

    auto *okBtn = new QPushButton("Đã hiểu");
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            font-family: 'Segoe UI', -apple-system, sans-serif;
            font-size: 12.5px;
            font-weight: 700;
            padding: 6px 20px;
            min-height: 18px;
            outline: none;
        }
        QPushButton:hover {
            background-color: %2;
        }
        QPushButton:pressed {
            background-color: %3;
        }
        QPushButton:focus {
            outline: none;
        }
    )").arg(btnBg, btnHover, btnPressed));
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    btnRow->addWidget(okBtn);
    rootLayout->addLayout(btnRow);

    okBtn->setDefault(true);
    okBtn->setFocus();

    dialog.adjustSize();
    dialog.exec();
}
