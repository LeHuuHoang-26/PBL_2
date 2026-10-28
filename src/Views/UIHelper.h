#ifndef UIHELPER_H
#define UIHELPER_H

#include <QString>
#include <QStringList>
#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QIcon>
#include <QHBoxLayout>
#include <QVector>
#include <string>
#include <algorithm>
#include <utility>

class QCalendarWidget;
class QLineEdit;

class UIHelper {
public:
    // --- THUẬT TOÁN SẮP XẾP NHANH HOARE (TEMPLATE QUICKSORT) ---
    template <typename Container, typename Compare>
    static void quickSort(Container &c, int low, int high, Compare comp) {
        if (low >= high) return;

        // Chọn pivot ở vị trí giữa tránh suy thoái O(N^2)
        auto pivot = c[low + (high - low) / 2];
        int i = low;
        int j = high;

        while (i <= j) {
            while (comp(c[i], pivot)) {
                ++i;
            }
            while (comp(pivot, c[j])) {
                --j;
            }
            if (i <= j) {
                std::swap(c[i], c[j]);
                ++i;
                --j;
            }
        }

        if (low < j) {
            quickSort(c, low, j, comp);
        }
        if (i < high) {
            quickSort(c, i, high, comp);
        }
    }

    template <typename Container, typename Compare>
    static void quickSort(Container &c, Compare comp) {
        int size = static_cast<int>(c.size());
        if (size > 1) {
            quickSort(c, 0, size - 1, comp);
        }
    }

    template <typename Container>
    static void quickSort(Container &c) {
        quickSort(c, [](const auto &a, const auto &b) {
            return a < b;
        });
    }

    // So sánh chuẩn tên tiếng Việt (ưu tiên Tên chính ở cuối, trùng tên xét Họ & Tên đệm)
    static int compareVietnameseNames(const QString &fullNameA, const QString &fullNameB) {
        QString a = fullNameA.trimmed();
        QString b = fullNameB.trimmed();
        if (a.isEmpty() || b.isEmpty()) return QString::localeAwareCompare(a, b);

        int lastSpaceA = a.lastIndexOf(' ');
        int lastSpaceB = b.lastIndexOf(' ');
        QString firstNameA = (lastSpaceA >= 0) ? a.mid(lastSpaceA + 1) : a;
        QString firstNameB = (lastSpaceB >= 0) ? b.mid(lastSpaceB + 1) : b;

        int cmpFirst = QString::localeAwareCompare(firstNameA, firstNameB);
        if (cmpFirst != 0) return cmpFirst;

        return QString::localeAwareCompare(a, b);
    }

    static inline QString qs(const std::string &s) {
        return QString::fromUtf8(s.c_str());
    }

    static inline std::string ss(const QString &s) {
        return s.toUtf8().toStdString();
    }

    static QWidget *page(const QString &title, const QString &subtitle, QWidget *parent = nullptr);
    static QFrame *panel();
    static QPushButton *button(const QString &text, const QString &kind = "primary");
    static QTableWidget *table(const QStringList &headers);
    static QFrame *createTableCard(QTableWidget *table, QWidget *parent = nullptr);
    static void setupTableScaling(QTableWidget *table, const QVector<double> &ratios, int fixedLastColWidth = 0);
    static void cell(QTableWidget *t, int row, int column, const QString &text, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter);
    static QString formatRoom(const QString &rawRoom);
    static QPixmap createAvatar(const QString &fullName, int size = 38, const QColor &bgColor = QColor("#0d9488"), const QColor &textColor = Qt::white);
    static QIcon createEyeIcon(bool isVisible);
    static QIcon trashIcon(const QColor &color = QColor("#ef4444"));
    static QPair<QWidget*, QPushButton*> createActionCell(const QIcon &icon, const QString &tooltip);
    static QString dialogFormStyle();
    static QHBoxLayout *createDialogHeader(const QString &iconText, const QString &titleText, const QString &subtitleText);
    static void applyCalendarStyle(QCalendarWidget *cal);
    static void setupPhoneInput(QLineEdit *lineEdit);

    enum class DialogType {
        Danger,
        Warning,
        Info,
        Success
    };

    static bool showConfirmDialog(QWidget *parent,
                                 const QString &title,
                                 const QString &message,
                                 const QString &confirmBtnText = "Xác nhận",
                                 const QString &cancelBtnText = "Hủy bỏ",
                                 DialogType type = DialogType::Danger);

    static void showNoticeDialog(QWidget *parent,
                                const QString &title,
                                const QString &message,
                                DialogType type = DialogType::Info);

    static inline bool confirmCancel(QWidget *parent, const QString &title, const QString &message,
                                    const QString &confirmText = "Hủy lịch hẹn", const QString &cancelText = "Không hủy") {
        return showConfirmDialog(parent, title, message, confirmText, cancelText, DialogType::Danger);
    }

    static inline bool confirmDelete(QWidget *parent, const QString &title, const QString &message,
                                    const QString &confirmText = "Xóa", const QString &cancelText = "Không xóa") {
        return showConfirmDialog(parent, title, message, confirmText, cancelText, DialogType::Danger);
    }

    static inline bool confirmWarning(QWidget *parent, const QString &title, const QString &message,
                                     const QString &confirmText = "Tiếp tục", const QString &cancelText = "Quay lại") {
        return showConfirmDialog(parent, title, message, confirmText, cancelText, DialogType::Warning);
    }

    static inline void showWarning(QWidget *parent, const QString &title, const QString &message) {
        showNoticeDialog(parent, title, message, DialogType::Warning);
    }

    static inline void showInfo(QWidget *parent, const QString &title, const QString &message) {
        showNoticeDialog(parent, title, message, DialogType::Info);
    }

    static inline void showSuccess(QWidget *parent, const QString &title, const QString &message) {
        showNoticeDialog(parent, title, message, DialogType::Success);
    }
};

#endif // UIHELPER_H
