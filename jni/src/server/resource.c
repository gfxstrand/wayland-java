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

#include <wayland-server.h>
#include <wayland-util.h>

#include "server-jni.h"

/* NOTE ON RESOURCES AND GARBAGE COLLECTION:
 *
 * At all times the resources->data member will contain either a weak global or
 * a global reference to the java resource object. This way the reference is
 * always available from any thread. In order for garbage collection to work
 * properly the exact type of reference is determined as follows:
 *
 * If the resource is unattached, i.e. the resource->client member is NULL then
 * resources->data will hold a weak global reference.  This way it can be
 * garbage collected
 *
 * If the resource is attached, i.e. the resource->client member is not NULL
 * then resources->data will hold a global reference. In this way, it is
 * considered to be "owned" by the client to which it is attached and will not
 * be arbitrarily garbage-collected.
 */

struct {
    jclass class;
    jfieldID resource_ptr;
    jfieldID data;
    jmethodID init_long_obj;
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
        struct {
            jmethodID intValue;
        } Integer;
    } lang;
} java;

static void ensure_resource_object_cache(JNIEnv * env, jclass cls);

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
    return (*env)->NewLocalRef(env, resource->data);
}

void
wl_jni_resource_set_client(JNIEnv * env, struct wl_resource * resource,
        struct wl_client * client)
{
    jobject local_ref, new_ref;

    if (client == NULL && resource->client == NULL)
        return;

    local_ref = (*env)->NewLocalRef(env, resource->data);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return; /* Exception Thrown */

    // TODO: What if local_ref == NULL? Can this even happen?

    if (client == NULL) {
        /*
         * We are unsetting the client. We already know that resource->data is
         * not null.
         */
        new_ref = (*env)->NewWeakGlobalRef(env, local_ref);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
            (*env)->DeleteLocalRef(env, local_ref);
            return; /* Exception Thrown */
        }

        (*env)->DeleteGlobalRef(env, resource->data);
        resource->data = new_ref;
        resource->client = NULL;
    } else {
        if (resource->client == NULL) {
            /* We are setting the client */
            new_ref = (*env)->NewGlobalRef(env, local_ref);
            if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
                (*env)->DeleteLocalRef(env, local_ref);
                return; /* Exception Thrown */
            }

            (*env)->DeleteWeakGlobalRef(env, resource->data);
            resource->data = new_ref;
            resource->client = client;
            return;
        } else {
            /* This case is an error. */
        }
    }

    /* Delete the local reference */
    (*env)->DeleteLocalRef(env, local_ref);
}

static void
resource_destroy_func(struct wl_resource * resource)
{
    JNIEnv * env;

    if (resource == NULL)
        return;

    /* This will ensure that we don't get a recursive call */
    resource->destroy = NULL;

    env = wl_jni_get_env();

    wl_jni_resource_set_client(env, resource, NULL);
}

jobject
wl_jni_resource_create_from_native(JNIEnv * env, struct wl_resource * resource,
        jobject jdata)
{
    jobject jresource;

    ensure_resource_object_cache(env, NULL);

    jresource = (*env)->NewObject(env, Resource.class, Resource.init_long_obj,
            (jlong)(intptr_t)resource, jdata);
    if (jresource == NULL)
        return NULL; /* Exception Thrown */

    /* Set the apropreate type of pointer */
    if (resource->client == NULL) {
        resource->data = (*env)->NewWeakGlobalRef(env, jresource);
    } else {
        resource->data = (*env)->NewGlobalRef(env, jresource);
    }

    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        (*env)->DeleteLocalRef(env, jresource);
        return NULL; /* Exception Thrown */
    }

    resource->destroy = resource_destroy_func;

    return jresource;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource__1create(JNIEnv * env,
        jobject jresource, jint id, jobject jiface)
{
    struct wl_resource * resource;

    resource = malloc(sizeof(struct wl_resource));
    if (resource == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    memset(resource, 0, sizeof(struct wl_resource));

    resource->object.id = (uint32_t)id;

    resource->destroy = resource_destroy_func;
    wl_signal_init(&resource->destroy_signal);

    wl_jni_interface_init_object(env, jiface, &resource->object);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        free(resource);
        return;
    }

    (*env)->SetLongField(env, jresource, Resource.resource_ptr,
            (jlong)(intptr_t)resource);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        free(resource);
        return;
    }

    resource->data = (*env)->NewWeakGlobalRef(env, jresource);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        free(resource);
        return;
    }
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource__1destroy(JNIEnv * env,
        jobject jresource)
{
    struct wl_resource * resource;

    resource = wl_jni_resource_from_java(env, jresource);
    if (resource == NULL)
        return;

    free(resource);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_addDestroyListener(JNIEnv * env,
        jobject jresource, jobject jlistener)
{
    struct wl_resource * resource;
    struct wl_jni_listener * jni_listener;

    resource = wl_jni_resource_from_java(env, jresource);
    jni_listener = wl_jni_listener_from_java(env, jlistener);

    if (jni_listener == NULL) {
        wl_jni_throw_NullPointerException(env,
                "Listener not allowed to be null");
        return;
    }

    wl_signal_add(&resource->destroy_signal, &jni_listener->listener);
    wl_signal_add(&resource->destroy_signal, &jni_listener->destroy_listener);
    wl_jni_listener_added_to_signal(env, jlistener);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_destroy(JNIEnv * env,
        jobject jresource)
{
    struct wl_resource * resource = wl_jni_resource_from_java(env, jresource);

    if (resource == NULL)
        return;

    if (resource->client) {
        /* This only works if it has been added to a client */
        wl_resource_destroy(resource);
    } else {
        /* In this case we clean up ourselves */
        wl_signal_emit(&resource->destroy_signal, resource);
    }
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_postEvent(JNIEnv * env,
        jobject jresource, jint opcode, jarray jargs)
{
    struct wl_resource *resource;
    union wl_argument *args;
    const char *sig_tmp;
    char arg_type;
    int num_args, arg;
    void **extras;

    jobject jobj;

    resource = wl_jni_resource_from_java(env, jresource);

    num_args = (*env)->GetArrayLength(env, jargs);
    args = malloc(num_args * sizeof(union wl_argument));
    if (args == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }
    memset(args, 0, num_args * sizeof(union wl_argument));

    extras = malloc(num_args * sizeof(void *));
    if (args == NULL) {
        free(args);
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }
    memset(extras, 0, num_args * sizeof(void *));

    sig_tmp = resource->object.interface->events[opcode].signature;
    for (arg = 0; arg < num_args; ++arg) {
        arg_type = *sig_tmp++;
        if (arg_type == '?') {
            --arg;
            continue;
        }

        jobj = (*env)->GetObjectArrayElement(env, jargs, arg);
        if ((*env)->ExceptionCheck(env))
            goto free_args;

        switch(arg_type) {
        case 'i':
            args[arg].i = (int32_t)(*env)->CallIntMethod(env, jobj,
                    java.lang.Integer.intValue);
            break;
        case 'u':
            args[arg].u = (uint32_t)(*env)->CallIntMethod(env, jobj,
                    java.lang.Integer.intValue);
            break;
        case 'f':
            args[arg].f = wl_jni_fixed_from_java(env, jobj);
            break;
        case 's':
            args[arg].s = wl_jni_string_to_utf8(env, jobj);
            extras[arg] = args[arg].s;
            break;
        case 'o':
            args[arg].o = &wl_jni_resource_from_java(env, jobj)->object;
            break;
        case 'n':
            /* This never happens from a resource */
            break;
        case 'a':
            extras[arg] = malloc(sizeof(struct wl_array));
            if (extras[arg] == NULL) {
                wl_jni_throw_OutOfMemoryError(env, NULL);
                goto free_args;
            }
            args[arg].a = (struct wl_array *)extras[arg];
            args[arg].a->alloc = 0;
            args[arg].a->data = (*env)->GetDirectBufferAddress(env, jobj);
            args[arg].a->size = (*env)->GetDirectBufferCapacity(env, jobj);
            break;
        case 'h':
            args[arg].h = (*env)->CallIntMethod(env,
                    jobj, java.lang.Integer.intValue);
            break;
        }
        (*env)->DeleteLocalRef(env, jobj);
        if ((*env)->ExceptionCheck(env))
            goto free_args;
    }

    wl_resource_post_event_a(resource, opcode, args);

free_args:
    free(args);
    for (arg = 0; arg < num_args; ++arg)
        free(extras[arg]);
    free(extras);
    return;
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
static void
handle_resource_errors(JNIEnv * env, struct wl_resource * resource)
{
    jthrowable exception, exception2;
    jstring message;
    char * c_msg;
    int error_code;

    exception = (*env)->ExceptionOccurred(env);

    if (exception == NULL)
        return;

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
            return;
        } else if ((*env)->IsInstanceOf(env, exception,
                RequestError.class)) {
            error_code = (*env)->GetIntField(env, exception,
                    RequestError.errorCode);
            wl_resource_post_error(resource, error_code, "%s", c_msg);
            free(c_msg);
            (*env)->ExceptionClear(env);
            return;
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
    return;

unhandled_exception:
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
    return;
}

void
wl_jni_resource_dispatcher(struct wl_object *target, uint32_t opcode,
        const struct wl_message *message, void *client,
        union wl_argument *args)
{
    struct wl_resource *resource, *res_tmp;
    struct wl_object *obj_tmp;
    struct wl_array *arr;
    const char *signature;
    int nargs, nrefs;

    jvalue *jargs, *jargs_tmp;
    union wl_argument *args_tmp;
    JNIEnv *env;
    jobject jimplementation, jresource, jtmp;
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

    if ((*env)->PushLocalFrame(env, nrefs + 3) < 0)
        goto handle_exceptions; /* Exception Thrown */

    jresource = wl_jni_resource_to_java(env, resource);
    if ((*env)->ExceptionCheck(env)) {
        goto pop_local_frame;
    } else if (resource == NULL) {
        wl_jni_throw_NullPointerException(env, "Resource should not be null");
        goto pop_local_frame;
    }

    jargs[0].l = wl_jni_client_to_java(env, (struct wl_client *)client);
    if (jargs[0].l == NULL)
        goto pop_local_frame; /* Exception Thrown */

    jargs_tmp = jargs + 1;
    args_tmp = args;
    for (signature = message->signature; *signature != '\0'; ++signature) {
        switch(*signature) {
        case 'i':
            (jargs_tmp++)->i = (jint)(args_tmp++)->i;
            break;
        case 'u':
            (jargs_tmp++)->i = (jint)(args_tmp++)->u;
            break;
        case 'f':
            jtmp = wl_jni_fixed_to_java(env, (args_tmp++)->f);
            if (jtmp == NULL)
                goto pop_local_frame;
            (jargs_tmp++)->l = jtmp;
            break;
        case 's':
            jtmp = wl_jni_string_from_utf8(env, (args_tmp++)->s);
            if ((*env)->ExceptionCheck(env))
                goto pop_local_frame;
            (jargs_tmp++)->l = jtmp;
            break;
        case 'o':
            obj_tmp = (args_tmp++)->o;
            res_tmp = wl_container_of(obj_tmp, res_tmp, object);

            jtmp = wl_jni_resource_to_java(env, res_tmp);
            if ((*env)->ExceptionCheck(env))
                goto pop_local_frame;

            (jargs_tmp++)->l = jtmp;
            break;
        case 'n':
            (jargs_tmp++)->i = (jint)(args_tmp++)->n;
            break;
        case 'a':
            arr = (args_tmp++)->a;
            jtmp = (*env)->NewDirectByteBuffer(env, arr->data, arr->size);
            if (jtmp == NULL)
                goto pop_local_frame; /* Exception Thrown */
            (jargs_tmp++)->l = jtmp;
            break;
        case 'h':
            (jargs_tmp++)->i = (jint)(args_tmp++)->h;
            break;
        case '?':
            continue;
        default:
            wl_jni_throw_IllegalArgumentException(env,
                    "Invalid wayland request prototype");
            goto pop_local_frame; /* Exception Thrown */
        }
    }

    jimplementation = (*env)->GetObjectField(env, jresource, Resource.data);
    if ((*env)->ExceptionCheck(env))
        goto pop_local_frame;

    mid = ((jmethodID *)target->implementation)[opcode];
    (*env)->CallVoidMethodA(env, jimplementation, mid, jargs);

pop_local_frame:
    (*env)->PopLocalFrame(env, NULL);
    free(jargs);

handle_exceptions:
    /* Handle Exceptions here */
    handle_resource_errors(env, resource);
    return;
}

void
wl_jni_resource_call_request(struct wl_client * client,
        struct wl_resource * resource,
        const char * method_name, const char * wl_prototype,
        const char * java_prototype, ...)
{
    JNIEnv * env = wl_jni_get_env();

    int nargs, arg, i, nrefs;
    jvalue * args;
    const char * wl_prot_tmp;
    va_list ap;
    jobject jresource;
    jobject jclient;
    jobject jobj;

    /* We'll need these for calling constructors */
    jclass cls;
    jmethodID cid;
    jmethodID mid;

    /* Some temporaries for converting arguments */
    wl_fixed_t fixed_tmp;
    struct wl_array * array_tmp;
    struct wl_resource * res_tmp;

    LOG_DEBUG("Calling Request: %s", method_name);

    /* Calculate the number of references and the number of arguments */
    for (nargs = 1, nrefs = 0, wl_prot_tmp = wl_prototype;
            *wl_prot_tmp != '\0'; ++wl_prot_tmp) {
        switch (*wl_prot_tmp) {
        /* These types will require references */
        case 'f':
        case 's':
        case 'o':
        case 'a':
            ++nrefs;
            ++nargs;
            break;
        case 'u':
        case 'i':
        case 'n':
        case 'h':
            ++nargs;
            break;
        case '?':
            /* TODO: Do something useful here? */
            break;
        default:
            /* TODO: Throw an error here? */
            break;
        }
    }
    
    /* Ensure the required number of references */
    if ((*env)->EnsureLocalCapacity(env, nrefs + 4) < 0) {
        goto early_exception; /* Exception Thrown */
    }

    args = malloc(nargs * sizeof(jvalue));
    if (args == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto early_exception; /* Exception Thrown */
    }

    args[0].l = wl_jni_client_to_java(env, client);
    if (args[0].l == NULL) {
        free(args);
        goto early_exception;
    }

    va_start(ap, java_prototype);
    for (arg = 1, wl_prot_tmp = wl_prototype; arg < nargs; ++wl_prot_tmp) {
        switch(*wl_prot_tmp) {
        case 'f':
            fixed_tmp = va_arg(ap, wl_fixed_t);
            args[arg].l = wl_jni_fixed_to_java(env, va_arg(ap, wl_fixed_t));
            if (args[arg].l == NULL) {
                va_end(ap);
                goto free_arguments;
            }
            break;
        case 'u':
            args[arg].i = (jint)va_arg(ap, uint32_t);
            break;
        case 'i':
            args[arg].i = (jint)va_arg(ap, int32_t);
            break;
        case 's':
            args[arg].l = wl_jni_string_from_utf8(env, va_arg(ap, char *));
            if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
                va_end(ap);
                goto free_arguments;
            }
            break;
        case 'o':
            res_tmp = va_arg(ap, struct wl_resource *);
            args[arg].l = wl_jni_resource_to_java(env, res_tmp);
            if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
                va_end(ap);
                goto free_arguments;
            }
            break;
        case 'n':
            args[arg].i = (jint)va_arg(ap, uint32_t);
            break;
        case 'a':
            array_tmp = va_arg(ap, struct wl_array *);
            args[arg].l = (*env)->NewByteArray(env, array_tmp->size);
            if (args[arg].l == NULL) {
                va_end(ap);
                goto free_arguments;
            }

            (*env)->SetByteArrayRegion(env, args[arg].l, 0, array_tmp->size,
                    array_tmp->data);
            if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
                va_end(ap);
                ++arg;
                goto free_arguments;
            }
            break;
        case 'h':
            args[arg].i = (jint)va_arg(ap, int);
            break;
        case '?':
            /* Do something useful here? */
            continue;
        default:
            /* Throw an error here? */
            break;
        }

        /* we do this here instead of the loop so that we can continue in the
         * '?' case */
        ++arg;
    }
    va_end(ap);

    jresource = wl_jni_resource_to_java(env, resource);
    if (jresource == NULL)
        goto free_arguments;

    jobj = (*env)->GetObjectField(env, jresource, Resource.data);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->DeleteLocalRef(env, jresource);
        goto free_arguments;
    }

    cls = (*env)->GetObjectClass(env, jobj);
    if (cls == NULL) {
        (*env)->DeleteLocalRef(env, jresource);
        (*env)->DeleteLocalRef(env, jobj);
        goto free_arguments;
    }

    mid = (*env)->GetMethodID(env, cls, method_name, java_prototype);
    (*env)->DeleteLocalRef(env, cls);
    if (mid == NULL) {
        (*env)->DeleteLocalRef(env, jresource);
        (*env)->DeleteLocalRef(env, jobj);
        goto free_arguments;
    }

    (*env)->CallVoidMethodA(env, jobj, mid, args);
    (*env)->DeleteLocalRef(env, jresource);

free_arguments:
    --arg;
    --wl_prot_tmp;
    for (; arg >= 0; --arg, --wl_prot_tmp) {
        if (*wl_prot_tmp == '?')
            --wl_prot_tmp;

        switch(*wl_prot_tmp) {
        case 'f':
        case 's':
        case 'o':
        case 'a':
            (*env)->DeleteLocalRef(env, args[arg].l);
        }
    }

    free(args);

early_exception:
    /* Handle Exceptions here */
    handle_resource_errors(env, resource);
    return;
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

    Resource.init_long_obj = (*env)->GetMethodID(env, Resource.class,
            "<init>", "(JLjava/lang/Object;)V");
    if (Resource.init_long_obj == NULL)
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

    cls = (*env)->FindClass(env, "java/lang/Integer");
    if (cls == NULL)
        return; /* Exception Thrown */
    java.lang.Integer.intValue = (*env)->GetMethodID(env,
            cls, "intValue", "()I");
    (*env)->DeleteLocalRef(env, cls);
    if (java.lang.Integer.intValue == NULL)
        return; /* Exception Thrown */
}

static void
ensure_resource_object_cache(JNIEnv * env, jclass cls)
{
    if (Resource.class != NULL)
        return;

    if (cls == NULL) {
        cls = (*env)->FindClass(env, "org/freedesktop/wayland/server/Resource");
        if (cls == NULL)
            return;
        Java_org_freedesktop_wayland_server_Resource_initializeJNI(env, cls);
        (*env)->DeleteLocalRef(env, cls);
    } else {
        Java_org_freedesktop_wayland_server_Resource_initializeJNI(env, cls);
    }
}

