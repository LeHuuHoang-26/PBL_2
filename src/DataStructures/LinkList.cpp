#ifndef LINKLIST_CPP
#define LINKLIST_CPP

#include "LinkList.h"
#include <new>

template <class T>
LinkList<T>::LinkList() : header(nullptr), footer(nullptr), cursor(nullptr), count(0) {
    try {
        header = new Node;
        footer = new Node;
        header->next = footer;
        header->prev = nullptr;
        footer->next = nullptr;
        footer->prev = header;
    } catch (const std::bad_alloc &) {
        delete header;
        delete footer;
        header = nullptr;
        footer = nullptr;
        throw MemoryAllocationException("Không đủ bộ nhớ để khởi tạo cấu trúc LinkList.");
    } catch (...) {
        delete header;
        delete footer;
        header = nullptr;
        footer = nullptr;
        throw;
    }
}

template <class T>
LinkList<T>::LinkList(const LinkList &L) : header(nullptr), footer(nullptr), cursor(nullptr), count(0) {
    try {
        header = new Node;
        footer = new Node;
        header->next = footer;
        header->prev = nullptr;
        footer->next = nullptr;
        footer->prev = header;

        for (Node *curr = L.header ? L.header->next : nullptr; curr && curr->next; curr = curr->next) {
            insert(curr->info);
        }
    } catch (...) {
        Node *curr = header;
        while (curr) {
            Node *next = curr->next;
            delete curr;
            curr = next;
        }
        header = nullptr;
        footer = nullptr;
        count = 0;
        throw;
    }
}

template <class T>
LinkList<T>::~LinkList() {
    Node *curr = header;
    while (curr) {
        Node *next = curr->next;
        delete curr;
        curr = next;
    }
    header = nullptr;
    footer = nullptr;
    cursor = nullptr;
    count = 0;
}

template <class T>
typename LinkList<T>::Node *LinkList<T>::getHeader() const {
    return header;
}

template <class T>
typename LinkList<T>::Node *LinkList<T>::getFooter() const {
    return footer;
}

template <class T>
typename LinkList<T>::Node *LinkList<T>::getCursor() const {
    return cursor;
}

template <class T>
void LinkList<T>::setCursor(Node *N) const {
    if (N && N != header && N != footer) {
        cursor = N;
    } else {
        cursor = nullptr;
    }
}

template <class T>
int LinkList<T>::getCount() const {
    return count;
}

template <class T>
bool LinkList<T>::insert(const T &info) {
    Node *tmp = nullptr;
    try {
        tmp = new Node;
        tmp->info = info;
        tmp->next = footer;
        tmp->prev = footer->prev;
        if (footer->prev) {
            footer->prev->next = tmp;
        }
        footer->prev = tmp;
        count++;
        return true;
    } catch (const std::bad_alloc &) {
        delete tmp;
        throw MemoryAllocationException("Không đủ bộ nhớ để cấp phát Node trong LinkList.");
    } catch (...) {
        delete tmp;
        throw;
    }
}

template <class T>
bool LinkList<T>::remove(Node *N) {
    if (!N || N == header || N == footer) {
        return false;
    }

    if (cursor == N) {
        if (N->prev && N->prev != header) {
            cursor = N->prev;
        } else if (N->next && N->next != footer) {
            cursor = N->next;
        } else {
            cursor = nullptr;
        }
    }

    if (N->next) {
        N->next->prev = N->prev;
    }
    if (N->prev) {
        N->prev->next = N->next;
    }

    delete N;
    count--;
    return true;
}

#endif
