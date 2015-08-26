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

#include <stdint.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <binder/MemoryHeapBase.h>

#include <IAndroitShmem.h>

using namespace android;

enum {
	// "FIRST_CALL_TRANSACTION - The first transaction code available for user commands." (IBinder reference)
    GET_SHMEM = IBinder::FIRST_CALL_TRANSACTION
};

/////////////////////////// Client //////////////////////

// Remote Binder Interface to IAndroitShmem
class BpAndroitShmem: public BpInterface<IAndroitShmem> {
public:
// impl - implementation of remote Binder interface
BpAndroitShmem(const sp<IBinder>& impl) : BpInterface<IAndroitShmem>(impl) {}
	
	sp<IMemoryHeap> getShmem() {
		Parcel data, reply;
		sp<IMemoryHeap> shmem = NULL;
		/* Write token of IAndroitShmem to the data. 
		 * "This is used to validate that the marshalled transaction is intended for the target interface." (Parcel reference) */
		data.writeInterfaceToken(IAndroitShmem::getInterfaceDescriptor());

		// call into previously set Binder interface (impl)
		remote()->transact(GET_SHMEM, data, &reply);
		shmem = interface_cast<IMemoryHeap> (reply.readStrongBinder());

		return shmem;
	}
};

// Implements objects previously declared in IAndroitShmem.h (DECLARE_META_INTERFACE(AndroitShmem);)
IMPLEMENT_META_INTERFACE(AndroitShmem, "android.vendor.IAndroitShmem");


/////////////////////////// Server //////////////////////

// Gets called, when client sends request for shared memory (remote()->transact(GET_SHMEM, data, &reply);)
status_t BnAndroitShmem::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
	if (code == GET_SHMEM) {
		// Check if client is allowed to access
		CHECK_INTERFACE(IAndroitShmem, data, reply);
		sp<IMemoryHeap> Data = getShmem();
		// Respond with shared data if available
		if (Data != NULL) {
			reply->writeStrongBinder(Data->asBinder());
		}
		return NO_ERROR;
	}

	return BBinder::onTransact(code, data, reply, flags);
}
