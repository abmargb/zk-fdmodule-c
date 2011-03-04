/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "phiaccrual_failuredetector.h"
#include "failuredetector.h"
#include "fd_hashtable.h"
#include "../hashtable/hashtable.h"
#include "interarrival_window.h"
#include "fd_opt_parser.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#define DEF_THRESHOLD 2.
#define DEF_MIN_WINDOW_SIZE 500

typedef struct {
	char* id;
	long timeout;
	long last_heard;
	long last_sent;
	long eta; //interrogation interval
	interarrival_window_t *sampling_window;

} monitored_t;

static void destroy_monitored(monitored_t *m) {
	destroy_window(m->sampling_window);
	free(m);
}

void phiaccrual_reg_monitored(phiaccrualfd_t *this, char *id, long now, long timeout) {
	monitored_t *m;
	m = calloc(1, sizeof(*m));

	m->id = id;
	m->last_heard = now;
	m->last_sent = now;
	m->timeout = timeout;
	m->eta = timeout / 2;
	m->sampling_window = init_window();

	fd_hashtable_insert(this->monitoreds, id, m);
}

void phiaccrual_set_to(phiaccrualfd_t *this, char *id, long timeout) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->timeout = timeout;
}

long phiaccrual_get_to(phiaccrualfd_t *this, char *id) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return m->timeout;
}

static void update_timeout(phiaccrualfd_t *this, monitored_t* m, long now) {
	long mean = (long) m->sampling_window->mean;
	m->timeout = (long) (-log(pow(10, -this->threshold)) * mean);
}

void phiaccrual_msg_rcv(phiaccrualfd_t *this, char *id, long now, int type) {
	monitored_t* m = hashtable_search(this->monitoreds, id);

	if (type == PING) {
		add_ping(m->sampling_window, now);
		if (m->sampling_window->size >= this->min_window_size) {
			update_timeout(this, m, now);
		}
	}

	m->last_heard = now;
}

void phiaccrual_msg_sent(phiaccrualfd_t *this, char *id, long now, int type) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->last_sent = now;
}

int phiaccrual_failed(phiaccrualfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return now > m->last_heard + phiaccrual_get_to(this, id);
}

long phiaccrual_get_idle(phiaccrualfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return now - m->last_heard;
}

int phiaccrual_time_next_ping(phiaccrualfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return m->eta - (now - m->last_sent);
}

int phiaccrual_should_ping(phiaccrualfd_t *this, char *id, long now) {
	return phiaccrual_time_next_ping(this, id, now) <= 0;
}

void phiaccrual_release(phiaccrualfd_t *this, char *id) {
	monitored_t* m = hashtable_remove(this->monitoreds, id);
	destroy_monitored(m);
}

void phiaccrual_set_ping_interval(phiaccrualfd_t *this, char *id, long interval) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->eta = interval;
}

phiaccrualfd_t* phiaccrualfd_init_params(double threshold, int min_window_size) {
	phiaccrualfd_t *p_fd;
	p_fd = calloc(1, sizeof(*p_fd));

	p_fd->fdetector.message_received = (void*)phiaccrual_msg_rcv;
	p_fd->fdetector.message_sent = (void*)phiaccrual_msg_sent;
	p_fd->fdetector.register_monitored = (void*)phiaccrual_reg_monitored;
	p_fd->fdetector.set_timeout = (void*)phiaccrual_set_to;
	p_fd->fdetector.get_timeout = (void*)phiaccrual_get_to;
	p_fd->fdetector.is_failed = (void*)phiaccrual_failed;
	p_fd->fdetector.get_idle_time = (void*)phiaccrual_get_idle;
	p_fd->fdetector.get_time_to_next_ping = (void*)phiaccrual_time_next_ping;
	p_fd->fdetector.should_ping = (void*)phiaccrual_should_ping;
	p_fd->fdetector.release_monitored = (void*)phiaccrual_release;
	p_fd->fdetector.set_ping_interval = (void*)phiaccrual_set_ping_interval;

	p_fd->monitoreds = create_fd_hashtable();
	p_fd->threshold = threshold;
	p_fd->min_window_size = min_window_size;
	return p_fd;
}

phiaccrualfd_t* phiaccrualfd_init(struct hashtable *params_table) {
	return phiaccrualfd_init_params(
			parse_double(DEF_THRESHOLD, hashtable_search(params_table, "threshold")),
			parse_long(DEF_MIN_WINDOW_SIZE, hashtable_search(params_table, "minwindowsize")));
}

phiaccrualfd_t* phiaccrualfd_init_def() {
	return phiaccrualfd_init_params(DEF_THRESHOLD, DEF_MIN_WINDOW_SIZE);
}
