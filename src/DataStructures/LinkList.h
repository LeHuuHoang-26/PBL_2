#ifndef LINKLIST_H
#define LINKLIST_H

#include "AppExceptions.h"

template <class T>
class LinkList {
public:
    struct Node {
        T info;
        Node *next = nullptr;
        Node *prev = nullptr;
    };

private:
    Node *header;
    Node *footer;
    int count;

public:
    LinkList();
    LinkList(const LinkList &L);
    LinkList &operator=(const LinkList &) = delete;
    ~LinkList();

    Node *getHeader() const;
    Node *getFooter() const;
    int getCount() const;

    bool insert(const T &info);
    bool remove(Node *N);
};

#include "LinkList.cpp"

#endif
