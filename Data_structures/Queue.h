#ifndef QUEUE_H
#define QUEUE_H

template <class T>
class Queue {
    public:
        struct Node {
            Node *next;
            T *data;
        }
    private:
        Node *header;
        int n = 0;
    public:
        
        
        
};

#endif