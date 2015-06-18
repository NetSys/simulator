#ifndef NODE_H
#define NODE_H

#include <vector>
#include <queue>
#include "queue.h"


#define HOST 0
#define SWITCH 1

#define CORE_SWITCH 10
#define AGG_SWITCH 11

#define CPU 0
#define MEM 1
#define DISK 2

class Packet;
class Flow;


class FlowComparator{
    public:
        bool operator() (Flow *a, Flow *b);
};


class Node {
    public:
        Node(uint32_t id, uint32_t type);
        uint32_t id;
        uint32_t type;
};

class Host : public Node {
    public:
        Host(uint32_t id, double rate, uint32_t queue_type, uint32_t host_type);
        Queue *queue;
        int host_type;
};

class Switch : public Node {
    public:
        Switch(uint32_t id, uint32_t switch_type);
        uint32_t switch_type;
        std::vector<Queue *> queues;
};

class CoreSwitch : public Switch {
    public:
        //All queues have same rate
        CoreSwitch(uint32_t id, uint32_t nq, double rate, uint32_t queue_type);
};

class AggSwitch : public Switch {
    public:
        // Different Rates
        AggSwitch(uint32_t id, uint32_t nq1, double r1, uint32_t nq2, double r2, uint32_t queue_type);
};

#endif
