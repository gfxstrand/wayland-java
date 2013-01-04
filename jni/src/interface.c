#include "wayland-jni.h"

#include <stdlib.h>

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

struct wl_interface *
wl_jni_interface_from_java(JNIEnv * env, jobject jinterface)
{
    if (jinterface == NULL)
        return NULL;

    return (struct wl_interface *)(*env)->GetLongField(env, jinterface,
            Interface.interface_ptr);
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

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_Interface_createNative(JNIEnv * env,
        jobject jinterface)
{
    struct wl_interface * interface;
    jstring jstr;
    jarray jarr;
    jobject jobj;
    int method, event;
    struct wl_message * methods, * events;

    interface = malloc(sizeof(struct wl_interface));
    if (interface == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    (*env)->SetLongField(env, jinterface, Interface.interface_ptr,
            (long)interface);

    jstr = (*env)->GetObjectField(env, jinterface, Interface.name);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_interface;

    interface->name = wl_jni_string_to_utf8(env, jstr);
    (*env)->DeleteLocalRef(env, jstr);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_interface;

    interface->version =
            (*env)->GetIntField(env, jinterface, Interface.version);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_name;

    /* Get the requests */
    jarr = (*env)->GetObjectField(env, jinterface, Interface.requests); 
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_name;
    if (jarr == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);
        goto delete_name;
    }

    interface->method_count = (*env)->GetArrayLength(env, jarr);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_name;

    methods = malloc(interface->method_count * sizeof(struct wl_message)); 
    if (methods == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto delete_name;
    }

    interface->methods = methods;

    for (method = 0; method < interface->method_count; ++method) {
        jobj = (*env)->GetObjectArrayElement(env, jarr, method);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_methods;

        get_native_message(env, jobj, methods + method);
        (*env)->DeleteLocalRef(env, jobj);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) goto delete_methods;
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

    return;

delete_events:
    --event;
    for (; event >= 0; --event) {
        free((void *)interface->events[event].name);
        free((void *)interface->events[event].signature);
        free((void *)interface->events[event].types);
    }
    free((void *)interface->events);

delete_methods:
    --method;
    for (; method >= 0; --method) {
        free((void *)interface->methods[method].name);
        free((void *)interface->methods[method].signature);
        free((void *)interface->methods[method].types);
    }
    free((void *)interface->methods);

delete_name:
    if (interface->name != NULL)
        free((void *)interface->name);

delete_interface:
    free(interface);

    (*env)->SetLongField(env, jinterface, Interface.interface_ptr, 0);
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

