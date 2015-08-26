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

package com.androit;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

// Library to give access to AndroitShmem to java applications
public class SharedMem {
	private float bbFloat;
	private int bbInt;
	
	public float getBBFloat() {
		// If data is outdated, reread from shared memory
		if (seq != getSeqCount())
			readBBValues();
		
		return bbFloat;
	}

	public int getBBInt() {
		// If data is outdated, reread from shared memory
		if (seq != getSeqCount())
			readBBValues();
		
		return bbInt;
	}
	
	public void setBBValues(float bbFloat, int bbInt) {
		updateByteBuffer(bbFloat, bbInt);
		readBBValues();
	}
	
    private void readBBValues() {
		ByteBuffer res;
		int data_offset;
		
		res = (ByteBuffer)getByteBuffer();
		
		// x86 (for instance) has a different native byte order than java's big endian
		if (ByteOrder.nativeOrder() != ByteOrder.BIG_ENDIAN) { 
			res.order(ByteOrder.LITTLE_ENDIAN);
		}
		
		/* Get sequence value before read and compare it to sequence value after read.
		 * If equal: data read is consistent, if unequal: retry */
		do {
			readSeqBegin();
			data_offset = getActiveDataOffset();
			bbInt = res.getInt(data_offset + 0);
			bbFloat =  res.getFloat(data_offset + 4);
		} while (readSeqRetry()); 
		
    }
    
    public void readSeqBegin() {
		// Wait for RT-Writes to finish before reading data
    	while (((seq = getSeqCount()) & 2) == 1)
			try {
				Thread.sleep(10);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
    }
    
    public boolean readSeqRetry() {
    	long curr_seq = getSeqCount();
    	
    	return (seq != curr_seq); 
    }
    
    // Methods provided by the native library
    private native Object getByteBuffer();
    private native void updateByteBuffer(float bbFloat, int bbInt);
    private native int getActiveDataOffset();
    private native long getSeqCount();

    // Native private elements
    private long seq = -1;
    
    static {
    	System.load("/system/lib/libandroitshmem.so");
    }
}
