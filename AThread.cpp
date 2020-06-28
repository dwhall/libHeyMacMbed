/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <stdint.h>
#include "mbed.h"
#include "Thread.h"

#include "AThread.h"


AThread::AThread
    (
    uint32_t const stack_size,
    uint32_t const period_ms,
    char const * thrd_name
    )
{
    _thread = new rtos::Thread(osPriorityNormal, stack_size, nullptr, thrd_name);
    _period_ms = period_ms;
    _ticker = nullptr;
}


AThread::~AThread()
{
}


/** Start the thread running the derived class's _main method */
void AThread::thread_start(void)
{
    _thread->start(callback(this, &AThread::_main));
    _thread->flags_set(EVT_THRD_INIT);
}

void AThread::_main(void)
{
}


void AThread::_ticker_clbk(void)
{
    /* Post the periodic event flag */
    _thread->flags_set(EVT_THRD_PRDC);
}