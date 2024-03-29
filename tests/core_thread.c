/*
 * Astra: Unit tests
 * http://cesbo.com/astra
 *
 * Copyright (C) 2015-2016, Artem Kharitonov <artem@3phase.pw>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "unit_tests.h"
#include <core/list.h>
#include <core/mainloop.h>
#include <core/thread.h>
#include <core/mutex.h>

typedef struct
{
    asc_thread_t *thread;
    asc_mutex_t *mutex;
    asc_list_t *list;
    unsigned int id;
    unsigned int value;
} thread_test_t;

/* set variable and exit */
static void set_value_proc(void *arg)
{
    thread_test_t *const tt = (thread_test_t *)arg;

    tt->value = 0xdeadbeef;
    asc_usleep(150 * 1000); /* 150ms */
}

static void set_value_close(void *arg)
{
    thread_test_t *const tt = (thread_test_t *)arg;

    asc_main_loop_shutdown();
    asc_thread_join(tt->thread);
}

START_TEST(set_value)
{
    thread_test_t tt;
    memset(&tt, 0, sizeof(tt));

    asc_thread_t *const thr = asc_thread_init();
    ck_assert(thr != NULL);

    tt.thread = thr;

    asc_thread_start(thr, &tt, set_value_proc, set_value_close);
    ck_assert(asc_main_loop_run() == false);
    ck_assert(tt.value == 0xdeadbeef);
}
END_TEST

/* multiple threads adding items to list */
#define PRODUCER_THREADS 10
#define PRODUCER_ITEMS 100

static unsigned int producer_running;

static void producer_proc(void *arg)
{
    thread_test_t *const tt = (thread_test_t *)arg;

    for (size_t i = 0; i < PRODUCER_ITEMS; i++)
    {
        const unsigned int item = (tt->id << 16) | (tt->value++ & 0xFFFF);

        asc_mutex_lock(tt->mutex);
        asc_list_insert_tail(tt->list, (void *)((intptr_t)item));
        asc_mutex_unlock(tt->mutex);

        asc_usleep(1000);
    }

    asc_mutex_lock(tt->mutex);
    if (--producer_running == 0)
        asc_main_loop_shutdown();
    asc_mutex_unlock(tt->mutex);
}

START_TEST(producers)
{
    thread_test_t tt[PRODUCER_THREADS];

    asc_list_t *const list = asc_list_init();
    ck_assert(list != NULL);

    asc_mutex_t mutex;
    asc_mutex_init(&mutex);

    /* lock threads until we complete startup */
    asc_mutex_lock(&mutex);

    producer_running = 0;
    for (size_t i = 0; i < ASC_ARRAY_SIZE(tt); i++)
    {
        tt[i].thread = asc_thread_init();
        ck_assert(tt[i].thread != NULL);

        tt[i].list = list;
        tt[i].mutex = &mutex;
        tt[i].id = i;
        tt[i].value = 0;

        asc_thread_start(tt[i].thread, &tt[i], producer_proc, NULL);
        producer_running++;
    }

    /* start "production" */
    asc_mutex_unlock(&mutex);
    ck_assert(asc_main_loop_run() == false);
    ck_assert(producer_running == 0);

    /* check total item count */
    ck_assert(asc_list_size(list) == PRODUCER_THREADS * PRODUCER_ITEMS);

    /* check item order */
    unsigned int counts[PRODUCER_THREADS] = { 0 };
    asc_list_for(list)
    {
        void *const ptr = asc_list_data(list);
        const unsigned int data = (unsigned)((intptr_t)ptr);

        const unsigned int id = data >> 16;
        const unsigned int value = data & 0xFFFF;

        ck_assert(counts[id]++ == value);
    }
}
END_TEST

/* thread that never gets started */
START_TEST(no_start)
{
    asc_thread_t *const thr = asc_thread_init();
    __uarg(thr);
}
END_TEST

/* buggy cleanup routine */
static void no_destroy_proc(void *arg)
{
    __uarg(arg);
    asc_usleep(50 * 1000); /* 50ms */
}

static void no_destroy_close(void *arg)
{
    __uarg(arg);
    asc_main_loop_shutdown();
}

START_TEST(no_destroy)
{
    asc_thread_t *const thr = asc_thread_init();
    asc_thread_start(thr, NULL, no_destroy_proc, no_destroy_close);

    ck_assert(asc_main_loop_run() == false);
}
END_TEST

/* main thread wake up */
static uint64_t wake_time = 0;

static void wake_up_cb(void *arg)
{
    __uarg(arg);

    const uint64_t now = asc_utime();
    ck_assert_msg(now - wake_time < (5 * 1000), "didn't wake up within 5ms");
}

static void wake_up_proc(void *arg)
{
    __uarg(arg);

    /*
     * TODO: add APIs for conditional variables, simulate data exchange
     *       between main and auxiliary threads.
     */
    asc_usleep(50 * 1000); /* 50ms */

    wake_time = asc_utime();
    asc_job_queue(NULL, wake_up_cb, NULL);
    asc_wake();
}

static void wake_up_close(void *arg)
{
    asc_thread_t *const thr = (asc_thread_t *)arg;

    asc_thread_join(thr);
    asc_wake_close();

    asc_main_loop_shutdown();
}

START_TEST(wake_up)
{
    asc_thread_t *const thr = asc_thread_init();

    asc_wake_open();
    asc_thread_start(thr, thr, wake_up_proc, wake_up_close);

    ck_assert(asc_main_loop_run() == false);
}
END_TEST

/* mutex timed lock */
#define TL_P1_WAIT (100 * 1000) /* 100ms */
#define TL_P2_WAIT (200 * 1000) /* 200ms */
#define TL_MS 500 /* timeout in msecs */

#define TL_CHECK_TIME(__start, __timeout) \
    do { \
        const uint64_t __bench = asc_utime() - (__start); \
        ck_assert(__bench <= (__timeout) * 1.3 \
                  && __bench >= (__timeout) * 0.7); \
    } while (0)

static asc_mutex_t tl_mutex1;
static asc_mutex_t tl_mutex2;
static asc_mutex_t tl_mutex3;

static void timedlock_proc(void *arg)
{
    __uarg(arg);

    asc_mutex_lock(&tl_mutex2); /* locked M2 */

    /* part 1 */
    const uint64_t start = asc_utime();
    const bool ret = asc_mutex_timedlock(&tl_mutex1, TL_MS); /* locked M1 */
    ck_assert(ret == true);
    TL_CHECK_TIME(start, TL_P1_WAIT);

    /* part 2 */
    asc_usleep(TL_P2_WAIT);
    asc_mutex_unlock(&tl_mutex2); /* released M2 */

    /* part 3 */
    asc_mutex_lock(&tl_mutex3); /* locked M3 */
    asc_mutex_unlock(&tl_mutex1); /* released M1 */
    asc_mutex_unlock(&tl_mutex3); /* released M3 */
}

START_TEST(timedlock)
{
    asc_mutex_init(&tl_mutex1);
    asc_mutex_init(&tl_mutex2);
    asc_mutex_init(&tl_mutex3);

    asc_mutex_lock(&tl_mutex3); /* locked M3 */

    /* part 1: aux thread waits for main */
    asc_mutex_lock(&tl_mutex1); /* locked M1 */
    asc_thread_t *thr = asc_thread_init();
    asc_thread_start(thr, NULL, timedlock_proc, NULL);
    asc_usleep(TL_P1_WAIT);
    asc_mutex_unlock(&tl_mutex1); /* released M1 */

    /* part 2: main thread waits for aux */
    uint64_t start = asc_utime();
    bool ret = asc_mutex_timedlock(&tl_mutex2, TL_MS); /* locked M2 */
    ck_assert(ret == true);
    TL_CHECK_TIME(start, TL_P2_WAIT);

    /* part 3: timedlock failure */
    start = asc_utime();
    ret = asc_mutex_timedlock(&tl_mutex1, TL_MS); /* timeout for M1 */
    ck_assert(ret == false);
    TL_CHECK_TIME(start, TL_MS * 1000);

    /* join thread and clean up */
    asc_mutex_unlock(&tl_mutex2); /* released M2 */
    asc_mutex_unlock(&tl_mutex3); /* released M3 */
    asc_thread_join(thr);

    asc_mutex_destroy(&tl_mutex1);
    asc_mutex_destroy(&tl_mutex2);
    asc_mutex_destroy(&tl_mutex3);
}
END_TEST

Suite *core_thread(void)
{
    Suite *const s = suite_create("thread");

    TCase *const tc = tcase_create("default");
    tcase_add_checked_fixture(tc, lib_setup, lib_teardown);

    tcase_add_test(tc, set_value);
    tcase_add_test(tc, producers);
    tcase_add_test(tc, no_start);
    tcase_add_test(tc, wake_up);
    tcase_add_test(tc, timedlock);

    if (can_fork != CK_NOFORK)
    {
        tcase_set_timeout(tc, 5);
        tcase_add_exit_test(tc, no_destroy, EXIT_ABORT);
    }

    suite_add_tcase(s, tc);

    return s;
}
