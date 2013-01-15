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
#include <string.h>
#include <wayland-server.h>

#include "server/server-jni.h"

struct {
    jclass class;
    jfieldID global_ptr;
    jfieldID self_ref;
    jfieldID iface;
    jfieldID handler;

    struct {
        jclass class;
        jmethodID bindClient;
    } BindHandler;
} Global;

struct wl_global *
wl_jni_global_from_java(JNIEnv * env, jobject jglobal)
{
    if (jglobal == NULL)
        return NULL;

    return (struct wl_global *)(intptr_t)
            (*env)->GetLongField(env, jglobal, Global.global_ptr);
}

jobject
wl_jni_global_get_interface(JNIEnv * env, jobject jglobal)
{
    return (*env)->GetObjectField(env, jglobal, Global.iface);
}

/**
 * Sets the native data for the global.  Note that the jglobal variable MUST be
 * a global reference. This global reference will be deleted by
 * wl_jni_global_release.  If wl_jni_global_set_data fails, the reference not
 * be deleted.
 */
void
wl_jni_global_set_data(JNIEnv * env, jobject jglobal, jobject self_ref,
        struct wl_global * global)
{
    (*env)->SetLongField(env, jglobal, Global.self_ref,
            (jlong)(intptr_t)self_ref);
    (*env)->SetLongField(env, jglobal, Global.global_ptr,
            (jlong)(intptr_t)global);
}

// FIXME: This isn't exception-safe!!!
void
wl_jni_global_bind_func(struct wl_client * client, void * data,
        uint32_t version, uint32_t id)
{
    JNIEnv * env;
    jobject jglobal;
    jobject jhandler;
    jobject jclient;

    env = wl_jni_get_env();

    jglobal = (*env)->NewLocalRef(env, data);
    if (jglobal == NULL)
        return;

    jhandler = (*env)->GetObjectField(env, jglobal, Global.handler);
    if (jhandler == NULL)
        return;

    jclient = wl_jni_client_to_java(env, client);

    (*env)->CallVoidMethod(env, jhandler, Global.BindHandler.bindClient,
            jclient, (jint)version, (jint)id);

    (*env)->DeleteLocalRef(env, jglobal);
}

void
wl_jni_global_release(JNIEnv * env, jobject jglobal)
{
    jobject self_ref;

    self_ref = (jobject)(intptr_t)
            (*env)->GetLongField(env, jglobal, Global.self_ref);

    (*env)->DeleteWeakGlobalRef(env, self_ref);

    (*env)->SetLongField(env, jglobal, Global.global_ptr, 0);
    (*env)->SetLongField(env, jglobal, Global.self_ref, 0);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Global__1destroy(JNIEnv * env,
        jobject jglobal)
{
    jobject self_ref;

    self_ref = (jobject)(intptr_t)
            (*env)->GetLongField(env, jglobal, Global.self_ref);

    if (self_ref == NULL)
        return;

    (*env)->DeleteWeakGlobalRef(env, self_ref);

    (*env)->SetLongField(env, jglobal, Global.global_ptr, 0);
    (*env)->SetLongField(env, jglobal, Global.self_ref, 0);
}
        
JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Global_initializeJNI(JNIEnv * env,
        jclass cls)
{
    Global.class = (*env)->NewGlobalRef(env, cls);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    Global.global_ptr = (*env)->GetFieldID(env, Global.class,
            "global_ptr", "J");
    if (Global.global_ptr == NULL)
        return; /* Exception Thrown */

    Global.self_ref = (*env)->GetFieldID(env, Global.class,
            "self_ref", "J");
    if (Global.self_ref == NULL)
        return; /* Exception Thrown */

    Global.iface = (*env)->GetFieldID(env, Global.class,
            "iface", "Lorg/freedesktop/wayland/Interface;");
    if (Global.iface == NULL)
        return; /* Exception Thrown */

    Global.handler = (*env)->GetFieldID(env, Global.class,
            "handler", "Lorg/freedesktop/wayland/server/Global$BindHandler;");
    if (Global.handler == NULL)
        return; /* Exception Thrown */

    cls = (*env)->FindClass(env,
            "org/freedesktop/wayland/server/Global$BindHandler");
    if (cls == NULL)
        return; /* Exception Thrown */
    Global.BindHandler.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }
    Global.BindHandler.bindClient = (*env)->GetMethodID(env,
            Global.BindHandler.class, "bindClient",
            "(Lorg/freedesktop/wayland/server/Client;II)V");
    if (Global.BindHandler.bindClient == NULL)
        return; /* Exception Thrown */
}

