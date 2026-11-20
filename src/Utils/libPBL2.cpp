#ifndef LIBPBL2_CPP
#define LIBPBL2_CPP

#include "libPBL2.h"

#include <cctype>
#include <climits>
#include <stdexcept>
#include <QString>
#include <QCollator>
#include <QLocale>
#include <QCoreApplication>

namespace libPBL2 {

namespace detail {
inline QCollator *&cachedViCollatorPtr() {
    static QCollator *ptr = nullptr;
    return ptr;
}

inline void cleanupViCollator() {
    delete cachedViCollatorPtr();
    cachedViCollatorPtr() = nullptr;
}

inline int compareViQString(const QString &a, const QString &b) {
    if (QCoreApplication::instance()) {
        QCollator *&ptr = cachedViCollatorPtr();
        if (!ptr) {
            ptr = new QCollator(QLocale(QLocale::Vietnamese));
            ptr->setCaseSensitivity(Qt::CaseInsensitive);
            qAddPostRoutine(cleanupViCollator);
        }
        return ptr->compare(a, b);
    }
    QCollator local(QLocale(QLocale::Vietnamese));
    local.setCaseSensitivity(Qt::CaseInsensitive);
    return local.compare(a, b);
}

inline bool isAsciiDigit(char ch) {
    return ch >= '0' && ch <= '9';
}
}

inline void trim(std::string &str) {
    size_t head = str.find_first_not_of(" \t\r\n");
    if (head == std::string::npos) {
        str.clear();
        return;
    }
    size_t tail = str.find_last_not_of(" \t\r\n");
    str = str.substr(head, tail - head + 1);
}

inline size_t utf8Length(const std::string &str) {
    size_t count = 0;
    for (unsigned char byte : str) {
        if ((byte & 0xC0) != 0x80) count++;
    }
    return count;
}

inline std::string padRightUtf8(const std::string &str, size_t targetWidth) {
    size_t ulen = utf8Length(str);
    if (ulen >= targetWidth) return str;
    return str + std::string(targetWidth - ulen, ' ');
}

inline bool containsForbiddenStorageChars(const std::string &str) {
    for (char ch : str) {
        if (ch == '|' || ch == '\r' || ch == '\n') {
            return true;
        }
    }
    return false;
}

inline bool isValidPhone(const std::string &phone) {
    std::string s = phone;
    trim(s);
    if (s.length() != 10 || s[0] != '0') return false;
    for (char ch : s) {
        if (!detail::isAsciiDigit(ch)) return false;
    }
    return true;
}

inline bool isValidCccd(const std::string &cccd) {
    std::string s = cccd;
    trim(s);
    if (s.length() != 12 || s[0] != '0') return false;
    for (char ch : s) {
        if (!detail::isAsciiDigit(ch)) return false;
    }
    return true;
}

inline std::string extractRoomCode(const std::string &roomStr) {
    QString r = QString::fromUtf8(roomStr.c_str()).trimmed();
    if (r.isEmpty()) return "";

    auto stripPrefix = [](QString &str, const QString &prefix) -> bool {
        if (!str.startsWith(prefix, Qt::CaseInsensitive)) return false;
        QString rest = str.mid(prefix.length());
        if (rest.isEmpty()) {
            str.clear();
            return true;
        }
        QChar first = rest[0];
        if (first == ' ' || first == '_' || first == '-' || (first >= '0' && first <= '9')) {
            str = rest.trimmed();
            while (!str.isEmpty() && (str[0] == '_' || str[0] == '-' || str[0] == ' ')) {
                str.remove(0, 1);
            }
            return true;
        }
        return false;
    };

    if (!stripPrefix(r, QStringLiteral("Phòng"))) {
        if (!stripPrefix(r, QStringLiteral("Phong"))) {
            if (!stripPrefix(r, QStringLiteral("Room"))) {
                stripPrefix(r, QStringLiteral("R"));
            }
        }
    }

    QString collapsed;
    bool prevSep = false;
    for (const QChar &ch : r.trimmed()) {
        if (ch == '_' || ch == ' ') {
            if (!prevSep && !collapsed.isEmpty()) {
                collapsed.append(' ');
                prevSep = true;
            }
        } else {
            collapsed.append(ch);
            prevSep = false;
        }
    }
    return collapsed.trimmed().toUtf8().toStdString();
}

inline std::string canonicalRoomKey(const std::string &roomStr) {
    QString code = QString::fromUtf8(extractRoomCode(roomStr).c_str()).toLower();
    QString key;
    for (const QChar &ch : code) {
        if (ch != ' ' && ch != '_' && ch != '-') {
            key.append(ch);
        }
    }
    return key.toUtf8().toStdString();
}

inline std::string formatRoomDisplay(const std::string &roomStr) {
    std::string code = extractRoomCode(roomStr);
    if (code.empty()) return "";
    return "Room " + code;
}

inline std::string storageRoomId(const std::string &roomStr) {
    std::string code = extractRoomCode(roomStr);
    if (code.empty()) return "";
    for (char &ch : code) {
        if (ch == ' ') ch = '_';
    }
    return "Room_" + code;
}

inline int safeParseInt(const std::string &str, const std::string &fieldName, int defaultVal) {
    std::string text = str;
    trim(text);
    if (text.empty()) return defaultVal;
    try {
        size_t processed = 0;
        int parsed = std::stoi(text, &processed);
        if (processed != text.length()) {
            throw DataFormatException("Trường [" + fieldName + "] chứa ký tự không hợp lệ: '" + str + "'");
        }
        return parsed;
    } catch (const std::invalid_argument &) {
        throw DataFormatException("Trường [" + fieldName + "] không đúng định dạng số nguyên: '" + str + "'");
    } catch (const std::out_of_range &) {
        throw DataFormatException("Trường [" + fieldName + "] vượt quá giới hạn số nguyên: '" + str + "'");
    }
}

inline double safeParseDouble(const std::string &str, const std::string &fieldName, double defaultVal) {
    std::string text = str;
    trim(text);
    if (text.empty()) return defaultVal;
    try {
        size_t processed = 0;
        double parsed = std::stod(text, &processed);
        if (processed != text.length()) {
            throw DataFormatException("Trường [" + fieldName + "] chứa ký tự không hợp lệ: '" + str + "'");
        }
        return parsed;
    } catch (const std::invalid_argument &) {
        throw DataFormatException("Trường [" + fieldName + "] không đúng định dạng số thực: '" + str + "'");
    } catch (const std::out_of_range &) {
        throw DataFormatException("Trường [" + fieldName + "] vượt quá giới hạn số thực: '" + str + "'");
    }
}

inline void safeAtomicCommit(const std::string &tempPath, const std::string &finalPath) {
    const std::string backupPath = finalPath + ".bak";
    std::remove(backupPath.c_str());
    bool backedUp = (std::rename(finalPath.c_str(), backupPath.c_str()) == 0);
    if (std::rename(tempPath.c_str(), finalPath.c_str()) != 0) {
        if (backedUp) {
            std::rename(backupPath.c_str(), finalPath.c_str());
        }
        std::remove(tempPath.c_str());
        throw FileIOException(finalPath, "Không thể cập nhật tệp dữ liệu sau khi ghi tạm.");
    }
    if (backedUp) {
        std::remove(backupPath.c_str());
    }
}

template <typename T, typename IdGetter>
std::string generateNextId(const LinkList<T> *list, const std::string &prefix, IdGetter getId, const std::string &altPrefix) {
    int highest = 0;
    if (list && list->getHeader()) {
        auto *node = list->getHeader()->next;
        while (node && node->next) {
            if (!node->info.isDefault()) {
                std::string currentId = getId(node->info);
                size_t skip = 0;
                if (currentId.rfind(prefix, 0) == 0 && currentId.length() > prefix.length()) {
                    skip = prefix.length();
                } else if (!altPrefix.empty() && currentId.rfind(altPrefix, 0) == 0 && currentId.length() > altPrefix.length()) {
                    skip = altPrefix.length();
                }
                if (skip > 0) {
                    try {
                        int num = std::stoi(currentId.substr(skip));
                        if (num > highest) highest = num;
                    } catch (const std::invalid_argument &) {
                    } catch (const std::out_of_range &) {
                    }
                }
            }
            node = node->next;
        }
    }
    char buffer[48];
    std::snprintf(buffer, sizeof(buffer), "%s%03d", prefix.c_str(), highest + 1);
    return std::string(buffer);
}

template <typename T, typename IdGetter>
T *findByIdWithCursor(LinkList<T> *list, const std::string &id, IdGetter getId) {
    if (id.empty() || !list || !list->getHeader() || !list->getFooter()) return nullptr;
    auto *header = list->getHeader();
    auto *footer = list->getFooter();
    if (header->next == footer) return nullptr;

    auto *cur = list->getCursor();
    if (cur && cur->prev && cur->next) {
        if (!cur->info.isDefault() && getId(cur->info) == id) {
            return &cur->info;
        }
        if (cur->prev != header && !cur->prev->info.isDefault() && getId(cur->prev->info) == id) {
            list->setCursor(cur->prev);
            return &cur->prev->info;
        }
        if (cur->next != footer && !cur->next->info.isDefault() && getId(cur->next->info) == id) {
            list->setCursor(cur->next);
            return &cur->next->info;
        }
    }

    if (!header->next->info.isDefault() && getId(header->next->info) == id) {
        list->setCursor(header->next);
        return &header->next->info;
    }
    if (!footer->prev->info.isDefault() && getId(footer->prev->info) == id) {
        list->setCursor(footer->prev);
        return &footer->prev->info;
    }

    auto *fwd = (cur && cur->prev && cur->next)
        ? (cur->next != footer ? cur->next->next : footer)
        : header->next->next;
    auto *bwd = (cur && cur->prev && cur->next)
        ? (cur->prev != header ? cur->prev->prev : header)
        : header;

    while ((fwd && fwd != footer) || (bwd && bwd != header)) {
        if (fwd && fwd != footer) {
            if (!fwd->info.isDefault() && getId(fwd->info) == id) {
                list->setCursor(fwd);
                return &fwd->info;
            }
            fwd = fwd->next;
        }
        if (bwd && bwd != header) {
            if (!bwd->info.isDefault() && getId(bwd->info) == id) {
                list->setCursor(bwd);
                return &bwd->info;
            }
            bwd = bwd->prev;
        }
    }
    return nullptr;
}

inline int compareNatural(const std::string &a, const std::string &b) {
    size_t ia = 0, ib = 0;
    const size_t szA = a.size(), szB = b.size();
    while (ia < szA && ib < szB) {
        unsigned char ca = static_cast<unsigned char>(a[ia]);
        unsigned char cb = static_cast<unsigned char>(b[ib]);
        if (std::isdigit(ca) && std::isdigit(cb)) {
            size_t begA = ia;
            while (ia < szA && std::isdigit(static_cast<unsigned char>(a[ia]))) ia++;
            size_t begB = ib;
            while (ib < szB && std::isdigit(static_cast<unsigned char>(b[ib]))) ib++;

            size_t nonZeroA = begA;
            while (nonZeroA < ia && a[nonZeroA] == '0') nonZeroA++;
            size_t nonZeroB = begB;
            while (nonZeroB < ib && b[nonZeroB] == '0') nonZeroB++;

            size_t spanA = ia - nonZeroA;
            size_t spanB = ib - nonZeroB;
            if (spanA != spanB) return (spanA < spanB) ? -1 : 1;
            for (size_t k = 0; k < spanA; ++k) {
                if (a[nonZeroA + k] != b[nonZeroB + k]) {
                    return (a[nonZeroA + k] < b[nonZeroB + k]) ? -1 : 1;
                }
            }
        } else {
            unsigned char la = static_cast<unsigned char>(std::tolower(ca));
            unsigned char lb = static_cast<unsigned char>(std::tolower(cb));
            if (la != lb) return (la < lb) ? -1 : 1;
            ia++;
            ib++;
        }
    }
    if (ia < szA || ib < szB) return (ia < szA) ? 1 : -1;
    if (szA != szB) return (szA < szB) ? -1 : 1;
    return 0;
}

inline std::string extractLastName(const std::string &fullName) {
    size_t right = fullName.find_last_not_of(" \t\r\n");
    if (right == std::string::npos) return "";
    size_t left = fullName.find_last_of(" \t", right);
    if (left == std::string::npos) return fullName.substr(0, right + 1);
    return fullName.substr(left + 1, right - left);
}

inline int compareVietnameseStrings(const std::string &s1, const std::string &s2) {
    int res = detail::compareViQString(QString::fromUtf8(s1.c_str()), QString::fromUtf8(s2.c_str()));
    if (res != 0) return (res < 0) ? -1 : 1;
    return 0;
}

inline int compareVietnameseNames(const std::string &name1, const std::string &name2) {
    std::string l1 = extractLastName(name1);
    std::string l2 = extractLastName(name2);
    int res = detail::compareViQString(QString::fromUtf8(l1.c_str()), QString::fromUtf8(l2.c_str()));
    if (res != 0) return (res < 0) ? -1 : 1;
    res = detail::compareViQString(QString::fromUtf8(name1.c_str()), QString::fromUtf8(name2.c_str()));
    if (res != 0) return (res < 0) ? -1 : 1;
    return 0;
}

inline int compareDates(const Date &da, const Date &db) {
    if (da.getYear() != db.getYear()) return (da.getYear() < db.getYear()) ? -1 : 1;
    if (da.getMonth() != db.getMonth()) return (da.getMonth() < db.getMonth()) ? -1 : 1;
    if (da.getDay() != db.getDay()) return (da.getDay() < db.getDay()) ? -1 : 1;
    if (da.getHour() != db.getHour()) return (da.getHour() < db.getHour()) ? -1 : 1;
    if (da.getMinute() != db.getMinute()) return (da.getMinute() < db.getMinute()) ? -1 : 1;
    return 0;
}

inline int extractRoomNumber(const std::string &roomStr) {
    int value = 0;
    bool hasDigits = false;
    for (char ch : roomStr) {
        if (detail::isAsciiDigit(ch)) {
            int digit = ch - '0';
            if (value > (INT_MAX - digit) / 10) {
                value = INT_MAX;
                hasDigits = true;
                break;
            }
            value = value * 10 + digit;
            hasDigits = true;
        } else if (hasDigits) {
            break;
        }
    }
    return hasDigits ? value : -1;
}

template <typename T>
void swapVal(T &a, T &b) {
    if (&a == &b) return;
    T hold = static_cast<T&&>(a);
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(hold);
}

template <typename T, typename Compare>
typename LinkList<T>::Node* mergeSortedNodes(typename LinkList<T>::Node *a,
                                             typename LinkList<T>::Node *b,
                                             Compare comp) {
    if (!a) return b;
    if (!b) return a;
    typename LinkList<T>::Node *head = nullptr;
    typename LinkList<T>::Node *tail = nullptr;

    auto appendNode = [&](typename LinkList<T>::Node *&src) {
        typename LinkList<T>::Node *pick = src;
        src = src->next;
        pick->next = nullptr;
        pick->prev = tail;
        if (tail) {
            tail->next = pick;
        } else {
            head = pick;
        }
        tail = pick;
    };

    while (a && b) {
        if (comp(b->info, a->info)) {
            appendNode(b);
        } else {
            appendNode(a);
        }
    }

    typename LinkList<T>::Node *rest = a ? a : b;
    if (rest) {
        rest->prev = tail;
        if (tail) {
            tail->next = rest;
        } else {
            head = rest;
        }
    }
    if (head) head->prev = nullptr;
    return head;
}

template <typename T, typename Compare>
typename LinkList<T>::Node* sortNodeChain(typename LinkList<T>::Node *head, Compare comp) {
    if (!head || !head->next) return head;

    typename LinkList<T>::Node *slow = head;
    typename LinkList<T>::Node *fast = head->next;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }

    typename LinkList<T>::Node *mid = slow->next;
    slow->next = nullptr;
    if (mid) mid->prev = nullptr;

    typename LinkList<T>::Node *leftSorted = sortNodeChain<T, Compare>(head, comp);
    typename LinkList<T>::Node *rightSorted = sortNodeChain<T, Compare>(mid, comp);
    return mergeSortedNodes<T, Compare>(leftSorted, rightSorted, comp);
}

template <typename T, typename Compare>
void qsort(LinkList<T> &list, Compare comp) {
    if (list.getCount() <= 1 || !list.getHeader() || !list.getFooter()) return;
    typename LinkList<T>::Node *header = list.getHeader();
    typename LinkList<T>::Node *footer = list.getFooter();
    typename LinkList<T>::Node *headNode = header->next;
    typename LinkList<T>::Node *tailNode = footer->prev;
    if (!headNode || !tailNode || headNode == footer || headNode == tailNode) return;

    headNode->prev = nullptr;
    tailNode->next = nullptr;

    typename LinkList<T>::Node *sortedHead = sortNodeChain<T, Compare>(headNode, comp);
    typename LinkList<T>::Node *sortedTail = sortedHead;
    while (sortedTail && sortedTail->next) {
        sortedTail = sortedTail->next;
    }

    header->next = sortedHead;
    sortedHead->prev = header;
    sortedTail->next = footer;
    footer->prev = sortedTail;
}

template <typename T, typename Compare>
void qsort(LinkList<T> *list, Compare comp) {
    if (list) qsort<T, Compare>(*list, comp);
}

template <typename T, typename Compare>
void qsort(T *arr, int low, int high, Compare comp) {
    if (!arr || low >= high) return;
    int center = low + (high - low) / 2;
    if (center != high) swapVal(arr[center], arr[high]);
    T pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        if (comp(arr[j], pivot)) {
            i++;
            if (i != j) swapVal(arr[i], arr[j]);
        }
    }
    if (i + 1 != high) swapVal(arr[i + 1], arr[high]);
    int sep = i + 1;
    qsort(arr, low, sep - 1, comp);
    qsort(arr, sep + 1, high, comp);
}

template <typename Container, typename Compare>
void qsortRandomAccess(Container &c, int low, int high, Compare comp) {
    if (low >= high) return;
    int center = low + (high - low) / 2;
    if (center != high) swapVal(c[center], c[high]);
    auto pivot = c[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        if (comp(c[j], pivot)) {
            i++;
            if (i != j) swapVal(c[i], c[j]);
        }
    }
    if (i + 1 != high) swapVal(c[i + 1], c[high]);
    int sep = i + 1;
    qsortRandomAccess(c, low, sep - 1, comp);
    qsortRandomAccess(c, sep + 1, high, comp);
}

template <typename Container, typename Compare>
void qsort(Container &c, Compare comp) {
    if (c.size() <= 1) return;
    qsortRandomAccess(c, 0, static_cast<int>(c.size()) - 1, comp);
}

}

#endif
