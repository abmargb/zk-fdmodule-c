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

#include "failuredetector.h"
#include "fixed_failuredetector.h"
#include "failuredetector_factory.h"
#include "fd_hashtable.h"
#include <time.h>
#include <stdio.h>

int main() {

	fdetector_t* fd = create_failure_detector("phiaccrual", create_fd_hashtable());

	char* monitoredName = "object";

	fd->register_monitored(fd, monitoredName, 0l, 100);
	fd->message_received(fd, monitoredName, 20l, PING);
	fd->message_received(fd, monitoredName, 40l, PING);
	fd->message_received(fd, monitoredName, 50l, PING);

	fd->message_sent(fd, monitoredName, 30l, PING);

	printf("Timeout: %ld\n", fd->get_timeout(fd, monitoredName));
	printf("Failed at 110: %u\n", fd->is_failed(fd, monitoredName, 110l));
	printf("Failed at 130: %u\n", fd->is_failed(fd, monitoredName, 130l));
	printf("Time to next ping at 40: %ld\n", fd->get_time_to_next_ping(fd, monitoredName, 40l));
	printf("Should ping at 70: %u\n", fd->should_ping(fd, monitoredName, 70l));
	printf("Should ping at 90: %u\n", fd->should_ping(fd, monitoredName, 90l));

	fd->release_monitored(fd, monitoredName);

	return 0;
}
