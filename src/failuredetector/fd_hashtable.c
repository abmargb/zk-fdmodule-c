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

#include "../hashtable/hashtable.h"
#include <string.h>
#include <stdlib.h>

static unsigned int string_hash_djb2(void *str) {
    unsigned int hash = 5381;
    int c;

    while ((c = *(const char*)str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static int string_equal(void *key1,void *key2) {
    return strcmp((const char*)key1,(const char*)key2)==0;
}

struct hashtable* create_fd_hashtable() {
	return create_hashtable(32,string_hash_djb2,string_equal);
}

void fd_hashtable_insert(struct hashtable *hashtable, char *key, void *value) {
	char *key_copy = malloc(sizeof(key));
	strcpy(key_copy, key);
	hashtable_insert(hashtable, key_copy, value);
}
