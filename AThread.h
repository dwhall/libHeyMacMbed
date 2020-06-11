/* Copyright 2020 Dean Hall.  See LICENSE for details. */
/**
 * AThread - a Thread augmented with a Ticker, Event flags
 */

#ifndef ATHREAD_H_
#define ATHREAD_H_

#include <stdint.h>
#include "mbed.h"
#include "Thread.h"
#include "EventFlags.h"


enum
{
    EVT_INIT = 1 << 0,
    EVT_TERM = 1 << 1,
    EVT_PRDC = 1 << 2,

    EVT_BASE_LAST = EVT_PRDC,
    EVT_BASE_ALL = EVT_INIT | EVT_TERM | EVT_PRDC
};


class AThread
{
public:
    AThread
        (
        uint32_t const stack_size = OS_STACK_SIZE,
        uint32_t const period = 0
        );
    ~AThread();
    void thread_start(void);

protected:
    rtos::Thread *_thread;
    rtos::EventFlags *_evt_flags;
    LowPowerTicker *_ticker;
    uint32_t _period_ms;

private:
    // AThread callbacks
    void _ticker_clbk(void);

    virtual void _main(void);
};

#endif /* ATHREAD_H_ */
