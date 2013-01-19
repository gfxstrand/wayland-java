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
#include <string.h>
#include <wayland-server.h>

#include "server/server-jni.h"

struct {
    jclass class;
    jfieldID display_ptr;
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

JNIEXPORT jint JNICALL
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

    return (jint)fd;
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
Java_org_freedesktop_wayland_server_Display_addGlobal(JNIEnv * env,
        jobject jdisplay, jobject jglobal)
{
    struct wl_display * display;

    display = wl_jni_display_from_java(env, jdisplay);
    if ((*env)->ExceptionCheck(env))
        return /* Exception Thrown */

    wl_jni_global_add_to_display(env, jglobal, display);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Display_removeGlobal(JNIEnv * env,
        jobject jdisplay, jobject jglobal)
{
    struct wl_display * display;

    display = wl_jni_display_from_java(env, jdisplay);
    if ((*env)->ExceptionCheck(env))
        return; /* Exception Thrown */

    if ((*env)->IsSameObject(env, jglobal, NULL)) {
        wl_jni_throw_NullPointerException(env, NULL);
        return; /* Exception Thrown */
    }

    wl_jni_global_remove_from_display(env, jglobal, display);
}

JNIEXPORT jint JNICALL
Java_org_freedesktop_wayland_server_Display_getSerial(JNIEnv * env,
        jobject jdisplay)
{
    return wl_display_get_serial(wl_jni_display_from_java(env, jdisplay));
}

JNIEXPORT jint JNICALL
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
    (*env)->SetLongField(env, jdisplay, fid, (jlong)(intptr_t)display);
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

