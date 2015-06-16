#include "otherevents.h"

HostProcessingEvent::HostProcessingEvent(double time, SchedulingHost *h) : Event(HOST_PROCESSING, time) {
    this->host = h;
}

HostProcessingEvent::~HostProcessingEvent() {
    if (host->host_proc_event == this) {
        host->host_proc_event = NULL;
    }
}

void HostProcessingEvent::process_event() {
    this->host->host_proc_event = NULL;
    this->host->send();
}


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


SenderNotifyEvent::SenderNotifyEvent(double time, CapabilityHost* h) : Event(SENDER_NOTIFY, time) {
    this->host = h;
}

SenderNotifyEvent::~SenderNotifyEvent() {
}

void SenderNotifyEvent::process_event() {
    this->host->sender_notify_evt = NULL;
    this->host->notify_flow_status();
}


ArbiterProcessingEvent::ArbiterProcessingEvent(double time, FastpassArbiter* a) : Event(ARBITER_PROCESSING, time) {
    this->arbiter = a;
}

ArbiterProcessingEvent::~ArbiterProcessingEvent() {
}

void ArbiterProcessingEvent::process_event() {
    this->arbiter->arbiter_proc_evt = NULL;
    this->arbiter->schedule_epoch();
}


FastpassFlowProcessingEvent::FastpassFlowProcessingEvent(double time, FastpassFlow* f)
    : Event(FASTPASS_FLOW_PROCESSING, time) {
    this->flow = f;
}

FastpassFlowProcessingEvent::~FastpassFlowProcessingEvent() {
}

void FastpassFlowProcessingEvent::process_event() {
    this->flow->send_data_pkt();
}


FastpassTimeoutEvent::FastpassTimeoutEvent(double time, FastpassFlow* f) : Event(FASTPASS_TIMEOUT, time) {
    this->flow = f;
}

FastpassTimeoutEvent::~FastpassTimeoutEvent() {
}

void FastpassTimeoutEvent::process_event() {
    this->flow->fastpass_timeout();
}

