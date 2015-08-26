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
#include <binder/IPCThreadState.h>
#include <stddef.h> // offsetof()

using namespace android;
using namespace androit;

// NOTE: <cutils/atomic.h> contains definitions for atomic operations,
// but they don't seem to provide anything that goes beyond the gcc
// builtins we're currently using.

/* AndroitShmemService inherits from BnAndroitShmem, thus implements
 * the server-local Binder interface. */
class AndroitShmemService : public BnAndroitShmem {
public:
	static void instantiate();
	AndroitShmemService();
	virtual ~AndroitShmemService();
	virtual sp<IMemoryHeap> getShmem();
private:
	sp<MemoryHeapBase> shmemHeap;
};


sp<IMemoryHeap> AndroitShmemService::getShmem() {
	return shmemHeap;
}

void AndroitShmemService::instantiate() {
	status_t status;
	 // Make AndroitShmemService known to the ServiceManager
	status = defaultServiceManager()->addService(String16("vendor.androit.shmem"), new AndroitShmemService());
  
	if (status != NO_ERROR)
		LOGE("Could not register AndroitShmem Service to Service Manager");
	else
		LOGD("AndroitShmem Service successfully registered to Service Manager");
}

AndroitShmemService::AndroitShmemService() {
	struct androit::shared *container;
	void *ptr;
	int res = 0;
	
	// NOTE: We explicitely _don't_ use the ashmem allocator because we
	// don't want the shared memory to be reclaimable. Mapping
	// a file (or file descriptor) ensures that Android bases the
	// mapping on mmap.	
	shmemHeap = new MemoryHeapBase("/mnt/shm/map", sizeof(struct androit::shared));
	ftruncate(shmemHeap->getHeapID(), sizeof(struct androit::shared));

	LOGD("Base: %p, size: %d (%s), ID: %d", shmemHeap->getBase(), shmemHeap->getSize(), 
		shmemHeap->getDevice(), shmemHeap->getHeapID());
	
	if (shmemHeap->getSize() != 0) {
		// Lock file mapping into memory to avoid it to be swapped out
		res = mlock(shmemHeap->getBase(), shmemHeap->getSize());

		if (res < 0) {
			LOGE("Could not lock memory: %d (%s)\n", errno, 
				 strerror(errno));
		}
	} else {
		LOGE("Could not allocate memory!");
	}
	
	// use allocated "raw" memory as struct shared	
	container = (struct androit::shared*) shmemHeap->getBase();
	
	if (init_shared(container) != 0)
		LOGE("Concurrency protections could not be initialised correctly");
	else
		LOGD("Concurrency protections initialised");
}

AndroitShmemService::~AndroitShmemService() {
	shmemHeap = NULL;
}

// Server obtains shared data itself through Binder for debug messages
struct shared *getSharedData(void) {
	static sp<IAndroitShmem> androitShmem;
	static sp<IMemoryHeap> receiverMemBase;
  
	// Acquire remote interface to AndroitShmem service from ServiceManager
	if (androitShmem == NULL) {
		sp<IServiceManager> sm = defaultServiceManager();
		sp<IBinder> binder;
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
	if (receiverMemBase == NULL)
		receiverMemBase = androitShmem->getShmem();
		
	// Return pointer to shared memory
	return (struct shared*)receiverMemBase->getBase();
}


int main(int argc, char *argv[]) {
	struct shared *container;
	int active_data;

	AndroitShmemService::instantiate();
  
	// Start thread pool to handle incoming Binder calls
	ProcessState::self()->startThreadPool();
	LOGD("Androit shmem service started");
  
	container = (struct shared*)getSharedData();
  
	// Periodically log debug messages
	for(;;) {
		active_data = container->protect.sequence & 1;
		LOGD("AndroitShmemServer: active=%d, base=%p, integer=%d, float=%f, seq=%d", 
		     active_data, container, container->data[active_data].integer, 
		     container->data[active_data].fp, container->protect.sequence);
		     
		sleep(5);
	}

	return 0;
}
