#include "Doctor.h"
#include "Appointment.h"
#include "AuthController.h"

#include "LinkList.h"

#include <iostream>

template <typename T>
LinkList<T>::LinkList(): header(new LinkList::Node), footer(new LinkList::Node)  {
    header->next = footer;
    header->prev = nullptr;

    footer->next = nullptr;
    footer->prev = header;
}

template <typename T>
LinkList<T>::LinkList(const LinkList &L): header(new LinkList::Node), footer(new LinkList::Node) {
    header->next = footer;
    header->prev = nullptr;

    footer->next = nullptr;
    footer->prev = header;

    LinkList::Node *tmp = L.header->next;
    while (tmp->next != nullptr) {
        LinkList::Node *tmpNode = new LinkList::Node;
        tmpNode->info = tmp->info;

        tmpNode->next = footer;
        tmpNode->prev = footer->prev;
        if (footer->prev != nullptr)
            footer->prev->next = tmpNode;
        footer->prev = tmpNode;

        tmp = tmp->next;
    }
}

template <typename T>
LinkList<T>::~LinkList() {
    LinkList::Node *tmp = header;
    while (tmp != nullptr) {
        LinkList::Node *tmptotmp = tmp->next;
        delete tmp;

        tmp = tmptotmp;
    }
}

template <typename T>
bool LinkList<T>::insert(const T &info) {
    LinkList::Node *tmp = new LinkList::Node;
    if (tmp == nullptr)
        return false;

    tmp->info = info;
    tmp->next = header->next;
    tmp->prev = header;
    if (header->next != nullptr)
        header->next->prev = tmp;
    header->next = tmp;

    return true;
}   

template <typename T>
bool LinkList<T>::remove(LinkList::Node *N) {
    if (N == header || N == footer)
        return false;

    N->next->prev = N->prev;
    N->prev->next = N->next;

    delete N;
    return true;
}

template <typename T>
typename LinkList<T>::Node *LinkList<T>::getHeader() const {
    return header;
}

template <typename T>
typename LinkList<T>::Node *LinkList<T>::getFooter() const {
    return footer;
}