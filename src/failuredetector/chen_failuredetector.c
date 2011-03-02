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

#include "chen_failuredetector.h"
#include "failuredetector.h"
#include "fd_hashtable.h"
#include "../hashtable/hashtable.h"
#include "interarrival_window.h"

#include <string.h>
#include <stdlib.h>

#define DEF_ALPHA 5000l

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

void chen_reg_monitored(chenfd_t *this, char *id, long now, long timeout) {
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

void chen_set_to(chenfd_t *this, char *id, long timeout) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->timeout = timeout;
}

long chen_get_to(chenfd_t *this, char *id) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return m->timeout;
}

static void update_timeout(chenfd_t *this, monitored_t* m, long now) {
	if (m->sampling_window->size > 0) {
		double ea = now + m->sampling_window->mean;
		long t = (long)ea + this->alpha;
		m->timeout = t - now;
	}
}

void chen_msg_rcv(chenfd_t *this, char *id, long now, int type) {
	monitored_t* m = hashtable_search(this->monitoreds, id);

	if (type == PING) {
		add_ping(m->sampling_window, now);
		update_timeout(this, m, now);
	}

	m->last_heard = now;
}

void chen_msg_sent(chenfd_t *this, char *id, long now, int type) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->last_sent = now;
}

int chen_failed(chenfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return now > m->last_heard + chen_get_to(this, id);
}

long chen_get_idle(chenfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return now - m->last_heard;
}

int chen_time_next_ping(chenfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return m->eta - (now - m->last_sent);
}

int chen_should_ping(chenfd_t *this, char *id, long now) {
	return chen_time_next_ping(this, id, now) <= 0;
}

void chen_release(chenfd_t *this, char *id) {
	monitored_t* m = hashtable_remove(this->monitoreds, id);
	destroy_monitored(m);
}

void chen_set_ping_interval(chenfd_t *this, char *id, long interval) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->eta = interval;
}

chenfd_t* chenfd_init() {
	chenfd_t *p_fd;
	p_fd = calloc(1, sizeof(*p_fd));

	p_fd->fdetector.message_received = (void*)chen_msg_rcv;
	p_fd->fdetector.message_sent = (void*)chen_msg_sent;
	p_fd->fdetector.register_monitored = (void*)chen_reg_monitored;
	p_fd->fdetector.set_timeout = (void*)chen_set_to;
	p_fd->fdetector.get_timeout = (void*)chen_get_to;
	p_fd->fdetector.is_failed = (void*)chen_failed;
	p_fd->fdetector.get_idle_time = (void*)chen_get_idle;
	p_fd->fdetector.get_time_to_next_ping = (void*)chen_time_next_ping;
	p_fd->fdetector.should_ping = (void*)chen_should_ping;
	p_fd->fdetector.release_monitored = (void*)chen_release;
	p_fd->fdetector.set_ping_interval = (void*)chen_set_ping_interval;

	p_fd->monitoreds = create_fd_hashtable();
	p_fd->alpha = DEF_ALPHA;
	return p_fd;
}

chenfd_t* chenfd_init_params(long alpha) {
	chenfd_t *p_fd = chenfd_init();
	p_fd->alpha = alpha;
	return p_fd;
}
