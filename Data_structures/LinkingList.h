#ifndef LINKINGLIST_H
#define LINKINGLIST_H

#include "Doctor.h"
#include "Appointment.h"
#include "AuthController.h"
#include "Patient.h"

template <typename T>
class LinkingList {
    private:
        struct Node {
            T info;
            Node *next;
            Node *prev;
        };

        Node *header;
    public:
        LinkingList();
        LinkingList(const LinkingList &Ll);
        ~LinkingList();

        void insert(const );
        void remove();
        T* next() const;
        T* prev() const;
};

#endif // LINKINGLIST_H
