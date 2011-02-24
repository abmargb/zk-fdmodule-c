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

#ifndef FAILUREDETECTOR_H_
#define FAILUREDETECTOR_H_

#include <sys/time.h>
#define APPLICATION 0
#define PING 1

typedef struct fdetector {
	void (*message_received)(void *this, char *id, long last_recv, int type);
	void (*message_sent)(void *this, char *id, long last_recv, int type);
	void (*set_timeout)(void *this, char *id, long timeout);
	int (*is_failed)(void *this, char *id, long now);
	int (*should_ping)(void *this, char *id, long now);
	void (*register_monitored)(void *this, char *id, long now, long timeout);
	void (*release_monitored)(void *this, char *id);
	void (*set_ping_interval)(void *this, char *id, long interval);
	long (*get_idle_time)(void *this, char *id, long now);
	long (*get_time_to_next_ping)(void *this, char *id, long now);
	long (*get_timeout)(void *this, char *id);
} fdetector_t;

#endif /* FAILUREDETECTOR_H_ */
