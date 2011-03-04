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

#include "bertier_failuredetector.h"
#include "failuredetector.h"
#include "fd_hashtable.h"
#include "fd_opt_parser.h"
#include "../hashtable/hashtable.h"
#include "interarrival_window.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#define DEF_MOD_STEP 500l
#define DEF_PHI 4.
#define DEF_BETA 1.
#define DEF_GAMMA 0.1

typedef struct {
	char* id;
	long timeout;
	long last_heard;
	long last_sent;
	long eta; //interrogation interval

	long ea; //estimate arrival
	long delta_p; //moderation param
	long delay; //estimate margin

	double alpha; //calculated safety margin
	double var; //magnitude between errors
	double error; //error of the last estimation

	interarrival_window_t *sampling_window;

} monitored_t;

static void destroy_monitored(monitored_t *m) {
	destroy_window(m->sampling_window);
	free(m);
}

void bertier_reg_monitored(bertierfd_t *this, char *id, long now, long timeout) {
	monitored_t *m;
	m = calloc(1, sizeof(*m));

	m->id = id;
	m->last_heard = now;
	m->last_sent = now;
	m->timeout = timeout;
	m->eta = timeout / 2;
	m->delay = timeout / 4;
	m->sampling_window = init_window();
	m->ea = now + timeout;

	fd_hashtable_insert(this->monitoreds, id, m);
}

void bertier_set_to(bertierfd_t *this, char *id, long timeout) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->timeout = timeout;
}

long bertier_get_to(bertierfd_t *this, char *id) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return m->timeout;
}

static void update_timeout(bertierfd_t *this, monitored_t* m, long now, int failed) {
	if (m->sampling_window->size > 0) {
		m->error = now - m->ea - m->delay;
		m->delay += (long)round(this->gamma * m->error);
		m->var += this->gamma * (labs(m->error) - m->var);
		m->alpha = this->beta * (double)m->delay + this->phi * m->var;

		m->ea = now + (long)round(m->sampling_window->mean);
		long t = m->ea + (long)round(m->alpha);

		if (failed) {
			m->delta_p += this->moderation_step;
		}

		m->timeout = t - now + m->delta_p;
	}
}

void bertier_msg_rcv(bertierfd_t *this, char *id, long now, int type) {
	monitored_t* m = hashtable_search(this->monitoreds, id);

	if (type == PING) {
		int failed = now > m->last_heard + m->timeout;
		add_ping(m->sampling_window, now);
		update_timeout(this, m, now, failed);
	}

	m->last_heard = now;
}

void bertier_msg_sent(bertierfd_t *this, char *id, long now, int type) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->last_sent = now;
}

int bertier_failed(bertierfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return now > m->last_heard + bertier_get_to(this, id);
}

long bertier_get_idle(bertierfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return now - m->last_heard;
}

int bertier_time_next_ping(bertierfd_t *this, char *id, long now) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	return m->eta - (now - m->last_sent);
}

int bertier_should_ping(bertierfd_t *this, char *id, long now) {
	return bertier_time_next_ping(this, id, now) <= 0;
}

void bertier_release(bertierfd_t *this, char *id) {
	monitored_t* m = hashtable_remove(this->monitoreds, id);
	destroy_monitored(m);
}

void bertier_set_ping_interval(bertierfd_t *this, char *id, long interval) {
	monitored_t* m = hashtable_search(this->monitoreds, id);
	m->eta = interval;
}

bertierfd_t* bertierfd_init_params(double gamma, double beta,
		double phi, long moderation_step) {
	bertierfd_t *p_fd;
	p_fd = calloc(1, sizeof(*p_fd));

	p_fd->fdetector.message_received = (void*)bertier_msg_rcv;
	p_fd->fdetector.message_sent = (void*)bertier_msg_sent;
	p_fd->fdetector.register_monitored = (void*)bertier_reg_monitored;
	p_fd->fdetector.set_timeout = (void*)bertier_set_to;
	p_fd->fdetector.get_timeout = (void*)bertier_get_to;
	p_fd->fdetector.is_failed = (void*)bertier_failed;
	p_fd->fdetector.get_idle_time = (void*)bertier_get_idle;
	p_fd->fdetector.get_time_to_next_ping = (void*)bertier_time_next_ping;
	p_fd->fdetector.should_ping = (void*)bertier_should_ping;
	p_fd->fdetector.release_monitored = (void*)bertier_release;
	p_fd->fdetector.set_ping_interval = (void*)bertier_set_ping_interval;

	p_fd->monitoreds = create_fd_hashtable();
	p_fd->gamma = gamma;
	p_fd->beta = beta;
	p_fd->phi = phi;
	p_fd->moderation_step = moderation_step;
	return p_fd;
}

bertierfd_t* bertierfd_init(struct hashtable *params_table) {
	return bertierfd_init_params(
			parse_double(DEF_GAMMA, hashtable_search(params_table, "gamma")),
			parse_double(DEF_BETA, hashtable_search(params_table, "beta")),
			parse_double(DEF_PHI, hashtable_search(params_table, "phi")),
			parse_long(DEF_MOD_STEP, hashtable_search(params_table, "moderationstep")));
}

bertierfd_t* bertierfd_init_def() {
	return bertierfd_init_params(DEF_GAMMA, DEF_BETA, DEF_PHI, DEF_MOD_STEP);
}
