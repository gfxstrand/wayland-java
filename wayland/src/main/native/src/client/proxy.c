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
#include <errno.h>

#include <wayland-client.h>

#include "client-jni.h"

struct {
    jclass class;
    jfieldID proxy_ptr;
    jfieldID userData;
    jfieldID listener;
    jfieldID iface;
} Proxy;

static int
wl_jni_proxy_dispatcher(const void *data, void *target, uint32_t opcode,
        const struct wl_message *message, union wl_argument *args);

struct wl_proxy *
wl_jni_proxy_from_java(JNIEnv * env, jobject jproxy)
{
    if (jproxy == NULL)
        return NULL;

    return (struct wl_proxy *)(intptr_t)
            (*env)->GetLongField(env, jproxy, Proxy.proxy_ptr);
}

jobject
wl_jni_proxy_to_java(JNIEnv * env, struct wl_proxy * proxy)
{
    return (*env)->NewLocalRef(env, wl_proxy_get_user_data(proxy));
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Proxy_createNative(JNIEnv * env,
        jobject jproxy, jobject jfactory, jobject jinterface)
{
    struct wl_proxy *proxy, *factory;
    struct wl_jni_interface *interface;
    jobject self_ref;

    factory = wl_jni_proxy_from_java(env, jfactory);
    if (factory == NULL) {
        wl_jni_throw_NullPointerException(env,
                "factory not allowed to be null");
        return;
    }

    interface = wl_jni_interface_from_java(env, jinterface);
    if ((*env)->ExceptionCheck(env))
        return;
    if (interface == NULL) {
        wl_jni_throw_NullPointerException(env,
                "interface not allowed to be null");
        return;
    }

    proxy = wl_proxy_create(factory, &interface->interface);
    if (proxy == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
    }

    self_ref = (*env)->NewGlobalRef(env, jproxy);
    if (self_ref == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        wl_proxy_destroy(proxy);
        return;
    }

    (*env)->SetLongField(env, jproxy, Proxy.proxy_ptr, (jlong)(intptr_t)proxy);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->DeleteGlobalRef(env, self_ref);
        wl_proxy_destroy(proxy);
    }
    wl_proxy_add_dispatched_listener(proxy, wl_jni_proxy_dispatcher,
            interface->events, self_ref);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Proxy_marshal(JNIEnv * env, jobject jproxy,
        jint opcode, jarray jargs)
{
    struct wl_proxy *proxy;
    struct wl_jni_interface *interface;
    union wl_argument *args;
    const char *signature;
    int nargs;
    jobject jinterface;

    proxy = wl_jni_proxy_from_java(env, jproxy);
    if (proxy == NULL) {
        wl_jni_throw_IllegalStateException(env, "proxy already destroyed");
        return;
    }

    jinterface = (*env)->GetObjectField(env, jproxy, Proxy.iface);
    if ((*env)->ExceptionCheck(env))
        return;

    interface = wl_jni_interface_from_java(env, jinterface);
    if ((*env)->ExceptionCheck(env))
        return;
    if (interface == NULL) {
        wl_jni_throw_NullPointerException(env,
                "INTERNAL ERROR: null Proxy.iface");
        return;
    }

    signature = interface->interface.methods[opcode].signature;
    nargs = 0;
    while (*signature) {
        if (*signature != '?')
            nargs++;
        signature++;
    }

    args = malloc(nargs * sizeof(union wl_argument));
    if (args == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    signature = interface->interface.methods[opcode].signature;
    wl_jni_arguments_from_java(env, args, jargs, signature, nargs,
            (struct wl_object *(*)(JNIEnv *, jobject))&wl_jni_proxy_from_java);
    if ((*env)->ExceptionCheck(env))
        return;

    wl_proxy_marshal_a(proxy, opcode, args);

    wl_jni_arguments_from_java_destroy(args, signature, nargs);
    free(args);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Proxy_destroy(JNIEnv * env, jobject jproxy)
{
    struct wl_proxy * proxy = wl_jni_proxy_from_java(env, jproxy);

    if (proxy == NULL)
        return;

    (*env)->DeleteGlobalRef(env, wl_proxy_get_user_data(proxy));

    wl_proxy_destroy(proxy);

    (*env)->SetLongField(env, jproxy, Proxy.proxy_ptr, (jlong)0);
}

JNIEXPORT jint JNICALL
Java_org_freedesktop_wayland_client_Proxy_getID(JNIEnv * env, jobject jproxy)
{
    struct wl_proxy *proxy;

    proxy = wl_jni_proxy_from_java(env, jproxy);
    if (proxy == NULL) {
        wl_jni_throw_IllegalStateException(env, "proxy already destroyed");
        return -1;
    }

    return wl_proxy_get_id(proxy);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Proxy_setQueue(JNIEnv * env, jobject jproxy,
        jobject jqueue)
{
    struct wl_proxy *proxy;
    struct wl_event_queue *queue;

    proxy = wl_jni_proxy_from_java(env, jproxy);
    if (proxy == NULL) {
        wl_jni_throw_IllegalStateException(env, "proxy already destroyed");
        return;
    }

    queue = wl_jni_event_queue_from_java(env, jqueue);
    if (queue == NULL) {
        wl_jni_throw_NullPointerException(env, "queue not allowed to be null");
        return;
    }

    return wl_proxy_set_queue(proxy, queue);
}

static int
wl_jni_proxy_dispatcher(const void *data, void *target, uint32_t opcode,
        const struct wl_message *message, union wl_argument *args)
{
    struct wl_proxy *proxy;
    const char *signature;
    int nargs, nrefs;

    jvalue *jargs;
    JNIEnv *env;
    jobject jlistener, jproxy;
    jmethodID mid;

    proxy = target;

    env = wl_jni_get_env();

    /* Count the number of arguments and references */
    nargs = 0;
    nrefs = 0;
    for (signature = message->signature; *signature != '\0'; ++signature) {
        switch (*signature) {
        /* These types will require references */
        case 'f':
        case 's':
        case 'o':
        case 'a':
        case 'n':
            ++nrefs;
        /* These types don't require references */
        case 'u':
        case 'i':
        case 'h':
            ++nargs;
            break;
        case '?':
            break;
        }
    }

    jargs = malloc((nargs + 1) * sizeof *jargs);
    if (jargs == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto exception_check;
    }

    if ((*env)->PushLocalFrame(env, nrefs + 2) < 0)
        goto exception_check;

    jproxy = wl_jni_proxy_to_java(env, proxy);
    if ((*env)->ExceptionCheck(env)) {
        goto pop_local_frame;
    } else if (jproxy == NULL) {
        wl_jni_throw_NullPointerException(env, "Proxy should not be null");
        goto pop_local_frame;
    }

    jlistener = (*env)->GetObjectField(env, jproxy, Proxy.listener);
    if ((*env)->ExceptionCheck(env))
        goto pop_local_frame;

    wl_jni_arguments_to_java(env, args, jargs + 1, message->signature, nargs,
            JNI_TRUE,
            (jobject(*)(JNIEnv *, struct wl_object *))&wl_jni_proxy_to_java);

    if ((*env)->ExceptionCheck(env))
        goto pop_local_frame;

    jargs[0].l = jproxy;
    mid = ((jmethodID *)data)[opcode];
    (*env)->CallVoidMethodA(env, jlistener, mid, jargs);

pop_local_frame:
    (*env)->PopLocalFrame(env, NULL);
    free(jargs);

exception_check:
    if ((*env)->ExceptionCheck(env))
        return -1;
    else
        return 0;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_client_Proxy_initializeJNI(JNIEnv * env,
        jclass cls)
{
    Proxy.class = (*env)->NewGlobalRef(env, cls);
    if (Proxy.class == NULL)
        return; /* Exception Thrown */

    Proxy.proxy_ptr = (*env)->GetFieldID(env, Proxy.class,
            "proxy_ptr", "J");
    if (Proxy.proxy_ptr == NULL)
        return; /* Exception Thrown */

    Proxy.userData = (*env)->GetFieldID(env, Proxy.class,
            "userData", "Ljava/lang/Object;");
    if (Proxy.userData == NULL)
        return; /* Exception Thrown */
    Proxy.listener = (*env)->GetFieldID(env, Proxy.class,
            "listener", "Ljava/lang/Object;");
    if (Proxy.listener == NULL)
        return; /* Exception Thrown */

    Proxy.iface = (*env)->GetFieldID(env, Proxy.class,
            "iface", "Lorg/freedesktop/wayland/Interface;");
    if (Proxy.iface == NULL)
        return; /* Exception Thrown */
}

