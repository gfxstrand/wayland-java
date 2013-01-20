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
    jmethodID init_long;

    struct {
        jclass class;
        jfieldID source_ptr;
        jfieldID handler_ptr;

        jmethodID init_long_long;
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

struct event_handler {
    jobject jhandler;
    jmethodID mid;
    struct wl_listener destroy_listener;
};

struct wl_event_loop *
wl_jni_event_loop_from_java(JNIEnv * env, jobject jevent_loop)
{
    return (struct wl_event_loop *)
            wl_jni_object_wrapper_get_data(env, jevent_loop);
}

jobject
wl_jni_event_loop_to_java(JNIEnv * env, struct wl_event_loop * event_loop)
{
    return wl_jni_object_wrapper_get_java_from_data(env, event_loop);
}

jobject
wl_jni_event_loop_create(JNIEnv * env, struct wl_event_loop *loop)
{
    ensure_event_loop_object_cache(env, NULL);

    return (*env)->NewObject(env, EventLoop.class, EventLoop.init_long,
            (jlong)(intptr_t)loop);
}

static void
event_handler_destroy(JNIEnv *env, struct event_handler *handler)
{
    (*env)->DeleteGlobalRef(env, handler->jhandler);
    wl_list_remove(&handler->destroy_listener.link);
    free(handler);
}

static void
event_handler_destroy_func(struct wl_listener *listener, void *data)
{
    struct event_handler *handler;
    JNIEnv *env;

    env = wl_jni_get_env();
    handler = wl_container_of(listener, handler, destroy_listener);

    event_handler_destroy(env, handler);
}

static jobject
create_source_wrapper(JNIEnv *env, struct wl_event_loop *loop,
        struct wl_event_source *source, struct event_handler *handler,
        jobject jhandler, jmethodID method)
{
    handler->jhandler = (*env)->NewGlobalRef(env, jhandler);
    if (handler->jhandler == NULL)
        return NULL; /* Exception Thrown */

    handler->mid = method;
    handler->destroy_listener.notify = event_handler_destroy_func;
    wl_event_loop_add_destroy_listener(loop, &handler->destroy_listener);

    return (*env)->NewObject(env, EventLoop.EventSource.class,
            EventLoop.EventSource.init_long_long,
            (jlong)(intptr_t)source, (jlong)(intptr_t)handler);
}

static int
handle_event_loop_fd_call(int fd, uint32_t mask, void *data)
{
    struct event_handler * handler = data;

    JNIEnv * env = wl_jni_get_env();

    int ret = (*env)->CallIntMethod(env, handler->jhandler, handler->mid,
            (jint)fd, (jint)mask);

    // TODO: Handle Exceptions

    return ret;
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_EventLoop_addFileDescriptor(JNIEnv * env,
        jobject jevent_loop, jint fd, jint mask, jobject jhandler)
{
    struct wl_event_loop *loop;
    struct event_handler *handler;
    struct wl_event_source *source;
    jobject jsource;
    
    loop = wl_jni_event_loop_from_java(env, jevent_loop);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return NULL;

    if (fd < 0) {
        wl_jni_throw_IllegalArgumentException(env,
                "File descriptor is negative");
        return NULL; /* Exception Thrown */
    }
    
    handler = malloc(sizeof(struct event_handler));
    if (handler == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    source = wl_event_loop_add_fd(loop, fd, (uint32_t)mask,
            handle_event_loop_fd_call, handler);
    if (source == NULL) {
        free(handler);
        wl_jni_throw_from_errno(env, errno);
        return NULL; /* Exception Thrown */
    }
    
    jsource = create_source_wrapper(env, loop, source, handler, jhandler,
            EventLoop.FileDescriptorEventHandler.handleFileDescriptorEvent);

    if (jsource == NULL) {
        free(handler);
        wl_event_source_remove(source);
        return NULL; /* Exception Thrown */
    }

    return jsource;
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
    struct wl_event_loop *loop;
    struct event_handler *handler;
    struct wl_event_source *source;
    jobject jsource;
    
    loop = wl_jni_event_loop_from_java(env, jevent_loop);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return NULL;
    
    handler = malloc(sizeof(struct event_handler));
    if (handler == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    source = wl_event_loop_add_timer(loop,
                handle_event_loop_timer_call, handler);
    if (source == NULL) {
        free(handler);
        wl_jni_throw_from_errno(env, errno);
        return NULL; /* Exception Thrown */
    }
    
    jsource = create_source_wrapper(env, loop, source, handler, jhandler,
            EventLoop.TimerEventHandler.handleTimerEvent);

    if (jsource == NULL) {
        free(handler);
        wl_event_source_remove(source);
        return NULL; /* Exception Thrown */
    }

    return jsource;
}

static int
handle_event_loop_signal_call(int signal_number, void * data)
{
    struct event_handler * handler = data;

    JNIEnv * env = wl_jni_get_env();

    int ret = (*env)->CallIntMethod(env, handler->jhandler, handler->mid,
            (jint)signal_number);

    // TODO: Handle Exceptions

    return ret;
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_EventLoop_addSignal(JNIEnv * env,
        jobject jevent_loop, jint signal_number, jobject jhandler)
{
    struct wl_event_loop *loop;
    struct event_handler *handler;
    struct wl_event_source *source;
    jobject jsource;
    
    loop = wl_jni_event_loop_from_java(env, jevent_loop);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return NULL;
    
    handler = malloc(sizeof(struct event_handler));
    if (handler == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    source = wl_event_loop_add_signal(loop, signal_number,
                handle_event_loop_signal_call, handler);
    if (source == NULL) {
        free(handler);
        wl_jni_throw_from_errno(env, errno);
        return NULL; /* Exception Thrown */
    }
    
    jsource = create_source_wrapper(env, loop, source, handler, jhandler,
            EventLoop.SignalEventHandler.handleSignalEvent);

    if (jsource == NULL) {
        free(handler);
        wl_event_source_remove(source);
        return NULL; /* Exception Thrown */
    }

    return jsource;
}

static void
handle_event_loop_idle_call(void * data)
{
    struct event_handler * handler = data;

    JNIEnv * env = wl_jni_get_env();

    (*env)->CallVoidMethod(env, handler->jhandler, handler->mid);

    event_handler_destroy(env, handler);

    // TODO: Handle Exceptions
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_EventLoop_addIdle(JNIEnv * env,
        jobject jevent_loop, jobject jhandler)
{
    struct wl_event_loop *loop;
    struct event_handler *handler;
    struct wl_event_source *source;
    jobject jsource;
    
    loop = wl_jni_event_loop_from_java(env, jevent_loop);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return NULL;
    
    handler = malloc(sizeof(struct event_handler));
    if (handler == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    source = wl_event_loop_add_idle(loop, handle_event_loop_idle_call, handler);
    if (source == NULL) {
        free(handler);
        wl_jni_throw_from_errno(env, errno);
        return NULL; /* Exception Thrown */
    }
    
    jsource = create_source_wrapper(env, loop, source, handler, jhandler,
            EventLoop.IdleHandler.handleIdle);

    if (jsource == NULL) {
        free(handler);
        wl_event_source_remove(source);
        return NULL; /* Exception Thrown */
    }

    return jsource;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_EventLoop_dispatch(JNIEnv * env,
        jobject jevent_loop, jint timeout)
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
        jobject jevent_loop, jlong native_ptr)
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

    wl_jni_object_wrapper_set_data(env, jevent_loop, event_loop);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_EventLoop__1destroy(JNIEnv *env,
        jobject jloop)
{
    struct wl_event_loop *loop;
    
    loop = wl_jni_event_loop_from_java(env, jloop);

    if (loop == NULL)
        return;

    wl_event_loop_destroy(loop);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_EventLoop_initializeJNI(JNIEnv * env,
        jclass cls)
{
    EventLoop.class = (*env)->NewGlobalRef(env, cls);
    if (EventLoop.class == NULL)
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

    EventLoop.EventSource.source_ptr =
            (*env)->GetFieldID(env, EventLoop.EventSource.class,
                "source_ptr", "J");
    if (EventLoop.EventSource.source_ptr == NULL)
        return; /* Exception Thrown */

    EventLoop.EventSource.handler_ptr =
            (*env)->GetFieldID(env, EventLoop.EventSource.class,
                "handler_ptr", "J");
    if (EventLoop.EventSource.handler_ptr == NULL)
        return; /* Exception Thrown */

    EventLoop.EventSource.init_long_long =
            (*env)->GetMethodID(env, EventLoop.EventSource.class,
                "<init>", "(JJ)V");
    if (EventLoop.EventSource.init_long_long == NULL)
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

