/*
* Copyright (c) 2015-2021 Cadence Design Systems Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*******************************************************************************
 * osal-thread.h
 *
 * OS absraction layer (minimalistic) for Linux
 ******************************************************************************/

/*******************************************************************************
 * Includes
 ******************************************************************************/
#ifndef _OSAL_THREAD_H
#define _OSAL_THREAD_H

#include <string.h>
#include <stdint.h>
#include <semaphore.h>
#include <unistd.h>
/*******************************************************************************
 * Tracing primitive
 ******************************************************************************/

#define __xf_puts(str)                  \
    puts((str))


/*******************************************************************************
 * Lock operation
 ******************************************************************************/

/* ...lock definition */
typedef sem_t xf_lock_t;

/* ...lock initialization */
static inline void __xf_lock_init(xf_lock_t *lock)
{
	sem_init(lock, 0, 1);
}

/* ...lock deletion */
static inline void __xf_lock_destroy(xf_lock_t *lock)
{
	sem_destroy(lock);
}

/* ...lock acquisition */
static inline void __xf_lock(xf_lock_t *lock)
{
	sem_wait(lock);
}

/* ...lock release */
static inline void __xf_unlock(xf_lock_t *lock)
{
	sem_post(lock);
}

/*******************************************************************************
 * Event support
 ******************************************************************************/

typedef int     xf_event_t;

static inline void __xf_event_init(xf_event_t *event, uint32_t mask)
{
}

static inline void __xf_event_destroy(xf_event_t *event)
{
}

static inline uint32_t __xf_event_get(xf_event_t *event)
{
    uint32_t rv = 0;

    return rv;
}

static inline void __xf_event_set(xf_event_t *event, uint32_t mask)
{
}

static inline void __xf_event_set_isr(xf_event_t *event, uint32_t mask)
{
}

static inline void __xf_event_clear(xf_event_t *event, uint32_t mask)
{
}

static inline void __xf_event_wait_any(xf_event_t *event, uint32_t mask)
{
}

static inline void __xf_event_wait_all(xf_event_t *event, uint32_t mask)
{
}


/*******************************************************************************
 * Thread support
 ******************************************************************************/
#include <stdint.h>
#include <pthread.h>
#include <signal.h>

/* ...thread handle definition */
typedef pthread_t           xf_thread_t;
typedef void *xf_entry_t(void *);


/* This is dummy for Linux */
static inline int __xf_thread_init(xf_thread_t *thread)
{
    return 0;
}

/* ...thread creation
 *
 * return: 0 -- OK, negative -- OS-specific error code
 */

static inline int __xf_thread_create(xf_thread_t *thread, xf_entry_t *f,
                                     void *arg, const char *name, void *stack,
                                     unsigned int stack_size, int priority)
{  
    return pthread_create(thread, 0, f, arg);
}

static inline void __xf_thread_yield(void)
{
    sched_yield();
}

static inline int __xf_thread_cancel(xf_thread_t *thread)
{
    return pthread_cancel(*thread);
}

/* TENA-2117*/
static inline int __xf_thread_join(xf_thread_t *thread, int32_t * p_exitcode)
{
    int    r;

    if (*thread != 0UL)
        r = pthread_join(*thread, (void **)&p_exitcode);

    return r;
}

/* ...terminate thread operation */
static inline int __xf_thread_destroy(xf_thread_t *thread)
{
    int    r;
    
    if (*thread != 0UL)
        r = pthread_kill(*thread, SIGUSR1);

    /* ...return final status */
    return r;
}

static inline const char *__xf_thread_name(xf_thread_t *thread)
{
    return NULL;
}

/* ... Put calling thread to sleep for at least the specified number of msec */
static inline int32_t __xf_thread_sleep_msec(uint64_t msecs)
{
    int32_t    r;
    
    usleep(msecs * 1000);
    
    /* ...return final status */
    return r;
}

/* ... state of the thread */
#define XF_THREAD_STATE_INVALID (0)
#define XF_THREAD_STATE_READY	(1)
#define XF_THREAD_STATE_RUNNING	(2)
#define XF_THREAD_STATE_BLOCKED (3)
#define XF_THREAD_STATE_EXITED  (4) 

static inline int32_t __xf_thread_get_state (xf_thread_t *thread)
{
    int32_t    r = 0;
    
    /* ...return final status */
    return r;
}

#endif
