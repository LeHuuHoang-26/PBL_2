#ifndef LINKLIST_H
#define LINKLIST_H

#include "../Utils/AppExceptions.h"

template <class T>
class LinkList {
public:
    struct Node {
        T info;
        Node *next;
        Node *prev;
    };

private:
    Node *header;
    Node *footer;

public:
    int count;

    LinkList();
    LinkList(const LinkList &L);
    LinkList &operator=(const LinkList &) = delete;
    ~LinkList();

    bool insert(const T &info);
    bool remove(Node *N);
    int getCount() const;
    Node *getHeader() const;
    Node *getFooter() const;
};

#include "LinkList.cpp"

#endif // LINKLIST_H
