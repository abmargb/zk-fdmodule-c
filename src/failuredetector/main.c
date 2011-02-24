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
#include <time.h>
#include <stdio.h>

int main() {

	fdetector_t* fd = create_failure_detector("fixed");

	fd->register_monitored(fd, "objectA", 0, 100);
	fd->message_received(fd, "objectA", 20, PING);
	fd->message_sent(fd, "objectA", 30, PING);

	printf("Timeout: %ld\n", fd->get_timeout(fd, "objectA"));
	printf("Failed at 110: %u\n", fd->is_failed(fd, "objectA", 110));
	printf("Failed at 130: %u\n", fd->is_failed(fd, "objectA", 130));
	printf("Time to next ping at 40: %ld\n", fd->get_time_to_next_ping(fd, "objectA", 40));
	printf("Should ping at 70: %u\n", fd->should_ping(fd, "objectA", 70));
	printf("Should ping at 90: %u\n", fd->should_ping(fd, "objectA", 90));

	return 0;
}
