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

#ifndef INTERARRIVAL_WINDOW_H_
#define INTERARRIVAL_WINDOW_H_
#define MAX_SIZE 1000

typedef struct window_el {
	long interarrival;
	struct window_el *next;
} window_el_t;

typedef struct {
	int size;
	float mean;
	long last_ping;
	window_el_t* head;
	window_el_t* tail;
} interarrival_window_t;

interarrival_window_t* init_window();
void add_ping(interarrival_window_t *window, long ping);
void destroy_window(interarrival_window_t *window);

#endif /* INTERARRIVAL_WINDOW_H_ */
