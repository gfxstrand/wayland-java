#include <string.h>

#include <wayland-server.h>
#include <wayland-util.h>

#include "server-jni.h"

struct wl_resource *
wl_jni_resource_from_java(JNIEnv * env, jobject jresource)
{
    jclass cls = (*env)->GetObjectClass(env, jresource);
    jfieldID fid = (*env)->GetFieldID(env, cls, "object_ptr", "J");
    return (struct wl_resource *)(*env)->GetLongField(env, jresource, fid);
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
        jobject jresource)
{
    struct wl_resource * resource = malloc(sizeof(struct wl_resource));
    memset(resource, 0, sizeof(struct wl_resource));

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

