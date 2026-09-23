#ifndef LINKLIST_H
#define LINKLIST_H


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
        LinkList(): header(new Node), footer(new Node)  {
            header->next = footer;
            header->prev = nullptr;

            footer->next = nullptr;
            footer->prev = header;
        }

        LinkList(const LinkList &Ll): header(new Node), footer(new Node) {
            header->next = footer;
            header->prev = nullptr;

            footer->next = nullptr;
            footer->prev = header;

            Node *tmp = L.header->next;
            while (tmp->next != nullptr) {
                Node *tmpNode = new Node;
                tmpNode->info = tmp->info;

                tmpNode->next = footer;
                tmpNode->prev = footer->prev;
                if (footer->prev != nullptr)
                    footer->prev->next = tmpNode;
                footer->prev = tmpNode;

                tmp = tmp->next;
            }
        }
        ~LinkList() {
            Node *tmp = header;
            while (tmp != nullptr) {
                Node *tmptotmp = tmp->next;
                delete tmp;

                tmp = tmptotmp;
            }
        }

        bool insert(const T &info) {
            Node *tmp = new Node;
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

        bool remove(Node *N) {
            if (N == header || N == footer)
                return false;

            N->next->prev = N->prev;
            N->prev->next = N->next;

            delete N;
            return true;
        }

        Node *getHeader() const {
            return header;
        }
        Node *getFooter() const {
            return footer;
        }
};

#endif // LINKLIST_H
