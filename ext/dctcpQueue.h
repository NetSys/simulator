// Akshay Narayan
// 11 January 2016

#ifndef DCTCP_QUEUE_H
#define DCTCP_QUEUE_H

#include "../coresim/queue.h"
#include "../coresim/packet.h"

#define DCTCP_QUEUE 5

class DctcpQueue : public Queue {
    public:
        DctcpQueue(uint32_t id, double rate, uint32_t limit_bytes, int location);
        void enque(Packet *packet);
};

#endif
