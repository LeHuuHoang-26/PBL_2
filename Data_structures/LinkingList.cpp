#include "LinkingList.h"
#include "Doctor.h"
#include "Appointment.h"
#include "AuthController.h"
#include "Patient.h"

template <typename T>

LinkingList::LinkingList () {
    header = new Node;
    
    header->next = nullptr;
    header->prev = nullptr;
}

LinkingList::LinkingList (const LinkingList &Ll) {
    header = new Node;
    header->next = nullptr;
    header->prev = nullptr;

    Node *curr = Ll.header->next;
    Node *tail = header;
    while (curr != nullptr) {
        Node *tmp = new Node;
        tmp->info = curr->info;

        tmp->next = tail->next;
        tmp->prev = tail;
        
        tail->next = tmp;
        tail = tmp;

        curr = curr->next;
    }
}

~LinkingList::LinkingList() {
    Node *curr = header;
    while (curr != nullptr) {
        Node *next = curr->next;
        delete curr;
        curr = next;
    }
}

