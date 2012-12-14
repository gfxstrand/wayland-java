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
    jmethodID destroy;
} Resource;

struct wl_resource *
wl_jni_resource_from_java(JNIEnv * env, jobject jresource)
{
    return (struct wl_resource *)(*env)->GetLongField(env, jresource,
            Resource.resource_ptr);
}

jobject
wl_jni_resource_to_java(JNIEnv * env, struct wl_resource * resource)
{
    return (*env)->NewLocalRef(env, (jobject)resource->data);
}

static void
resource_destroy(struct wl_resource * resource)
{
    if (resource == NULL)
        return;

    /* This will ensure that we don't get a recursive call */
    resource->destroy = NULL;

    JNIEnv * env = wl_jni_get_env();

    jobject jresource = wl_jni_resource_to_java(env, resource);
    if (jresource == NULL)
        return;

    (*env)->CallVoidMethod(env, jresource, Resource.destroy, NULL);
    (*env)->DeleteLocalRef(env, jresource);

    // TODO: Handle Exceptions
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource__1create(JNIEnv * env,
        jobject jresource, int id)
{
    struct wl_resource * resource;
    jobject global_ref;

    global_ref = (*env)->NewWeakGlobalRef(env, jresource);
    if (global_ref == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    resource = malloc(sizeof(struct wl_resource));
    if (resource == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        (*env)->DeleteWeakGlobalRef(env, global_ref);
        return;
    }

    memset(resource, 0, sizeof(struct wl_resource));

    resource->object.id = id;

    resource->destroy = resource_destroy;
    resource->data = global_ref;
    wl_signal_init(&resource->destroy_signal);

    (*env)->SetLongField(env, jresource, Resource.resource_ptr, (long)resource);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->DeleteWeakGlobalRef(env, global_ref);
        free(resource);
    }
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_destroy(JNIEnv * env,
        jobject jresource, jobject jclient)
{
    struct wl_resource * resource = wl_jni_resource_from_java(env, jresource);

    if (resource) {
        /*
         * We will only run the destroy if resource->destroy is not null. This
         * prevents recursive calls. If wl_resouce_destroy() gets called
         * directly on this resource, it will call resource_destroy() which will
         * cause the destroy even to propogate through the chain and we will
         * end up here. For this reason, resource_destroy() sets the destroy
         * pointer to null so that we can detect that casae and not call
         * wl_resource_destroy() twice. If, however, the destroy event
         * originates from a request, this pointer will still be non-null.
         */
        if (resource->destroy != NULL)
            wl_resource_destroy(resource);

        if (resource->client == NULL) {
            (*env)->DeleteWeakGlobalRef(env, (jobject)resource->data);
        } else {
            (*env)->DeleteGlobalRef(env, (jobject)resource->data);
        }

        free(resource);

        (*env)->SetLongField(env, jresource, Resource.resource_ptr, 0);
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
        wl_jni_throw_OutOfMemoryError(env, NULL);
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
    if (cls == NULL) {
        (*env)->DeleteLocalRef(env, jresource);
        goto free_arguments;
    }

    mid = (*env)->GetMethodID(env, cls, method_name, java_prototype);
    (*env)->DeleteLocalRef(env, cls);
    if (mid == NULL) {
        (*env)->DeleteLocalRef(env, jresource);
        goto free_arguments;
    }

    (*env)->CallVoidMethodA(env, resource, mid, args);
    (*env)->DeleteLocalRef(env, jresource);

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

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Resource_initializeJNI(JNIEnv * env,
        jclass cls)
{
    Resource.class = (*env)->NewGlobalRef(env, cls);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    Resource.resource_ptr = (*env)->GetFieldID(env, cls, "resource_ptr", "J");
    if (Resource.resource_ptr == NULL)
        return; /* Exception Thrown */

    Resource.destroy = (*env)->GetMethodID(env, cls, "destroy",
            "(Lorg/freedesktop/wayland/server/Client;)V");
    if (Resource.destroy == NULL)
        return; /* Exception Thrown */
}

