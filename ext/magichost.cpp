#include <assert.h>

#include "../coresim/event.h"
#include "../coresim/flow.h"
#include "../coresim/debug.h"

#include "magichost.h"
#include "magicflow.h"
#include "factory.h"

#include "../run/params.h"

extern double get_current_time();
extern void add_to_event_queue(Event*);
extern DCExpParams params;

MagicHostScheduleEvent::MagicHostScheduleEvent(double time, MagicHost *h) : Event(MAGIC_HOST_SCHEDULE, time) {
    this->host = h;
}

MagicHostScheduleEvent::~MagicHostScheduleEvent() {
}

void MagicHostScheduleEvent::process_event() {
    //std::cout << "calling schd() at event.cpp 369 for host" << this->host->id << "\n";
    this->host->schedule();
    this->host->try_send();
}

bool has_higher_priority(MagicFlow* a, MagicFlow* b) {
    // use FIFO ordering since all flows are same size
    if(a->size_in_pkt < b->size_in_pkt)
        return true;
    else if(a->size_in_pkt == b->size_in_pkt)
        return a->remaining_pkt() < b->remaining_pkt();
    else
        return false;
}

bool MagicHostFlowComparator::operator() (MagicFlow* a, MagicFlow* b) {
    // use FIFO ordering since all flows are same size
    if(a->remaining_pkt() > b->remaining_pkt())
        return true;
    else if(a->remaining_pkt() == b->remaining_pkt())
        return a->start_time > b->start_time;
    else
        return false;
}


bool MagicFlowTimeoutComparator::operator() (MagicFlow* a, MagicFlow* b) {
    // use FIFO ordering since all flows are same size
    return a->ack_timeout > b->ack_timeout;
}




MagicHost::MagicHost(uint32_t id, double rate, uint32_t queue_type) : SchedulingHost(id, rate, queue_type) {
    this->flow_sending = NULL;
    this->recv_busy_until = 0;
    this->host_type = MAGIC_HOST;
}

void MagicHost::start(Flow* f) {
    if(debug_flow(f->id) || debug_host(this->id))
        std::cout << get_current_time() << " host:" << this->id << " flow started " << f->id << " " << f->src->id << "->" << f->dst->id << " sz:" << f->size_in_pkt << "\n";

    ((MagicFlow*)f)->last_pkt_sent_at = get_current_time();

    this->active_sending_flows.push((MagicFlow*)f);


    this->schedule();
    this->try_send();

}


void MagicHost::schedule() {

    if(debug_host(this->id))
        std::cout << get_current_time() << " host:" << this->id << " calling schedule()\n";

    if(this->flow_sending == NULL){
        bool scheduled = false;
        double min_finish_time = 999999;

        std::queue<Flow*> flows_tried;

        bool has_short_flow_to_delay = false;
        while(!active_sending_flows.empty()){
            MagicFlow* f = (MagicFlow*)active_sending_flows.top();
            active_sending_flows.pop();

            if(f->finished)
                continue;

            if (f->ack_timeout > get_current_time()){
                if(f->ack_timeout < min_finish_time )
                    min_finish_time = f->ack_timeout;
                flows_tried.push(f);
            }
            else if(((MagicHost*)(f->dst))->recv_busy_until <= get_current_time() + f->get_propa_time() * params.magic_trans_slack
                    || f->size_in_pkt < ((MagicHost*)(f->dst))->flow_receiving->size_in_pkt
                   ){
                //schedule the current flow
                f->virtual_rts_send_count++;
                f->schedule_time = get_current_time();
                f->total_waiting_time += get_current_time() - f->last_pkt_sent_at;
                assert(((MagicHost*)(f->src))->flow_sending == NULL);
                //if(debug_flow(f->id))
                //    std::cout << get_current_time() << "!!!!!!!!! host:" << f->src->id << " flow_sending setting to f " << f->id << "\n";

                ((MagicHost*)(f->src))->flow_sending = f;
                ((MagicHost*)(f->dst))->flow_receiving = f;
                int pkt_to_schd = std::max((unsigned)1, std::min((unsigned)params.reauth_limit, f->remaining_pkt()));
                f->remaining_pkt_this_round = pkt_to_schd;
                ((MagicHost*)(f->dst))->recv_busy_until = get_current_time() + f->get_propa_time() + 0.0000012 * pkt_to_schd;
                scheduled = true;
                break;
            }
            else
            {
                f->virtual_rts_send_count++;
                if(((MagicHost*)(f->dst))->recv_busy_until < min_finish_time ){
                    assert(((MagicHost*)(f->dst))->recv_busy_until - f->get_propa_time() * params.magic_trans_slack > get_current_time());
                    min_finish_time = ((MagicHost*)(f->dst))->recv_busy_until - f->get_propa_time()  * params.magic_trans_slack;
                }

                flows_tried.push(f);

                if(params.magic_delay_scheduling){
                    double slack = ((MagicHost*)(f->dst))->recv_busy_until - (get_current_time() + f->get_propa_time() * params.magic_trans_slack);
                    if(f->size_in_pkt < 10 && slack < params.reauth_limit * 0.0000012 * 0.9){
                        has_short_flow_to_delay = true;
                    }
                    if(active_sending_flows.size() > 0 && active_sending_flows.top()->size_in_pkt > 10 && has_short_flow_to_delay)
                        break;
                }
            }
        }

        //push flows back
        while(!flows_tried.empty()){
            Flow* f = flows_tried.front();
            flows_tried.pop();
            active_sending_flows.push((MagicFlow*)f);
        }

        //has sending flow, but no flow can be scheduled
        if(!scheduled && !active_sending_flows.empty() && min_finish_time < 999999){
            if (min_finish_time > get_current_time())
                add_to_event_queue(new MagicHostScheduleEvent(min_finish_time, this) );
            else
                add_to_event_queue(new MagicHostScheduleEvent(min_finish_time, this) );
        }
    }
}

void MagicHost::reschedule(){
    ((MagicFlow*)this->flow_sending)->last_pkt_sent_at = get_current_time();
    this->flow_sending = NULL;
    this->schedule();
}

void MagicHost::try_send() {
    if(this->host_proc_event == NULL || this->is_host_proc_event_a_timeout){
        send();
    }
}

void MagicHost::send() {

    if(debug_host(this->id))
        std::cout << get_current_time() << " send at host " << this->id << "\n";

    if(this->flow_sending && this->flow_sending->finished)
    {
        if(debug_host(this->id))
            std::cout << get_current_time() << " reschedule() at host " << this->id << " flow finished " << this->flow_sending->id << "\n";
        this->reschedule();
    }



    if(this->flow_sending && ((MagicFlow*)(this->flow_sending))->remaining_pkt_this_round == 0 ){
        if( ((MagicFlow*)(this->flow_sending))->send_count >= (int)ceil(this->flow_sending->size_in_pkt * 1) ){
            ((MagicFlow*)(this->flow_sending))->ack_timeout = get_current_time() + 0.0000095;
        }
        this->active_sending_flows.push((MagicFlow*)(this->flow_sending));
        this->reschedule();
    }



    if(this->queue->busy){
        //queue busy, try send later
        if(this->host_proc_event == NULL){
            QueueProcessingEvent *qpe = this->queue->queue_proc_event;
            uint32_t queue_size = this->queue->bytes_in_queue;
            double td = this->queue->get_transmission_delay(queue_size);
            this->host_proc_event = new HostProcessingEvent(qpe->time + td + INFINITESIMAL_TIME, this);
            this->is_host_proc_event_a_timeout = false;
            add_to_event_queue(this->host_proc_event);
        }
    }else{
        //queue free, try send and schedule another HostProcessingEvent
        if(this->flow_sending)
        {
            if(debug_flow(this->flow_sending->id))
                std::cout << get_current_time() << " flow " << this->flow_sending->id << " send pkt " << this->flow_sending->total_pkt_sent << "\n";
            this->flow_sending->send_pending_data();
            this->is_host_proc_event_a_timeout = false;
        }

    }


}

