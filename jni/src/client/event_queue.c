
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
#include <wayland-client.h>

#include "client/client-jni.h"

struct wl_event_queue *
wl_jni_event_queue_from_java(JNIEnv * env, jobject jqueue)
{
    jclass cls;
    jfieldID fid;

    cls = (*env)->GetObjectClass(env, jqueue);
    if (cls == NULL)
        return NULL;
    
    fid = (*env)->GetFieldID(env, cls, "event_queue_ptr", "J");
    if (fid == NULL)
        return NULL;

    return (struct wl_event_queue *)(intptr_t)
            (*env)->GetLongField(env, jqueue, fid);
}

jobject
wl_jni_event_queue_create_from_native(JNIEnv * env,
        struct wl_event_queue *queue)
{
    jclass cls;
    jmethodID ctor;
    jobject jobj;

    if (queue == NULL)
        return NULL;

    cls = (*env)->FindClass(env, "org/freedesktop/wayland/client/EventQueue");
    if (cls == NULL)
        return NULL;

    ctor = (*env)->GetMethodID(env, cls, "<init>", "J");
    if (ctor == NULL)
        return NULL; /* Exception Thrown */

    jobj = (*env)->NewObject(env, cls, ctor, (jlong)(intptr_t)queue);
    if (jobj == NULL)
        return NULL; /* Exception Thrown */

    return jobj;
}

