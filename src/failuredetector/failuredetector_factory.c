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
#include "failuredetector_factory.h"
#include "fixed_failuredetector.h"
#include "chen_failuredetector.h"
#include "bertier_failuredetector.h"
#include "phiaccrual_failuredetector.h"

#include <string.h>

fdetector_t* create_failure_detector(char *fd_name, struct hashtable *params_table) {

	if (strcmp(fd_name, "fixed") == 0) {
		return (fdetector_t*)fixedfd_init();
	}
	if (strcmp(fd_name, "chen") == 0) {
		return (fdetector_t*)chenfd_init(params_table);
	}
	if (strcmp(fd_name, "bertier") == 0) {
		return (fdetector_t*)bertierfd_init(params_table);
	}

	if (strcmp(fd_name, "phiaccrual") == 0) {
		return (fdetector_t*)phiaccrualfd_init(params_table);
	}

	return 0;
}
