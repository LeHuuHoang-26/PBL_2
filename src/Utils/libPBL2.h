#ifndef LIBPBL2_H
#define LIBPBL2_H

#include <string>
#include <cstdio>
#include "LinkList.h"
#include "Date.h"
#include "AppExceptions.h"

namespace libPBL2 {

    inline void trim(std::string &str);
    inline size_t utf8Length(const std::string &str);
    inline std::string padRightUtf8(const std::string &str, size_t targetWidth);

    inline bool containsForbiddenStorageChars(const std::string &str);
    inline bool isValidPhone(const std::string &phone);
    inline bool isValidCccd(const std::string &cccd);

    inline std::string extractRoomCode(const std::string &roomStr);
    inline std::string canonicalRoomKey(const std::string &roomStr);
    inline std::string formatRoomDisplay(const std::string &roomStr);
    inline std::string storageRoomId(const std::string &roomStr);

    inline int safeParseInt(const std::string &str, const std::string &fieldName, int defaultVal = 0);
    inline double safeParseDouble(const std::string &str, const std::string &fieldName, double defaultVal = 0.0);

    inline void safeAtomicCommit(const std::string &tempPath, const std::string &finalPath);

    template <typename T, typename IdGetter>
    std::string generateNextId(const LinkList<T> *list, const std::string &prefix, IdGetter getId, const std::string &altPrefix = "");

    template <typename T, typename IdGetter>
    T *findByIdWithCursor(LinkList<T> *list, const std::string &id, IdGetter getId);

    inline int compareNatural(const std::string &a, const std::string &b);
    inline std::string extractLastName(const std::string &fullName);
    inline int compareVietnameseStrings(const std::string &s1, const std::string &s2);
    inline int compareVietnameseNames(const std::string &name1, const std::string &name2);
    inline int compareDates(const Date &da, const Date &db);
    inline int extractRoomNumber(const std::string &roomStr);

    template <typename T>
    void swapVal(T &a, T &b);

    template <typename T, typename Compare>
    typename LinkList<T>::Node* mergeSortedNodes(typename LinkList<T>::Node *a,
                                                 typename LinkList<T>::Node *b,
                                                 Compare comp);

    template <typename T, typename Compare>
    typename LinkList<T>::Node* sortNodeChain(typename LinkList<T>::Node *head, Compare comp);

    template <typename T, typename Compare>
    void qsort(LinkList<T> &list, Compare comp);

    template <typename T, typename Compare>
    void qsort(LinkList<T> *list, Compare comp);

    template <typename T, typename Compare>
    void qsort(T *arr, int low, int high, Compare comp);

    template <typename Container, typename Compare>
    void qsortRandomAccess(Container &c, int low, int high, Compare comp);

    template <typename Container, typename Compare>
    void qsort(Container &c, Compare comp);

}

#include "libPBL2.cpp"

#endif
