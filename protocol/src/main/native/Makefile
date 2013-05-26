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

include files.mk

BUILD_DIR := ../build
OBJECT_DIR := $(BUILD_DIR)/objects

ifeq ($(JAVA_HOME),)
JAVA_HOME = $(realpath $(dir $(realpath $(shell which javac)))/../ )
endif

JNI_CFLAGS := -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux

WAYLAND_CFLAGS := $(CFLAGS) $(shell pkg-config --cflags wayland-server wayland-egl wayland-client)

CFLAGS := -g -fPIC $(WAYLAND_JNI_CFLAGS) $(WAYLAND_CFLAGS) $(JNI_CFLAGS)
CFLAGS += $(foreach file, $(WAYLAND_JNI_C_INCLUDES), -I$(file))

SERVER_LDFLAGS := $(shell pkg-config --libs wayland-server)
SERVER_LDFLAGS += -L$(BUILD_DIR) -lwayland-java-util
CLIENT_LDFLAGS := $(shell pkg-config --libs wayland-client)
CLIENT_LDFLAGS += -L$(BUILD_DIR) -lwayland-java-util

UTIL_OBJECTS := $(foreach file, $(WAYLAND_JNI_UTIL_SRC), \
		$(patsubst src/%.c, $(OBJECT_DIR)/%.o, $(file)))
SERVER_OBJECTS := $(foreach file, $(WAYLAND_JNI_SERVER_SRC), \
		$(patsubst src/%.c, $(OBJECT_DIR)/%.o, $(file)))
CLIENT_OBJECTS := $(foreach file, $(WAYLAND_JNI_CLIENT_SRC), \
		$(patsubst src/%.c, $(OBJECT_DIR)/%.o, $(file)))

all: $(BUILD_DIR)/libwayland-java-server.so $(BUILD_DIR)/libwayland-java-client.so

$(OBJECT_DIR)%.o: src/%.c
	mkdir -p $(dir $@)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libwayland-java-util.so: $(UTIL_OBJECTS)
	gcc -shared -Wl,-soname,libwayland-java-util.so -o $@ $^ $(UTIL_LDFLAGS)

$(BUILD_DIR)/libwayland-java-server.so: $(BUILD_DIR)/libwayland-java-util.so

$(BUILD_DIR)/libwayland-java-server.so: $(SERVER_OBJECTS)
	gcc -shared -Wl,-soname,libwayland-java-server.so -o $@ $^ $(SERVER_LDFLAGS)

$(BUILD_DIR)/libwayland-java-client.so: $(BUILD_DIR)/libwayland-java-util.so

$(BUILD_DIR)/libwayland-java-client.so: $(CLIENT_OBJECTS)
	gcc -shared -Wl,-soname,libwayland-java-client.so -o $@ $^ $(CLIENT_LDFLAGS)

.PHONY: clean
clean:
	rm -rf $(OBJECT_DIR)
	rm -f $(BUILD_DIR)/libwayland-java-server.so
	rm -f $(BUILD_DIR)/libwayland-java-client.so

