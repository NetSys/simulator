#include <assert.h>
#include <stdlib.h>

#include "../coresim/event.h"
#include "../coresim/flow.h"
#include "../coresim/packet.h"
#include "../coresim/debug.h"

#include "capabilityflow.h"
#include "capabilityhost.h"
#include "factory.h"

#include "../run/params.h"

extern double get_current_time();
extern void add_to_event_queue(Event*);
extern DCExpParams params;

CapabilityProcessingEvent::CapabilityProcessingEvent(double time, CapabilityHost *h, bool is_timeout)
    : Event(CAPABILITY_PROCESSING, time) {
        this->host = h;
        this->is_timeout_evt = is_timeout;
    }

CapabilityProcessingEvent::~CapabilityProcessingEvent() {
    if (host->capa_proc_evt == this) {
        host->capa_proc_evt = NULL;
    }
}

void CapabilityProcessingEvent::process_event() {
    this->host->capa_proc_evt = NULL;
    this->host->send_capability();
}

SenderNotifyEvent::SenderNotifyEvent(double time, CapabilityHost* h) : Event(SENDER_NOTIFY, time) {
    this->host = h;
}

SenderNotifyEvent::~SenderNotifyEvent() {
}

void SenderNotifyEvent::process_event() {
    this->host->sender_notify_evt = NULL;
    this->host->notify_flow_status();
}

bool CapabilityFlowComparator::operator() (CapabilityFlow* a, CapabilityFlow* b){
    //return a->remaining_pkts_at_sender > b->remaining_pkts_at_sender;
    if(params.deadline && params.schedule_by_deadline) {
        return a->deadline > b->deadline;
    }
    else {
        if (a->remaining_pkts_at_sender > b->remaining_pkts_at_sender)
            return true;
        else if (a->remaining_pkts_at_sender == b->remaining_pkts_at_sender)
            return a->start_time > b->start_time;
        else
            return false;
        //return a->latest_data_pkt_send_time > b->latest_data_pkt_send_time;
        //return a->start_time > b->start_time;
    }
}

bool CapabilityFlowComparatorAtReceiver::operator() (CapabilityFlow* a, CapabilityFlow* b){
    //return a->size_in_pkt > b->size_in_pkt;
    if(params.deadline && params.schedule_by_deadline) {
        return a->deadline > b->deadline;
    }
    else {
        if (a->notified_num_flow_at_sender > b->notified_num_flow_at_sender)
            return true;
        else if(a->notified_num_flow_at_sender == b->notified_num_flow_at_sender) {
            if(a->remaining_pkts() > b->remaining_pkts())
                return true;
            else if (a->remaining_pkts() == b->remaining_pkts())
                return a->start_time > b->start_time; //TODO: this is cheating. but not a big problem
            else
                return false;
        }
        else
            return false;
        //return a->latest_cap_sent_time > b->latest_cap_sent_time;
        //return a->start_time > b->start_time;
    }
}

CapabilityHost::CapabilityHost(uint32_t id, double rate, uint32_t queue_type) : SchedulingHost(id, rate, queue_type) {
    this->capa_proc_evt = NULL;
    this->hold_on = 0;
    this->total_capa_schd_evt_count = 0;
    this->could_better_schd_count = 0;
    this->sender_notify_evt = NULL;
    this->host_type = CAPABILITY_HOST;
}

void CapabilityHost::start_capability_flow(CapabilityFlow* f) {
    if(debug_flow(f->id) || debug_host(this->id))
        std::cout 
            << get_current_time() 
            << " flow " << f->id 
            << " src " << this->id
            << " curr q size " << this->queue->bytes_in_queue 
            << " num flows " << this->active_receiving_flows.size() <<"\n";

    this->active_sending_flows.push(f);
    f->send_rts_pkt();
    if(f->has_capability() && ((CapabilityHost*)(f->src))->host_proc_event == NULL) {
        ((CapabilityHost*)(f->src))->schedule_host_proc_evt();
    }
    if(CAPABILITY_NOTIFY_BLOCKING && this->sender_notify_evt == NULL)
        this->notify_flow_status();
}

void CapabilityHost::schedule_host_proc_evt(){
    assert(this->host_proc_event == NULL);

    double qpe_time = 0;
    double td_time = 0;
    if(this->queue->busy){
        qpe_time = this->queue->queue_proc_event->time;
    }
    else{
        qpe_time = get_current_time();
    }

    uint32_t queue_size = this->queue->bytes_in_queue;
    td_time = this->queue->get_transmission_delay(queue_size);

    this->host_proc_event = new HostProcessingEvent(qpe_time + td_time + INFINITESIMAL_TIME, this);
    add_to_event_queue(this->host_proc_event);
}

void CapabilityHost::schedule_capa_proc_evt(double time, bool is_timeout)
{
    assert(this->capa_proc_evt == NULL);
    this->capa_proc_evt = new CapabilityProcessingEvent(get_current_time() + time + INFINITESIMAL_TIME, this, is_timeout);
    add_to_event_queue(this->capa_proc_evt);
}

void CapabilityHost::schedule_sender_notify_evt()
{
    assert(this->sender_notify_evt == NULL);
    this->sender_notify_evt = new SenderNotifyEvent(get_current_time() + (params.get_full_pkt_tran_delay() * 20) + INFINITESIMAL_TIME, this);
    add_to_event_queue(this->sender_notify_evt);
}


//should only be called in HostProcessingEvent::process()
void CapabilityHost::send(){
    assert(this->host_proc_event == NULL);


    if(this->queue->busy)
    {
        schedule_host_proc_evt();
    }
    else
    {
        bool pkt_sent = false;
        std::queue<CapabilityFlow*> flows_tried;
        while(!this->active_sending_flows.empty()){
            if(this->active_sending_flows.top()->finished){
                this->active_sending_flows.pop();
                continue;
            }

            CapabilityFlow* top_flow = this->active_sending_flows.top();


            if(top_flow->has_capability())
            {
                top_flow->send_pending_data();
                pkt_sent = true;
                break;
            }
            else{
                this->active_sending_flows.pop();
                flows_tried.push(top_flow);
            }

        }

        //code for 4th priority level
        if(params.capability_fourth_level && !pkt_sent && flows_tried.size() > 0){
            std::vector<CapabilityFlow*> candidate;
            for(int i = 0; i < flows_tried.size(); i++){
                if(flows_tried.front()->size_in_pkt > params.capability_initial)
                    candidate.push_back(flows_tried.front());
            }

            if(candidate.size()){
                int f_index = rand()%candidate.size();
                candidate[f_index]->send_pending_data_low_prio();
            }

        }

        while(!flows_tried.empty())
        {
            CapabilityFlow* f = flows_tried.front();
            flows_tried.pop();
            this->active_sending_flows.push(f);
        }

    }

}

void CapabilityHost::notify_flow_status()
{
    std::queue<CapabilityFlow*> flows_tried;
    int num_large_flow = 0;

    while(!this->active_sending_flows.empty())
    {
        CapabilityFlow* f = this->active_sending_flows.top();
        this->active_sending_flows.pop();
        if(!f->finished){
            flows_tried.push(f);
            if(f->size_in_pkt > params.capability_initial)
                num_large_flow++;
        }
    }

    while(!flows_tried.empty()){
        this->active_sending_flows.push(flows_tried.front());
        if(flows_tried.front()->size_in_pkt > params.capability_initial)
            flows_tried.front()->send_notify_pkt(num_large_flow>2?2:1);
        flows_tried.pop();
    }

    if(!this->active_sending_flows.empty())
        this->schedule_sender_notify_evt();
}

bool CapabilityHost::check_better_schedule(CapabilityFlow* f)
{
    return ((CapabilityHost*)f->src)->active_sending_flows.top() == f;
}

bool CapabilityHost::is_sender_idle(){
    bool idle = true;
    std::queue<CapabilityFlow*> flows_tried;
    while(!this->active_sending_flows.empty())
    {
        CapabilityFlow* f = this->active_sending_flows.top();
        this->active_sending_flows.pop();
        flows_tried.push(f);
        if(f->has_capability()){
            idle = false;
            break;
        }
    }
    while(!flows_tried.empty())
    {
        this->active_sending_flows.push(flows_tried.front());
        flows_tried.pop();
    }

    return idle;
}

void CapabilityHost::send_capability(){
    //if(debug_host(this->id))
    //    std::cout << get_current_time() << " CapabilityHost::send_capability() at host " << this->id << "\n";
    assert(capa_proc_evt == NULL);

    bool capability_sent = false;
    bool could_schd_better = false;
    this->total_capa_schd_evt_count++;
    std::queue<CapabilityFlow*> flows_tried;
    double closet_timeout = 999999;

    if(CAPABILITY_HOLD && this->hold_on > 0){
        hold_on--;
        capability_sent = true;
    }

    while(!this->active_receiving_flows.empty() && !capability_sent)
    {
        CapabilityFlow* f = this->active_receiving_flows.top();
        this->active_receiving_flows.pop();
        //if(debug_flow(f->id))
        //    std::cout << get_current_time() << " pop out flow " << f->id << "\n";


        if(f->finished_at_receiver)
        {
            continue;
        }
        flows_tried.push(f);

        //not yet timed out, shouldn't send
        if(f->redundancy_ctrl_timeout > get_current_time()){
            if(check_better_schedule(f))
                could_schd_better = true;
            if(f->redundancy_ctrl_timeout < closet_timeout)
            {
                closet_timeout = f->redundancy_ctrl_timeout;
            }
        }
        //ok to send
        else
        {
            //just timeout, reset timeout state
            if(f->redundancy_ctrl_timeout > 0)
            {
                f->redundancy_ctrl_timeout = -1;
                f->capability_goal += f->remaining_pkts();
            }

            if(f->capability_gap() > params.capability_window)
            {
                if(get_current_time() >= f->latest_cap_sent_time + params.capability_window_timeout * params.get_full_pkt_tran_delay())
                    f->relax_capability_gap();
                else{
                    if(f->latest_cap_sent_time + params.capability_window_timeout * params.get_full_pkt_tran_delay() < closet_timeout)
                    {
                        closet_timeout = f->latest_cap_sent_time + params.capability_window_timeout* params.get_full_pkt_tran_delay();
                    }
                }

            }


            if(f->capability_gap() <= params.capability_window)
            {
                f->send_capability_pkt();
                capability_sent = true;

                if(f->capability_count == f->capability_goal){
                    f->redundancy_ctrl_timeout = get_current_time() + params.capability_resend_timeout * params.get_full_pkt_tran_delay();
                }

                break;
            }


        }
    }

    while(!flows_tried.empty()){
        CapabilityFlow* tf = flows_tried.front();
        flows_tried.pop();
        this->active_receiving_flows.push(tf);
    }



    if(capability_sent)// pkt sent
    {
        this->schedule_capa_proc_evt(params.get_full_pkt_tran_delay(1500/* + 40*/), false);
    }
    else if(closet_timeout < 999999) //has unsend flow, but its within timeout
    {
        assert(closet_timeout > get_current_time());
        this->schedule_capa_proc_evt(closet_timeout - get_current_time(), true);
    }
    else{
        //do nothing, no unfinished flow
    }

    if(could_schd_better)
        this->could_better_schd_count++;
}

