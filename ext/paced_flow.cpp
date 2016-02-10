#include "paced_flow.h"

#include "../coresim/event.h"

extern double get_current_time();
extern void add_to_event_queue(Event* ev);

PacedFlow::PacedFlow(uint32_t id, double start_time, uint32_t size, Host* s, Host* d) : Flow(id, start_time, size, s, d) {
    counter = 0;
}

void PacedFlow::start_flow() {
    std::cout << get_current_time() << " starting flow\n";
    add_to_event_queue(new FlowProcessingEvent(get_current_time(), this));
}

void PacedFlow::send_pending_data() {
    //std::cout << get_current_time() << " sending packet: " << counter << "\n";
    Packet* p = new Packet(get_current_time(), this, counter++, 1, 1500, this->src, this->dst);
    add_to_event_queue(new PacketQueuingEvent(get_current_time(), p, this->src->queue));
    if (get_current_time() < 4995) {
        add_to_event_queue(new FlowProcessingEvent(get_current_time() + 0.012, this));
    }
}

void PacedFlow::receive(Packet* p) {
    if (p->type == ACK_PACKET) {
        //do nothing
        return;
    }
    else {
        //send ACK
        //std::cout << get_current_time() << " sending ACK to " << p->seq_no << "\n";
        PlainAck* a = new PlainAck(this, 0, 1500, this->dst, this->src);
        add_to_event_queue(new PacketQueuingEvent(get_current_time(), a, this->dst->queue));
    }
}

uint32_t PacedFlow::get_priority(uint32_t seq) {
    return 1;
}
