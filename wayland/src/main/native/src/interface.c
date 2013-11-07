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

    jfieldID interface_ptr;

    jfieldID name;
    jfieldID version;
    jfieldID requests;
    jfieldID requestsIfaces;
    jfieldID events;
    jfieldID eventsIfaces;
    jfieldID proxyClass;
    jfieldID resourceClass;

    struct {
        jclass class;

        jfieldID name;
        jfieldID signature;
        jfieldID types;
    } Message;
} Interface;

struct {
    struct {
        struct {
            jclass class;

            jmethodID getName;
        } Class;
    } lang;
} java;

enum interface_type {
    INTERFACE_REQUESTS = 0,
    INTERFACE_EVENTS = 1,
    INTERFACE_PROXY = 2,
    INTERFACE_RESOURCE = 3
};

char *
get_proxy_java_name(JNIEnv *env, jobject jinterface, enum interface_type iface)
{
    jclass cls;
    jstring jname;
    char *name, *p;

    switch (iface) {
    case INTERFACE_PROXY:
        cls = (*env)->GetObjectField(env, jinterface, Interface.proxyClass);
        break;
    case INTERFACE_RESOURCE:
        cls = (*env)->GetObjectField(env, jinterface, Interface.resourceClass);
        break;
    default:
        break;
    }

    if ((*env)->ExceptionCheck(env))
        return NULL;

    jname = (*env)->CallObjectMethod(env, cls, java.lang.Class.getName);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionCheck(env))
        return NULL;

    name = wl_jni_string_to_default(env, jname);

    for (p = name; *p; p++)
        if (*p == '.')
            *p = '/';

    (*env)->DeleteLocalRef(env, jname);
    return name;
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

        msg->types[type] = &wl_jni_interface_from_java(env, jobj)->interface;
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

#define MAX_JSIG_LEN 1024

static jmethodID
get_java_method(JNIEnv * env, jobject jinterface, struct wl_message *message,
        enum interface_type iface)
{
    const char *signature, *proxyName;
    char jsignature[MAX_JSIG_LEN];
    jclass cls;
    jarray classList;

    if (iface == INTERFACE_EVENTS) {
        classList = (*env)->GetObjectField(env,
                jinterface, Interface.eventsIfaces);
    } else if (iface == INTERFACE_REQUESTS) {
        classList = (*env)->GetObjectField(env,
                jinterface, Interface.requestsIfaces);
    }
    if ((*env)->ExceptionCheck(env))
        return NULL;
    cls = (*env)->GetObjectArrayElement(env, classList,
            (*env)->GetArrayLength(env, classList) - 1);
    if ((*env)->ExceptionCheck(env))
        return NULL;

    jsignature[0] = '(';
    jsignature[1] = '\0';

    if (iface == INTERFACE_EVENTS) {
        proxyName = get_proxy_java_name(env, jinterface, INTERFACE_PROXY);
    } else if (iface == INTERFACE_REQUESTS) {
        proxyName = get_proxy_java_name(env, jinterface, INTERFACE_RESOURCE);
    }
    if (proxyName == NULL)
        return NULL;

    strncat(jsignature, "L", MAX_JSIG_LEN);
    strncat(jsignature, proxyName, MAX_JSIG_LEN);
    strncat(jsignature, ";", MAX_JSIG_LEN);

    for (signature = message->signature; *signature; ++signature) {
        switch (*signature) {
        case '?':
            continue;
        case 'i':
            strncat(jsignature, "I", MAX_JSIG_LEN);
            break;
        case 'u':
            strncat(jsignature, "I", MAX_JSIG_LEN);
            break;
        case 'f':
            strncat(jsignature, "Lorg/freedesktop/wayland/Fixed;", MAX_JSIG_LEN);
            break;
        case 's':
            strncat(jsignature, "Ljava/lang/String;", MAX_JSIG_LEN);
            break;
        case 'o':
            if (iface == INTERFACE_EVENTS)
                strncat(jsignature, "Lorg/freedesktop/wayland/client/Proxy;",
                        MAX_JSIG_LEN);
            else if (iface == INTERFACE_REQUESTS)
                strncat(jsignature, "Lorg/freedesktop/wayland/server/Resource;",
                        MAX_JSIG_LEN);
            break;
        case 'n':
            if (iface == INTERFACE_EVENTS)
                strncat(jsignature, "Lorg/freedesktop/wayland/client/Proxy;",
                        MAX_JSIG_LEN);
            else if (iface == INTERFACE_REQUESTS)
                strncat(jsignature, "I", MAX_JSIG_LEN);
            break;
        case 'h':
            strncat(jsignature, "I", MAX_JSIG_LEN);
            break;
        default:
            break;
        }
    }
    strncat(jsignature, ")V", MAX_JSIG_LEN);

    return (*env)->GetMethodID(env, cls, message->name, jsignature);
}

static struct wl_jni_interface *
create_native_interface(JNIEnv *env, jobject jinterface)
{
    struct wl_jni_interface *jni_interface;
    struct wl_interface *interface;
    struct wl_message * methods, * events;
    int method, event;
    jarray jarr;
    jobject jobj;
    jstring jstr;

    jni_interface = malloc(sizeof(struct wl_jni_interface));
    if (jni_interface == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }
    memset(jni_interface, 0, sizeof(*jni_interface));

    interface = &jni_interface->interface;

    jstr = (*env)->GetObjectField(env, jinterface, Interface.name);
    if ((*env)->ExceptionCheck(env))
        goto delete_interface;

    interface->name = wl_jni_string_to_utf8(env, jstr);
    (*env)->DeleteLocalRef(env, jstr);
    if ((*env)->ExceptionCheck(env))
        goto delete_interface;

    interface->version =
            (*env)->GetIntField(env, jinterface, Interface.version);
    if ((*env)->ExceptionCheck(env))
        goto delete_name;

    /* Get the requests */
    jarr = (*env)->GetObjectField(env, jinterface, Interface.requests); 
    if ((*env)->ExceptionCheck(env))
        goto delete_name;
    if (jarr == NULL) {
        wl_jni_throw_NullPointerException(env, "Null requests array");
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
    jni_interface->requests = malloc(interface->method_count
            * sizeof(*jni_interface->requests)); 
    if (jni_interface->requests == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        free(methods);
        goto delete_name;
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
                methods + method, INTERFACE_REQUESTS);
        (*env)->DeleteLocalRef(env, jobj);
        if ((*env)->ExceptionCheck(env))
            goto delete_methods;
    }
    (*env)->DeleteLocalRef(env, jarr);

    jarr = (*env)->GetObjectField(env, jinterface, Interface.events); 
    if ((*env)->ExceptionCheck(env)) goto delete_methods;
    if (jarr == NULL) {
        wl_jni_throw_NullPointerException(env, "Null events array");
        goto delete_methods;
    }

    interface->event_count = (*env)->GetArrayLength(env, jarr);
    if ((*env)->ExceptionCheck(env)) goto delete_methods;

    events = malloc(interface->event_count * sizeof(struct wl_message)); 
    if (events == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto delete_methods;
    }
    jni_interface->events = malloc(interface->event_count
            * sizeof(*jni_interface->events)); 
    if (jni_interface->events == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        free(events);
        goto delete_methods;
    }

    interface->events = events;

    for (event = 0; event < interface->event_count; ++event) {
        jobj = (*env)->GetObjectArrayElement(env, jarr, event);
        if ((*env)->ExceptionCheck(env)) goto delete_events;

        get_native_message(env, jobj, events + event);
        (*env)->DeleteLocalRef(env, jobj);
        if ((*env)->ExceptionCheck(env)) goto delete_events;

        jni_interface->events[event] = get_java_method(env, jinterface,
                events + event, INTERFACE_EVENTS);
        if ((*env)->ExceptionCheck(env))
            goto delete_events;
    }
    (*env)->DeleteLocalRef(env, jarr);

    (*env)->SetLongField(env, jinterface, Interface.interface_ptr,
            (jlong)(intptr_t)jni_interface);
    if ((*env)->ExceptionCheck(env))
        goto delete_events;

    return jni_interface;

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

    return NULL;
}

struct wl_jni_interface *
wl_jni_interface_from_java(JNIEnv * env, jobject jinterface)
{
    struct wl_jni_interface *jni_interface;

    if (jinterface == NULL)
        return NULL;

    jni_interface = (struct wl_jni_interface *)(intptr_t)(*env)->GetLongField(
            env, jinterface, Interface.interface_ptr);

    if (jni_interface != NULL)
        return jni_interface;
    else
        return create_native_interface(env, jinterface);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_Interface_destroyNative(JNIEnv * env,
        jobject jinterface)
{
    struct wl_jni_interface * jni_interface;
    int i;

    jni_interface = wl_jni_interface_from_java(env, jinterface);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE || jni_interface == NULL) {
        return;
    }

    /* Free the events */
    for (i = 0; i < jni_interface->interface.event_count; ++i) {
        free((void *)jni_interface->interface.events[i].name);
        free((void *)jni_interface->interface.events[i].signature);
        free((void *)jni_interface->interface.events[i].types);
    }
    free((void *)jni_interface->interface.events);

    /* Free the methods */
    for (i = 0; i < jni_interface->interface.method_count; ++i) {
        free((void *)jni_interface->interface.methods[i].name);
        free((void *)jni_interface->interface.methods[i].signature);
        free((void *)jni_interface->interface.methods[i].types);
    }
    free((void *)jni_interface->interface.methods);

    /* Free the name */
    if (jni_interface->interface.name != NULL)
        free((void *)jni_interface->interface.name);

    /* Free the methodID arrays */
    free(jni_interface->requests);
    free(jni_interface->events);

    /* Free the actual interface */
    free(jni_interface);

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

    Interface.version = (*env)->GetFieldID(env, Interface.class,
            "version", "I");
    if (Interface.version == NULL) return; /* Exception Thrown */

    Interface.requests = (*env)->GetFieldID(env, Interface.class,
            "requests", "[Lorg/freedesktop/wayland/Interface$Message;");
    if (Interface.requests == NULL) return; /* Exception Thrown */

    Interface.requestsIfaces = (*env)->GetFieldID(env, Interface.class,
            "requestsIfaces", "[Ljava/lang/Class;");
    if (Interface.requestsIfaces == NULL) return; /* Exception Thrown */

    Interface.events = (*env)->GetFieldID(env, Interface.class,
            "events", "[Lorg/freedesktop/wayland/Interface$Message;");
    if (Interface.events == NULL) return; /* Exception Thrown */

    Interface.eventsIfaces = (*env)->GetFieldID(env, Interface.class,
            "eventsIfaces", "[Ljava/lang/Class;");
    if (Interface.eventsIfaces == NULL) return; /* Exception Thrown */

    Interface.proxyClass = (*env)->GetFieldID(env, Interface.class,
            "proxyClass", "Ljava/lang/Class;");
    if (Interface.proxyClass == NULL) return; /* Exception Thrown */

    Interface.resourceClass = (*env)->GetFieldID(env, Interface.class,
            "resourceClass", "Ljava/lang/Class;");
    if (Interface.resourceClass == NULL) return; /* Exception Thrown */

    Interface.interface_ptr = (*env)->GetFieldID(env, Interface.class,
            "interface_ptr", "J");
    if (Interface.interface_ptr == NULL) return; /* Exception Thrown */

    cls = (*env)->FindClass(env, "org/freedesktop/wayland/Interface$Message");
    Interface.Message.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return;

    Interface.Message.name = (*env)->GetFieldID(env, Interface.Message.class,
            "name", "Ljava/lang/String;");
    if (Interface.Message.name == NULL) return; /* Exception Thrown */

    Interface.Message.signature = (*env)->GetFieldID(env,
            Interface.Message.class, "signature", "Ljava/lang/String;");
    if (Interface.Message.signature == NULL) return; /* Exception Thrown */

    Interface.Message.types = (*env)->GetFieldID(env, Interface.Message.class,
            "types", "[Lorg/freedesktop/wayland/Interface;");
    if (Interface.Message.types == NULL) return; /* Exception Thrown */

    cls = (*env)->FindClass(env, "java/lang/Class");
    java.lang.Class.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return;

    java.lang.Class.getName = (*env)->GetMethodID(env,
            java.lang.Class.class, "getName", "()Ljava/lang/String;");
    if (java.lang.Class.getName == NULL) return; /* Exception Thrown */
}

