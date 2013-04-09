/*
 * Copyright © 2012-2013 Jason Ekstrand.
 *  
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 * 
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
#ifndef __WAYLAND_JAVA_CLIENT_JNI_H__
#define __WAYLAND_JAVA_CLIENT_JNI_H__

#include "wayland-jni.h"

#include "wayland-client.h"

struct wl_display *
wl_jni_client_display_from_java(JNIEnv *env, jobject jdisplay);
struct wl_event_queue *
wl_jni_event_queue_from_java(JNIEnv *env, jobject jqueue);
jobject
wl_jni_event_queue_create_from_native(JNIEnv *env,
        struct wl_event_queue *queue);
struct wl_proxy *
wl_jni_proxy_from_java(JNIEnv *env, jobject jproxy);

#endif /* ! defined __WAYLAND_JAVA_CLIENT_JNI_H__ */

