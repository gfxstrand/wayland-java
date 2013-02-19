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
#include "wayland-jni.h"
#include "server/server-jni.h"

#include <stdlib.h>
#include <string.h>

static struct {
    jclass class;

    jfieldID name;
    jfieldID clazz;
    jfieldID version;
    jfieldID requests;
    jfieldID events;
    jfieldID interface_ptr;

    struct {
        jclass class;

        jfieldID name;
        jfieldID signature;
        jfieldID javaSignature;
        jfieldID types;
    } Message;
} Interface;

wl_interface_dispatcher_func_t wl_jni_resource_dispatcher;
wl_interface_dispatcher_func_t wl_jni_proxy_dispatcher;

struct wl_jni_interface
{
    struct wl_interface interface;
    jmethodID *requests;
    jmethodID *events;
};

struct wl_interface *
wl_jni_interface_from_java(JNIEnv * env, jobject jinterface)
{
    struct wl_jni_interface *jni_interface;

    if (jinterface == NULL)
        return NULL;

    jni_interface = (struct wl_jni_interface *)(intptr_t)(*env)->GetLongField(
            env, jinterface, Interface.interface_ptr);

    return &jni_interface->interface;
}

void
wl_jni_interface_init_object(JNIEnv * env, jobject jinterface,
        struct wl_object * obj)
{
    if ((*env)->IsSameObject(env, jinterface, NULL) || obj == NULL)
        return;

    struct wl_jni_interface *jni_interface;
    jni_interface = (struct wl_jni_interface *)(intptr_t)(*env)->GetLongField(
            env, jinterface, Interface.interface_ptr);

    obj->interface = &jni_interface->interface;
    /* TODO: Handle events too */
    obj->implementation = jni_interface->requests;
}

static void
get_native_message(JNIEnv * env, jobject jmsg, struct wl_message * msg)
{
    jobject jobj;
    jstring jstr;
    jarray jarr;
    int num_types, type;

    jstr = (*env)->GetObjectField(env, jmsg, Interface.Message.name);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) return;

    msg->name = wl_jni_string_to_utf8(env, jstr);
    (*env)->DeleteLocalRef(env, jstr);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return;

    jstr = (*env)->GetObjectField(env, jmsg, Interface.Message.signature);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_name;

    msg->signature = wl_jni_string_to_default(env, jstr);
    (*env)->DeleteLocalRef(env, jstr);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        goto delete_name;

    jarr = (*env)->GetObjectField(env, jmsg, Interface.Message.types);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_signature;
    if (jarr == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);;
        goto delete_signature;
    }

    num_types = (*env)->GetArrayLength(env, jarr);

    msg->types = malloc(num_types * sizeof(struct wl_interface *));
    if (msg->types == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);;
        goto delete_signature;
    }

    for (type = 0; type < num_types; ++type) {
        jobj = (*env)->GetObjectArrayElement(env, jarr, type);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_types;

        msg->types[type] = wl_jni_interface_from_java(env, jobj);
        (*env)->DeleteLocalRef(env, jobj);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_types;
    }

    return;

delete_types:
    free((void *)msg->types);
delete_signature:
    if (msg->signature)
        free((void *)msg->signature);
delete_name:
    if (msg->name)
        free((void *)msg->name);
}

static void
destroy_native_message(const struct wl_message * msg)
{
    free((void *)msg->name);
    free((void *)msg->signature);
    free((void *)msg->types);
}

static jmethodID
get_java_method(JNIEnv * env, jobject jinterface, jobject jmsg,
        const char *data_type)
{
    jobject jstr, cls;
    char *name, *signature, *full_signature;
    int len;
    jmethodID mid;

    mid = NULL;

    jstr = (*env)->GetObjectField(env, jmsg, Interface.Message.name);
    if ((*env)->ExceptionCheck(env))
        return NULL;

    name = wl_jni_string_to_default(env, jstr);
    (*env)->DeleteLocalRef(env, jstr);
    if ((*env)->ExceptionCheck(env))
        return NULL;

    jstr = (*env)->GetObjectField(env, jmsg, Interface.Message.javaSignature);
    if ((*env)->ExceptionCheck(env))
        goto delete_name;

    signature = wl_jni_string_to_utf8(env, jstr);
    if ((*env)->ExceptionCheck(env))
        goto delete_name;

    cls = (*env)->GetObjectField(env, jinterface, Interface.clazz);

    len = strlen(signature) + strlen(data_type) + strlen("()V") + 1;
    full_signature = malloc(len);
    if (full_signature == NULL) {
        (*env)->DeleteLocalRef(env, cls);
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto delete_signature;
    }

    full_signature[0] = '(';
    full_signature[1] = '\0';

    strcat(full_signature, data_type);
    strcat(full_signature, signature);
    strcat(full_signature, ")V");

    mid = (*env)->GetMethodID(env, cls, name, full_signature);
    (*env)->DeleteLocalRef(env, cls);

delete_signature:
    free(signature);
delete_name:
    free(name);

    return mid;
}

void
method_dispatch_forward(struct wl_object *target, uint32_t opcode,
        const struct wl_message *message, void *data, union wl_argument *args)
{
    (*wl_jni_resource_dispatcher)(target, opcode, message, data, args);
}

void
event_dispatch_forward(struct wl_object *target, uint32_t opcode,
        const struct wl_message *message, void *data, union wl_argument *args)
{
    (*wl_jni_proxy_dispatcher)(target, opcode, message, data, args);
}


JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_Interface_createNative(JNIEnv * env,
        jobject jinterface, jlong implementation_ptr)
{
    struct wl_jni_interface *jni_interface;
    struct wl_interface *interface;
    jstring jstr;
    jarray jarr;
    jobject jobj;
    int method, event;
    struct wl_message * methods, * events;

    jni_interface = malloc(sizeof(struct wl_jni_interface));
    if (jni_interface == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    interface = &jni_interface->interface;

    jstr = (*env)->GetObjectField(env, jinterface, Interface.name);
    if ((*env)->ExceptionCheck(env))
        goto delete_interface;

    interface->name = wl_jni_string_to_utf8(env, jstr);
    (*env)->DeleteLocalRef(env, jstr);
    if ((*env)->ExceptionCheck(env))
        goto delete_interface;

    interface->version = (1 << 16) |
            (*env)->GetIntField(env, jinterface, Interface.version);
    if ((*env)->ExceptionCheck(env))
        goto delete_name;

    /* Get the requests */
    jarr = (*env)->GetObjectField(env, jinterface, Interface.requests); 
    if ((*env)->ExceptionCheck(env))
        goto delete_name;
    if (jarr == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);
        goto delete_name;
    }

    interface->method_count = (*env)->GetArrayLength(env, jarr);
    if ((*env)->ExceptionCheck(env))
        goto delete_name;

    methods = malloc(interface->method_count * sizeof(struct wl_message)); 
    if (methods == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto delete_name;
    }

    jni_interface->requests =
            malloc(interface->method_count * sizeof(jmethodID));
    if (jni_interface->requests == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto delete_methods;
    }

    interface->methods = methods;

    for (method = 0; method < interface->method_count; ++method) {
        jobj = (*env)->GetObjectArrayElement(env, jarr, method);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE)
            goto delete_methods;

        get_native_message(env, jobj, methods + method);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
            (*env)->DeleteLocalRef(env, jobj);
            goto delete_methods;
        }

        jni_interface->requests[method] = get_java_method(env, jinterface,
                jobj, "Lorg/freedesktop/wayland/server/Client;");
        (*env)->DeleteLocalRef(env, jobj);
        if ((*env)->ExceptionCheck(env))
            goto delete_methods;
    }
    (*env)->DeleteLocalRef(env, jarr);

    /* Get the events */
    jarr = (*env)->GetObjectField(env, jinterface, Interface.events); 
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_methods;
    if (jarr == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);
        goto delete_methods;
    }

    interface->event_count = (*env)->GetArrayLength(env, jarr);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_methods;

    events = malloc(interface->event_count * sizeof(struct wl_message)); 
    if (events == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto delete_methods;
    }
    interface->events = events;

    for (event = 0; event < interface->event_count; ++event) {
        jobj = (*env)->GetObjectArrayElement(env, jarr, event);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_events;

        get_native_message(env, jobj, events + event);
        (*env)->DeleteLocalRef(env, jobj);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_events;
    }
    (*env)->DeleteLocalRef(env, jarr);

    interface->method_dispatcher = &method_dispatch_forward;
    interface->event_dispatcher = &event_dispatch_forward;

    (*env)->SetLongField(env, jinterface, Interface.interface_ptr,
            (jlong)jni_interface);
    if ((*env)->ExceptionCheck(env))
        goto delete_events;

    return;

delete_events:
    --event;
    for (; event >= 0; --event)
        destroy_native_message(&interface->events[event]);
    free((void *)interface->events);
    free(jni_interface->events);

delete_methods:
    --method;
    for (; method >= 0; --method)
        destroy_native_message(&interface->methods[method]);
    free((void *)interface->methods);
    free(jni_interface->requests);

delete_name:
    if (interface->name != NULL)
        free((void *)interface->name);

delete_interface:
    free(jni_interface);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_Interface_destroyNative(JNIEnv * env,
        jobject jinterface)
{
    struct wl_interface * interface;
    int i;

    interface = wl_jni_interface_from_java(env, jinterface);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE || interface == NULL) {
        return;
    }

    /* Free the events */
    for (i = 0; i < interface->event_count; ++i) {
        free((void *)interface->events[i].name);
        free((void *)interface->events[i].signature);
        free((void *)interface->events[i].types);
    }
    free((void *)interface->events);

    /* Free the methods */
    for (i = 0; i < interface->method_count; ++i) {
        free((void *)interface->methods[i].name);
        free((void *)interface->methods[i].signature);
        free((void *)interface->methods[i].types);
    }
    free((void *)interface->methods);

    /* Free the name */
    if (interface->name != NULL)
        free((void *)interface->name);

    /* Free the actual interface */
    free(interface);

    (*env)->SetLongField(env, jinterface, Interface.interface_ptr, 0);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_Interface_initializeJNI(JNIEnv * env,
        jclass cls)
{
    Interface.class = (*env)->NewGlobalRef(env, cls);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    Interface.name = (*env)->GetFieldID(env, Interface.class,
            "name", "Ljava/lang/String;");
    if (Interface.name == NULL) return; /* Exception Thrown */

    Interface.clazz = (*env)->GetFieldID(env, Interface.class,
            "clazz", "Ljava/lang/Class;");
    if (Interface.clazz == NULL) return; /* Exception Thrown */

    Interface.version = (*env)->GetFieldID(env, Interface.class,
            "version", "I");
    if (Interface.version == NULL) return; /* Exception Thrown */

    Interface.requests = (*env)->GetFieldID(env, Interface.class,
            "requests", "[Lorg/freedesktop/wayland/Interface$Message;");
    if (Interface.requests == NULL) return; /* Exception Thrown */

    Interface.events = (*env)->GetFieldID(env, Interface.class,
            "events", "[Lorg/freedesktop/wayland/Interface$Message;");
    if (Interface.events == NULL) return; /* Exception Thrown */

    Interface.interface_ptr = (*env)->GetFieldID(env, Interface.class,
            "interface_ptr", "J");
    if (Interface.interface_ptr == NULL) return; /* Exception Thrown */

    cls = (*env)->FindClass(env, "org/freedesktop/wayland/Interface$Message");
    Interface.Message.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    Interface.Message.name = (*env)->GetFieldID(env, Interface.Message.class,
            "name", "Ljava/lang/String;");
    if (Interface.Message.name == NULL) return; /* Exception Thrown */

    Interface.Message.signature = (*env)->GetFieldID(env,
            Interface.Message.class, "signature", "Ljava/lang/String;");
    if (Interface.Message.signature == NULL) return; /* Exception Thrown */

    Interface.Message.javaSignature = (*env)->GetFieldID(env,
            Interface.Message.class, "javaSignature", "Ljava/lang/String;");
    if (Interface.Message.javaSignature == NULL) return; /* Exception Thrown */

    Interface.Message.types = (*env)->GetFieldID(env, Interface.Message.class,
            "types", "[Lorg/freedesktop/wayland/Interface;");
    if (Interface.Message.types == NULL) return; /* Exception Thrown */
}

