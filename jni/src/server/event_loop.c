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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wayland-server.h>

#include "server/server-jni.h"

struct {
    jclass class;
    jfieldID event_loop_ptr;

    jmethodID init_long;

    struct {
        jclass class;
        jfieldID event_src_ptr;

        jmethodID init_long;
    } EventSource;

    struct {
        jclass class;
        jmethodID handleFileDescriptorEvent;
    } FileDescriptorEventHandler;

    struct {
        jclass class;
        jmethodID handleTimerEvent;
    } TimerEventHandler;

    struct {
        jclass class;
        jmethodID handleSignalEvent;
    } SignalEventHandler;

    struct {
        jclass class;
        jmethodID handleIdle;
    } IdleHandler;
} EventLoop;

static void ensure_event_loop_object_cache(JNIEnv * env, jclass cls);

struct event_loop_data {
    struct wl_event_loop * event_loop;
    struct wl_list event_handlers;
};

struct event_handler {
    jobject jhandler;
    jmethodID mid;
    struct wl_list link;
};

static struct event_loop_data *
event_loop_data_create(JNIEnv * env, struct wl_event_loop * event_loop)
{
    struct event_loop_data * loop_data = malloc(sizeof(struct event_loop_data));
    if (loop_data == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    loop_data->event_loop = event_loop;
    wl_list_init(&loop_data->event_handlers);

    return loop_data;
}

static void
event_loop_data_destroy(JNIEnv * env, struct event_loop_data * loop_data)
{
    // Clean up our handler references
    struct event_handler * handler, * tmp;

    wl_list_for_each_safe(handler, tmp, &loop_data->event_handlers, link) {
        (*env)->DeleteGlobalRef(env, handler->jhandler);
        wl_list_remove(&handler->link);
        free(handler);
    }

    // Finally, we delete the loop data structure
    free(loop_data);
}

static struct event_handler *
event_loop_data_add_event_handler(JNIEnv * env,
        struct event_loop_data * loop_data, jobject jhandler, jmethodID method)
{
    struct event_handler * handler = malloc(sizeof(struct event_handler));
    if (handler == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    handler->jhandler = (*env)->NewGlobalRef(env, jhandler);
    if (handler->jhandler == NULL) {
        free(handler);
        return NULL; /* Exception Thrown */
    }

    handler->mid = method;
    wl_list_insert(&loop_data->event_handlers, &handler->link);

    return handler;
}

void
event_loop_data_remove_event_handler(JNIEnv * env,
        struct event_handler * handler)
{
    wl_list_remove(&handler->link);
    (*env)->DeleteGlobalRef(env, handler->jhandler);
    free(handler);
}

static jobject
event_source_create(JNIEnv * env, struct wl_event_source * wl_event_source)
{
    return (*env)->NewObject(env, EventLoop.EventSource.class,
            EventLoop.EventSource.init_long, (long)(intptr_t)wl_event_source);
}

static struct event_loop_data *
event_loop_data_from_java(JNIEnv * env, jobject jevent_loop)
{
    return (struct event_loop_data *)(intptr_t)
            (*env)->GetLongField(env, jevent_loop, EventLoop.event_loop_ptr);
}

struct wl_event_loop *
wl_jni_event_loop_from_java(JNIEnv * env, jobject jevent_loop)
{
    return event_loop_data_from_java(env, jevent_loop)->event_loop;
}

jobject
wl_jni_event_loop_to_java(JNIEnv * env, struct wl_event_loop * event_loop)
{
    jobject jevent_loop;
    
    jevent_loop = wl_jni_find_reference(env, event_loop);
    if (jevent_loop != NULL)
        return jevent_loop;

    /* Looks like we have to create it */
    ensure_event_loop_object_cache(env, NULL);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return NULL; /* Exception Thrown */

    return (*env)->NewObject(env, EventLoop.class, EventLoop.init_long,
            (long)(intptr_t)event_loop);
}

static int
handle_event_loop_fd_call(int fd, uint32_t mask, void *data)
{
    struct event_handler * handler = data;

    JNIEnv * env = wl_jni_get_env();

    int ret = (*env)->CallIntMethod(env, handler->jhandler, handler->mid,
            fd, (int)mask);

    // TODO: Handle Exceptions

    return ret;
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_EventLoop_addFileDescriptor(JNIEnv * env,
        jobject jevent_loop, int fd, int mask, jobject jhandler)
{
    struct event_loop_data * loop_data =
            event_loop_data_from_java(env, jevent_loop);

    if (fd < 0) {
        wl_jni_throw_IllegalArgumentException(env,
                "File descriptor is negative");
        return NULL; /* Exception Thrown */
    }

    struct event_handler * event_handler =
            event_loop_data_add_event_handler(env, loop_data, jhandler,
            EventLoop.FileDescriptorEventHandler.handleFileDescriptorEvent);
    if (event_handler == NULL)
        return NULL; /* Exception Thrown */

    struct wl_event_source * event_source =
            wl_event_loop_add_fd(loop_data->event_loop, fd, mask,
                handle_event_loop_fd_call, event_handler);
    if (event_source == NULL) {
        event_loop_data_remove_event_handler(env, event_handler);
        wl_jni_throw_IllegalArgumentException(env,
                strerror(errno));
        return NULL; /* Exception Thrown */
    }

    jobject jevent_source = event_source_create(env, event_source);
    if (jevent_source == NULL) {
        wl_event_source_remove(event_source);
        event_loop_data_remove_event_handler(env, event_handler);
        return NULL; /* Exception Thrown */
    }

    return jevent_source;
}

static int
handle_event_loop_timer_call(void *data)
{
    struct event_handler * handler = data;

    JNIEnv * env = wl_jni_get_env();

    int ret = (*env)->CallIntMethod(env, handler->jhandler, handler->mid);

    // TODO: Handle Exceptions

    return ret;
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_EventLoop_addTier(JNIEnv * env,
        jobject jevent_loop, jobject jhandler)
{
    struct event_loop_data * loop_data =
            event_loop_data_from_java(env, jevent_loop);

    struct event_handler * event_handler =
            event_loop_data_add_event_handler(env, loop_data, jhandler,
            EventLoop.TimerEventHandler.handleTimerEvent);
    if (event_handler == NULL)
        return NULL; /* Exception Thrown */

    struct wl_event_source * event_source =
            wl_event_loop_add_timer(loop_data->event_loop,
                handle_event_loop_timer_call, event_handler);
    if (event_source == NULL) {
        event_loop_data_remove_event_handler(env, event_handler);
        return NULL; /* Exception Thrown */
    }

    jobject jevent_source = event_source_create(env, event_source);
    if (jevent_source == NULL) {
        wl_event_source_remove(event_source);
        event_loop_data_remove_event_handler(env, event_handler);
        return NULL; /* Exception Thrown */
    }

    return jevent_source;
}

static int
handle_event_loop_signal_call(int signal_number, void * data)
{
    struct event_handler * handler = data;

    JNIEnv * env = wl_jni_get_env();

    int ret = (*env)->CallIntMethod(env, handler->jhandler, handler->mid,
            signal_number);

    // TODO: Handle Exceptions

    return ret;
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_EventLoop_addSignal(JNIEnv * env,
        jobject jevent_loop, int signal_number, jobject jhandler)
{
    struct event_loop_data * loop_data =
            event_loop_data_from_java(env, jevent_loop);

    struct event_handler * event_handler =
            event_loop_data_add_event_handler(env, loop_data, jhandler,
            EventLoop.SignalEventHandler.handleSignalEvent);
    if (event_handler == NULL)
        return NULL; /* Exception Thrown */

    struct wl_event_source * event_source =
            wl_event_loop_add_signal(loop_data->event_loop, signal_number,
                handle_event_loop_signal_call, event_handler);
    if (event_source == NULL) {
        event_loop_data_remove_event_handler(env, event_handler);
        return NULL; /* Exception Thrown */
    }

    jobject jevent_source = event_source_create(env, event_source);
    if (jevent_source == NULL) {
        wl_event_source_remove(event_source);
        event_loop_data_remove_event_handler(env, event_handler);
        return NULL; /* Exception Thrown */
    }

    return jevent_source;
}

static void
handle_event_loop_idle_call(void * data)
{
    struct event_handler * handler = data;

    JNIEnv * env = wl_jni_get_env();

    (*env)->CallVoidMethod(env, handler->jhandler, handler->mid);

    event_loop_data_remove_event_handler(env, handler);

    // TODO: Handle Exceptions
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_EventLoop_addIdle(JNIEnv * env,
        jobject jevent_loop, jobject jhandler)
{
    struct event_loop_data * loop_data =
            event_loop_data_from_java(env, jevent_loop);

    struct event_handler * event_handler =
            event_loop_data_add_event_handler(env, loop_data, jhandler,
            EventLoop.IdleHandler.handleIdle);
    if (event_handler == NULL)
        return NULL; /* Exception Thrown */

    struct wl_event_source * event_source =
            wl_event_loop_add_idle(loop_data->event_loop,
                handle_event_loop_idle_call, event_handler);
    if (event_source == NULL) {
        event_loop_data_remove_event_handler(env, event_handler);
        return NULL; /* Exception Thrown */
    }

    jobject jevent_source = event_source_create(env, event_source);
    if (jevent_source == NULL) {
        wl_event_source_remove(event_source);
        event_loop_data_remove_event_handler(env, event_handler);
        return NULL; /* Exception Thrown */
    }

    return jevent_source;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_EventLoop_dispatch(JNIEnv * env,
        jobject jevent_loop, int timeout)
{
    struct wl_event_loop * event_loop =
            wl_jni_event_loop_from_java(env, jevent_loop);

    wl_event_loop_dispatch(event_loop, timeout);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_EventLoop_dispatchIdle(JNIEnv * env,
        jobject jevent_loop)
{
    struct wl_event_loop * event_loop =
            wl_jni_event_loop_from_java(env, jevent_loop);

    wl_event_loop_dispatch_idle(event_loop);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_EventLoop__1create(JNIEnv * env,
        jobject jevent_loop, long native_ptr)
{
    struct wl_event_loop * event_loop;
    if (native_ptr) {
        event_loop = (struct wl_event_loop *)(intptr_t)native_ptr;
    } else {
        event_loop = wl_event_loop_create();
        if (event_loop == NULL) {
            wl_jni_throw_OutOfMemoryError(env, NULL);
            return;
        }
    }

    wl_jni_register_weak_reference(env, event_loop, jevent_loop);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return; /* Exception Thrown */

    (*env)->SetLongField(env, jevent_loop, EventLoop.event_loop_ptr,
            (long)(intptr_t)event_loop_data_create(env, event_loop));
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_EventLoop__1destroy(JNIEnv * env,
        jobject jevent_loop)
{
    struct event_loop_data * loop_data =
            event_loop_data_from_java(env, jevent_loop);

    if (loop_data) {
        jclass cls = (*env)->GetObjectClass(env, jevent_loop);
        jfieldID fid = (*env)->GetFieldID(env, cls, "isWrapper", "Z");
        jboolean is_wrapper = (*env)->GetBooleanField(env, jevent_loop, fid);
        
        if (is_wrapper == JNI_FALSE)
            wl_event_loop_destroy(loop_data->event_loop);

        wl_jni_unregister_reference(env, loop_data->event_loop);

        event_loop_data_destroy(env, loop_data);

        (*env)->SetLongField(env, jevent_loop, EventLoop.event_loop_ptr, 0);
    }
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_EventLoop_initializeJNI(JNIEnv * env,
        jclass cls)
{
    EventLoop.class = (*env)->NewGlobalRef(env, cls);
    if (EventLoop.class == NULL)
        return; /* Exception Thrown */

    EventLoop.event_loop_ptr =
            (*env)->GetFieldID(env, EventLoop.class, "event_loop_ptr", "J");
    if (EventLoop.event_loop_ptr == NULL)
        return; /* Exception Thrown */

    EventLoop.init_long =
            (*env)->GetMethodID(env, EventLoop.class, "<init>", "(J)V");
    if (EventLoop.init_long == NULL)
        return; /* Exception Thrown */

    cls = (*env)->FindClass(env, "org/freedesktop/wayland/server/"
            "EventLoop$EventSource");
    if (cls == NULL)
        return; /* Exception Thrown */
    EventLoop.EventSource.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (EventLoop.EventSource.class == NULL)
        return; /* Exception Thrown */

    EventLoop.EventSource.event_src_ptr =
            (*env)->GetFieldID(env, EventLoop.EventSource.class,
                "event_src_ptr", "J");
    if (EventLoop.EventSource.event_src_ptr == NULL)
        return; /* Exception Thrown */

    EventLoop.EventSource.init_long =
            (*env)->GetMethodID(env, EventLoop.EventSource.class,
                "<init>", "(J)V");
    if (EventLoop.EventSource.init_long == NULL)
        return; /* Exception Thrown */

    cls = (*env)->FindClass(env, "org/freedesktop/wayland/server/"
            "EventLoop$FileDescriptorEventHandler");
    if (cls == NULL)
        return; /* Exception Thrown */
    EventLoop.FileDescriptorEventHandler.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (EventLoop.FileDescriptorEventHandler.class == NULL)
        return; /* Exception Thrown */

    EventLoop.FileDescriptorEventHandler.handleFileDescriptorEvent =
            (*env)->GetMethodID(env,
                EventLoop.FileDescriptorEventHandler.class,
                "handleFileDescriptorEvent", "(II)I");
    if (EventLoop.FileDescriptorEventHandler.handleFileDescriptorEvent == NULL)
        return; /* Exception Thrown */

    cls = (*env)->FindClass(env, "org/freedesktop/wayland/server/"
            "EventLoop$TimerEventHandler");
    if (cls == NULL)
        return; /* Exception Thrown */
    EventLoop.TimerEventHandler.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (EventLoop.TimerEventHandler.class == NULL)
        return; /* Exception Thrown */

    EventLoop.TimerEventHandler.handleTimerEvent =
            (*env)->GetMethodID(env, EventLoop.TimerEventHandler.class,
                "handleTimerEvent", "()I");
    if (EventLoop.TimerEventHandler.handleTimerEvent == NULL)
        return; /* Exception Thrown */

    cls = (*env)->FindClass(env, "org/freedesktop/wayland/server/"
            "EventLoop$SignalEventHandler");
    if (cls == NULL)
        return; /* Exception Thrown */
    EventLoop.SignalEventHandler.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (EventLoop.SignalEventHandler.class == NULL)
        return; /* Exception Thrown */

    EventLoop.SignalEventHandler.handleSignalEvent =
            (*env)->GetMethodID(env, EventLoop.SignalEventHandler.class,
                "handleSignalEvent", "(I)I");
    if (EventLoop.SignalEventHandler.handleSignalEvent == NULL)
        return; /* Exception Thrown */

    cls = (*env)->FindClass(env, "org/freedesktop/wayland/server/"
            "EventLoop$IdleHandler");
    if (cls == NULL)
        return; /* Exception Thrown */
    EventLoop.IdleHandler.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (EventLoop.IdleHandler.class == NULL)
        return; /* Exception Thrown */

    EventLoop.IdleHandler.handleIdle =
            (*env)->GetMethodID(env, EventLoop.IdleHandler.class,
                "handleIdle", "()V");
    if (EventLoop.IdleHandler.handleIdle == NULL)
        return; /* Exception Thrown */
}

static void
ensure_event_loop_object_cache(JNIEnv * env, jclass cls)
{
    if (EventLoop.class != NULL)
        return;

    if (cls == NULL) {
        cls = (*env)->FindClass(env,
                "org/freedesktop/wayland/server/EventLoop");
        if (cls == NULL)
            return;

        Java_org_freedesktop_wayland_server_EventLoop_initializeJNI(env, cls);
        (*env)->DeleteLocalRef(env, cls);
    } else {
        Java_org_freedesktop_wayland_server_EventLoop_initializeJNI(env, cls);
    }
}

