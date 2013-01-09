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
#include <wayland-util.h>

#include "wayland-jni.h"

struct wl_object *
wl_jni_object_from_java(JNIEnv * env, jobject jobj)
{
    struct wl_object * object;
    jclass cls = (*env)->GetObjectClass(env, jobj);
    jfieldID fid = (*env)->GetFieldID(env, cls, "object_ptr", "J");
    object = (struct wl_object *)(*env)->GetLongField(env, jobj, fid);
    (*env)->DeleteLocalRef(env, cls);
    return object;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_Object_setID(JNIEnv * env, jobject jobj, int id)
{
    struct wl_object * object;
    object = wl_jni_object_from_java(env, jobj);
    object->id = id;
}

JNIEXPORT int JNICALL
Java_org_freedesktop_wayland_Object_getID(JNIEnv * env, jobject jobj)
{
    struct wl_object * object;
    object = wl_jni_object_from_java(env, jobj);
    return object->id;
}

