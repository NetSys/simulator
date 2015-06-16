#ifndef PFABRIC_QUEUE_H
#define PFABRIC_QUEUE_H

#include "../core/queue.h"

#define PFABRIC_QUEUE 2

class PFabricQueue : public Queue {
    public:
        PFabricQueue(uint32_t id, double rate, uint32_t limit_bytes, int location);
        void enque(Packet *packet);
        Packet *deque();
};


#endif
