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
    jfieldID iface;
    jmethodID bindClient;
} Global;

struct wl_global *
wl_jni_global_from_java(JNIEnv * env, jobject jglobal)
{
    return (struct wl_global *)wl_jni_object_wrapper_get_data(env, jglobal);
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

void
wl_jni_global_add_to_display(JNIEnv *env, jobject jglobal,
        struct wl_display *display)
{
    struct wl_jni_object_wrapper *wrapper;
    struct wl_jni_interface * jni_interface;
    struct wl_global * global;
    jobject jinterface, self_ref;

    jinterface = (*env)->GetObjectField(env, jglobal, Global.iface);
    if ((*env)->ExceptionCheck(env))
        return; /* Exception Thrown */

    jni_interface = wl_jni_interface_from_java(env, jinterface);
    if ((*env)->ExceptionCheck(env))
        return; /* Exception Thrown */

    self_ref = (*env)->NewGlobalRef(env, jglobal);
    if ((*env)->ExceptionCheck(env))
        return; /* Exception Thrown */

    global = wl_display_add_global(display, &jni_interface->interface,
            self_ref, &wl_jni_global_bind_func);

    wl_jni_object_wrapper_set_data(env, jglobal, global);
    if ((*env)->ExceptionCheck(env)) {
        wl_display_remove_global(display, global);
        return; /* Exception Thrown */
    }

    /* The wrapper may not be available until after we set the data */
    wrapper = wl_jni_object_wrapper_from_java(env, jglobal);
    if (wrapper == NULL) {
        wl_display_remove_global(display, global);
        return; /* Exception Thrown */
    }

    wl_jni_object_wrapper_owned(env, jglobal, self_ref, JNI_TRUE);
    if ((*env)->ExceptionCheck(env)) {
        wl_display_remove_global(display, global);
        return; /* Exception Thrown */
    }

    wl_display_add_destroy_listener(display, &wrapper->destroy_listener);
}

void
wl_jni_global_remove_from_display(JNIEnv *env, jobject jglobal,
        struct wl_display *display)
{
    struct wl_global *global;

    global = wl_jni_global_from_java(env, jglobal);
    if ((*env)->ExceptionCheck(env))
        return /* Exception Thrown */

    wl_display_remove_global(display, global);
    wl_jni_object_wrapper_disowned(env, jglobal, JNI_TRUE);
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

    Global.iface = (*env)->GetFieldID(env, Global.class,
            "iface", "Lorg/freedesktop/wayland/Interface;");
    if (Global.iface == NULL)
        return; /* Exception Thrown */

    Global.bindClient = (*env)->GetMethodID(env, Global.class, "bindClient",
            "(Lorg/freedesktop/wayland/server/Client;II)V");
}

