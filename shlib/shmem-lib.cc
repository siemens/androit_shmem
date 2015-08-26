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

#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <android/log.h>
#include <IAndroitShmem.h>
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


// Function to pass the shared memory to the java application
extern "C"
jobject Java_com_androit_SharedMem_getByteBuffer(JNIEnv *env, jobject thiz) {
	struct shared *container;
	void *ptr;
	jobject bb = NULL;
	int active_data;
	
	container = getSharedData();

	if(container != NULL) {
		// Create instantiate ByteBuffer object, encapsulating shared memory for java application
		bb = env->NewDirectByteBuffer(container, sizeof(struct shared));
		
		// Determine active data copy by looking at 1-bit of sequence for debug output
		active_data = container->protect.sequence & 1;
		LOGD("AndroitShmem content: base=%p, active=%d, integer=%d, float=%f", 
		     container, active_data, container->data[active_data].integer, container->data[active_data].fp);

		// Sanity check: Both addresses must be identical
		ptr = env->GetDirectBufferAddress(bb);
		LOGD("Mapped addresss: %p, %p", container, ptr);

		if ((void*)container != ptr) {
			LOGE("Internal error: container != ptr!");
		}
	}
	else {
		LOGE("Error: Androit shared memory not available\n");
	}

	return bb;
}

// Procedure for the non-RT client to update the shared data
extern "C"
void Java_com_androit_SharedMem_updateByteBuffer(JNIEnv *, jobject, jfloat updateFloat, jint updateInt) {
	struct shared *container;
	unsigned int start_seq;
	int update_data;
	
	container = getSharedData();
	
	if(container != NULL) {
		begin_nonrt_write(container);
		
		// Determine inactive data copy to update it. It is alway outdated
		update_data = 1 - (container->protect.sequence & 1);
		
		do {
			start_seq = seq_begin(container);
			
			// Actual update
			container->data[update_data].fp = (float) updateFloat;
			container->data[update_data].integer = (int) updateInt;
			// Copy current array values to make inactive copy consistent to active copy
			for (int i = 0; i < 1024; i++)
				container->data[update_data].arbitrary[i] = container->data[1-update_data].arbitrary[i];
				
			sleep(3); // For TESTING
			
			/* Compare sequence counter value before write to inactive data copy to current value.
			 * 		- if equal: data still consistent, make inactive data copy active (invert 1-bit) and 
			 * 			increase sequence counter (by 4, because 2-bit denotes "RT-Write in progress").
			 * 		- if unequal: retry update 
			 * This happens atomically (CAS, compare and swap) */
		} while (!__sync_bool_compare_and_swap(&container->protect.sequence, start_seq, (start_seq+4)^1));
		
		// Update finished
		end_nonrt_write(container);
	}
	else {
		LOGE("Error: Androit shared memory not available\n");
	}
}

// Java application needs an offset to know where to start to read objects from shared memory
extern "C"
jint Java_com_androit_SharedMem_getActiveDataOffset(JNIEnv *env, jobject thiz) {
	size_t data_offset;
	struct shared *container;
	
	container = getSharedData();
	data_offset = offsetof(struct shared, data);
	
	// If second data copy is active, offset is moved there
	return data_offset + (container->protect.sequence & 1)*sizeof(struct data_struct);
}

// NOTE: jlong is used on purpose here; java does not support unsigned
// types, but since jlong is 64 bytes, the complete spectrum of unsigned int 
// is guaranteed to fit into it.
extern "C"
jlong Java_com_androit_SharedMem_getSeqCount(JNIEnv *env, jobject thiz) {
	struct shared *container = getSharedData();

	// sequence only changed by __sync* built-ins, therefore no additional memory barriers needed
	return container->protect.sequence;
}
