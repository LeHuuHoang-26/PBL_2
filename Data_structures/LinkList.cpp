#include "Doctor.h"
#include "Appointment.h"
#include "AuthController.h"

#include "LinkList.h"

#include <iostream>

template <typename T>

LinkList::LinkList() header(new LinkList::Node), footer(new LinkList::Node)  {
    header->next = footer;
    header->prev = nullptr;

    footer->next = nullptr;
    header->prev = header;
}

LinkList::LinkList(const LinkList &L): header(new LinkList::Node), footer(new LinkList::Node) {
    header->next = footer;
    header->prev = nullptr;

    footer->next = nullptr;
    header->prev = header;

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

LinkList::~LinkList() {
    LinkList::Node *tmp = header;
    while (tmp->next != nullptr) {
        LinkList::Node *tmptotmp = tmp->next;
        delete tmp;

        tmp = tmptotmp;
    }
}

bool LinkList::insert(const T &info) {
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

bool LinkList::remove(LinkList::Node *N) {
    tmp->next->prev = tmp->prev;
    tmp->prev->next = tmp->next;

    delete N;
    return true;
}

Node *getHeader() const {
    return header;
}

Node *getFooter() const {
    return footer;
}