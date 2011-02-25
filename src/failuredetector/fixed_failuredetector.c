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

#include "fixed_failuredetector.h"
#include "failuredetector.h"
#include "fd_hashtable.h"
#include "../hashtable/hashtable.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	char* id;
	long timeout;
	long last_heard;
	long last_sent;
	long ping_interval;
} monitored_t;

void fixed_reg_monitored(fixedfd_t *this, char *id, long now, long timeout) {
	monitored_t *m;
	m = calloc(1, sizeof(*m));

	m->id = id;
	m->last_heard = now;
	m->last_sent = now;
	m->timeout = timeout;
	m->ping_interval = timeout / 2;

	fd_hashtable_insert(this->monitoreds, id, m);
}

void fixed_set_to(fixedfd_t *this, char *id, long timeout) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->timeout = timeout;
}

long fixed_get_to(fixedfd_t *this, char *id) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return m->timeout;
}

void fixed_msg_rcv(fixedfd_t *this, char *id, long now, int type) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->last_heard = now;
}

void fixed_msg_sent(fixedfd_t *this, char *id, long now, int type) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->last_sent = now;
}

int fixed_failed(fixedfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return now > m->last_heard + fixed_get_to(this, id);
}

long fixed_get_idle(fixedfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return now - m->last_heard;
}

int fixed_time_next_ping(fixedfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return m->ping_interval - (now - m->last_sent);
}

int fixed_should_ping(fixedfd_t *this, char *id, long now) {
	return fixed_time_next_ping(this, id, now) <= 0;
}

void fixed_release(fixedfd_t *this, char *id) {
	monitored_t* m = hashtable_remove(this->monitoreds, id);
	free(m);
}

void fixed_set_ping_interval(fixedfd_t *this, char *id, long interval) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->ping_interval = interval;
}

fixedfd_t* fixedfd_init() {
	fixedfd_t *p_fd;
	p_fd = calloc(1, sizeof(*p_fd));

	p_fd->fdetector.message_received = (void*)fixed_msg_rcv;
	p_fd->fdetector.message_sent = (void*)fixed_msg_sent;
	p_fd->fdetector.register_monitored = (void*)fixed_reg_monitored;
	p_fd->fdetector.set_timeout = (void*)fixed_set_to;
	p_fd->fdetector.get_timeout = (void*)fixed_get_to;
	p_fd->fdetector.is_failed = (void*)fixed_failed;
	p_fd->fdetector.get_idle_time = (void*)fixed_get_idle;
	p_fd->fdetector.get_time_to_next_ping = (void*)fixed_time_next_ping;
	p_fd->fdetector.should_ping = (void*)fixed_should_ping;
	p_fd->fdetector.release_monitored = (void*)fixed_release;
	p_fd->fdetector.set_ping_interval = (void*)fixed_set_ping_interval;

	p_fd->monitoreds = create_fd_hashtable();
	return p_fd;
}
