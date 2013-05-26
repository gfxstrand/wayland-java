LIBWAYLAND_PATH := $(call my-dir)/wayland
LIBWAYLAND_EXTRA_PATH := $(call my-dir)/wayland-extra

LIBWAYLAND_UTIL_SRC = \
	src/connection.c				\
	src/wayland-util.c				\
	src/wayland-os.c				\

LIBWAYLAND_SERVER_SRC = $(LIBWAYLAND_UTIL_SRC) \
	src/wayland-protocol.c			\
	src/wayland-server.c			\
	src/wayland-shm.c				\
	src/data-device.c				\
	src/event-loop.c

LIBWAYLAND_CLIENT_SRC = $(LIBWAYLAND_UTIL_SRC) \
	src/wayland-protocol.c			\
	src/wayland-client.c

LIBWAYLAND_CFLAGS := -O2

include $(CLEAR_VARS)

LOCAL_MODULE    		:= libwayland-client
LOCAL_C_INCLUDES		:= $(LIBWAYLAND_PATH)/src $(LIBWAYLAND_EXTRA_PATH)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_CFLAGS    		:= $(LIBWAYLAND_CFLAGS)
LOCAL_SRC_FILES 		:= $(foreach file, $(LIBWAYLAND_CLIENT_SRC), $(LIBWAYLAND_PATH)/$(file))
LOCAL_STATIC_LIBRARIES 	:= libffi

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    		:= libwayland-server
LOCAL_C_INCLUDES		:= $(LIBWAYLAND_PATH)/src $(LIBWAYLAND_EXTRA_PATH)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_CFLAGS    		:= $(LIBWAYLAND_CFLAGS)
LOCAL_SRC_FILES 		:= $(foreach file, $(LIBWAYLAND_SERVER_SRC), $(LIBWAYLAND_PATH)/$(file))
LOCAL_STATIC_LIBRARIES  := libffi

include $(BUILD_STATIC_LIBRARY)

