LIBFFI_PATH := $(call my-dir)/libffi

# We need this because what gets tacked on to the end of it is
# external/libffi/filename
LIBFFI_INCLUDE_PREFIX := $(call my-dir)/..

# Architecture is auto-detected. On android, TARGET_OS is not defined so we set
# it to linux
ffi_arch := $(TARGET_ARCH)
ffi_os := linux

include $(CLEAR_VARS)

LOCAL_MODULE := libffi
include $(LIBFFI_PATH)/Libffi.mk

# We have to convert the include paths to be global paths in our directory
# structure
LOCAL_SRC_FILES := $(foreach path, $(LOCAL_SRC_FILES), $(LIBFFI_PATH)/$(path))

LOCAL_C_INCLUDES := $(foreach path, $(LOCAL_C_INCLUDES), $(LIBFFI_INCLUDE_PREFIX)/$(path))

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)

