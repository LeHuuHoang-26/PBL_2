#include "StatisticView.h"
#include "MainWindow.h"
#include "StatisticController.h"
#include "UIHelper.h"
#include "AppConstants.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QScrollBar>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QLineEdit>
#include <QIntValidator>
#include <QDialog>
#include <QTimer>
#include <QCalendarWidget>
#include <QPainter>
#include <QPainterPath>
#include <QLocale>
#include <QTime>
#include <QMouseEvent>
#include <QShowEvent>
#include <QSignalBlocker>
#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>
#include <utility>

namespace {

void computeLargestRemainderPercentages(const int *counts, int n, int *outPct) {
    int total = 0;
    for (int i = 0; i < n; ++i) {
        outPct[i] = 0;
        if (counts[i] > 0) total += counts[i];
    }
    if (total <= 0) return;

    double remainders[8] = {0.0};
    int sumPct = 0;
    for (int i = 0; i < n; ++i) {
        double exact = (counts[i] > 0) ? (counts[i] * 100.0 / total) : 0.0;
        int floored = static_cast<int>(std::floor(exact));
        outPct[i] = floored;
        remainders[i] = exact - floored;
        sumPct += floored;
    }

    int rem = 100 - sumPct;
    while (rem > 0) {
        int bestIdx = -1;
        double bestRem = -1.0;
        for (int i = 0; i < n; ++i) {
            if (counts[i] > 0 && remainders[i] > bestRem) {
                bestRem = remainders[i];
                bestIdx = i;
            }
        }
        if (bestIdx < 0) break;
        outPct[bestIdx]++;
        remainders[bestIdx] = -1.0;
        rem--;
    }
}

class LinearStatusBar : public QWidget {
public:
    int completed = 0;
    int pending = 0;
    int delayed = 0;
    int rescheduled = 0;

    explicit LinearStatusBar(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedHeight(18);
    }

    void setCounts(int c, int p, int d, int r) {
        completed = c;
        pending = p;
        delayed = d;
        rescheduled = r;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int total = completed + pending + delayed + rescheduled;
        if (total <= 0) {
            painter.setBrush(QColor("#e2e8f0"));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(rect(), 9, 9);
            return;
        }

        QPainterPath clipPath;
        clipPath.addRoundedRect(QRectF(rect()), 9, 9);
        painter.setClipPath(clipPath);

        const int counts[4] = {completed, pending, delayed, rescheduled};
        const QColor colors[4] = {
            QColor("#0d9488"), // Đã khám
            QColor("#f59e0b"), // Chưa khám
            QColor("#ea580c"), // Trễ hẹn
            QColor("#64748b")  // Đã dời lịch
        };

        int lastNonZero = -1;
        for (int i = 0; i < 4; ++i) {
            if (counts[i] > 0) lastNonZero = i;
        }

        double w = width();
        double x = 0.0;
        for (int i = 0; i < 4; ++i) {
            if (counts[i] <= 0) continue;
            double segW = (i == lastNonZero)
                ? (w - x)
                : (static_cast<double>(counts[i]) / total) * w;
            painter.fillRect(QRectF(x, 0, segW, height()), colors[i]);
            x += segW;
        }
    }
};

class PatientDonutChart : public QWidget {
public:
    int c1 = 0, c2 = 0, c3 = 0, c4 = 0;

    explicit PatientDonutChart(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(100, 100);
    }

    void setData(int under18, int from18to45, int from46to60, int over60) {
        c1 = under18;
        c2 = from18to45;
        c3 = from46to60;
        c4 = over60;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int total = c1 + c2 + c3 + c4;
        QRectF outerRect(5, 5, width() - 10, height() - 10);
        QRectF innerRect(21, 21, width() - 42, height() - 42);

        if (total <= 0) {
            painter.setBrush(QColor("#e2e8f0"));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(outerRect);
            painter.setBrush(QColor("#ffffff"));
            painter.drawEllipse(innerRect);

            painter.setFont(QFont("Segoe UI", 16, QFont::Bold));
            painter.setPen(QColor("#94a3b8"));
            painter.drawText(innerRect, Qt::AlignCenter, "0");
            return;
        }

        const int counts[4] = {c1, c2, c3, c4};
        const QColor colors[4] = {
            QColor("#38bdf8"),
            QColor("#0d9488"),
            QColor("#f59e0b"),
            QColor("#8b5cf6")
        };

        int lastNonZero = -1;
        for (int i = 0; i < 4; ++i) {
            if (counts[i] > 0) lastNonZero = i;
        }

        const int fullCircle = 360 * 16;
        int startAngle = 90 * 16;
        int usedSpan = 0;
        for (int i = 0; i < 4; ++i) {
            if (counts[i] <= 0) continue;
            int spanMag = (i == lastNonZero)
                ? (fullCircle - usedSpan)
                : qRound((static_cast<double>(counts[i]) / total) * fullCircle);
            int spanAngle = -spanMag;
            painter.setBrush(colors[i]);
            painter.setPen(Qt::NoPen);
            painter.drawPie(outerRect, startAngle, spanAngle);
            startAngle += spanAngle;
            usedSpan += spanMag;
        }

        painter.setBrush(QColor("#ffffff"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(innerRect);

        painter.setFont(QFont("Segoe UI", 16, QFont::Bold));
        painter.setPen(QColor("#0f172a"));
        painter.drawText(innerRect, Qt::AlignCenter, QString::number(total));
    }
};

struct CoordPoint {
    QString label;
    double value = 0.0;
};

class RevenueLineChart : public QWidget {
public:
    QVector<CoordPoint> points;
    int hoveredIndex = -1;

    explicit RevenueLineChart(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(240);
        setMouseTracking(true);
    }

    void setData(const QVector<CoordPoint> &pts) {
        points = pts;
        hoveredIndex = -1;
        update();
    }

protected:
    void mouseMoveEvent(QMouseEvent *event) override {
        if (points.isEmpty()) return;
        int n = points.size();
        const int left = 75, right = 30, top = 25, bottom = 40;
        int plotW = width() - left - right;
        int plotH = height() - top - bottom;
        if (plotW <= 0 || plotH <= 0) return;

        double mouseX = event->position().x();
        double mouseY = event->position().y();
        if (mouseX < left - 15 || mouseX > left + plotW + 15 ||
            mouseY < top - 8 || mouseY > top + plotH + 8) {
            if (hoveredIndex != -1) {
                hoveredIndex = -1;
                update();
            }
            return;
        }

        double stepX = (n > 1) ? static_cast<double>(plotW) / (n - 1) : 0.0;
        int closest = -1;
        if (stepX > 0.0) {
            closest = qBound(0, qRound((mouseX - left) / stepX), n - 1);
        } else if (n == 1) {
            closest = 0;
        }

        if (closest != hoveredIndex) {
            hoveredIndex = closest;
            update();
        }
    }

    void leaveEvent(QEvent *) override {
        if (hoveredIndex != -1) {
            hoveredIndex = -1;
            update();
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int w = width();
        int h = height();
        int left = 75, right = 30, top = 25, bottom = 40;
        int plotW = w - left - right;
        int plotH = h - top - bottom;

        if (points.isEmpty() || plotW <= 0 || plotH <= 0) {
            painter.setFont(QFont("Segoe UI", 11));
            painter.setPen(QColor("#94a3b8"));
            painter.drawText(rect(), Qt::AlignCenter, "Không có dữ liệu trong khoảng thời gian này.");
            return;
        }

        double maxVal = 100000.0;
        for (const auto &p : points) {
            if (p.value > maxVal) maxVal = p.value;
        }
        maxVal = std::ceil(maxVal / 100000.0) * 100000.0;

        painter.setFont(QFont("Segoe UI", 8.5));
        for (int i = 0; i <= 4; ++i) {
            double val = (maxVal / 4.0) * i;
            int y = top + plotH - static_cast<int>((val / maxVal) * plotH);

            painter.setPen(QPen(QColor("#f1f5f9"), 1, Qt::DashLine));
            painter.drawLine(left, y, left + plotW, y);

            painter.setPen(QColor("#64748b"));
            QString yStr;
            if (val >= 1000000000.0) {
                yStr = QString::number(val / 1000000000.0, 'f', 2) + " Tỷ";
            } else if (val >= 1000000.0) {
                yStr = QString::number(val / 1000000.0, 'f', 1) + " Tr";
            } else if (val >= 1000.0) {
                yStr = QString::number(val / 1000.0, 'f', 0) + " k";
            } else {
                yStr = QString::number(val, 'f', 0);
            }
            painter.drawText(QRect(0, y - 8, left - 8, 16), Qt::AlignRight | Qt::AlignVCenter, yStr);
        }

        painter.setPen(QPen(QColor("#cbd5e1"), 1.5));
        painter.drawLine(left, top + plotH, left + plotW, top + plotH);
        painter.drawLine(left, top, left, top + plotH);

        int n = points.size();
        double stepX = (n > 1) ? static_cast<double>(plotW) / (n - 1) : 0.0;

        QVector<QPointF> screenPts;
        for (int i = 0; i < n; ++i) {
            double px = (n > 1) ? left + i * stepX : left + plotW / 2.0;
            double py = top + plotH - (points[i].value / maxVal) * plotH;
            screenPts.append(QPointF(px, py));
        }

        if (n > 1) {
            QPainterPath areaPath;
            areaPath.moveTo(screenPts[0].x(), top + plotH);
            for (const auto &pt : screenPts) {
                areaPath.lineTo(pt);
            }
            areaPath.lineTo(screenPts.last().x(), top + plotH);
            areaPath.closeSubpath();

            QLinearGradient grad(0, top, 0, top + plotH);
            grad.setColorAt(0.0, QColor(13, 148, 136, 70));
            grad.setColorAt(1.0, QColor(13, 148, 136, 0));
            painter.setBrush(grad);
            painter.setPen(Qt::NoPen);
            painter.drawPath(areaPath);
        }

        painter.setPen(QPen(QColor("#0d9488"), (n > 100) ? 1.8 : 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        for (int i = 0; i < n - 1; ++i) {
            painter.drawLine(screenPts[i], screenPts[i + 1]);
        }

        int maxLabels = qMax(2, plotW / 58);
        int labelStep = qMax(1, (n + maxLabels - 1) / maxLabels);

        for (int i = 0; i < n; ++i) {
            bool isLast = (i == n - 1);
            bool isStep = (i % labelStep == 0);
            if (isStep || (isLast && (n - 1) % labelStep >= qMax(1, labelStep / 2))) {
                painter.setFont(QFont("Segoe UI", 8.5));
                painter.setPen(QColor("#64748b"));
                painter.drawText(QRect(static_cast<int>(screenPts[i].x()) - 28, top + plotH + 8, 56, 18), Qt::AlignCenter, points[i].label);
            }

            if (n <= 35) {
                painter.setPen(QPen(QColor("#0d9488"), 2));
                painter.setBrush(QColor("#ffffff"));
                painter.drawEllipse(screenPts[i], 4.0, 4.0);
            } else if (n <= 100) {
                painter.setPen(QPen(QColor("#0d9488"), 1.5));
                painter.setBrush(QColor("#ffffff"));
                painter.drawEllipse(screenPts[i], 2.2, 2.2);
            } else if (isStep || isLast) {
                painter.setPen(QPen(QColor("#0d9488"), 1.5));
                painter.setBrush(QColor("#ffffff"));
                painter.drawEllipse(screenPts[i], 2.0, 2.0);
            }
        }

        if (hoveredIndex >= 0 && hoveredIndex < n) {
            const auto &pt = screenPts[hoveredIndex];
            painter.setPen(QPen(QColor("#0f766e"), 3));
            painter.setBrush(QColor("#ffffff"));
            painter.drawEllipse(pt, 6.0, 6.0);

            QString tipText = QStringLiteral("%1: %2 đ").arg(points[hoveredIndex].label, QLocale(QLocale::Vietnamese).toString(points[hoveredIndex].value, 'f', 0));
            painter.setFont(QFont("Segoe UI", 8.5, QFont::Bold));
            QFontMetrics fm(painter.font());
            int tipW = fm.horizontalAdvance(tipText) + 16;
            int tipH = 24;
            int tipX = static_cast<int>(pt.x()) - tipW / 2;
            int tipY = static_cast<int>(pt.y()) - tipH - 10;
            if (tipY < top) {
                tipY = qMin(top + plotH - tipH - 2, static_cast<int>(pt.y()) + 12);
            }
            if (tipX < left) tipX = left;
            if (tipX + tipW > w - right) tipX = w - right - tipW;

            painter.setBrush(QColor(15, 23, 42, 235));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(QRect(tipX, tipY, tipW, tipH), 6, 6);

            painter.setPen(QColor("#ffffff"));
            painter.drawText(QRect(tipX, tipY, tipW, tipH), Qt::AlignCenter, tipText);
        }
    }
};

struct StatusPoint {
    QString label;
    int completed = 0;
    int delayed = 0;
    int rescheduled = 0;
};

class StatusMultiLineChart : public QWidget {
public:
    QVector<StatusPoint> points;
    int hoveredIndex = -1;

    explicit StatusMultiLineChart(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(240);
        setMouseTracking(true);
    }

    void setData(const QVector<StatusPoint> &pts) {
        points = pts;
        hoveredIndex = -1;
        update();
    }

protected:
    void mouseMoveEvent(QMouseEvent *event) override {
        if (points.isEmpty()) return;
        int n = points.size();
        const int left = 55, right = 30, top = 35, bottom = 40;
        int plotW = width() - left - right;
        int plotH = height() - top - bottom;
        if (plotW <= 0 || plotH <= 0) return;

        double mouseX = event->position().x();
        double mouseY = event->position().y();
        if (mouseX < left - 15 || mouseX > left + plotW + 15 ||
            mouseY < top || mouseY > top + plotH + 8) {
            if (hoveredIndex != -1) {
                hoveredIndex = -1;
                update();
            }
            return;
        }

        double stepX = (n > 1) ? static_cast<double>(plotW) / (n - 1) : 0.0;
        int closest = -1;
        if (stepX > 0.0) {
            closest = qBound(0, qRound((mouseX - left) / stepX), n - 1);
        } else if (n == 1) {
            closest = 0;
        }

        if (closest != hoveredIndex) {
            hoveredIndex = closest;
            update();
        }
    }

    void leaveEvent(QEvent *) override {
        if (hoveredIndex != -1) {
            hoveredIndex = -1;
            update();
        }
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int w = width();
        int h = height();
        int left = 55, right = 30, top = 35, bottom = 40;
        int plotW = w - left - right;
        int plotH = h - top - bottom;

        if (points.isEmpty() || plotW <= 0 || plotH <= 0) {
            painter.setFont(QFont("Segoe UI", 11));
            painter.setPen(QColor("#94a3b8"));
            painter.drawText(rect(), Qt::AlignCenter, "Không có dữ liệu trong khoảng thời gian này.");
            return;
        }

        int maxVal = 4;
        for (const auto &p : points) {
            if (p.completed > maxVal) maxVal = p.completed;
            if (p.delayed > maxVal) maxVal = p.delayed;
            if (p.rescheduled > maxVal) maxVal = p.rescheduled;
        }
        maxVal = ((maxVal + 3) / 4) * 4;

        painter.setFont(QFont("Segoe UI", 8.5));
        for (int i = 0; i <= 4; ++i) {
            int val = (maxVal * i) / 4;
            int y = top + plotH - static_cast<int>((static_cast<double>(val) / maxVal) * plotH);

            painter.setPen(QPen(QColor("#f1f5f9"), 1, Qt::DashLine));
            painter.drawLine(left, y, left + plotW, y);

            painter.setPen(QColor("#64748b"));
            painter.drawText(QRect(0, y - 8, left - 8, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(val));
        }

        painter.setPen(QPen(QColor("#cbd5e1"), 1.5));
        painter.drawLine(left, top + plotH, left + plotW, top + plotH);
        painter.drawLine(left, top, left, top + plotH);

        int legY = 10;
        int legRight = w - right;

        auto drawLegendItem = [&](const QString &txt, const QColor &col, int &curRight) {
            QFont font("Segoe UI", 8.5, QFont::Bold);
            QFontMetrics fm(font);
            int txtW = fm.horizontalAdvance(txt);
            int itemW = txtW + 20;
            curRight -= itemW;

            painter.setPen(Qt::NoPen);
            painter.setBrush(col);
            painter.drawEllipse(curRight, legY + 2, 8, 8);

            painter.setFont(font);
            painter.setPen(QColor("#334155"));
            painter.drawText(curRight + 12, legY + 10, txt);
            curRight -= 12;
        };

        int curLegRight = legRight;
        drawLegendItem("Đã dời lịch", QColor("#64748b"), curLegRight);
        drawLegendItem("Trễ hẹn", QColor("#ea580c"), curLegRight);
        drawLegendItem("Đã khám", QColor("#0d9488"), curLegRight);

        int n = points.size();
        double stepX = (n > 1) ? static_cast<double>(plotW) / (n - 1) : 0.0;

        int maxLabels = qMax(2, plotW / 58);
        int labelStep = qMax(1, (n + maxLabels - 1) / maxLabels);

        QVector<QPointF> ptsComp, ptsDelay, ptsResched;
        for (int i = 0; i < n; ++i) {
            double px = (n > 1) ? left + i * stepX : left + plotW / 2.0;
            double pyC = top + plotH - (static_cast<double>(points[i].completed) / maxVal) * plotH;
            double pyD = top + plotH - (static_cast<double>(points[i].delayed) / maxVal) * plotH;
            double pyR = top + plotH - (static_cast<double>(points[i].rescheduled) / maxVal) * plotH;

            ptsComp.append(QPointF(px, pyC));
            ptsDelay.append(QPointF(px, pyD));
            ptsResched.append(QPointF(px, pyR));
        }

        auto drawSeriesLine = [&](const QVector<QPointF> &pts, const QColor &col) {
            painter.setPen(QPen(col, (n > 100) ? 1.8 : 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            for (int i = 0; i < pts.size() - 1; ++i) {
                painter.drawLine(pts[i], pts[i + 1]);
            }
            if (n <= 35) {
                for (const auto &p : pts) {
                    painter.setPen(QPen(col, 1.8));
                    painter.setBrush(QColor("#ffffff"));
                    painter.drawEllipse(p, 3.5, 3.5);
                }
            } else if (n <= 100) {
                for (const auto &p : pts) {
                    painter.setPen(QPen(col, 1.2));
                    painter.setBrush(QColor("#ffffff"));
                    painter.drawEllipse(p, 2.0, 2.0);
                }
            } else {
                for (int i = 0; i < pts.size(); ++i) {
                    if (i % labelStep == 0 || i == pts.size() - 1) {
                        painter.setPen(QPen(col, 1.2));
                        painter.setBrush(QColor("#ffffff"));
                        painter.drawEllipse(pts[i], 1.8, 1.8);
                    }
                }
            }
        };

        drawSeriesLine(ptsResched, QColor("#64748b"));
        drawSeriesLine(ptsDelay, QColor("#ea580c"));
        drawSeriesLine(ptsComp, QColor("#0d9488"));

        for (int i = 0; i < n; ++i) {
            bool isLast = (i == n - 1);
            bool isStep = (i % labelStep == 0);
            if (isStep || (isLast && (n - 1) % labelStep >= qMax(1, labelStep / 2))) {
                painter.setFont(QFont("Segoe UI", 8.5));
                painter.setPen(QColor("#64748b"));
                painter.drawText(QRect(static_cast<int>(ptsComp[i].x()) - 28, top + plotH + 8, 56, 18), Qt::AlignCenter, points[i].label);
            }
        }

        if (hoveredIndex >= 0 && hoveredIndex < n) {
            const auto &p = points[hoveredIndex];
            double px = ptsComp[hoveredIndex].x();

            painter.setPen(QPen(QColor("#cbd5e1"), 1, Qt::DashLine));
            painter.drawLine(QPointF(px, top), QPointF(px, top + plotH));

            painter.setPen(QPen(QColor("#64748b"), 2));
            painter.setBrush(QColor("#ffffff"));
            painter.drawEllipse(ptsResched[hoveredIndex], 4.5, 4.5);

            painter.setPen(QPen(QColor("#ea580c"), 2));
            painter.setBrush(QColor("#ffffff"));
            painter.drawEllipse(ptsDelay[hoveredIndex], 4.5, 4.5);

            painter.setPen(QPen(QColor("#0d9488"), 2));
            painter.setBrush(QColor("#ffffff"));
            painter.drawEllipse(ptsComp[hoveredIndex], 4.5, 4.5);

            QString tipText = QStringLiteral("%1: %2 khám · %3 trễ · %4 dời").arg(p.label).arg(p.completed).arg(p.delayed).arg(p.rescheduled);
            painter.setFont(QFont("Segoe UI", 8.5, QFont::Bold));
            QFontMetrics fm(painter.font());
            int tipW = fm.horizontalAdvance(tipText) + 16;
            int tipH = 24;
            int tipX = static_cast<int>(px) - tipW / 2;
            int tipY = top + 4;
            if (tipX < left) tipX = left;
            if (tipX + tipW > w - right) tipX = w - right - tipW;

            painter.setBrush(QColor(15, 23, 42, 230));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(QRect(tipX, tipY, tipW, tipH), 6, 6);

            painter.setPen(QColor("#ffffff"));
            painter.drawText(QRect(tipX, tipY, tipW, tipH), Qt::AlignCenter, tipText);
        }
    }
};

class PeriodSelectionDialog : public QDialog {
public:
    enum PeriodType { Week, Month, Quarter, Year };

    PeriodType pType;
    int selectedYear;
    int selectedMonth;
    int selectedQuarter;
    int selectedWeekIndex = 0;
    QVector<QPair<QDate, QDate>> weeksList;

    int originYear = 0;
    int originWeekIndex = -1;
    int originMonth = -1;
    int originQuarter = -1;

    QDate resultStart;
    QDate resultEnd;

    QLineEdit *txtYear = nullptr;
    QPushButton *btnConfirm = nullptr;
    QVBoxLayout *contentBox = nullptr;

    bool eventFilter(QObject *obj, QEvent *ev) override {
        if (obj == txtYear) {
            if (ev->type() == QEvent::FocusIn) {
                QTimer::singleShot(0, txtYear, &QLineEdit::selectAll);
            } else if (ev->type() == QEvent::FocusOut) {
                commitYearInput();
            }
        }
        return QDialog::eventFilter(obj, ev);
    }

    void commitYearInput() {
        if (!txtYear) return;
        bool ok = false;
        int y = txtYear->text().trimmed().toInt(&ok);
        if (ok && y >= AppConstants::MIN_CALENDAR_YEAR && y <= AppConstants::MAX_CALENDAR_YEAR) {
            if (y != selectedYear) {
                changeYear(y);
            } else {
                QSignalBlocker blocker(txtYear);
                txtYear->setText(QString::number(selectedYear));
            }
        } else {
            QSignalBlocker blocker(txtYear);
            txtYear->setText(QString::number(selectedYear));
        }
    }

    PeriodSelectionDialog(PeriodType type, const QDate &initDate, QWidget *parent = nullptr)
        : QDialog(parent), pType(type) {
        setWindowTitle("Chọn thời gian thống kê");
        setModal(true);
        setStyleSheet("QDialog { background-color: #ffffff; border-radius: 14px; font-family: 'Segoe UI'; }");

        if (pType == Week) {
            setFixedSize(480, 480);
        } else if (pType == Month) {
            setFixedSize(420, 275);
        } else if (pType == Quarter) {
            setFixedSize(370, 245);
        } else {
            setFixedSize(340, 215);
        }

        QDate safeDate = initDate.isValid() ? initDate : QDate::currentDate();
        if (pType == Week) {
            // Use the Thursday of the week so cross-year Week 1 stays anchored in its target year.
            QDate anchorThursday = safeDate.addDays(4 - safeDate.dayOfWeek());
            selectedYear = qBound(AppConstants::MIN_CALENDAR_YEAR, anchorThursday.year(), AppConstants::MAX_CALENDAR_YEAR);
        } else {
            selectedYear = qBound(AppConstants::MIN_CALENDAR_YEAR, safeDate.year(), AppConstants::MAX_CALENDAR_YEAR);
        }
        originYear = selectedYear;
        selectedMonth = safeDate.month();
        selectedQuarter = (selectedMonth - 1) / 3 + 1;

        if (pType == Week) {
            calculateWeeks();
            selectedWeekIndex = 0;
            for (int i = 0; i < weeksList.size(); ++i) {
                if (safeDate >= weeksList[i].first && safeDate <= weeksList[i].second) {
                    selectedWeekIndex = i;
                    break;
                }
            }
            originWeekIndex = selectedWeekIndex;
        } else if (pType == Month) {
            originMonth = selectedMonth;
        } else if (pType == Quarter) {
            originQuarter = selectedQuarter;
        }

        setupUI();
    }

    void updateConfirmButtonState() {
        if (!btnConfirm) return;
        bool canConfirm = true;
        if (pType == Week && (selectedWeekIndex < 0 || selectedWeekIndex >= weeksList.size())) {
            canConfirm = false;
        } else if (pType == Month && (selectedMonth < 1 || selectedMonth > 12)) {
            canConfirm = false;
        } else if (pType == Quarter && (selectedQuarter < 1 || selectedQuarter > 4)) {
            canConfirm = false;
        }

        btnConfirm->setEnabled(canConfirm);
        if (canConfirm) {
            btnConfirm->setStyleSheet(
                "QPushButton { "
                "    background: #0d9488; color: #ffffff; font-family: 'Segoe UI'; font-weight: 700; font-size: 12.5px; "
                "    border: 1px solid #0d9488; border-radius: 8px; padding: 0 22px; outline: none; "
                "} "
                "QPushButton:hover { background: #0f766e; border: 1px solid #0f766e; border-radius: 8px; }"
            );
        } else {
            btnConfirm->setStyleSheet(
                "QPushButton { "
                "    background: #e2e8f0; color: #94a3b8; font-family: 'Segoe UI'; font-weight: 700; font-size: 12.5px; "
                "    border: 1px solid #cbd5e1; border-radius: 8px; padding: 0 22px; outline: none; "
                "}"
            );
        }
    }

    void changeYear(int newYear) {
        if (newYear < AppConstants::MIN_CALENDAR_YEAR || newYear > AppConstants::MAX_CALENDAR_YEAR || newYear == selectedYear) return;
        selectedYear = newYear;
        if (txtYear) {
            QSignalBlocker blocker(txtYear);
            txtYear->setText(QString::number(selectedYear));
        }

        if (pType == Week) {
            calculateWeeks();
            if (selectedYear == originYear) {
                selectedWeekIndex = originWeekIndex;
            } else {
                selectedWeekIndex = -1;
            }
        } else if (pType == Month) {
            if (selectedYear == originYear) {
                selectedMonth = originMonth;
            } else {
                selectedMonth = -1;
            }
        } else if (pType == Quarter) {
            if (selectedYear == originYear) {
                selectedQuarter = originQuarter;
            } else {
                selectedQuarter = -1;
            }
        }

        rebuildContent();
        updateConfirmButtonState();
    }

    void setupUI() {
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(20, 16, 20, 16);
        root->setSpacing(10);

        auto *yearNav = new QHBoxLayout;
        yearNav->setSpacing(6);

        auto *btnPrevYear = new QPushButton("‹");
        btnPrevYear->setAutoDefault(false);
        btnPrevYear->setDefault(false);
        btnPrevYear->setFixedSize(28, 28);
        btnPrevYear->setCursor(Qt::PointingHandCursor);
        btnPrevYear->setStyleSheet(
            "QPushButton { "
            "    background: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; "
            "    font-family: 'Segoe UI'; font-size: 15px; font-weight: bold; color: #475569; padding-bottom: 2px; outline: none; "
            "} "
            "QPushButton:hover { background: #f0fdfa; border: 1.5px solid #0d9488; border-radius: 8px; color: #0d9488; } "
            "QPushButton:pressed { background: #ccfbf1; border: 1.5px solid #0f766e; border-radius: 8px; color: #0f766e; }"
        );

        auto *lblYearPrefix = new QLabel("Năm");
        lblYearPrefix->setStyleSheet("font-family: 'Segoe UI'; font-size: 13.5px; font-weight: 700; color: #334155;");

        txtYear = new QLineEdit(QString::number(selectedYear));
        txtYear->setFixedSize(58, 28);
        txtYear->setAlignment(Qt::AlignCenter);
        txtYear->setMaxLength(4);
        txtYear->setValidator(new QIntValidator(AppConstants::MIN_CALENDAR_YEAR, AppConstants::MAX_CALENDAR_YEAR, txtYear));
        txtYear->setStyleSheet(
            "QLineEdit { "
            "    background-color: #ffffff; border: 1.5px solid #cbd5e1; border-radius: 8px; "
            "    font-family: 'Segoe UI'; font-size: 13.5px; font-weight: 700; color: #0f766e; "
            "    padding: 0px 2px; margin: 0px; outline: none; "
            "} "
            "QLineEdit:hover { border: 1.5px solid #0d9488; border-radius: 8px; background-color: #f0fdfa; } "
            "QLineEdit:focus { border: 1.5px solid #0d9488; border-radius: 8px; background-color: #ffffff; color: #0d9488; }"
        );
        txtYear->installEventFilter(this);

        auto *btnNextYear = new QPushButton("›");
        btnNextYear->setAutoDefault(false);
        btnNextYear->setDefault(false);
        btnNextYear->setFixedSize(28, 28);
        btnNextYear->setCursor(Qt::PointingHandCursor);
        btnNextYear->setStyleSheet(
            "QPushButton { "
            "    background: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; "
            "    font-family: 'Segoe UI'; font-size: 15px; font-weight: bold; color: #475569; padding-bottom: 2px; outline: none; "
            "} "
            "QPushButton:hover { background: #f0fdfa; border: 1.5px solid #0d9488; border-radius: 8px; color: #0d9488; } "
            "QPushButton:pressed { background: #ccfbf1; border: 1.5px solid #0f766e; border-radius: 8px; color: #0f766e; }"
        );

        yearNav->addStretch();
        yearNav->addWidget(btnPrevYear);
        yearNav->addSpacing(2);
        yearNav->addWidget(lblYearPrefix);
        yearNav->addWidget(txtYear);
        yearNav->addSpacing(2);
        yearNav->addWidget(btnNextYear);
        yearNav->addStretch();
        root->addLayout(yearNav);

        connect(btnPrevYear, &QPushButton::clicked, this, [this]() {
            changeYear(selectedYear - 1);
        });
        connect(btnNextYear, &QPushButton::clicked, this, [this]() {
            changeYear(selectedYear + 1);
        });
        connect(txtYear, &QLineEdit::editingFinished, this, [this]() {
            commitYearInput();
        });

        contentBox = new QVBoxLayout;
        contentBox->setContentsMargins(0, 4, 0, 4);
        contentBox->setSpacing(8);
        root->addLayout(contentBox, 1);

        rebuildContent();

        auto *footH = new QHBoxLayout;
        footH->setSpacing(10);

        auto *btnCancel = new QPushButton("Hủy bỏ");
        btnCancel->setAutoDefault(false);
        btnCancel->setDefault(false);
        btnCancel->setFixedHeight(34);
        btnCancel->setCursor(Qt::PointingHandCursor);
        btnCancel->setStyleSheet(
            "QPushButton { "
            "    background: #f1f5f9; color: #475569; font-family: 'Segoe UI'; font-weight: 600; font-size: 12.5px; "
            "    border: 1px solid #cbd5e1; border-radius: 8px; padding: 0 18px; outline: none; "
            "} "
            "QPushButton:hover { background: #e2e8f0; color: #0f172a; border: 1px solid #94a3b8; border-radius: 8px; }"
        );

        btnConfirm = new QPushButton("Xác nhận");
        btnConfirm->setAutoDefault(false);
        btnConfirm->setDefault(false);
        btnConfirm->setFixedHeight(34);
        btnConfirm->setCursor(Qt::PointingHandCursor);
        updateConfirmButtonState();

        footH->addStretch();
        footH->addWidget(btnCancel);
        footH->addWidget(btnConfirm);
        root->addLayout(footH);

        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(btnConfirm, &QPushButton::clicked, this, [this]() {
            commitYearInput();
            if (!btnConfirm->isEnabled()) return;
            calculateResultBounds();
            accept();
        });
    }

    void calculateWeeks() {
        weeksList.clear();
        selectedYear = qBound(AppConstants::MIN_CALENDAR_YEAR, selectedYear, AppConstants::MAX_CALENDAR_YEAR);
        QDate jan1(selectedYear, 1, 1);
        if (!jan1.isValid()) return;
        int dow = jan1.dayOfWeek();
        QDate wStart = jan1.addDays(-(dow - 1));

        for (int guard = 0; guard < 55 && wStart.year() <= selectedYear; ++guard) {
            QDate wEnd = wStart.addDays(6);
            weeksList.append(qMakePair(wStart, wEnd));
            wStart = wStart.addDays(7);
        }
    }

    void clearLayoutRecursive(QLayout *lay) {
        if (!lay) return;
        while (QLayoutItem *item = lay->takeAt(0)) {
            if (QLayout *sub = item->layout()) {
                clearLayoutRecursive(sub);
            }
            if (QWidget *w = item->widget()) {
                w->hide();
                w->deleteLater();
            }
            delete item;
        }
    }

    void rebuildContent() {
        clearLayoutRecursive(contentBox);

        if (pType == Week) {

            if (weeksList.isEmpty()) calculateWeeks();
            if (selectedWeekIndex >= weeksList.size()) selectedWeekIndex = -1;

            auto *scroll = new QScrollArea;
            scroll->setWidgetResizable(true);
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setStyleSheet(
                "QScrollArea { background: transparent; border: none; } "
                "QScrollBar:vertical { width: 6px; background: transparent; } "
                "QScrollBar::handle:vertical { background: #cbd5e1; border-radius: 3px; } "
                "QScrollBar::handle:vertical:hover { background: #94a3b8; } "
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
            );

            auto *weekWidget = new QWidget;
            weekWidget->setStyleSheet("background: transparent; border: none;");
            auto *weekGrid = new QGridLayout(weekWidget);
            weekGrid->setHorizontalSpacing(6);
            weekGrid->setVerticalSpacing(6);
            weekGrid->setContentsMargins(2, 2, 6, 2);

            QPushButton *activeBtn = nullptr;

            for (int i = 0; i < weeksList.size(); ++i) {
                const auto &w = weeksList[i];
                bool isSel = (i == selectedWeekIndex);

                auto *btnW = new QPushButton;
                btnW->setAutoDefault(false);
                btnW->setDefault(false);
                btnW->setFixedHeight(44);
                btnW->setCursor(Qt::PointingHandCursor);

                auto *bLay = new QVBoxLayout(btnW);
                bLay->setContentsMargins(4, 3, 4, 3);
                bLay->setSpacing(1);
                bLay->setAlignment(Qt::AlignCenter);

                auto *wTitle = new QLabel(QStringLiteral("Tuần %1").arg(i + 1));
                wTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
                wTitle->setStyleSheet(QStringLiteral(
                    "font-family: 'Segoe UI'; font-size: 12px; font-weight: 700; color: %1; background: transparent; border: none;"
                ).arg(isSel ? "#ffffff" : "#0f172a"));
                wTitle->setAlignment(Qt::AlignCenter);

                QString dateRangeStr = QStringLiteral("(%1 - %2)")
                    .arg(w.first.toString("dd/MM"))
                    .arg(w.second.toString("dd/MM"));
                auto *wDateRange = new QLabel(dateRangeStr);
                wDateRange->setAttribute(Qt::WA_TransparentForMouseEvents);
                wDateRange->setStyleSheet(QStringLiteral(
                    "font-family: 'Segoe UI'; font-size: 10.5px; font-weight: 500; color: %1; background: transparent; border: none;"
                ).arg(isSel ? "#ccfbf1" : "#64748b"));
                wDateRange->setAlignment(Qt::AlignCenter);

                bLay->addWidget(wTitle);
                bLay->addWidget(wDateRange);

                btnW->setStyleSheet(QStringLiteral(
                    "QPushButton { "
                    "    background-color: %1; "
                    "    border: 1.5px solid %2; "
                    "    border-radius: 8px; "
                    "    outline: none; "
                    "} "
                    "QPushButton:hover { "
                    "    background-color: %3; "
                    "    border: 1.5px solid %4; "
                    "    border-radius: 8px; "
                    "}")
                    .arg(isSel ? "#0d9488" : "#ffffff",
                         isSel ? "#0f766e" : "#cbd5e1",
                         isSel ? "#0f766e" : "#f0fdfa",
                         isSel ? "#0f766e" : "#0d9488"));

                if (isSel) activeBtn = btnW;

                connect(btnW, &QPushButton::clicked, this, [this, i]() {
                    selectedWeekIndex = i;
                    if (selectedYear == originYear) {
                        originWeekIndex = i;
                    }
                    rebuildContent();
                    updateConfirmButtonState();
                });

                weekGrid->addWidget(btnW, i / 4, i % 4);
            }

            scroll->setWidget(weekWidget);
            contentBox->addWidget(scroll, 1);

            if (activeBtn) {
                QTimer::singleShot(30, scroll, [scroll, activeBtn]() {
                    scroll->ensureWidgetVisible(activeBtn, 0, 40);
                });
            }
        }
        else if (pType == Month) {

            auto *grid = new QGridLayout;
            grid->setSpacing(8);
            grid->setContentsMargins(0, 4, 0, 4);

            for (int m = 1; m <= 12; ++m) {
                auto *btnM = new QPushButton;
                btnM->setAutoDefault(false);
                btnM->setDefault(false);
                btnM->setFixedHeight(46);
                btnM->setCursor(Qt::PointingHandCursor);
                bool isSel = (m == selectedMonth);

                auto *bLay = new QVBoxLayout(btnM);
                bLay->setContentsMargins(4, 4, 4, 4);
                bLay->setSpacing(1);
                bLay->setAlignment(Qt::AlignCenter);

                auto *lblTitle = new QLabel(QStringLiteral("Tháng %1").arg(m));
                lblTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
                lblTitle->setStyleSheet(QStringLiteral("font-family: 'Segoe UI'; font-size: 12.5px; font-weight: 700; color: %1; background: transparent; border: none;").arg(isSel ? "#ffffff" : "#0f172a"));
                lblTitle->setAlignment(Qt::AlignCenter);

                QDate dFirst(selectedYear, m, 1);
                auto *lblSub = new QLabel(QStringLiteral("01/%1 – %2/%1").arg(QString::number(m).rightJustified(2, '0')).arg(dFirst.daysInMonth()));
                lblSub->setAttribute(Qt::WA_TransparentForMouseEvents);
                lblSub->setStyleSheet(QStringLiteral("font-family: 'Segoe UI'; font-size: 10px; font-weight: 500; color: %1; background: transparent; border: none;").arg(isSel ? "#ccfbf1" : "#64748b"));
                lblSub->setAlignment(Qt::AlignCenter);

                bLay->addWidget(lblTitle);
                bLay->addWidget(lblSub);

                btnM->setStyleSheet(QStringLiteral(
                    "QPushButton { "
                    "    background-color: %1; "
                    "    border: 1.5px solid %2; "
                    "    border-radius: 8px; "
                    "    outline: none; "
                    "} "
                    "QPushButton:hover { "
                    "    background-color: %3; "
                    "    border: 1.5px solid %4; "
                    "    border-radius: 8px; "
                    "}")
                    .arg(isSel ? "#0d9488" : "#ffffff",
                         isSel ? "#0f766e" : "#cbd5e1",
                         isSel ? "#0f766e" : "#f0fdfa",
                         isSel ? "#0f766e" : "#0d9488"));

                connect(btnM, &QPushButton::clicked, this, [this, m]() {
                    selectedMonth = m;
                    if (selectedYear == originYear) {
                        originMonth = m;
                    }
                    rebuildContent();
                    updateConfirmButtonState();
                });
                grid->addWidget(btnM, (m - 1) / 4, (m - 1) % 4);
            }
            contentBox->addLayout(grid);
        }
        else if (pType == Quarter) {

            auto *grid = new QGridLayout;
            grid->setSpacing(10);
            grid->setContentsMargins(0, 4, 0, 4);

            const QString qTitles[4] = {"Quý 1", "Quý 2", "Quý 3", "Quý 4"};
            const QString qRanges[4] = {"Tháng 01 – Tháng 03", "Tháng 04 – Tháng 06", "Tháng 07 – Tháng 09", "Tháng 10 – Tháng 12"};

            for (int q = 1; q <= 4; ++q) {
                auto *btnQ = new QPushButton;
                btnQ->setAutoDefault(false);
                btnQ->setDefault(false);
                btnQ->setFixedHeight(54);
                btnQ->setCursor(Qt::PointingHandCursor);
                bool isSel = (q == selectedQuarter);

                auto *qLay = new QVBoxLayout(btnQ);
                qLay->setContentsMargins(8, 6, 8, 6);
                qLay->setSpacing(2);
                qLay->setAlignment(Qt::AlignCenter);

                auto *t = new QLabel(qTitles[q - 1]);
                t->setAttribute(Qt::WA_TransparentForMouseEvents);
                t->setStyleSheet(QStringLiteral("font-family: 'Segoe UI'; font-size: 13.5px; font-weight: 700; color: %1; background: transparent; border: none;").arg(isSel ? "#ffffff" : "#0f172a"));
                t->setAlignment(Qt::AlignCenter);

                auto *s = new QLabel(qRanges[q - 1]);
                s->setAttribute(Qt::WA_TransparentForMouseEvents);
                s->setStyleSheet(QStringLiteral("font-family: 'Segoe UI'; font-size: 11px; font-weight: 500; color: %1; background: transparent; border: none;").arg(isSel ? "#ccfbf1" : "#64748b"));
                s->setAlignment(Qt::AlignCenter);

                qLay->addWidget(t);
                qLay->addWidget(s);

                btnQ->setStyleSheet(QStringLiteral(
                    "QPushButton { "
                    "    background-color: %1; "
                    "    border: 1.5px solid %2; "
                    "    border-radius: 8px; "
                    "    outline: none; "
                    "} "
                    "QPushButton:hover { "
                    "    background-color: %3; "
                    "    border: 1.5px solid %4; "
                    "    border-radius: 8px; "
                    "}")
                    .arg(isSel ? "#0d9488" : "#ffffff",
                         isSel ? "#0f766e" : "#cbd5e1",
                         isSel ? "#0f766e" : "#f0fdfa",
                         isSel ? "#0f766e" : "#0d9488"));

                connect(btnQ, &QPushButton::clicked, this, [this, q]() {
                    selectedQuarter = q;
                    if (selectedYear == originYear) {
                        originQuarter = q;
                    }
                    rebuildContent();
                    updateConfirmButtonState();
                });
                grid->addWidget(btnQ, (q - 1) / 2, (q - 1) % 2);
            }
            contentBox->addLayout(grid);
        }
        else if (pType == Year) {

            auto *vBox = new QVBoxLayout;
            vBox->setAlignment(Qt::AlignCenter);
            vBox->setSpacing(6);

            auto *iconLbl = new QLabel("📅");
            iconLbl->setStyleSheet("font-size: 30px; background: transparent; border: none;");
            iconLbl->setAlignment(Qt::AlignCenter);

            auto *lbl = new QLabel(QStringLiteral("Toàn bộ năm %1").arg(selectedYear));
            lbl->setStyleSheet("font-family: 'Segoe UI'; font-size: 15.5px; font-weight: 800; color: #0d9488; background: transparent; border: none;");
            lbl->setAlignment(Qt::AlignCenter);

            auto *sub = new QLabel(QStringLiteral("Thống kê từ 01/01/%1 đến 31/12/%1").arg(selectedYear));
            sub->setStyleSheet("font-family: 'Segoe UI'; font-size: 12px; font-weight: 500; color: #64748b; background: transparent; border: none;");
            sub->setAlignment(Qt::AlignCenter);

            vBox->addStretch();
            vBox->addWidget(iconLbl);
            vBox->addWidget(lbl);
            vBox->addWidget(sub);
            vBox->addStretch();
            contentBox->addLayout(vBox);
        }
    }

    void calculateResultBounds() {
        const QDate minBound(AppConstants::MIN_CALENDAR_YEAR, 1, 1);
        const QDate maxBound(AppConstants::MAX_CALENDAR_YEAR, 12, 31);
        if (pType == Week) {
            if (weeksList.isEmpty()) calculateWeeks();
            if (selectedWeekIndex >= 0 && selectedWeekIndex < weeksList.size()) {
                resultStart = weeksList[selectedWeekIndex].first;
                resultEnd = weeksList[selectedWeekIndex].second;
            } else if (!weeksList.isEmpty()) {
                resultStart = weeksList[0].first;
                resultEnd = weeksList[0].second;
            }
            if (resultStart < minBound) resultStart = minBound;
            if (resultEnd > maxBound) resultEnd = maxBound;
        } else if (pType == Month) {
            int m = (selectedMonth >= 1 && selectedMonth <= 12) ? selectedMonth : 1;
            resultStart = QDate(selectedYear, m, 1);
            resultEnd = QDate(selectedYear, m, resultStart.daysInMonth());
        } else if (pType == Quarter) {
            int q = (selectedQuarter >= 1 && selectedQuarter <= 4) ? selectedQuarter : 1;
            int sMonth = (q - 1) * 3 + 1;
            int eMonth = sMonth + 2;
            resultStart = QDate(selectedYear, sMonth, 1);
            QDate lastM(selectedYear, eMonth, 1);
            resultEnd = QDate(selectedYear, eMonth, lastM.daysInMonth());
        } else if (pType == Year) {
            resultStart = QDate(selectedYear, 1, 1);
            resultEnd = QDate(selectedYear, 12, 31);
        }
    }
};

class DateSelectionDialog : public QDialog {
public:
    QCalendarWidget *calendar = nullptr;
    QDate chosenDate;
    QLineEdit *txtMonth = nullptr;
    QLineEdit *txtYear = nullptr;

    void syncInputs() {
        if (txtMonth) {
            QSignalBlocker b(txtMonth);
            txtMonth->setText(QString::number(chosenDate.month()));
        }
        if (txtYear) {
            QSignalBlocker b(txtYear);
            txtYear->setText(QString::number(chosenDate.year()));
        }
    }

    void commitMonthInput() {
        if (!txtMonth) return;
        bool ok = false;
        int m = txtMonth->text().trimmed().toInt(&ok);
        if (ok && m >= 1 && m <= 12) {
            int y = chosenDate.year();
            int maxD = QDate(y, m, 1).daysInMonth();
            int day = qMin(chosenDate.day(), maxD);
            chosenDate = QDate(y, m, day);
            if (calendar) {
                QSignalBlocker sb(calendar);
                calendar->setSelectedDate(chosenDate);
                calendar->setCurrentPage(y, m);
            }
        }
        syncInputs();
    }

    void commitYearInput() {
        if (!txtYear) return;
        bool ok = false;
        int y = txtYear->text().trimmed().toInt(&ok);
        if (ok && y >= AppConstants::MIN_CALENDAR_YEAR && y <= AppConstants::MAX_CALENDAR_YEAR) {
            int m = chosenDate.month();
            int maxD = QDate(y, m, 1).daysInMonth();
            int day = qMin(chosenDate.day(), maxD);
            chosenDate = QDate(y, m, day);
            if (calendar) {
                QSignalBlocker sb(calendar);
                calendar->setSelectedDate(chosenDate);
                calendar->setCurrentPage(y, m);
            }
        }
        syncInputs();
    }

    bool eventFilter(QObject *obj, QEvent *ev) override {
        if (obj == txtMonth || obj == txtYear) {
            if (ev->type() == QEvent::FocusIn) {
                auto *le = qobject_cast<QLineEdit*>(obj);
                if (le) {
                    QTimer::singleShot(0, le, &QLineEdit::selectAll);
                }
            } else if (ev->type() == QEvent::FocusOut) {
                if (obj == txtMonth) commitMonthInput();
                else commitYearInput();
            }
        }
        return QDialog::eventFilter(obj, ev);
    }

    DateSelectionDialog(const QString &title, const QDate &initDate, QWidget *parent = nullptr)
        : QDialog(parent), chosenDate(initDate.isValid() ? initDate : QDate::currentDate()) {
        setWindowTitle(title);
        setFixedSize(330, 340);
        setModal(true);
        setStyleSheet("QDialog { background-color: #ffffff; border-radius: 14px; font-family: 'Segoe UI'; }");

        const QDate minBound(AppConstants::MIN_CALENDAR_YEAR, 1, 1);
        const QDate maxBound(AppConstants::MAX_CALENDAR_YEAR, 12, 31);
        if (chosenDate < minBound) chosenDate = minBound;
        if (chosenDate > maxBound) chosenDate = maxBound;

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(16, 14, 16, 14);
        root->setSpacing(8);

        auto *navH = new QHBoxLayout;
        navH->setSpacing(4);

        auto *btnPrevM = new QPushButton("‹");
        btnPrevM->setAutoDefault(false);
        btnPrevM->setDefault(false);
        btnPrevM->setFixedSize(28, 28);
        btnPrevM->setCursor(Qt::PointingHandCursor);
        btnPrevM->setStyleSheet(
            "QPushButton { "
            "    background: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; "
            "    font-family: 'Segoe UI'; font-size: 15px; font-weight: bold; color: #475569; padding-bottom: 2px; outline: none; "
            "} "
            "QPushButton:hover { background: #f0fdfa; border: 1.5px solid #0d9488; border-radius: 8px; color: #0d9488; } "
            "QPushButton:pressed { background: #ccfbf1; border: 1.5px solid #0f766e; border-radius: 8px; color: #0f766e; }"
        );

        auto *lblMonthPrefix = new QLabel("Tháng");
        lblMonthPrefix->setStyleSheet("font-family: 'Segoe UI'; font-size: 13.5px; font-weight: 700; color: #334155; background: transparent; border: none;");

        const QString flatInputStyle =
            "QLineEdit { "
            "    background: transparent; border: none; border-bottom: 2px solid transparent; "
            "    font-family: 'Segoe UI'; font-size: 14px; font-weight: 700; color: #0f766e; "
            "    padding: 1px 2px; margin: 0px; outline: none; "
            "} "
            "QLineEdit:hover { border-bottom: 2px solid #cbd5e1; color: #0d9488; } "
            "QLineEdit:focus { background-color: #f0fdfa; border: none; border-bottom: 2px solid #0d9488; border-radius: 4px; color: #0d9488; }";

        txtMonth = new QLineEdit(QString::number(chosenDate.month()));
        txtMonth->setFixedSize(28, 26);
        txtMonth->setAlignment(Qt::AlignCenter);
        txtMonth->setMaxLength(2);
        txtMonth->setValidator(new QIntValidator(1, 12, txtMonth));
        txtMonth->setStyleSheet(flatInputStyle);
        txtMonth->installEventFilter(this);

        txtYear = new QLineEdit(QString::number(chosenDate.year()));
        txtYear->setFixedSize(48, 26);
        txtYear->setAlignment(Qt::AlignCenter);
        txtYear->setMaxLength(4);
        txtYear->setValidator(new QIntValidator(AppConstants::MIN_CALENDAR_YEAR, AppConstants::MAX_CALENDAR_YEAR, txtYear));
        txtYear->setStyleSheet(flatInputStyle);
        txtYear->installEventFilter(this);

        auto *btnNextM = new QPushButton("›");
        btnNextM->setAutoDefault(false);
        btnNextM->setDefault(false);
        btnNextM->setFixedSize(28, 28);
        btnNextM->setCursor(Qt::PointingHandCursor);
        btnNextM->setStyleSheet(
            "QPushButton { "
            "    background: #f8fafc; border: 1.5px solid #cbd5e1; border-radius: 8px; "
            "    font-family: 'Segoe UI'; font-size: 15px; font-weight: bold; color: #475569; padding-bottom: 2px; outline: none; "
            "} "
            "QPushButton:hover { background: #f0fdfa; border: 1.5px solid #0d9488; border-radius: 8px; color: #0d9488; } "
            "QPushButton:pressed { background: #ccfbf1; border: 1.5px solid #0f766e; border-radius: 8px; color: #0f766e; }"
        );

        navH->addStretch();
        navH->addWidget(btnPrevM);
        navH->addSpacing(8);
        navH->addWidget(lblMonthPrefix);
        navH->addSpacing(2);
        navH->addWidget(txtMonth);
        navH->addSpacing(6);
        navH->addWidget(txtYear);
        navH->addSpacing(8);
        navH->addWidget(btnNextM);
        navH->addStretch();
        root->addLayout(navH);

        calendar = new QCalendarWidget(this);
        calendar->setGridVisible(false);
        calendar->setDateRange(minBound, maxBound);
        calendar->setSelectedDate(chosenDate);
        UIHelper::applyCalendarStyle(calendar);
        calendar->setNavigationBarVisible(false);
        calendar->setMinimumHeight(220);
        root->addWidget(calendar, 1);

        connect(calendar, &QCalendarWidget::selectionChanged, this, [this]() {
            QDate d = calendar->selectedDate();
            if (d.isValid()) {
                chosenDate = d;
                syncInputs();
            }
        });

        connect(calendar, &QCalendarWidget::clicked, this, [this](const QDate &d) {
            if (d.isValid()) {
                chosenDate = d;
                syncInputs();
            }
        });

        connect(btnPrevM, &QPushButton::clicked, this, [this]() {
            QDate d = chosenDate.addMonths(-1);
            if (d.year() >= AppConstants::MIN_CALENDAR_YEAR) {
                chosenDate = d;
                {
                    QSignalBlocker sb(calendar);
                    calendar->setSelectedDate(chosenDate);
                    calendar->setCurrentPage(chosenDate.year(), chosenDate.month());
                }
                syncInputs();
            }
        });

        connect(btnNextM, &QPushButton::clicked, this, [this]() {
            QDate d = chosenDate.addMonths(1);
            if (d.year() <= AppConstants::MAX_CALENDAR_YEAR) {
                chosenDate = d;
                {
                    QSignalBlocker sb(calendar);
                    calendar->setSelectedDate(chosenDate);
                    calendar->setCurrentPage(chosenDate.year(), chosenDate.month());
                }
                syncInputs();
            }
        });

        connect(txtMonth, &QLineEdit::editingFinished, this, [this]() { commitMonthInput(); });
        connect(txtYear, &QLineEdit::editingFinished, this, [this]() { commitYearInput(); });

        auto *footH = new QHBoxLayout;
        footH->setSpacing(10);
        auto *btnCancel = new QPushButton("Hủy");
        btnCancel->setAutoDefault(false);
        btnCancel->setDefault(false);
        btnCancel->setFixedHeight(34);
        btnCancel->setCursor(Qt::PointingHandCursor);
        btnCancel->setStyleSheet(
            "QPushButton { "
            "    background: #f1f5f9; color: #475569; font-family: 'Segoe UI'; font-weight: 600; font-size: 12.5px; "
            "    border: 1px solid #cbd5e1; border-radius: 8px; padding: 0 18px; outline: none; "
            "} "
            "QPushButton:hover { background: #e2e8f0; color: #0f172a; border: 1px solid #94a3b8; border-radius: 8px; }"
        );

        auto *btnOk = new QPushButton("Xác nhận");
        btnOk->setAutoDefault(false);
        btnOk->setDefault(false);
        btnOk->setFixedHeight(34);
        btnOk->setCursor(Qt::PointingHandCursor);
        btnOk->setStyleSheet(
            "QPushButton { "
            "    background: #0d9488; color: #ffffff; font-family: 'Segoe UI'; font-weight: 700; font-size: 12.5px; "
            "    border: 1px solid #0d9488; border-radius: 8px; padding: 0 22px; outline: none; "
            "} "
            "QPushButton:hover { background: #0f766e; border: 1px solid #0f766e; border-radius: 8px; }"
        );

        footH->addStretch();
        footH->addWidget(btnCancel);
        footH->addWidget(btnOk);
        root->addLayout(footH);

        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(btnOk, &QPushButton::clicked, this, [this]() {
            commitMonthInput();
            commitYearInput();
            accept();
        });
    }
};

void clearPagesWidgetChildren(QWidget *page) {
    if (!page) return;
    if (QLayout *oldLay = page->layout()) {
        QLayoutItem *item;
        while ((item = oldLay->takeAt(0)) != nullptr) {
            if (QWidget *w = item->widget()) {
                w->hide();
                delete w;
            }
            delete item;
        }
        delete oldLay;
    }
    const QObjectList oldKids = page->children();
    for (QObject *child : oldKids) {
        if (auto *w = qobject_cast<QWidget*>(child)) {
            w->hide();
        }
        delete child;
    }
}

inline void styleCard(QWidget *card) {
    card->setStyleSheet(
        "QFrame#panel { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 14px; } "
        "QFrame#panel QLabel { background: transparent; border: none; }"
    );
}

QFrame *makeKpiCard(const QString &accent, const QString &title, const QString &value, const QString &sub) {
    auto *card = new QFrame;
    card->setObjectName("kpiCard");
    card->setFixedHeight(96);
    card->setStyleSheet(QStringLiteral(
        "QFrame#kpiCard { background: #ffffff; border: 1px solid #e2e8f0; "
        "border-left: 4px solid %1; border-radius: 12px; }").arg(accent));

    auto *lay = new QVBoxLayout(card);
    lay->setContentsMargins(18, 12, 18, 12);
    lay->setSpacing(2);

    auto *t = new QLabel(title);
    t->setStyleSheet("font-size: 11.5px; font-weight: 600; color: #64748b; background: transparent; border: none;");
    auto *v = new QLabel(value);
    v->setStyleSheet("font-size: 24px; font-weight: 800; color: #0f172a; background: transparent; border: none;");
    auto *s = new QLabel(sub);
    s->setStyleSheet("font-size: 11px; color: #94a3b8; background: transparent; border: none;");

    lay->addWidget(t);
    lay->addWidget(v);
    lay->addWidget(s);
    return card;
}

}

StatisticView::StatisticView(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent), mainWindow(mainWindow) {

    QDate today = QDate::currentDate();
    int dow = today.dayOfWeek();
    filterStartDate = today.addDays(-(dow - 1));
    filterEndDate = filterStartDate.addDays(6);

    setupUI();
    refresh();
}

void StatisticView::refresh() {
    try {
        refreshTab1();
        refreshTab2();
    } catch (const std::exception &e) {
        UIHelper::showWarning(this, "Lỗi thống kê", QString::fromUtf8(e.what()));
    }
}

void StatisticView::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (event->spontaneous()) return;
    refresh();
}

void StatisticView::setupUI() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("StatisticView { background-color: #eaedf2; }");

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *topBar = new QFrame;
    topBar->setObjectName("statTopBar");
    topBar->setFixedHeight(52);
    topBar->setStyleSheet("QFrame#statTopBar { background-color: #ffffff; border-bottom: 1px solid #e2e8f0; }");
    auto *topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(24, 0, 24, 0);
    topBarLayout->setSpacing(12);

    auto *tabGroup = new QFrame;
    tabGroup->setObjectName("mainTabPillGroup");
    tabGroup->setFixedHeight(36);
    tabGroup->setStyleSheet(
        "QFrame#mainTabPillGroup { "
        "    background-color: #f1f5f9; "
        "    border: 1px solid #cbd5e1; "
        "    border-radius: 9px; "
        "}"
    );
    auto *tabLay = new QHBoxLayout(tabGroup);
    tabLay->setContentsMargins(3, 3, 3, 3);
    tabLay->setSpacing(3);

    btnTab1 = new QPushButton("Tổng quan && Cơ cấu");
    btnTab1->setFixedHeight(28);
    btnTab1->setCursor(Qt::PointingHandCursor);

    btnTab2 = new QPushButton("Doanh thu");
    btnTab2->setFixedHeight(28);
    btnTab2->setCursor(Qt::PointingHandCursor);

    tabLay->addWidget(btnTab1);
    tabLay->addWidget(btnTab2);

    topBarLayout->addWidget(tabGroup, 0, Qt::AlignVCenter);
    topBarLayout->addStretch();

    auto *filterContainer = new QWidget;
    filterContainer->setFixedHeight(34);
    filterContainer->setStyleSheet("background: transparent; border: none;");
    auto *filterLay = new QHBoxLayout(filterContainer);
    filterLay->setContentsMargins(0, 0, 0, 0);
    filterLay->setSpacing(8);

    auto *pillGroup = new QFrame;
    pillGroup->setObjectName("periodPillGroup");
    pillGroup->setFixedHeight(32);
    pillGroup->setStyleSheet(
        "QFrame#periodPillGroup { "
        "    background-color: #f1f5f9; "
        "    border: 1px solid #cbd5e1; "
        "    border-radius: 8px; "
        "}"
    );
    auto *pillLay = new QHBoxLayout(pillGroup);
    pillLay->setContentsMargins(2, 2, 2, 2);
    pillLay->setSpacing(2);

    const QString pillLabels[4] = {"Tuần", "Tháng", "Quý", "Năm"};
    for (int i = 0; i < 4; ++i) {
        btnPeriods[i] = new QPushButton(pillLabels[i]);
        btnPeriods[i]->setFixedHeight(26);
        btnPeriods[i]->setCursor(Qt::PointingHandCursor);
        connect(btnPeriods[i], &QPushButton::clicked, this, [this, i]() {
            openPeriodSelectionDialog(i);
        });
        pillLay->addWidget(btnPeriods[i]);
    }
    updatePeriodButtonStyles(currentPeriodIndex);

    btnStartDate = new QPushButton(filterStartDate.toString("dd/MM/yyyy"));
    btnStartDate->setFixedHeight(32);
    btnStartDate->setFixedWidth(108);
    btnStartDate->setCursor(Qt::PointingHandCursor);

    auto *lblDash = new QLabel("–");
    lblDash->setStyleSheet("font-size: 12px; font-weight: bold; color: #64748b; padding: 0 1px;");

    btnEndDate = new QPushButton(filterEndDate.toString("dd/MM/yyyy"));
    btnEndDate->setFixedHeight(32);
    btnEndDate->setFixedWidth(108);
    btnEndDate->setCursor(Qt::PointingHandCursor);

    updateDateButtonStyles();

    filterLay->addWidget(pillGroup);
    filterLay->addWidget(btnStartDate);
    filterLay->addWidget(lblDash);
    filterLay->addWidget(btnEndDate);

    topBarLayout->addWidget(filterContainer, 0, Qt::AlignVCenter);
    rootLayout->addWidget(topBar);

    tabStack = new QStackedWidget(this);
    tabStack->setStyleSheet("QStackedWidget { background-color: #eaedf2; border: none; }");

    pageTab1 = new QWidget;
    pageTab1->setObjectName("pageTab1");
    pageTab1->setAttribute(Qt::WA_StyledBackground, true);
    pageTab1->setStyleSheet("QWidget#pageTab1 { background-color: #eaedf2; }");

    pageTab2 = new QWidget;
    pageTab2->setObjectName("pageTab2");
    pageTab2->setAttribute(Qt::WA_StyledBackground, true);
    pageTab2->setStyleSheet("QWidget#pageTab2 { background-color: #eaedf2; }");

    tabStack->addWidget(pageTab1);
    tabStack->addWidget(pageTab2);
    rootLayout->addWidget(tabStack, 1);

    connect(btnTab1, &QPushButton::clicked, this, [this]() {
        tabStack->setCurrentIndex(0);
        updateTabStyles(0);
    });
    connect(btnTab2, &QPushButton::clicked, this, [this]() {
        tabStack->setCurrentIndex(1);
        updateTabStyles(1);
    });

    connect(btnStartDate, &QPushButton::clicked, this, [this]() {
        openDateDialog(true);
    });
    connect(btnEndDate, &QPushButton::clicked, this, [this]() {
        openDateDialog(false);
    });

    updateTabStyles(0);
}

void StatisticView::updateTabStyles(int activeIndex) {
    const QString activeStyle =
        "QPushButton { "
        "   background-color: #0d9488; color: #ffffff; font-weight: 700; font-size: 13px; "
        "   border: 1px solid #0d9488; border-radius: 7px; padding: 0 16px; outline: none; "
        "}";
    const QString inactiveStyle =
        "QPushButton { "
        "   background-color: transparent; color: #64748b; font-weight: 600; font-size: 13px; "
        "   border: 1px solid transparent; border-radius: 7px; padding: 0 16px; outline: none; "
        "} "
        "QPushButton:hover { background-color: #e2e8f0; color: #0f172a; border: 1px solid transparent; border-radius: 7px; }";

    if (activeIndex == 0) {
        btnTab1->setStyleSheet(activeStyle);
        btnTab2->setStyleSheet(inactiveStyle);
    } else {
        btnTab1->setStyleSheet(inactiveStyle);
        btnTab2->setStyleSheet(activeStyle);
    }
}

void StatisticView::updatePeriodButtonStyles(int selectedIndex) {
    const QString activePill =
        "QPushButton { "
        "   background-color: #0d9488; color: #ffffff; font-weight: 700; font-size: 12px; "
        "   border: 1px solid #0d9488; border-radius: 6px; padding: 2px 10px; outline: none; "
        "}";
    const QString inactivePill =
        "QPushButton { "
        "   background-color: transparent; color: #475569; font-weight: 600; font-size: 12px; "
        "   border: 1px solid transparent; border-radius: 6px; padding: 2px 10px; outline: none; "
        "} "
        "QPushButton:hover { background-color: #e2e8f0; color: #0f172a; border: 1px solid transparent; border-radius: 6px; }";

    for (int i = 0; i < 4; ++i) {
        if (btnPeriods[i]) {
            btnPeriods[i]->setStyleSheet((i == selectedIndex && selectedIndex >= 0) ? activePill : inactivePill);
        }
    }
}

void StatisticView::updateDateButtonStyles() {
    bool isCustom = (currentPeriodIndex == -1);
    QString dateBtnStyle = QStringLiteral(
        "QPushButton { "
        "    background-color: %1; "
        "    border: 1.5px solid %2; "
        "    border-radius: 8px; "
        "    padding: 0 6px; "
        "    font-family: 'Segoe UI'; "
        "    font-size: 12.5px; "
        "    font-weight: 700; "
        "    color: %3; "
        "    outline: none; "
        "} "
        "QPushButton:hover { "
        "    background-color: %4; "
        "    border: 1.5px solid %5; "
        "    border-radius: 8px; "
        "    color: %5; "
        "} "
        "QPushButton:pressed { "
        "    background-color: #ccfbf1; "
        "    border: 1.5px solid #0f766e; "
        "    border-radius: 8px; "
        "    color: #0f766e; "
        "}")
        .arg(isCustom ? "#f0fdfa" : "#ffffff",
             isCustom ? "#0d9488" : "#cbd5e1",
             isCustom ? "#0d9488" : "#0f766e",
             isCustom ? "#e6fffa" : "#f0fdfa",
             isCustom ? "#0f766e" : "#0d9488");

    if (btnStartDate) btnStartDate->setStyleSheet(dateBtnStyle);
    if (btnEndDate) btnEndDate->setStyleSheet(dateBtnStyle);
}

void StatisticView::openPeriodSelectionDialog(int periodTypeIndex) {
    PeriodSelectionDialog::PeriodType ptype = PeriodSelectionDialog::Week;
    if (periodTypeIndex == 1) ptype = PeriodSelectionDialog::Month;
    else if (periodTypeIndex == 2) ptype = PeriodSelectionDialog::Quarter;
    else if (periodTypeIndex == 3) ptype = PeriodSelectionDialog::Year;

    PeriodSelectionDialog dlg(ptype, filterStartDate, this);

    if (window()) {
        dlg.move(window()->geometry().center() - dlg.rect().center());
    }

    if (dlg.exec() == QDialog::Accepted) {
        filterStartDate = dlg.resultStart;
        filterEndDate = dlg.resultEnd;
        btnStartDate->setText(filterStartDate.toString("dd/MM/yyyy"));
        btnEndDate->setText(filterEndDate.toString("dd/MM/yyyy"));
        currentPeriodIndex = periodTypeIndex;
        updatePeriodButtonStyles(currentPeriodIndex);
        updateDateButtonStyles();
        refresh();
    }
}

void StatisticView::openDateDialog(bool isStartDate) {
    QString title = isStartDate ? "Chọn ngày bắt đầu" : "Chọn ngày kết thúc";
    QDate currentD = isStartDate ? filterStartDate : filterEndDate;

    DateSelectionDialog dlg(title, currentD, this);
    if (window()) {
        dlg.move(window()->geometry().center() - dlg.rect().center());
    }

    if (dlg.exec() == QDialog::Accepted) {
        if (isStartDate) {
            filterStartDate = dlg.chosenDate;
            btnStartDate->setText(filterStartDate.toString("dd/MM/yyyy"));
        } else {
            filterEndDate = dlg.chosenDate;
            btnEndDate->setText(filterEndDate.toString("dd/MM/yyyy"));
        }

        if (filterStartDate.isValid() && filterEndDate.isValid()) {
            if (filterStartDate > filterEndDate) {

                std::swap(filterStartDate, filterEndDate);
                btnStartDate->setText(filterStartDate.toString("dd/MM/yyyy"));
                btnEndDate->setText(filterEndDate.toString("dd/MM/yyyy"));
            }

            currentPeriodIndex = -1;
            updatePeriodButtonStyles(-1);
            updateDateButtonStyles();
            refresh();
        }
    }
}

void StatisticView::refreshTab1() {
    if (!mainWindow || !pageTab1) return;

    clearPagesWidgetChildren(pageTab1);

    auto *rootLay = new QVBoxLayout(pageTab1);
    rootLay->setContentsMargins(24, 18, 24, 20);
    rootLay->setSpacing(16);

    auto *patients = mainWindow->getPatients();
    auto *doctors = mainWindow->getDoctors();
    auto *appts = mainWindow->getAppointments();
    auto *symptomPatients = mainWindow->getSymptomPatients();

    QDateTime nowDt = QDateTime::currentDateTime();
    int curDay = nowDt.date().day(), curMonth = nowDt.date().month(), curYear = nowDt.date().year();
    int curHour = nowDt.time().hour(), curMin = nowDt.time().minute(), curSec = nowDt.time().second();

    int sDay = filterStartDate.day(), sMonth = filterStartDate.month(), sYear = filterStartDate.year();
    int eDay = filterEndDate.day(), eMonth = filterEndDate.month(), eYear = filterEndDate.year();

    int completed = 0, pending = 0, delayed = 0, rescheduled = 0;
    StatisticController::appointmentStatusCounts(
        appts, completed, pending, delayed, rescheduled,
        curDay, curMonth, curYear, curHour, curMin, curSec,
        sDay, sMonth, sYear, eDay, eMonth, eYear
    );
    int totalAppts = completed + pending + delayed + rescheduled;

    int male = 0, female = 0;
    int under18 = 0, from18to45 = 0, from46to60 = 0, over60 = 0;
    StatisticController::patientDemographicsInRange(
        patients, appts, symptomPatients,
        sDay, sMonth, sYear, eDay, eMonth, eYear,
        male, female, under18, from18to45, from46to60, over60
    );
    int totalPatients = male + female;

    auto *row1 = new QHBoxLayout;
    row1->setSpacing(16);

    auto *statusCard = UIHelper::panel();
    statusCard->setFixedHeight(215);
    styleCard(statusCard);
    auto *statusLay = new QVBoxLayout(statusCard);
    statusLay->setContentsMargins(22, 18, 22, 18);
    statusLay->setSpacing(12);

    auto *statusTitle = new QLabel("Phân bổ trạng thái lịch khám");
    statusTitle->setStyleSheet("font-size: 14.5px; font-weight: 700; color: #0f172a; background: transparent; border: none;");
    statusLay->addWidget(statusTitle);

    statusLay->addStretch(1);
    auto *statusBar = new LinearStatusBar;
    statusBar->setCounts(completed, pending, delayed, rescheduled);
    statusLay->addWidget(statusBar);
    statusLay->addSpacing(14);

    auto *chipsGrid = new QGridLayout;
    chipsGrid->setHorizontalSpacing(20);
    chipsGrid->setVerticalSpacing(10);
    chipsGrid->setContentsMargins(4, 4, 4, 4);

    auto makeStatusChip = [](const QString &dotColor, const QString &name, int count, int total) {
        auto *w = new QWidget;
        w->setStyleSheet("background: transparent; border: none;");
        auto *h = new QHBoxLayout(w);
        h->setContentsMargins(2, 2, 2, 2);
        h->setSpacing(8);

        auto *dot = new QLabel("●");
        dot->setStyleSheet(QStringLiteral("color: %1; font-size: 13px;").arg(dotColor));
        auto *title = new QLabel(name);
        title->setStyleSheet("font-size: 12px; font-weight: 600; color: #334155;");

        double pct = (total > 0) ? (count * 100.0 / total) : 0.0;
        auto *val = new QLabel(QStringLiteral("<b>%1</b> (%2%)").arg(count).arg(QString::number(pct, 'f', 1)));
        val->setStyleSheet("font-size: 12px; color: #0f172a;");

        h->addWidget(dot);
        h->addWidget(title);
        h->addStretch();
        h->addWidget(val);
        return w;
    };

    chipsGrid->addWidget(makeStatusChip("#0d9488", "Đã khám", completed, totalAppts), 0, 0);
    chipsGrid->addWidget(makeStatusChip("#f59e0b", "Chưa khám", pending, totalAppts), 0, 1);
    chipsGrid->addWidget(makeStatusChip("#ea580c", "Trễ hẹn", delayed, totalAppts), 1, 0);
    chipsGrid->addWidget(makeStatusChip("#64748b", "Đã dời lịch", rescheduled, totalAppts), 1, 1);
    statusLay->addLayout(chipsGrid);
    statusLay->addStretch(1);
    row1->addWidget(statusCard, 1);

    auto *demoCard = UIHelper::panel();
    demoCard->setFixedHeight(215);
    styleCard(demoCard);
    auto *demoLay = new QVBoxLayout(demoCard);
    demoLay->setContentsMargins(22, 18, 22, 18);
    demoLay->setSpacing(8);

    auto *demoTitle = new QLabel("Đặc điểm bệnh nhân");
    demoTitle->setStyleSheet("font-size: 14.5px; font-weight: 700; color: #0f172a; background: transparent; border: none;");
    demoLay->addWidget(demoTitle);
    demoLay->addSpacing(4);

    int genderCounts[2] = {male, female};
    int genderPct[2] = {0, 0};
    computeLargestRemainderPercentages(genderCounts, 2, genderPct);
    auto *genderH = new QHBoxLayout;
    auto *maleLbl = new QLabel(QStringLiteral("Nam: <b>%1%</b> (%2)").arg(genderPct[0]).arg(male));
    maleLbl->setStyleSheet("font-size: 11.5px; color: #0284c7; font-weight: 600;");
    auto *femaleLbl = new QLabel(QStringLiteral("Nữ: <b>%1%</b> (%2)").arg(genderPct[1]).arg(female));
    femaleLbl->setStyleSheet("font-size: 11.5px; color: #db2777; font-weight: 600;");
    genderH->addWidget(maleLbl);
    genderH->addStretch();
    genderH->addWidget(femaleLbl);
    demoLay->addLayout(genderH);

    auto *genderBar = new QProgressBar;
    genderBar->setFixedHeight(6);
    genderBar->setTextVisible(false);
    genderBar->setRange(0, totalPatients > 0 ? totalPatients : 1);
    genderBar->setValue(totalPatients > 0 ? male : 0);
    genderBar->setStyleSheet(totalPatients > 0
        ? "QProgressBar { background: #fce7f3; border-radius: 3px; border: none; } QProgressBar::chunk { background: #0284c7; border-radius: 3px; }"
        : "QProgressBar { background: #e2e8f0; border-radius: 3px; border: none; } QProgressBar::chunk { background: #e2e8f0; border-radius: 3px; }");
    demoLay->addWidget(genderBar);

    demoLay->addSpacing(6);

    auto *ageH = new QHBoxLayout;
    ageH->setSpacing(28);

    auto *donut = new PatientDonutChart;
    donut->setData(under18, from18to45, from46to60, over60);

    auto *ageGrid = new QGridLayout;
    ageGrid->setHorizontalSpacing(6);
    ageGrid->setVerticalSpacing(5);
    ageGrid->setContentsMargins(0, 0, 0, 0);

    struct AgeRowItem {
        QString dotColor;
        QString label;
        int count;
    };
    AgeRowItem ageItems[4] = {
        {"#38bdf8", "Dưới 18 tuổi", under18},
        {"#0d9488", "18 – 45 tuổi", from18to45},
        {"#f59e0b", "46 – 60 tuổi", from46to60},
        {"#8b5cf6", "Trên 60 tuổi", over60}
    };
    int ageCounts[4] = {under18, from18to45, from46to60, over60};
    int agePct[4] = {0, 0, 0, 0};
    computeLargestRemainderPercentages(ageCounts, 4, agePct);

    for (int r = 0; r < 4; ++r) {
        auto *dot = new QLabel("●");
        dot->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;").arg(ageItems[r].dotColor));
        dot->setFixedWidth(12);

        auto *name = new QLabel(ageItems[r].label);
        name->setStyleSheet("font-size: 11.5px; color: #475569; font-weight: 600;");
        name->setFixedWidth(86);

        auto *countVal = new QLabel(QStringLiteral("<b>%1</b>").arg(ageItems[r].count));
        countVal->setStyleSheet("font-size: 11.5px; color: #0f172a;");
        countVal->setFixedWidth(24);
        countVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        auto *pctVal = new QLabel(QStringLiteral("(%1%)").arg(agePct[r]));
        pctVal->setStyleSheet("font-size: 11.5px; color: #64748b; font-weight: 500;");
        pctVal->setFixedWidth(46);
        pctVal->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        ageGrid->addWidget(dot, r, 0);
        ageGrid->addWidget(name, r, 1);
        ageGrid->addWidget(countVal, r, 2);
        ageGrid->addWidget(pctVal, r, 3);
    }

    ageH->addStretch(1);
    ageH->addWidget(donut, 0, Qt::AlignVCenter);
    ageH->addLayout(ageGrid);
    ageH->addStretch(1);

    demoLay->addLayout(ageH);
    demoLay->addStretch();

    row1->addWidget(demoCard, 1);
    rootLay->addLayout(row1, 0);

    auto *row2 = new QHBoxLayout;
    row2->setSpacing(16);

    auto *specCard = UIHelper::panel();
    styleCard(specCard);
    auto *specLay = new QVBoxLayout(specCard);
    specLay->setContentsMargins(22, 18, 22, 18);
    specLay->setSpacing(12);

    auto *specTitle = new QLabel("Nhu cầu khám theo chuyên khoa");
    specTitle->setStyleSheet("font-size: 14.5px; font-weight: 700; color: #0f172a; background: transparent; border: none;");
    specLay->addWidget(specTitle);

    auto *specScroll = new QScrollArea(specCard);
    specScroll->setWidgetResizable(true);
    specScroll->setFrameShape(QFrame::NoFrame);
    specScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    specScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    specScroll->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollBar:vertical, QScrollBar:horizontal { width: 0px; height: 0px; }");

    auto *specContainer = new QWidget(specScroll);
    specContainer->setStyleSheet("background: transparent;");
    auto *specListLay = new QVBoxLayout(specContainer);
    specListLay->setContentsMargins(0, 4, 8, 4);
    specListLay->setSpacing(10);

    SpecialtyDemandItem allSpecs[50];
    int specCount = StatisticController::getSpecialtyDemand(
        appts, doctors, allSpecs, 50,
        sDay, sMonth, sYear, eDay, eMonth, eYear
    );

    if (specCount > 0) {
        int maxDemand = qMax(1, allSpecs[0].count);
        for (int i = 0; i < specCount; ++i) {
            auto *itemBox = new QVBoxLayout;
            itemBox->setSpacing(3);

            auto *h = new QHBoxLayout;
            QString spName = mainWindow->specialty(allSpecs[i].spId);
            auto *nameLbl = new QLabel(QStringLiteral("<b>%1. %2</b>").arg(i + 1).arg(spName.toHtmlEscaped()));
            nameLbl->setStyleSheet("font-size: 12.5px; font-weight: 700; color: #1e293b;");

            auto *valLbl = new QLabel(QStringLiteral("<b>%1</b> lượt khám").arg(allSpecs[i].count));
            valLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #0284c7;");

            h->addWidget(nameLbl);
            h->addStretch();
            h->addWidget(valLbl);
            itemBox->addLayout(h);

            auto *bar = new QProgressBar;
            bar->setFixedHeight(6);
            bar->setTextVisible(false);
            bar->setRange(0, maxDemand);
            bar->setValue(allSpecs[i].count);
            bar->setStyleSheet("QProgressBar { background: #f1f5f9; border-radius: 3px; border: none; } QProgressBar::chunk { background: #0284c7; border-radius: 3px; }");
            itemBox->addWidget(bar);

            specListLay->addLayout(itemBox);
        }
        specListLay->addStretch();
    } else {
        auto *emptyLbl = new QLabel("Chưa có lượt khám nào trong khoảng thời gian này.");
        emptyLbl->setStyleSheet("color: #94a3b8; font-style: italic;");
        specListLay->addWidget(emptyLbl);
    }

    specScroll->setWidget(specContainer);
    specLay->addWidget(specScroll, 1);
    row2->addWidget(specCard, 1);

    auto *docCard = UIHelper::panel();
    styleCard(docCard);
    auto *docLay = new QVBoxLayout(docCard);
    docLay->setContentsMargins(22, 18, 22, 18);
    docLay->setSpacing(8);

    auto *docTitle = new QLabel("Top bác sĩ hoàn thành nhiều ca nhất");
    docTitle->setStyleSheet("font-size: 14.5px; font-weight: 700; color: #0f172a; background: transparent; border: none;");
    docLay->addWidget(docTitle);
    docLay->addSpacing(4);

    auto *docScroll = new QScrollArea(docCard);
    docScroll->setWidgetResizable(true);
    docScroll->setFrameShape(QFrame::NoFrame);
    docScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    docScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    docScroll->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollBar:vertical, QScrollBar:horizontal { width: 0px; height: 0px; }");
    docScroll->viewport()->setStyleSheet("background: transparent; border: none;");

    connect(docScroll->verticalScrollBar(), &QScrollBar::valueChanged, docScroll->viewport(), [docScroll]() {
        docScroll->viewport()->update();
    });

    auto *docContainer = new QWidget(docScroll);
    docContainer->setStyleSheet("background: transparent; border: none;");
    auto *docListLay = new QVBoxLayout(docContainer);
    docListLay->setContentsMargins(4, 4, 8, 4);
    docListLay->setSpacing(8);

    DoctorLeaderboardItem topDocs[10];
    int docRankCount = StatisticController::getTopDoctors(
        appts, doctors, topDocs, 10,
        sDay, sMonth, sYear, eDay, eMonth, eYear
    );
    const QString medals[3] = {"🥇", "🥈", "🥉"};

    for (int i = 0; i < docRankCount; ++i) {
        auto *itemFrame = new QFrame;
        itemFrame->setAttribute(Qt::WA_StyledBackground, true);
        itemFrame->setObjectName(QStringLiteral("topDocCard%1").arg(i));
        itemFrame->setFixedHeight(46);

        QString frameStyle;
        if (i == 0) {
            frameStyle =
                "QFrame#topDocCard0 { "
                "   background-color: #fffbeb; "
                "   border: 1px solid #facc15; "
                "   border-radius: 8px; "
                "} "
                "QFrame#topDocCard0:hover { "
                "   background-color: #fef3c7; "
                "   border: 1px solid #eab308; "
                "   border-radius: 8px; "
                "}";
        } else if (i == 1) {
            frameStyle =
                "QFrame#topDocCard1 { "
                "   background-color: #f8fafc; "
                "   border: 1px solid #cbd5e1; "
                "   border-radius: 8px; "
                "} "
                "QFrame#topDocCard1:hover { "
                "   background-color: #f1f5f9; "
                "   border: 1px solid #94a3b8; "
                "   border-radius: 8px; "
                "}";
        } else if (i == 2) {
            frameStyle =
                "QFrame#topDocCard2 { "
                "   background-color: #fff7ed; "
                "   border: 1px solid #fed7aa; "
                "   border-radius: 8px; "
                "} "
                "QFrame#topDocCard2:hover { "
                "   background-color: #ffedd5; "
                "   border: 1px solid #fb923c; "
                "   border-radius: 8px; "
                "}";
        } else {
            frameStyle = QStringLiteral(
                "QFrame#topDocCard%1 { "
                "   background-color: #f8fafc; "
                "   border: 1px solid #e2e8f0; "
                "   border-radius: 8px; "
                "} "
                "QFrame#topDocCard%1:hover { "
                "   background-color: #f1f5f9; "
                "   border: 1px solid #cbd5e1; "
                "   border-radius: 8px; "
                "}").arg(i);
        }
        itemFrame->setStyleSheet(frameStyle);

        auto *h = new QHBoxLayout(itemFrame);
        h->setContentsMargins(12, 4, 12, 4);
        h->setSpacing(12);

        QLabel *badge = nullptr;
        if (i < 3) {
            badge = new QLabel(medals[i]);
            badge->setFixedWidth(28);
            badge->setAlignment(Qt::AlignCenter);
            badge->setStyleSheet("font-size: 17px; background: transparent; border: none;");
        } else {
            badge = new QLabel(QString::number(i + 1));
            badge->setFixedSize(22, 22);
            badge->setAlignment(Qt::AlignCenter);
            badge->setStyleSheet("background-color: #e2e8f0; color: #475569; font-size: 11px; font-weight: 800; border-radius: 11px; border: none;");
        }

        auto *infoCol = new QVBoxLayout;
        infoCol->setSpacing(1);
        auto *dName = new QLabel(QStringLiteral("BS. %1").arg(UIHelper::qs(topDocs[i].doctorName)));
        dName->setStyleSheet("font-size: 12px; font-weight: 700; color: #0f172a; background: transparent; border: none;");
        QString specStr = mainWindow->specialty(topDocs[i].specialtyId);
        QString roomStr = UIHelper::formatRoom(UIHelper::qs(topDocs[i].roomId));
        auto *dSub = new QLabel(QStringLiteral("%1 · %2").arg(specStr, roomStr));
        dSub->setStyleSheet("font-size: 10.5px; color: #64748b; background: transparent; border: none;");
        infoCol->addWidget(dName);
        infoCol->addWidget(dSub);

        auto *ratingLbl = new QLabel(QStringLiteral("⭐ %1").arg(QString::number(topDocs[i].rating, 'f', 1)));
        ratingLbl->setFixedWidth(56);
        ratingLbl->setAlignment(Qt::AlignCenter);
        ratingLbl->setStyleSheet("font-size: 11px; font-weight: 700; color: #b45309; background-color: #fef3c7; border: 1px solid #fde68a; border-radius: 5px; padding: 2px 4px;");

        auto *countLbl = new QLabel(QStringLiteral("<b>%1</b> ca").arg(topDocs[i].appointmentCount));
        countLbl->setFixedWidth(52);
        countLbl->setAlignment(Qt::AlignCenter);
        countLbl->setStyleSheet("font-size: 11.5px; font-weight: 700; color: #0f766e; background-color: #ccfbf1; border: 1px solid #99f6e4; border-radius: 5px; padding: 2px 4px;");

        h->addWidget(badge, 0, Qt::AlignVCenter);
        h->addLayout(infoCol, 1);
        h->addWidget(ratingLbl, 0, Qt::AlignVCenter);
        h->addWidget(countLbl, 0, Qt::AlignVCenter);

        docListLay->addWidget(itemFrame);
    }
    if (docRankCount == 0) {
        auto *emptyLbl = new QLabel("Chưa có bác sĩ nào hoàn thành ca khám trong khoảng thời gian này.");
        emptyLbl->setStyleSheet("color: #94a3b8; font-style: italic; font-size: 13px; padding: 28px 0;");
        emptyLbl->setAlignment(Qt::AlignCenter);
        docListLay->addWidget(emptyLbl);
    }
    docListLay->addStretch();
    docScroll->setWidget(docContainer);
    docLay->addWidget(docScroll, 1);
    row2->addWidget(docCard, 1);

    rootLay->addLayout(row2, 1);
}

void StatisticView::refreshTab2() {
    if (!mainWindow || !pageTab2) return;

    clearPagesWidgetChildren(pageTab2);

    auto *rootLay = new QVBoxLayout(pageTab2);
    rootLay->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(pageTab2);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background-color: #eaedf2; border: none; } QScrollBar:vertical, QScrollBar:horizontal { width: 0px; height: 0px; }");

    auto *contentWidget = new QWidget(scrollArea);
    contentWidget->setStyleSheet("background-color: #eaedf2;");
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(24, 18, 24, 24);
    contentLayout->setSpacing(16);

    auto *doctors = mainWindow->getDoctors();
    auto *appts = mainWindow->getAppointments();

    QDateTime nowDt = QDateTime::currentDateTime();
    int curDay = nowDt.date().day(), curMonth = nowDt.date().month(), curYear = nowDt.date().year();
    int curHour = nowDt.time().hour(), curMin = nowDt.time().minute(), curSec = nowDt.time().second();

    int daysDiff = filterStartDate.daysTo(filterEndDate);
    bool useMonthly = (daysDiff > 366);
    int maxPoints = useMonthly
        ? qMax(24, (filterEndDate.year() - filterStartDate.year()) * 12 + (filterEndDate.month() - filterStartDate.month()) + 2)
        : qMax(64, daysDiff + 5);

    auto items = std::make_unique<TimeSeriesItem[]>(maxPoints);
    int itemCount = 0;
    if (useMonthly) {
        itemCount = StatisticController::getTimeSeriesMonthlyRange(
            appts, doctors,
            filterStartDate.day(), filterStartDate.month(), filterStartDate.year(),
            filterEndDate.day(), filterEndDate.month(), filterEndDate.year(),
            curDay, curMonth, curYear, curHour, curMin,
            items.get(), maxPoints, curSec
        );
    } else {
        itemCount = StatisticController::getTimeSeriesDaily(
            appts, doctors,
            filterStartDate.day(), filterStartDate.month(), filterStartDate.year(),
            filterEndDate.day(), filterEndDate.month(), filterEndDate.year(),
            curDay, curMonth, curYear, curHour, curMin,
            items.get(), maxPoints, curSec
        );
    }

    QVector<CoordPoint> revPoints;
    QVector<StatusPoint> statusPoints;
    double totalFilterRev = 0.0;
    int totalFilterComp = 0, totalFilterDelay = 0, totalFilterResched = 0;

    double peakRev = 0.0;
    QString peakLabel = "—";
    int peakMonth = 0, peakYear = 0;

    for (int i = 0; i < itemCount; ++i) {
        CoordPoint cp;
        cp.label = QString::fromStdString(items[i].label);
        cp.value = items[i].revenue;
        revPoints.append(cp);
        totalFilterRev += items[i].revenue;

        if (items[i].revenue > peakRev) {
            peakRev = items[i].revenue;
            peakLabel = cp.label;
            peakMonth = items[i].month;
            peakYear = items[i].year;
        }

        StatusPoint sp;
        sp.label = cp.label;
        sp.completed = items[i].completedCount;
        sp.delayed = items[i].delayedCount;
        sp.rescheduled = items[i].rescheduledCount;
        statusPoints.append(sp);

        totalFilterComp += items[i].completedCount;
        totalFilterDelay += items[i].delayedCount;
        totalFilterResched += items[i].rescheduledCount;
    }

    {
        const double avgRev = (itemCount > 0) ? totalFilterRev / itemCount : 0.0;
        const QLocale vi(QLocale::Vietnamese);
        auto money = [&vi](double v) { return vi.toString(v, 'f', 0) + QStringLiteral(" đ"); };

        QString peakSub;
        if (peakRev <= 0.0) {
            peakSub = QStringLiteral("Chưa có dữ liệu");
        } else if (useMonthly) {
            peakSub = QStringLiteral("Tháng %1/%2").arg(peakMonth).arg(peakYear);
        } else {
            peakSub = QStringLiteral("Ngày %1").arg(peakLabel);
        }

        auto *kpiRow = new QHBoxLayout;
        kpiRow->setSpacing(16);
        kpiRow->addWidget(makeKpiCard("#0d9488", "Tổng doanh thu", money(totalFilterRev),
                                      QStringLiteral("%1 – %2").arg(filterStartDate.toString("dd/MM/yyyy"),
                                                                    filterEndDate.toString("dd/MM/yyyy"))), 1);
        kpiRow->addWidget(makeKpiCard("#0284c7", useMonthly ? "Trung bình mỗi tháng" : "Trung bình mỗi ngày",
                                      money(avgRev), QStringLiteral("%1 %2").arg(itemCount).arg(useMonthly ? "tháng" : "ngày")), 1);
        kpiRow->addWidget(makeKpiCard("#f59e0b", "Cao nhất", money(peakRev), peakSub), 1);
        kpiRow->addWidget(makeKpiCard("#16a34a", "Ca đã khám", QString::number(totalFilterComp),
                                      QStringLiteral("%1 trễ · %2 dời").arg(totalFilterDelay).arg(totalFilterResched)), 1);
        contentLayout->addLayout(kpiRow);
    }

    auto *revCard = UIHelper::panel();
    styleCard(revCard);
    auto *revLay = new QVBoxLayout(revCard);
    revLay->setContentsMargins(22, 18, 22, 20);
    revLay->setSpacing(12);

    auto *revHeader = new QHBoxLayout;
    auto *revTitle = new QLabel("Biểu đồ doanh thu (VND)");
    revTitle->setStyleSheet("font-size: 14.5px; font-weight: 700; color: #0f172a; background: transparent; border: none;");
    auto *revTotalLbl = new QLabel(QStringLiteral("Tổng doanh thu: <b>%1 đ</b>").arg(QLocale(QLocale::Vietnamese).toString(totalFilterRev, 'f', 0)));
    revTotalLbl->setStyleSheet("font-size: 13px; color: #0f766e; font-weight: 600; background: transparent; border: none;");
    revHeader->addWidget(revTitle);
    revHeader->addStretch();
    revHeader->addWidget(revTotalLbl);
    revLay->addLayout(revHeader);

    auto *revChart = new RevenueLineChart;
    revChart->setData(revPoints);
    revLay->addWidget(revChart);

    contentLayout->addWidget(revCard);

    auto *statusCard = UIHelper::panel();
    styleCard(statusCard);
    auto *statusLay = new QVBoxLayout(statusCard);
    statusLay->setContentsMargins(22, 18, 22, 20);
    statusLay->setSpacing(12);

    auto *statusHeader = new QHBoxLayout;
    auto *statusTitle = new QLabel("Biểu đồ lịch khám");
    statusTitle->setStyleSheet("font-size: 14.5px; font-weight: 700; color: #0f172a; background: transparent; border: none;");
    auto *statusSummary = new QLabel(QStringLiteral("Tổng: <b>%1</b> đã khám · <b>%2</b> trễ · <b>%3</b> dời").arg(totalFilterComp).arg(totalFilterDelay).arg(totalFilterResched));
    statusSummary->setStyleSheet("font-size: 12.5px; color: #475569; background: transparent; border: none;");
    statusHeader->addWidget(statusTitle);
    statusHeader->addStretch();
    statusHeader->addWidget(statusSummary);
    statusLay->addLayout(statusHeader);

    auto *statusChart = new StatusMultiLineChart;
    statusChart->setData(statusPoints);
    statusLay->addWidget(statusChart);

    contentLayout->addWidget(statusCard);

    scrollArea->setWidget(contentWidget);
    rootLay->addWidget(scrollArea);
}
