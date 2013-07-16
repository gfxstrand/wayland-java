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
#include <stdarg.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-util.h>

#include "server-jni.h"

struct {
    jclass class;
    jfieldID resource_ptr;
    jfieldID data;
    jmethodID destroy;
} Resource;

struct {
    jclass class;
    jfieldID errorCode;
} RequestError;

struct {
    struct {
        struct {
            jclass class;
            jmethodID getMessage;
        } Throwable;
        struct {
            jclass class;
        } OutOfMemoryError;
        struct {
            jclass class;
        } NoSuchMethodError;
    } lang;
} java;

struct wl_resource *
wl_jni_resource_from_java(JNIEnv * env, jobject jresource)
{
    if (jresource == NULL)
        return NULL;

    return (struct wl_resource *)(intptr_t)
            (*env)->GetLongField(env, jresource, Resource.resource_ptr);
}

jobject
wl_jni_resource_to_java(JNIEnv * env, struct wl_resource * resource)
{
    if (resource == NULL)
        return NULL;

    return (*env)->NewLocalRef(env, resource->data);
}

static void
resource_destroyed(struct wl_resource * resource)
{
    JNIEnv * env;

    if (resource == NULL)
        return;

    env = wl_jni_get_env();
    (*env)->DeleteGlobalRef(env, resource->data);
    free(resource);
}

JNIEXPORT jlong JNICALL
Java_org_freedesktop_wayland_server_Resource_createNative(JNIEnv * env,
        jobject jresource, jobject jclient, jobject jiface, jint version,
        jint id)
{
    struct wl_client * client;
    struct wl_resource * resource;
    struct wl_jni_interface *jni_interface;

    client = wl_jni_client_from_java(env, jclient);
    if (client == NULL) {
        wl_jni_throw_NullPointerException(env,
                "Client not allowed to be null");
    }

    jni_interface = wl_jni_interface_from_java(env, jiface);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return 0; /* Exception Thrown */
    if (jni_interface == NULL) {
        wl_jni_throw_NullPointerException(env,
                "Interface not allowed to be null");
        return 0;
    }

    jresource = (*env)->NewGlobalRef(env, jresource);
    if (jresource == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return 0;
    }

    resource = wl_resource_create(client, &jni_interface->interface,
            version, id);
    if (resource == NULL) {
        (*env)->DeleteGlobalRef(env, jresource);
        wl_jni_throw_from_errno(env, errno);
        return 0;
    }
    wl_resource_set_dispatcher(resource, wl_jni_resource_dispatcher,
            jni_interface->requests, jresource, resource_destroyed);

    return (jlong)(intptr_t)resource;
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_Resource_getClient(JNIEnv * env,
        jobject jresource)
{
    struct wl_resource *resource;

    resource = wl_jni_resource_from_java(env, jresource);
    if (resource == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);
        return NULL;
    }

    return wl_jni_client_to_java(env, resource->client);
}

JNIEXPORT jint JNICALL
Java_org_freedesktop_wayland_server_Resource_getId(JNIEnv * env,
        jobject jresource)
{
    struct wl_resource *resource;

    resource = wl_jni_resource_from_java(env, jresource);
    if (resource == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);
        return 0;
    }

    return resource->object.id;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_addDestroyListener(JNIEnv * env,
        jobject jresource, jobject jlistener)
{
    struct wl_resource * resource;
    struct wl_jni_destroy_listener * jni_listener;

    if ((*env)->IsSameObject(env, jlistener, NULL)) {
        wl_jni_throw_NullPointerException(env,
                "Listener not allowed to be null");
        return;
    }

    jni_listener = wl_jni_destroy_listener_add_to_signal(env, jlistener);
    if (jni_listener == NULL)
        return; /*Exception throw */

    resource = wl_jni_resource_from_java(env, jresource);

    wl_signal_add(&resource->destroy_signal, &jni_listener->listener);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_destroy(JNIEnv * env,
        jobject jresource)
{
    struct wl_resource * resource = wl_jni_resource_from_java(env, jresource);

    if (resource == NULL)
        return;

    wl_resource_destroy(resource);

    (*env)->SetLongField(env, jresource, Resource.resource_ptr, (jlong)0);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_postEvent(JNIEnv * env,
        jobject jresource, jint opcode, jarray jargs)
{
    struct wl_resource *resource;
    union wl_argument *args;
    const char *signature;
    int nargs;

    resource = wl_jni_resource_from_java(env, jresource);

    signature = resource->object.interface->events[opcode].signature;
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

    signature = resource->object.interface->events[opcode].signature;
    wl_jni_arguments_from_java(env, args, jargs, signature, nargs,
            (struct wl_object *(*)(JNIEnv *, jobject))&wl_jni_resource_from_java);
    if ((*env)->ExceptionCheck(env))
        return;

    wl_resource_post_event_array(resource, opcode, args);

    wl_jni_arguments_from_java_destroy(args, signature, nargs);
    free(args);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_postError(JNIEnv * env,
        jobject jresource, jint code, jstring jmsg)
{
    struct wl_resource *resource;
    char *msg;

    resource = wl_jni_resource_from_java(env, jresource);

    msg = wl_jni_string_to_utf8(env, jmsg);
    if ((*env)->ExceptionCheck(env))
        return;

    if (msg == NULL) {
        wl_resource_post_error(resource, code, "");
    } else {
        wl_resource_post_error(resource, code, "%s", msg);
    }

    free(msg);
}

/* TODO: This code DOES NOT WORK!!! */
static int
handle_resource_errors(JNIEnv * env, struct wl_resource * resource)
{
    jthrowable exception, exception2;
    jstring message;
    char * c_msg;
    int error_code;

    exception = (*env)->ExceptionOccurred(env);

    if (exception == NULL)
        return 0;

    (*env)->ExceptionDescribe(env);

    if ((*env)->IsInstanceOf(env, exception,
            java.lang.OutOfMemoryError.class)) {
        goto out_of_memory;
    }
    
    if ((*env)->IsInstanceOf(env, exception,
            java.lang.NoSuchMethodError.class) ||
        (*env)->IsInstanceOf(env, exception, RequestError.class))
    {
        (*env)->ExceptionClear(env);

        message = (*env)->CallObjectMethod(env, exception,
                java.lang.Throwable.getMessage);

        exception2 = (*env)->ExceptionOccurred(env);
        if (exception2 != NULL) {
            if ((*env)->IsInstanceOf(env, exception,
                    java.lang.OutOfMemoryError.class)) {
                goto out_of_memory;
            } else {
                goto unhandled_exception;
            }
        }

        c_msg = wl_jni_string_to_utf8(env, message);
        exception2 = (*env)->ExceptionOccurred(env);
        if (exception2 != NULL) {
            if ((*env)->IsInstanceOf(env, exception,
                    java.lang.OutOfMemoryError.class)) {
                goto out_of_memory;
            } else {
                goto unhandled_exception;
            }
        }

        if ((*env)->IsInstanceOf(env, exception,
                java.lang.NoSuchMethodError.class)) {
            wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_METHOD,
                    "%s", c_msg);
            free(c_msg);
            (*env)->ExceptionClear(env);
            return 0;
        } else if ((*env)->IsInstanceOf(env, exception,
                RequestError.class)) {
            error_code = (*env)->GetIntField(env, exception,
                    RequestError.errorCode);
            wl_resource_post_error(resource, error_code, "%s", c_msg);
            free(c_msg);
            (*env)->ExceptionClear(env);
            return 0;
        } else {
            free(c_msg);
            goto unhandled_exception;
        }
    } else {
        goto unhandled_exception;
    }

out_of_memory:
    wl_resource_post_no_memory(resource);
    (*env)->ExceptionClear(env);
    return 0;

unhandled_exception:
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
    return -1;
}

int
wl_jni_resource_dispatcher(const void *data, void *target, uint32_t opcode,
        const struct wl_message *message, union wl_argument *args)
{
    struct wl_resource *resource;
    const char *signature;
    int nargs, nrefs;

    jvalue *jargs;
    JNIEnv *env;
    jobject jimplementation, jresource;
    jmethodID mid;

    resource = wl_container_of(target, resource, object);

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
            ++nrefs;
        /* These types don't require references */
        case 'u':
        case 'i':
        case 'n':
        case 'h':
            ++nargs;
            break;
        case '?':
            break;
        }
    }

    jargs = malloc(sizeof(jvalue) * (nargs + 1));
    if (jargs == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto handle_exceptions; /* Exception Thrown */
    }

    if ((*env)->PushLocalFrame(env, nrefs + 2) < 0)
        goto handle_exceptions; /* Exception Thrown */

    jresource = wl_jni_resource_to_java(env, resource);
    if ((*env)->ExceptionCheck(env)) {
        goto pop_local_frame;
    } else if (jresource == NULL) {
        wl_jni_throw_NullPointerException(env, "Resource should not be null");
        goto pop_local_frame;
    }

    jimplementation = (*env)->GetObjectField(env, jresource, Resource.data);
    if ((*env)->ExceptionCheck(env))
        goto pop_local_frame;

    wl_jni_arguments_to_java(env, args, jargs + 1, message->signature, nargs,
            JNI_FALSE,
            (jobject(*)(JNIEnv *, struct wl_object *))&wl_jni_resource_to_java);

    if ((*env)->ExceptionCheck(env))
        goto pop_local_frame;

    jargs[0].l = jresource;
    mid = ((jmethodID *)data)[opcode];
    (*env)->CallVoidMethodA(env, jimplementation, mid, jargs);

pop_local_frame:
    (*env)->PopLocalFrame(env, NULL);
    free(jargs);

handle_exceptions:
    /* Handle Exceptions here */
    return handle_resource_errors(env, resource);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_initializeJNI(JNIEnv * env,
        jclass cls)
{
    Resource.class = (*env)->NewGlobalRef(env, cls);
    if (Resource.class == NULL)
        return; /* Exception Thrown */

    Resource.resource_ptr = (*env)->GetFieldID(env, Resource.class,
            "resource_ptr", "J");
    if (Resource.resource_ptr == NULL)
        return; /* Exception Thrown */

    Resource.data = (*env)->GetFieldID(env, Resource.class,
            "data", "Ljava/lang/Object;");
    if (Resource.data == NULL)
        return; /* Exception Thrown */

    cls = (*env)->FindClass(env,
            "org/freedesktop/wayland/server/RequestError");
    if (cls == NULL)
        return; /* Exception Thrown */
    RequestError.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (RequestError.class == NULL)
        return; /* Exception Thrown */

    RequestError.errorCode = (*env)->GetFieldID(env, RequestError.class,
            "errorCode", "I");
    if (RequestError.errorCode == NULL)
        return; /* Exception Thrown */

    cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
    if (cls == NULL)
        return; /* Exception Thrown */
    java.lang.OutOfMemoryError.class = (*env)->NewGlobalRef(env, cls);
    if (java.lang.OutOfMemoryError.class == NULL)
        return; /* Exception Thrown */
    (*env)->DeleteLocalRef(env, cls);

    cls = (*env)->FindClass(env, "java/lang/NoSuchMethodError");
    if (cls == NULL)
        return; /* Exception Thrown */
    java.lang.NoSuchMethodError.class = (*env)->NewGlobalRef(env, cls);
    if (java.lang.NoSuchMethodError.class == NULL)
        return; /* Exception Thrown */
    (*env)->DeleteLocalRef(env, cls);

    cls = (*env)->FindClass(env, "java/lang/Throwable");
    if (cls == NULL)
        return; /* Exception Thrown */
    java.lang.Throwable.class = (*env)->NewGlobalRef(env, cls);
    if (java.lang.Throwable.class == NULL)
        return; /* Exception Thrown */
    (*env)->DeleteLocalRef(env, cls);

    java.lang.Throwable.getMessage = (*env)->GetMethodID(env,
            java.lang.Throwable.class, "getMessage", "()Ljava/lang/String;");
    if (java.lang.Throwable.getMessage == NULL)
        return; /* Exception Thrown */
}

