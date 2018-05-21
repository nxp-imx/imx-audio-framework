/*******************************************************************************
 * Copyright (C) 2017 Cadence Design Systems, Inc.
 * Copyright 2018 NXP
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to use this Software with Cadence processor cores only and
 * not with any other processors and platforms, subject to
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

 ******************************************************************************/

#ifndef __XF_OSAL_H
#define __XF_OSAL_H

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <pthread.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>

/*******************************************************************************
 * Lock operation
 ******************************************************************************/

/* ...lock initialization */
static inline void __xf_lock_init(pthread_mutex_t *lock)
{
	pthread_mutex_init(lock, NULL);
}

/* ...lock acquisition */
static inline void __xf_lock(pthread_mutex_t *lock)
{
	pthread_mutex_lock(lock);
}

/* ...lock release */
static inline void __xf_unlock(pthread_mutex_t *lock)
{
	pthread_mutex_unlock(lock);
}

/*******************************************************************************
 * Waiting object
 ******************************************************************************/

/* ...waiting object handle */
struct __xf_wait {
	/* ...conditional variable */
	pthread_cond_t      wait;

	/* ...waiting mutex */
	pthread_mutex_t     mutex;

};

/* ...initialize waiting object */
static inline void __xf_wait_init(struct __xf_wait *w)
{
	pthread_cond_init(&w->wait, NULL);
	pthread_mutex_init(&w->mutex, NULL);
}

/* ...prepare to waiting */
static inline void __xf_wait_prepare(struct __xf_wait *w)
{
	pthread_mutex_lock(&w->mutex);
}

#define __xf_wait_prepare(w)                    \
({                                              \
	TRACE("prepare-wait\n");                    \
	(__xf_wait_prepare)(w);                     \
})

/* ...wait until event is signalled */
static inline int __xf_wait(struct __xf_wait *w, u32 timeout)
{
	struct timespec ts;
	struct timeval  tv;
	int             r;

	/* ...wait with or without timeout (communication mutex is taken) */
	if (!timeout) {
		r = -pthread_cond_wait(&w->wait, &w->mutex);
	} else {
		/* ...get current time */
		gettimeofday(&tv, NULL);

		/* ...set absolute timeout */
		ts.tv_sec = tv.tv_sec + timeout / 1000;
		ts.tv_nsec = tv.tv_usec * 1000 + (timeout % 1000) * 1000000;
		(ts.tv_nsec >= 1000000000 ? ts.tv_sec++,
						ts.tv_nsec -= 1000000000 : 0);

		/* ...wait conditionally with absolute timeout*/
		r = -pthread_cond_timedwait(&w->wait, &w->mutex, &ts);
	}

	/* ...leave with communication mutex taken */
	return r;
}

#define __xf_wait(w, timeout)                   \
({                                              \
	int  __r;                                   \
	TRACE("wait\n");                           \
	__r = (__xf_wait)(w, timeout);              \
	TRACE("resume\n");                         \
	__r;                                        \
})

/* ...wake up waiting handle */
static inline void __xf_wakeup(struct __xf_wait *w)
{
	/* ...take communication mutex before signaling */
	pthread_mutex_lock(&w->mutex);

	/* ...signalling will resume waiting thread */
	pthread_cond_signal(&w->wait);

	/* ...assure that waiting task will not resume */
	pthread_mutex_unlock(&w->mutex);
}

#define __xf_wakeup(w)                          \
({                                              \
	TRACE("wakeup\n");                         \
	(__xf_wakeup)(w);                           \
})

/* ...complete waiting operation */
static inline void __xf_wait_complete(struct __xf_wait *w)
{
	pthread_mutex_unlock(&w->mutex);
}

#define __xf_wait_complete(w)                   \
({                                              \
	TRACE("wait-complete\n");                  \
	(__xf_wait_complete)(w);                    \
})

/*******************************************************************************
 * Thread support
 ******************************************************************************/

/* ...thread creation */
static inline int __xf_thread_create(pthread_t *thread,
				     void * (*f)(void *),
				     void *arg)
{
	pthread_attr_t      attr;
	int                 r;

	/* ...initialize thread attributes - joinable with minimal stack */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);

	/* ...create proxy asynchronous thread managing SHMEM */
	r = -pthread_create(thread, &attr, f, arg);

	/* ...destroy thread attributes */
	pthread_attr_destroy(&attr);

	return r;
}

/* ...terminate thread operation */
static inline int __xf_thread_destroy(pthread_t *thread)
{
	void   *r;

	/* ...tell the thread to terminate */
	pthread_kill(*thread, SIGUSR1);

	/* ...wait until thread terminates */
	pthread_join(*thread, &r);

	/* ...return final status */
	return (int)(intptr_t)r;
}

#endif
