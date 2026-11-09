#ifndef LINKLIST_CPP
#define LINKLIST_CPP

#include "LinkList.h"
#include <new>

template <class T>
LinkList<T>::LinkList() : header(nullptr), footer(nullptr), count(0) {
    try {
        header = new Node;
        footer = new Node;
        header->next = footer;
        header->prev = nullptr;

        footer->next = nullptr;
        footer->prev = header;
    } catch (const std::bad_alloc &) {
        delete header;
        header = nullptr;
        footer = nullptr;
        throw MemoryAllocationException("Không đủ bộ nhớ để khởi tạo cấu trúc LinkList.");
    }
}

template <class T>
LinkList<T>::LinkList(const LinkList &L) : header(nullptr), footer(nullptr), count(0) {
    try {
        header = new Node;
        footer = new Node;
        header->next = footer;
        header->prev = nullptr;

        footer->next = nullptr;
        footer->prev = header;

        Node *tmp = L.header->next;
        while (tmp != nullptr && tmp->next != nullptr) {
            insert(tmp->info);
            tmp = tmp->next;
        }
    } catch (...) {
        Node *tmp = header;
        while (tmp != nullptr) {
            Node *next = tmp->next;
            delete tmp;
            tmp = next;
        }
        header = nullptr;
        footer = nullptr;
        count = 0;
        throw;
    }
}

template <class T>
LinkList<T>::~LinkList() {
    Node *tmp = header;
    while (tmp != nullptr) {
        Node *tmptotmp = tmp->next;
        delete tmp;
        tmp = tmptotmp;
    }
    header = nullptr;
    footer = nullptr;
    count = 0;
}

template <class T>
bool LinkList<T>::insert(const T &info) {
    try {
        Node *tmp = new Node;
        tmp->info = info;
        tmp->next = footer;
        tmp->prev = footer->prev;
        if (footer->prev != nullptr)
            footer->prev->next = tmp;
        footer->prev = tmp;

        count++;
        return true;
    } catch (const std::bad_alloc &) {
        throw MemoryAllocationException("Không đủ bộ nhớ để cấp phát Node trong LinkList.");
    }
}

template <class T>
bool LinkList<T>::remove(Node *N) {
    if (N == nullptr || N == header || N == footer)
        return false;

    if (N->next != nullptr)
        N->next->prev = N->prev;
    if (N->prev != nullptr)
        N->prev->next = N->next;

    delete N;
    count--;
    return true;
}

template <class T>
int LinkList<T>::getCount() const {
    return count;
}

template <class T>
typename LinkList<T>::Node *LinkList<T>::getHeader() const {
    return header;
}

template <class T>
typename LinkList<T>::Node *LinkList<T>::getFooter() const {
    return footer;
}

#endif // LINKLIST_CPP
