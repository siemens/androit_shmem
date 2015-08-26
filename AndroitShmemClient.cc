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

void doWrite(void *arg) {
	struct shared *container = getSharedData();
	int active_data;

	if(container != NULL) {
		// --- Write Test ---		
		LOGD("Write test");
		begin_rt_write(container);
		LOGD("sequence after lock:\t%d", container->protect.sequence);
		
		// Determine active data copy by looking at 1-bit of sequence counter
		active_data = container->protect.sequence & 1;
		
		LOGD("AndroitShmem before access: base=%p, integer=%d, float=%f", 
		     container, container->data[active_data].integer, container->data[active_data].fp);
		     
		sleep(3); // For TESTING
    
		container->data[active_data].integer = container->data[active_data].integer+1;
		container->data[active_data].fp = container->data[active_data].fp*1.234;
    
		LOGD("AndroitShmem after access: base=%p, integer=%d, float=%f", 
			container, container->data[active_data].integer, container->data[active_data].fp);	
			
		end_rt_write(container);
		LOGD("sequence after unlock:\t%d", container->protect.sequence);
		LOGD("Write test finished");
	} else {
		LOGE("Error: Androit shared memory not available\n");
	}
	
	//pthread_exit((void *) 0);
}

void doRead(void *arg) {
	struct shared *container = getSharedData();
	unsigned int start_seq;
	int active_data;

	if(container != NULL) {
		// --- Read Test ---
		LOGD("Read test");
		do {
			start_seq = seq_begin(container);
					
			sleep(3); // For TESTING
			
			// Determine active data copy by looking at 1-bit of sequence counter
			active_data = container->protect.sequence & 1;
			
			LOGD("AndroitShmem read content: base=%p, integer=%d, float=%f, start_seq=%d, seq=%d",
				container, container->data[active_data].integer, container->data[active_data].fp, start_seq, container->protect.sequence);
			
			LOGD("Sequence counter values after read try: start_seq=%d, seq=%d", start_seq, container->protect.sequence);
			
			// Check if read was consistent, retry if it was not
		} while (seq_doretry(container, start_seq));
		
		LOGD("Read was finished, current sequences: start_seq=%d, seq=%d (may have changed since then)", start_seq, container->protect.sequence);
	} else {
		LOGE("Error: Androit shared memory not available\n");
	}
	
	//pthread_exit((void *) 0);
}

int main(int argc, char *argv[]) {
	// -- two thread test --
	/* pthread_t writer[2];
	
	if(pthread_create(&writer[0], NULL, (void* (*)(void*))&doWrite, (void *)NULL) != 0)
      fprintf(stderr, "Was not able to create pthread\n");
      
    if(pthread_create(&writer[1], NULL, (void* (*)(void*))&doWrite, (void *)NULL) != 0)
      fprintf(stderr, "Was not able to create pthread\n");  
    
	pthread_join(writer[0], NULL);
	pthread_join(writer[1], NULL);*/
	
	// -- one thread test --
	doWrite((void *)NULL);
	doRead((void *)NULL);
		
	return 0;
}
