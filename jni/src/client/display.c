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
#include <errno.h>
#include <stdlib.h>

#include <wayland-client.h>

#include "client/client-jni.h"

static jobject
display_create_from_native(JNIEnv * env, jclass cls, struct wl_display *display)
{
    jmethodID ctor;
    jobject jobj;

    ctor = (*env)->GetMethodID(env, cls, "<init>", "(J)V");
    if (ctor == NULL) {
        wl_display_disconnect(display);
        return NULL; /* Exception Thrown */
    }

    jobj = (*env)->NewObject(env, cls, ctor, (jlong)(intptr_t)display);
    if (jobj == NULL) {
        wl_display_disconnect(display);
        return NULL; /* Exception Thrown */
    }

    return jobj;
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_client_Display_connect__I(JNIEnv * env,
        jclass cls, jint fd)
{
    struct wl_display *display;

    display = wl_display_connect_to_fd(fd);
    if (display == NULL) {
        wl_jni_throw_from_errno(env, errno);
        return NULL;
    }

    return display_create_from_native(env, cls, display);
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_client_Display_connect__Ljava_lang_String_2(
        JNIEnv * env, jclass cls, jstring jname)
{
    struct wl_display *display;
    char *name;

    name = wl_jni_string_to_default(env, jname);
    if ((*env)->ExceptionCheck(env))
        return NULL;

    display = wl_display_connect(name);
    free(name);
    if (display == NULL) {
        wl_jni_throw_from_errno(env, errno);
        return NULL;
    }

    return display_create_from_native(env, cls, display);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Display_disconnectNative(JNIEnv * env,
        jobject jdisplay)
{
    struct wl_display *display;

    display = (struct wl_display *)wl_jni_proxy_from_java(env, jdisplay);
    if (display == NULL)
        return;

    wl_display_disconnect(display);
}

JNIEXPORT jint JNICALL
Java_org_freedesktop_wayland_client_Display_getFD(JNIEnv * env,
        jobject jdisplay)
{
    struct wl_display *display;

    display = (struct wl_display *)wl_jni_proxy_from_java(env, jdisplay);
    if (display == NULL) {
        wl_jni_throw_IllegalStateException(env, "Display not connected");
        return -1;
    }
    
    return wl_display_get_fd(display);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Display_dispatch(JNIEnv * env,
        jobject jdisplay)
{
    struct wl_display *display;

    display = (struct wl_display *)wl_jni_proxy_from_java(env, jdisplay);
    if (display == NULL) {
        wl_jni_throw_IllegalStateException(env, "Display not connected");
        return;
    }

    if (wl_display_dispatch(display) < 0)
        wl_jni_throw_from_errno(env, errno);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Display_dispatchPending(JNIEnv * env,
        jobject jdisplay)
{
    struct wl_display *display;

    display = (struct wl_display *)wl_jni_proxy_from_java(env, jdisplay);
    if (display == NULL) {
        wl_jni_throw_IllegalStateException(env, "Display not connected");
        return;
    }

    if (wl_display_dispatch_pending(display) < 0)
        wl_jni_throw_from_errno(env, errno);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Display_dispatchQueue(JNIEnv * env,
        jobject jdisplay, jobject jqueue)
{
    struct wl_display *display;
    struct wl_event_queue *queue;

    display = (struct wl_display *)wl_jni_proxy_from_java(env, jdisplay);
    if (display == NULL) {
        wl_jni_throw_IllegalStateException(env, "Display not connected");
        return;
    }

    queue = wl_jni_event_queue_from_java(env, jqueue);
    if ((*env)->ExceptionCheck(env)) {
        return;
    } else if (queue == NULL) {
        wl_jni_throw_NullPointerException(env, "queue not allowed to be null");
        return;
    }

    if (wl_display_dispatch_queue(display, queue) < 0)
        wl_jni_throw_from_errno(env, errno);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Display_dispatchQueuePending(JNIEnv * env,
        jobject jdisplay, jobject jqueue)
{
    struct wl_display *display;
    struct wl_event_queue *queue;

    display = (struct wl_display *)wl_jni_proxy_from_java(env, jdisplay);
    if (display == NULL) {
        wl_jni_throw_IllegalStateException(env, "Display not connected");
        return;
    }

    queue = wl_jni_event_queue_from_java(env, jqueue);
    if ((*env)->ExceptionCheck(env)) {
        return;
    } else if (queue == NULL) {
        wl_jni_throw_NullPointerException(env, "queue not allowed to be null");
        return;
    }

    if (wl_display_dispatch_queue_pending(display, queue) < 0)
        wl_jni_throw_from_errno(env, errno);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Display_flush(JNIEnv * env,
        jobject jdisplay)
{
    struct wl_display *display;

    display = (struct wl_display *)wl_jni_proxy_from_java(env, jdisplay);
    if (display == NULL) {
        wl_jni_throw_IllegalStateException(env, "Display not connected");
        return;
    }

    if (wl_display_flush(display) < 0)
        wl_jni_throw_from_errno(env, errno);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Display_roundtrip(JNIEnv * env,
        jobject jdisplay)
{
    struct wl_display *display;

    display = (struct wl_display *)wl_jni_proxy_from_java(env, jdisplay);
    if (display == NULL) {
        wl_jni_throw_IllegalStateException(env, "Display not connected");
        return;
    }

    if (wl_display_roundtrip(display) < 0)
        wl_jni_throw_from_errno(env, errno);
}

