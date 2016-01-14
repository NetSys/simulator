#include "tcpflow.h"

#include "../coresim/event.h"

extern double get_current_time();
extern void add_to_event_queue(Event* ev);

TCPFlow::TCPFlow(uint32_t id, double start_time, uint32_t size, Host* s, Host* d) : Flow(id, start_time, size, s, d) {
    this->cwnd_mss = 1;
}

void TCPFlow::start_flow() {
    //send SYN
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), new RTSCTS(true, get_current_time(), this, hdr_size, this->src, this->dst), this->src->queue));
}

void TCPFlow::receive(Packet* p) {
    if (p->type == RTS_PACKET) {
        // send SYNACK
        add_to_event_queue(new PacketQueuingEvent(get_current_time(), new RTSCTS(false, get_current_time(), this, hdr_size, this->dst, this->src), this->dst->queue));
    }
    else if (p->type == CTS_PACKET) {
        // got SYNACK
        send_pending_data();
    }
    else {
        Flow::receive(p);
    }
}

uint32_t TCPFlow::get_priority(uint32_t seq) {
    return 1;
}
