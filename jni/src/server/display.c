#include <string.h>
#include <wayland-server.h>

#include "server/server-jni.h"

struct {
    jclass class;
    jfieldID display_ptr;

    struct {
        jclass class;
        jmethodID init_long;
    } Global;
} Display;

struct wl_display * wl_jni_display_from_java(JNIEnv * env, jobject jdisplay)
{
    return (struct wl_display *)(intptr_t)
            (*env)->GetLongField(env, jdisplay, Display.display_ptr);
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
        jobject jdisplay, jobject jname)
{
    char * name;
    struct wl_display * display;

    display = wl_jni_display_from_java(env, jdisplay);

    if (jname == NULL) {
        wl_jni_throw_NullPointerException(env, "No socket name given");
        return;
    }

    name = wl_jni_string_to_default(env, jname);
    if (name == NULL)
        return; /* Exception Thrown */

    int fd = wl_display_add_socket(display, name);

    free(name);

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

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Display_doAddGlobal(JNIEnv * env,
        jobject jdisplay, jobject jglobal)
{
    jobject jinterface, self_ref;
    struct wl_display * display;
    struct wl_interface * interface;
    struct wl_global * global;

    jinterface = wl_jni_global_get_interface(env, jglobal);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return;

    display = wl_jni_display_from_java(env, jdisplay);
    interface = wl_jni_interface_from_java(env, jinterface);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return;

    self_ref = (*env)->NewWeakGlobalRef(env, jglobal);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return;

    global = wl_display_add_global(display, interface, self_ref,
            &wl_jni_global_bind_func);
    if (global == NULL) {
        (*env)->DeleteWeakGlobalRef(env, self_ref);
        return;
    }

    wl_jni_global_set_data(env, jglobal, self_ref, global);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        wl_display_remove_global(display, global);
        (*env)->DeleteWeakGlobalRef(env, self_ref);
        return;
    }
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Display_doRemoveGlobal(JNIEnv * env,
        jobject jdisplay, jobject jglobal)
{
    struct wl_global * global;
    struct wl_display * display;
    jobject self_ref;

    if (jglobal == NULL)
        return;

    display = wl_jni_display_from_java(env, jdisplay);

    global = wl_jni_global_from_java(env, jglobal);

    wl_display_remove_global(display, global);

    wl_jni_global_release(env, jglobal);
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

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Display_initializeJNI(JNIEnv * env,
        jclass cls)
{
    Display.class = (*env)->NewGlobalRef(env, cls);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    Display.display_ptr = (*env)->GetFieldID(env, cls, "display_ptr", "J");
    if (Display.display_ptr == NULL)
        return; /* Exception Thrown */
}

