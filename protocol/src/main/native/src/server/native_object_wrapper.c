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

#include "server-jni.h"

struct {
    jfieldID data_ptr;
} NativeObjectWrapper;

struct wl_jni_object_wrapper *
wl_jni_object_wrapper_from_java(JNIEnv *env, jobject jwrapper)
{
    if ((*env)->IsSameObject(env, jwrapper, NULL))
        return NULL;

    return (struct wl_jni_object_wrapper *)(intptr_t)
            (*env)->GetLongField(env, jwrapper, NativeObjectWrapper.data_ptr);
}

void *
wl_jni_object_wrapper_get_data(JNIEnv *env, jobject jwrapper)
{
    struct wl_jni_object_wrapper *wrapper;

    wrapper = wl_jni_object_wrapper_from_java(env, jwrapper);
    if ((*env)->ExceptionCheck(env))
        return NULL; /* Exception Thrown */

    if (wrapper == NULL)
        return NULL;

    return wrapper->data;
}

void
native_object_wrapper_destroy_notify(struct wl_listener *listener, void *data)
{
    struct wl_jni_object_wrapper *wrapper;
    JNIEnv *env;

    env = wl_jni_get_env();
    
    wrapper = wl_container_of(listener, wrapper, destroy_listener);

    wl_jni_object_wrapper_disowned(env, wrapper->self_ref,
            wrapper->destroyed_by_owner);
}

struct wl_jni_object_wrapper *
wl_jni_object_wrapper_set_data(JNIEnv *env, jobject jwrapper, void *data)
{
    struct wl_jni_object_wrapper *wrapper;

    wrapper = wl_jni_object_wrapper_from_java(env, jwrapper);
    if ((*env)->ExceptionCheck(env))
        return NULL; /* Exception Thrown */

    if (wrapper != NULL) {
        wl_jni_throw_by_name(env, "java/lang/IllegalStateException",
                "NativeObjectWrapper: data already assigned");
        return NULL; /* Exception Thrown */
    }

    wrapper = malloc(sizeof(struct wl_jni_object_wrapper));
    if (wrapper == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL; /* Exception Thrown */
    }
    memset(wrapper, 0, sizeof(struct wl_jni_object_wrapper));

    wl_jni_register_weak_reference(env, data, jwrapper);
    if ((*env)->ExceptionCheck(env)) {
        free(wrapper);
        return NULL; /* Exception Thrown */
    }

    (*env)->SetLongField(env, jwrapper, NativeObjectWrapper.data_ptr,
            (jlong)(intptr_t)wrapper);
    if ((*env)->ExceptionCheck(env)) {
        wl_jni_unregister_reference(env, data);
        free(wrapper);
        return NULL; /* Exception Thrown */
    }

    wrapper->data = data;
    wrapper->destroy_listener.notify = &native_object_wrapper_destroy_notify;

    return wrapper;
}

void
wl_jni_object_wrapper_owned(JNIEnv * env, jobject jwrapper, jobject global_ref,
        jboolean destroyed_by_owner)
{
    struct wl_jni_object_wrapper *wrapper;

    wrapper = wl_jni_object_wrapper_from_java(env, jwrapper);
    if ((*env)->ExceptionCheck(env))
        return; /* Exception Thrown */

    if (wrapper == NULL) {
        wl_jni_throw_by_name(env, "java/lang/IllegalStateException",
                "NativeObjectWrapper: null object cannot be owned");
        return; /* Exception Thrown */
    }

    if (global_ref) {
        wrapper->self_ref = global_ref;
    } else {
        wrapper->self_ref = (*env)->NewGlobalRef(env, jwrapper);
    }
}

void
wl_jni_object_wrapper_disowned(JNIEnv * env, jobject jwrapper,
        jboolean destroy)
{
    struct wl_jni_object_wrapper *wrapper;

    wrapper = wl_jni_object_wrapper_from_java(env, jwrapper);
    if ((*env)->ExceptionCheck(env))
        return; /* Exception Thrown */

    if (wrapper == NULL)
        return; /* Exit silently here */

    /* TODO: Do I really want to throw an exception in this case? */
    if (wrapper->self_ref == NULL) {
        wl_jni_throw_by_name(env, "java/lang/IllegalStateException",
                "NativeObjectWrapper: object already disowned");
        return; /* Exception Thrown */
    }

    wl_list_remove(&wrapper->destroy_listener.link);

    if (destroy) {
        (*env)->SetLongField(env, jwrapper, NativeObjectWrapper.data_ptr, 0);
        (*env)->DeleteGlobalRef(env, wrapper->self_ref);
        free(wrapper);
    } else {
        (*env)->DeleteGlobalRef(env, wrapper->self_ref);
        wrapper->self_ref = NULL;
    }
}

jobject
wl_jni_object_wrapper_get_java_from_data(JNIEnv * env, void * data)
{
    return wl_jni_find_reference(env, data);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_NativeObjectWrapper_destroyNative(
        JNIEnv *env, jobject jwrapper)
{
    struct wl_jni_object_wrapper *wrapper;

    wrapper = wl_jni_object_wrapper_from_java(env, jwrapper);
    if ((*env)->ExceptionCheck(env))
        return; /* Exception Thrown */

    if (wrapper == NULL)
        return; /* Exit silently here */

    if (wrapper->self_ref) {
        /* This should never happen */
        wl_jni_object_wrapper_disowned(env, jwrapper, JNI_FALSE);
    }

    (*env)->SetLongField(env, jwrapper, NativeObjectWrapper.data_ptr, 0);
    free(wrapper);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_NativeObjectWrapper_initializeJNI(
        JNIEnv *env, jclass cls)
{
    NativeObjectWrapper.data_ptr = (*env)->GetFieldID(env, cls,
            "data_ptr", "J");
    if ((*env)->ExceptionCheck(env))
        return; /* Exception Thrown */
}

