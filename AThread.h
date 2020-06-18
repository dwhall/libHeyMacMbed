/* Copyright 2020 Dean Hall.  See LICENSE for details. */
/**
 * AThread - a Thread augmented with a Ticker
 */

#ifndef ATHREAD_H_
#define ATHREAD_H_

#include <stdint.h>
#include "mbed.h"
#include "Thread.h"


enum
{
    EVT_THRD_INIT = 1 << 0,
    EVT_THRD_TERM = 1 << 1,
    EVT_THRD_PRDC = 1 << 2,

    EVT_BASE_LAST = EVT_THRD_PRDC,
    EVT_BASE_ALL = EVT_THRD_INIT | EVT_THRD_TERM | EVT_THRD_PRDC
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
    LowPowerTicker *_ticker;
    uint32_t _period_ms;

private:
    // AThread callbacks
    void _ticker_clbk(void);

    virtual void _main(void);
};

#endif /* ATHREAD_H_ */
