#ifndef LINKLIST_H
#define LINKLIST_H

#include "Doctor.h"
#include "Appointment.h"
#include "AuthController.h"

template <typename T>

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
        LinkList();
        LinkList(const LinkList &Ll);
        ~LinkList();

        bool insert(const T &info);
        bool remove(const T &info);

        Node *getHeader() const;
        Node *getFooter() const;
};

#endif // LINKLIST_H
