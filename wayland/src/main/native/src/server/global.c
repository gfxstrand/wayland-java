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
    jmethodID bindClient;
} Global;

struct wl_global *
wl_jni_global_from_java(JNIEnv * env, jobject jglobal)
{
    if (jglobal == NULL)
        return NULL;

    return (struct wl_global *)(intptr_t)
            (*env)->GetLongField(env, jglobal, Global.global_ptr);
}

// FIXME: This isn't exception-safe!!!
static void
wl_jni_global_bind_func(struct wl_client * client, void * data,
        uint32_t version, uint32_t id)
{
    JNIEnv * env;
    jobject jglobal;
    jobject jclient;

    env = wl_jni_get_env();

    jglobal = (*env)->NewLocalRef(env, data);
    if (jglobal == NULL)
        goto exception;

    jclient = wl_jni_client_to_java(env, client);
    if ((*env)->ExceptionCheck(env))
        goto exception;

    (*env)->CallVoidMethod(env, jglobal, Global.bindClient, jclient,
            (jint)version, (jint)id);

    (*env)->DeleteLocalRef(env, jglobal);
    (*env)->DeleteLocalRef(env, jclient);

exception:
    if ((*env)->ExceptionCheck(env))
        (*env)->ExceptionDescribe(env);
}

JNIEXPORT jlong JNICALL
Java_org_freedesktop_wayland_server_Global_createNative(JNIEnv * env,
        jobject jglobal, jobject jdisplay, jobject jinterface, jint version)
{
    struct wl_display *display;
    struct wl_jni_interface * jni_interface;
    struct wl_global * global;

    display = wl_jni_display_from_java(env, jdisplay);
    if ((*env)->ExceptionCheck(env))
        return 0;
    jni_interface = wl_jni_interface_from_java(env, jinterface);
    if ((*env)->ExceptionCheck(env))
        return 0;

    jglobal = (*env)->NewGlobalRef(env, jglobal);
    if (jglobal == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return 0;
    }

    global = wl_global_create(display, &jni_interface->interface,
            version, jglobal, &wl_jni_global_bind_func);

    if (global == NULL) {
        (*env)->DeleteGlobalRef(env, jglobal);
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return 0;
    }

    return (jlong)(intptr_t)global;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Global_destroy(JNIEnv * env, jclass jglobal)
{
    struct wl_global *global;

    global = wl_jni_global_from_java(env, jglobal);
    if ((*env)->ExceptionCheck(env))
        return /* Exception Thrown */

    (*env)->DeleteGlobalRef(env, wl_global_get_user_data(global));
    wl_global_destroy(global);
    (*env)->SetLongField(env, jglobal, Global.global_ptr, 0);
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

    Global.bindClient = (*env)->GetMethodID(env, Global.class, "bindClient",
            "(Lorg/freedesktop/wayland/server/Client;II)V");
}

