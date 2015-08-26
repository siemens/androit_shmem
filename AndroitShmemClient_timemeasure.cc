/*
 * Copyright (C) 2012 Wolfgang Mauerer, Siemens AG
 *           (C) 2012 Marvin Damschen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "IAndroitShmem.h"
#include <binder/MemoryHeapBase.h>
#include <binder/IServiceManager.h>
#include <time.h>

using namespace android;
using namespace androit;

// Function for client to obtain pointer to shared memory
struct shared* getSharedData(void) {
	static sp<IAndroitShmem> androitShmem = NULL;
	static sp<IMemoryHeap> receiverMemBase = NULL;

	sp<IBinder> binder;
	
	// Acquire remote interface to AndroitShmem service from ServiceManager
	if (androitShmem == NULL) {
		sp<IServiceManager> sm = defaultServiceManager();
		binder = sm->getService(String16("vendor.androit.shmem"));

		if (binder != 0) {
			androitShmem = IAndroitShmem::asInterface(binder);
		}
	}
	
	// Abort if AndroitShmem service is not published
	if (androitShmem == NULL) {
		LOGE("The AndroitShmem service is not published");
		return NULL;
	}
	
	// Acquire pointer to shared MemoryHeap from AndroitShmem service if not already done
	if (receiverMemBase == NULL) {
		LOGD("Getting handle to shared data via binder...");
		receiverMemBase = androitShmem->getShmem();		
	}
	
	// Return pointer to shared memory
	return (struct shared*)receiverMemBase->getBase();
}

// Function to calculate time difference
struct timespec timediff(struct timespec start, struct timespec end) {
	struct timespec temp;
	
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	
	return temp;
}

int main(int argc, char *argv[]) {
	volatile struct shared *container = getSharedData();
	struct timespec time1, time2, diff, res;
	
	// Read clock resolution
	clock_getres(CLOCK_PROCESS_CPUTIME_ID, &res);	

	if(container != NULL) {
		LOGD("AndroitShmem content: base=%p, integer=%d, float=%f", 
		     container, container->data.integer, container->data.fp);

		// Get time before access
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
	
		for (int i = 0; i < 1; i++) {
			container->data.integer = container->data.integer+1;
			container->data.fp = container->data.fp*1.0001;
			for (int j = 0; j < 1024; j++)
				container->data.arbitrary[j] = container->data.arbitrary[j]+1;
		}
		
		// Get time after access
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);
		// Get time difference
		diff = timediff(time1, time2);
		
		LOGD("AndroitShmem after change: base=%p, integer=%d, float=%f, elapsed time: %ld ns (resolution: %ld ns)", 
		     container, container->data.integer, container->data.fp, diff.tv_sec*1000000000+diff.tv_nsec, res.tv_sec*1000000000+res.tv_nsec);
	}
	else {
		LOGE("Error: Androit shared memory not available\n");
	}
	return 0;
}
