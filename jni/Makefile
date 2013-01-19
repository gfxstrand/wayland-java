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

JNI_CFLAGS := -I/lib/jvm/java/include -I/lib/jvm/java/include/linux

WAYLAND_CFLAGS := $(shell pkg-config --cflags wayland-server)
WAYLAND_SERVER_LDFLAGS := $(shell pkg-config --libs wayland-server)

CFLAGS := -g -fPIC $(WAYLAND_JNI_CFLAGS) $(WAYLAND_CFLAGS) $(JNI_CFLAGS)
CFLAGS += $(foreach file, $(WAYLAND_JNI_C_INCLUDES), -I$(file))

SERVER_OBJECTS := $(foreach file, $(WAYLAND_JNI_SERVER_SRC), \
		$(patsubst src/%.c, $(OBJECT_DIR)/%.o, $(file)))

$(OBJECT_DIR)%.o: src/%.c
	mkdir -p $(dir $@)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/libwayland-java-server.so: $(SERVER_OBJECTS)
	gcc -shared -o $@ $^ $(WAYLAND_SERVER_LDFLAGS)

.PHONY: clean
clean:
	rm -r $(OBJECT_DIR)
	rm $(BUILD_DIR)/libwayland-java-server.so

all: $(BUILD_DIR)/libwayland-java-server.so

