#include <wayland-server.h>

#include "server/server-jni.h"

struct wl_client *
wl_jni_client_from_java(JNIEnv * env, jobject jclient)
{
    jclass cls = (*env)->GetObjectClass(env, jclient);
    jfieldID fid = (*env)->GetFieldID(env, cls, "client_ptr", "J");
    return (struct wl_client *)(*env)->GetLongField(env, jclient, fid);
}

jobject
wl_jni_client_to_java(JNIEnv * env, struct wl_client * client)
{
    return wl_jni_find_reference(env, client);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Client_create(JNIEnv * env, jobject jclient,
        jobject jdisplay, int fd)
{
    struct wl_display * display = wl_jni_display_from_java(env, jdisplay);
    struct wl_client * client = wl_client_create(display, fd);

    wl_jni_register_reference(env, client, jdisplay);

    jclass cls = (*env)->GetObjectClass(env, jclient);
    jfieldID fid = (*env)->GetFieldID(env, cls, "client_ptr", "J");
    (*env)->SetLongField(env, jclient, fid, (long)client);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Client_flush(JNIEnv * env, jobject jclient)
{
    wl_client_flush(wl_jni_client_from_java(env, jclient));
}

JNIEXPORT int JNICALL
Java_org_freedesktop_wayland_server_Client_addResource(JNIEnv * env,
        jobject jclient, jobject jresource)
{
    /* TODO: Handle the case of already attached resources */
    struct wl_client * client;
    struct wl_resource * resource;
    jobject global_ref;
    int id;

    if (jresource == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);
        return;
    }

    client = wl_jni_client_from_java(env, jclient);
    resource = wl_jni_resource_from_java(env, jresource);
    if (client == NULL || resource == NULL)
        return; /* Exception Thrown */

    /* We need to convert the weak global reference to a global reference for
     * proper garbage collection. See also: resource.c */
    global_ref = (*env)->NewGlobalRef(env, (jobject)resource->data);
    if (global_ref == NULL) {
        /* The only way this can happen is an out-of-memory error */
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }
    (*env)->DeleteWeakGlobalRef(env, (jobject)resource->data);
    resource->data = global_ref;

    id = (int)wl_client_add_resource(client, resource);
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_Client_getDisplay(JNIEnv * env,
        jobject jclient)
{
    struct wl_client * client = wl_jni_client_from_java(env, jclient);
    return wl_jni_display_to_java(env, wl_client_get_display(client));
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Client_destroy(JNIEnv * env,
        jobject jclient)
{
    struct wl_client * client = wl_jni_client_from_java(env, jclient);
    if (client) {
        wl_client_destroy(client);

        wl_jni_unregister_reference(env, client);

        jclass cls = (*env)->GetObjectClass(env, jclient);
        jfieldID fid = (*env)->GetFieldID(env, cls, "client_ptr", "J");
        (*env)->SetLongField(env, jclient, fid, 0);
    }
}

