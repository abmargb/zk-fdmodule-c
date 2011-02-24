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

#include "interarrival_window.h"
#include <stdlib.h>

interarrival_window_t* init_window() {
	interarrival_window_t *window;
	window = calloc(1, sizeof(*window));
	window->size = 0;

	return window;
}

void update_mean(interarrival_window_t* window, window_el_t *added,
		window_el_t *removed) {

	int size = window->size;

	if (!removed) {
		window->mean = (window->mean * (size-1) + added->interarrival) / size;
	} else {
		window->mean += (added->interarrival - removed->interarrival) / size;
	}
}

void add_interarrival(interarrival_window_t* window, long interarrival) {
	window_el_t *window_el;
	window_el = calloc(1, sizeof(*window_el));

	if (!window->head) {
		window->head = window_el;
		window->tail = window_el;
	} else {
		window->tail->next = window_el;
	}
	window->size++;

	window_el_t *removed;
	if (window->size > MAX_SIZE) {
		removed = window->head;
		window->head = removed->next;
		window->size--;
	}

	update_mean(window, window_el, removed);

	free(removed);
}

void add_ping(interarrival_window_t* window, long ping) {
	if (window->last_ping) {
		add_interarrival(window, ping - window->last_ping);
	}
	window->last_ping = ping;
}
