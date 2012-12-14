#include <string.h>
#include <stdarg.h>

#include <wayland-server.h>
#include <wayland-util.h>

#include "server-jni.h"

struct wl_resource *
wl_jni_resource_from_java(JNIEnv * env, jobject jresource)
{
    // FIXME: This should use container_of and the object pointer
    struct wl_resource * resource;
    jclass cls = (*env)->GetObjectClass(env, jresource);
    jfieldID fid = (*env)->GetFieldID(env, cls, "object_ptr", "J");
    resource = (struct wl_resource *)(*env)->GetLongField(env, jresource, fid);
    (*env)->DeleteLocalRef(env, cls);
    return resource;
}

jobject
wl_jni_resource_to_java(JNIEnv * env, struct wl_resource * resource)
{
    return (jobject)resource->data;
}

static void
resource_destroy(struct wl_resource * resource)
{
    JNIEnv * env = wl_jni_get_env();

    jobject jresource = wl_jni_resource_to_java(env, resource);

    jclass cls = (*env)->GetObjectClass(env, jresource);
    jmethodID mid = (*env)->GetMethodID(env, cls, "destroy", "()V");
    (*env)->CallVoidMethod(env, jresource, mid);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_create(JNIEnv * env,
        jobject jresource, int id)
{
    struct wl_resource * resource = malloc(sizeof(struct wl_resource));
    memset(resource, 0, sizeof(struct wl_resource));

    resource->object.id = id;

    resource->destroy = resource_destroy;
    resource->data = jresource;
    wl_signal_init(&resource->destroy_signal);

    jclass cls = (*env)->GetObjectClass(env, jresource);
    jfieldID fid = (*env)->GetFieldID(env, cls, "object_ptr", "J");
    (*env)->SetLongField(env, jresource, fid, (long)resource);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_destroy(JNIEnv * env,
        jobject jresource)
{
    struct wl_resource * resource = wl_jni_resource_from_java(env, jresource);

    if (resource) {
        // Set this to 0 to prevent recursive calls
        resource->destroy = 0;
        wl_resource_destroy(resource);

        free(resource);

        jclass cls = (*env)->GetObjectClass(env, jresource);
        jfieldID fid = (*env)->GetFieldID(env, cls, "object_ptr", "J");
        (*env)->SetLongField(env, jresource, fid, 0);
    }
}

void
wl_jni_resource_call_request(struct wl_client * client,
        struct wl_resource * resource,
        const char * method_name, const char * wl_prototype,
        const char * java_prototype, ...)
{
    JNIEnv * env = wl_jni_get_env();

    int nargs, arg, i, nrefs;
    va_list ap;
    jobject jresource;
    jobject jclient;

    /* We'll need these for calling constructors */
    jclass cls;
    jmethodID cid;
    jmethodID mid;

    /* Some temporaries for converting arguments */
    wl_fixed_t fixed_tmp;
    struct wl_array * array_tmp;
    struct wl_resource * res_tmp;

    /* Calculate the number of references and the number of arguments */
    for (nargs = 1, nrefs = 0; wl_prototype[nargs] != '\0'; ++nargs) {
        switch (wl_prototype[nargs - 1]) {
        /* These types will require references */
        case 'f':
        case 's':
        case 'o':
        case 'a':
            ++nrefs;
            break;
        default:
            break;
        }
    }
    
    /* Ensure the required number of references */
    if ((*env)->EnsureLocalCapacity(env, nrefs + 3) < 0) {
        goto early_exception; /* Exception Thrown */
    }

    jvalue * args = malloc(nargs * sizeof(jvalue));
    if (args == NULL) {
        (*env)->ThrowNew(env,
                (*env)->FindClass(env, "java/lang/OutOfMemoryError"), NULL);
        goto early_exception; /* Exception Thrown */
    }

    args[0].l = wl_jni_client_to_java(env, client);
    if (args[0].l == NULL) {
        free(args);
        goto early_exception;
    }

    va_start(ap, java_prototype);
    for (arg = 1; arg < nargs; ++arg) {
        switch(wl_prototype[arg - 1]) {
        case 'f':
            fixed_tmp = va_arg(ap, wl_fixed_t);
            args[arg].l = wl_jni_fixed_to_java(env, va_arg(ap, wl_fixed_t));
            if (args[arg].l == NULL) {
                va_end(ap);
                --arg;
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
                --arg;
                goto free_arguments;
            }
            break;
        case 'o':
            res_tmp = va_arg(ap, struct wl_resource *);
            args[arg].l = wl_jni_resource_to_java(env, res_tmp);
            if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
                va_end(ap);
                --arg;
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
                --arg;
                goto free_arguments;
            }

            (*env)->SetByteArrayRegion(env, args[arg].l, 0, array_tmp->size,
                    array_tmp->data);
            if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
                va_end(ap);
                goto free_arguments;
            }
            break;
        case 'h':
            args[arg].i = va_arg(ap, int);
            break;
        }
    }
    va_end(ap);

    jresource = wl_jni_resource_to_java(env, resource);
    if (jresource == NULL)
        goto free_arguments;

    cls = (*env)->GetObjectClass(env, jresource);
    if (cls == NULL)
        goto free_arguments;

    mid = (*env)->GetMethodID(env, cls, method_name, java_prototype);
    (*env)->DeleteLocalRef(env, cls);
    if (mid == NULL)
        goto free_arguments;

    (*env)->CallVoidMethodA(env, resource, mid, args);

free_arguments:
    for (; arg >= 0; --arg) {
        switch(wl_prototype[arg - 1]) {
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
    return;
}

