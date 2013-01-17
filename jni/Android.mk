# Copyright © 2012-2013 Jason Ekstrand.
#  
# Permission to use, copy, modify, distribute, and sell this software and its
# documentation for any purpose is hereby granted without fee, provided that
# the above copyright notice appear in all copies and that both that copyright
# notice and this permission notice appear in supporting documentation, and
# that the name of the copyright holders not be used in advertising or
# publicity pertaining to distribution of the software without specific,
# written prior permission.  The copyright holders make no representations
# about the suitability of this software for any purpose.  It is provided "as
# is" without express or implied warranty.
# 
# THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
# EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
# DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.

LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/external/libffi.mk
include $(LOCAL_PATH)/external/wayland.mk

# Begin local wayland-java build
include $(LOCAL_PATH)/files.mk

include $(CLEAR_VARS)

LOCAL_MODULE 			:= libwayland-java-server
LOCAL_CFLAGS			:= $(WAYLAND_JNI_CFLAGS)
LOCAL_C_INCLUDES		:= $(foreach file, $(WAYLAND_JNI_C_INCLUDES), $(LOCAL_PATH)/$(file))
LOCAL_SRC_FILES 		:= $(foreach file, $(WAYLAND_JNI_SERVER_SRC), $(LOCAL_PATH)/$(file))
LOCAL_LDLIBS 			:= -llog
LOCAL_STATIC_LIBRARIES	:= libwayland-server

include $(BUILD_SHARED_LIBRARY)

