#include <string.h>
#include <wayland-server.h>

#include "server/server-jni.h"

struct wl_display * wl_jni_display_from_java(JNIEnv * env, jobject jdisplay)
{
    jclass cls = (*env)->GetObjectClass(env, jdisplay);
    jfieldID fid = (*env)->GetFieldID(env, cls, "display_ptr", "J");
    return (struct wl_display *)(*env)->GetLongField(env, jdisplay, fid);
}

jobject wl_jni_display_to_java(JNIEnv * env, struct wl_display * display)
{
    return wl_jni_find_reference(env, display);
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_Display_getEventLoop(JNIEnv * env,
        jobject jdisplay)
{
    struct wl_display * display = wl_jni_display_from_java(env, jdisplay);
    return wl_jni_event_loop_to_java(env, wl_display_get_event_loop(display));
}

JNIEXPORT int JNICALL
Java_org_freedesktop_wayland_server_Display_addSocket(JNIEnv * env,
        jobject jdisplay, jobject name)
{
    if (name == NULL)
        (*env)->ThrowNew(env,
                (*env)->FindClass(env, "java/lang/NullPointerException"),
                "No socket name given");

    jclass cls = (*env)->GetObjectClass(env, name);
    jmethodID mid = (*env)->GetMethodID(env, cls, "getPath",
            "()Ljava/lang/string");
    jstring path = (*env)->CallObjectMethod(env, name, mid);
    if ((*env)->ExceptionOccurred(env))
        return;

    cls = (*env)->GetObjectClass(env, path);
    mid = (*env)->GetMethodID(env, cls, "getBytes", "()[B");
    jarray bytes = (*env)->CallObjectMethod(env, path, mid);
    if ((*env)->ExceptionOccurred(env))
        return;

    int num_bytes = (*env)->GetArrayLength(env, bytes);

    char * c_bytes = malloc(num_bytes + 1);
    (*env)->GetByteArrayRegion(env, bytes, 0, num_bytes, c_bytes);
    c_bytes[num_bytes] = 0;

    struct wl_display * display = wl_jni_display_from_java(env, jdisplay);
    int fd = wl_display_add_socket(display, c_bytes);
    free(c_bytes);

    return fd;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Display_terminate(JNIEnv * env,
        jobject jdisplay)
{
    wl_display_terminate(wl_jni_display_from_java(env, jdisplay));
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Display_run(JNIEnv * env,
        jobject jdisplay)
{
    wl_display_run(wl_jni_display_from_java(env, jdisplay));
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Display_flushClients(JNIEnv * env,
        jobject jdisplay)
{
    wl_display_flush_clients(wl_jni_display_from_java(env, jdisplay));
}

static void
global_bind_func(struct wl_client * client, void * data, uint32_t version,
        uint32_t id)
{
    // TODO
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_Display_addGlobal(JNIEnv * env,
        jobject jdisplay, jobject jresource)
{
    if (jresource == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);
        return;
    }

    jclass cls = (*env)->FindClass(env,
            "org/freedesktop/wayland/server/Display$Global");
    if (cls == NULL)
        return NULL; /* Exception Thrown */

    jmethodID mid = (*env)->GetMethodID(env, cls, "<init>", "(J)V");
    if (mid == NULL)
        return NULL; /* Exception Thrown */

    struct wl_resource * resource = wl_jni_resource_from_java(env, jresource);
    if (resource == NULL)
        return NULL; /* Exception Thrown */

    struct wl_display * display = wl_jni_display_from_java(env, jdisplay);
    struct wl_global * global = wl_display_add_global(display,
            resource->object.interface, resource, &global_bind_func);

    return (*env)->NewObject(env, cls, mid, global);
}

JNIEXPORT int JNICALL
Java_org_freedesktop_wayland_server_Display_getSerial(JNIEnv * env,
        jobject jdisplay)
{
    return wl_display_get_serial(wl_jni_display_from_java(env, jdisplay));
}

JNIEXPORT int JNICALL
Java_org_freedesktop_wayland_server_Display_nextSerial(JNIEnv * env,
        jobject jdisplay)
{
    return wl_display_next_serial(wl_jni_display_from_java(env, jdisplay));
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Display_create(JNIEnv * env,
        jobject jdisplay)
{
    struct wl_display * display = wl_display_create();

    wl_jni_register_reference(env, display, jdisplay);

    jclass cls = (*env)->GetObjectClass(env, jdisplay);
    jfieldID fid = (*env)->GetFieldID(env, cls, "display_ptr", "J");
    (*env)->SetLongField(env, jdisplay, fid, (long)display);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Display_destroy(JNIEnv * env,
        jobject jdisplay)
{
    struct wl_display * display = wl_jni_display_from_java(env, jdisplay);
    if (display) {
        wl_display_destroy(display);

        wl_jni_unregister_reference(env, display);

        jclass cls = (*env)->GetObjectClass(env, jdisplay);
        jfieldID fid = (*env)->GetFieldID(env, cls, "display_ptr", "J");
        (*env)->SetLongField(env, jdisplay, fid, 0);
    }
}

