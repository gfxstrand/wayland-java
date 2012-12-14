#ifndef __WAYLAND_JAVA_WAYLAND_JNI_H__
#define __WAYLAND_JAVA_WAYLAND_JNI_H__

#include <jni.h>
#include "wayland-util.h"

wl_fixed_t wl_jni_fixed_from_java(JNIEnv * env, jobject jobj);
jobject wl_jni_fixed_to_java(JNIEnv * env, wl_fixed_t fixed);

struct wl_object * wl_jni_object_from_java(JNIEnv * env, jobject jobj);

extern JavaVM * java_vm;

JNIEnv * wl_jni_get_env();

int wl_jni_register_reference(JNIEnv * env, void * native_ptr, jobject jobj);
int wl_jni_register_weak_reference(JNIEnv * env, void * native_ptr,
        jobject jobj);
void wl_jni_unregister_reference(JNIEnv * env, void * native_ptr);
jobject wl_jni_find_reference(JNIEnv * env, void * native_ptr);

jstring wl_jni_string_from_utf8(JNIEnv * env, const char * str);
char * wl_jni_string_to_utf8(JNIEnv * env, jstring java_str);

#endif /* ! defined __WAYLAND_JAVA_WAYLAND_JNI_H__ */

