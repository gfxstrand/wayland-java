LIBFFI_PATH := $(call my-dir)/libffi

# Architecture is auto-detected. On android, TARGET_OS is not defined so we set
# it to linux
ffi_arch := $(TARGET_ARCH)
ffi_os := linux

include $(CLEAR_VARS)

LOCAL_MODULE := libffi
include $(LIBFFI_PATH)/Libffi.mk

# We have to convert the include paths to be global paths in our directory
# structure
LOCAL_C_INCLUDES := $(foreach path, $(LOCAL_C_INCLUDES), $(LOCAL_PATH)/$(path))
LOCAL_SRC_FILES := $(foreach path, $(LOCAL_SRC_FILES), external/libffi/$(path))

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)

