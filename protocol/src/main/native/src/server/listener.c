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
#include "server-jni.h"

#include <stdlib.h>
#include <string.h>

struct {
    jclass class;
    jfieldID listener_ptr;
    jmethodID onDestroy;
} DestroyListener;

void Java_org_freedesktop_wayland_server_DestroyListener_detach(JNIEnv * env,
        jobject jlistener);

struct wl_jni_destroy_listener *
wl_jni_destroy_listener_from_java(JNIEnv * env, jobject jlistener)
{
    if (jlistener == NULL)
        return NULL;

    return (struct wl_jni_destroy_listener *)(intptr_t)
            (*env)->GetLongField(env, jlistener, DestroyListener.listener_ptr);
}

static void
listener_notify_func(struct wl_listener * listener, void * data)
{
    struct wl_jni_destroy_listener * jni_listener;
    JNIEnv * env;
    jobject jlistener;

    jni_listener = wl_container_of(listener, jni_listener, listener);

    env = wl_jni_get_env();

    jlistener = (*env)->NewLocalRef(env, jni_listener->self_ref);

    /* TODO: Do something with the data parameter? */
    (*env)->CallVoidMethod(env, jni_listener->self_ref, DestroyListener.onDestroy);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        (*env)->DeleteLocalRef(env, jlistener);
        (*env)->ExceptionDescribe(env);
        return;
        /* TODO */
    }

    Java_org_freedesktop_wayland_server_DestroyListener_detach(env,
            jlistener);
    (*env)->DeleteLocalRef(env, jlistener);
}

struct wl_jni_destroy_listener *
wl_jni_destroy_listener_add_to_signal(JNIEnv * env, jobject jlistener)
{
    struct wl_jni_destroy_listener * jni_listener;

    jni_listener = wl_jni_destroy_listener_from_java(env, jlistener);

    if (jni_listener != NULL) {
        wl_jni_throw_IllegalStateException(env, "DestroyListener already attached");
        return NULL;
    }

    jni_listener = malloc(sizeof(struct wl_jni_destroy_listener));
    if (jni_listener == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    memset(jni_listener, 0, sizeof(struct wl_jni_destroy_listener));
    jni_listener->listener.notify = &listener_notify_func;

    jni_listener->self_ref = (*env)->NewGlobalRef(env, jlistener);
    if (jni_listener->self_ref == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        free(jni_listener);
        return NULL;
    }

    (*env)->SetLongField(env, jlistener, DestroyListener.listener_ptr,
            (jlong)(intptr_t)jni_listener);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->DeleteGlobalRef(env, jni_listener->self_ref);
        free(jni_listener);
        return NULL;
    }

    return jni_listener;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_DestroyListener_detach(JNIEnv * env,
        jobject jlistener)
{
    struct wl_jni_destroy_listener * jni_listener;
    
    jni_listener = wl_jni_destroy_listener_from_java(env, jlistener);
    if ((*env)->ExceptionCheck(env))
        return;

    if (jni_listener == NULL)
        return;

    wl_list_remove(&jni_listener->listener.link);

    (*env)->DeleteGlobalRef(env, jni_listener->self_ref);
    free(jni_listener);
    (*env)->SetLongField(env, jlistener, DestroyListener.listener_ptr, 0);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_DestroyListener_initializeJNI(JNIEnv * env,
        jclass cls)
{
    DestroyListener.class = (*env)->NewGlobalRef(env, cls);
    if (DestroyListener.class == NULL)
        return; /* Exception Thrown */
        
    DestroyListener.listener_ptr = (*env)->GetFieldID(env, DestroyListener.class,
            "listener_ptr", "J");
    if (DestroyListener.listener_ptr == NULL)
        return; /* Exception Thrown */

    DestroyListener.onDestroy = (*env)->GetMethodID(env, DestroyListener.class,
            "onDestroy", "()V");
    if (DestroyListener.onDestroy == NULL)
        return; /* Exception Thrown */
}

