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

#ifndef I_ANDROIT_SHMEM_H
#define I_ANDROIT_SHMEM_H

#include <utils/Timers.h>
#include <utils/RefBase.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <binder/IInterface.h>
#include <pthread.h>

namespace android {
	// Base class for Binder Interface
	class IAndroitShmem: public IInterface { 
	public:
		// Declares objects used by Binder internally, see IInterface.h
		DECLARE_META_INTERFACE(AndroitShmem);
		virtual sp<IMemoryHeap> getShmem() = 0;
	};
	
	/////////// Server ///////////////
	
	// Server-local Binder Interface
	class BnAndroitShmem: public BnInterface<IAndroitShmem> {
	public:
		virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0);
	};
}; // namespace android

namespace androit {
	// struct that declares the actual data to be shared
	struct data_struct {
			int   integer;
			float fp;
			long  arbitrary[1024];
	};
	
	// struct to be shared, containing concurrency protection and data
	struct shared {
		/* Readers ensure consistent reads with the sequence counter.
		 * RT Writers exclude each other mutually with a lock, so do non-RT
		 * Writers. Non-RT Writers ensure consistency through transactions. */
		struct {
			pthread_mutex_t rt_wlock;
			pthread_mutex_t nonrt_wlock;
			/* 1-bit of sequence denotes which data copy is active,
			 * 2-bit denotes if RT-Write is in progress. 2-bit is set/unset by
			 * adding 2 to the sequence counter and thus increasing it. This signals
			 * the data was updated. */
			unsigned int sequence;
		} protect;
		struct data_struct data[2]; // Store data twice for non-RT Writer transactions
	};

	///////////////////////////////////////////////////////////////////
	// Synchronisation for concurrent RT writers
	// TODO: The __sync_* functions should eventually be replaced
	// with __atomic_* functions since we could make do with less
	// strong memory assumptions (i.e., distinguish between read and
	// write memory barriers). However, they are only available
	// from 4.7 onwards, which is not yet supported as Android toolchain
	static inline int begin_rt_write(struct shared *shared) {
		int ret;
		
		ret = pthread_mutex_lock(&shared->protect.rt_wlock);
		if (ret)
			return ret;
			
		// Set 2-bit (was unset before), denotes "RT-Write in progress"
		__sync_add_and_fetch(&shared->protect.sequence, 2);
		
		return ret;
	}

	static inline int end_rt_write(struct shared *shared) {
		/* Unset 2-bit by increasing the sequence counter by two,
		 * denotes "_no_ RT-Write in progress and data was updated" */
		__sync_add_and_fetch(&shared->protect.sequence, 2);
		return pthread_mutex_unlock(&shared->protect.rt_wlock);
	}
	
	///////////////////////////////////////////////////////////////////
	// Synchronisation for concurrent non-RT writers
	static inline int begin_nonrt_write(struct shared *shared) {		
		return pthread_mutex_lock(&shared->protect.nonrt_wlock);
	}

	static inline int end_nonrt_write(struct shared *shared) {
		return pthread_mutex_unlock(&shared->protect.nonrt_wlock);
	}
	
	///////////////////////////////////////////////////////////////////
	// "Synchronisation" for readers against RT writers
	static inline void rep_nop() {
		// TODO: This is x86 specific, naturally. Provide variants for
		// other architectures as well (note: stolen from the Linux
		// kernel).
        asm volatile("rep; nop" ::: "memory");
	}
	
	static inline void cpu_relax() {
        rep_nop();
	}
	
	// Wait for unfinished RT-Writes to finish, get sequence number
	static inline unsigned seq_begin(const struct shared *shared) {
		unsigned int sequence;
		
		// sequence only changed by __sync* built-ins, therefore no additional memory barriers needed
		sequence = shared->protect.sequence;
		
		// Wait for 2-bit unset. This bit denotes "RT-Write in progress"
		while (sequence & 2) {
			cpu_relax();
			sequence = shared->protect.sequence;
		}
			
		return sequence;
	}

	// Compares current sequence counter with the one recorded by "start", returns true if not equal
	static inline bool seq_doretry(const struct shared *shared, const unsigned int start) {
		unsigned int sequence;
		bool inconsistent = false;

        // sequence only changed by __sync* built-ins, therefore no additional memory barriers needed
		sequence = shared->protect.sequence;

		if (sequence != start)
			inconsistent = true;

        return inconsistent;
	}

	// Initialises concurrency protections in shared struct
	static int init_shared(struct shared *shared) {
		int result;		
		pthread_mutexattr_t attr;
		
		// Initialise sequence counter
		shared->protect.sequence = 0;
		
		// Create attribute PTHREAD_PROCESS_SHARED
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        // Initialise _shared_ mutexes
        result = pthread_mutex_init(&shared->protect.rt_wlock, &attr);
        result += pthread_mutex_init(&shared->protect.nonrt_wlock, &attr);
        // Destroy attribute
        pthread_mutexattr_destroy(&attr);
        
        // Initialise data (sample values)
        for (int i = 0; i < 2; i++) {
			shared->data[i].integer = 42;
			shared->data[i].fp = 23.42;	
			for (int j = 0; j < 1024; j++)
				shared->data[i].arbitrary[j] = 1024 - j;
		}
        
		return result;
	}
}; // namespace androit

#endif /* I_ANDROIT_SHMEM_H */
