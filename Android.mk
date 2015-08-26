############# Server ################
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION:=.cc
LOCAL_SRC_FILES:=        \
 IAndroitShmem.cc      \
 AndroitShmemServer.cc \

LOCAL_SHARED_LIBRARIES:= libcutils libutils libbinder

LOCAL_MODULE:= AndroitShmemServer
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS+=-DLOG_TAG=\"AndroitShmemServer\"
LOCAL_CPPFLAGS  := -I$(LOCAL_PATH)/include

LOCAL_PRELINK_MODULE:=false
include $(BUILD_EXECUTABLE)

### Move init.androitshmem.rc/.sh to the right places 
# init.androitshmem.rc (NOTE: to actually use this, manually add 'import /init.androitshmem.rc' to init.rc)
include $(CLEAR_VARS) 
LOCAL_MODULE := init.androitshmem.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SCRIPT 
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)
LOCAL_SRC_FILES := init/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)

# init.androitshmem.sh
include $(CLEAR_VARS) 
LOCAL_MODULE := init.androitshmem.sh
LOCAL_MODULE_TAGS := optional 
LOCAL_MODULE_CLASS := SCRIPT 
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
LOCAL_SRC_FILES := init/$(LOCAL_MODULE)
include $(BUILD_PREBUILT)


############# Client ################
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION:=.cc
LOCAL_SRC_FILES:=        \
 IAndroitShmem.cc      \
 AndroitShmemClient.cc \

LOCAL_SHARED_LIBRARIES:= libcutils libutils libbinder

LOCAL_MODULE:= AndroitShmemClient
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS+=-DLOG_TAG=\"AndroitShmemClient\"
LOCAL_CPPFLAGS  := -I$(LOCAL_PATH)/include

LOCAL_PRELINK_MODULE:=false
include $(BUILD_EXECUTABLE)


############# Shared Library ################
include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION:=.cc
LOCAL_MODULE    := libandroitshmem
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS  := -I$(LOCAL_PATH)/include
LOCAL_CFLAGS  +=-DLOG_TAG=\"AndroitShLib\"

LOCAL_PATH	:= $(LOCAL_PATH)/shlib
LOCAL_SRC_FILES := shmem-lib.cc ../IAndroitShmem.cc
# NOTE: libutils is required for strong pointers, libbinder for the
# service manager interaction
LOCAL_SHARED_LIBRARIES := liblog libutils libbinder

include $(BUILD_SHARED_LIBRARY)
