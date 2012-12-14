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

