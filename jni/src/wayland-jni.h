#ifndef __WAYLAND_JAVA_WAYLAND_JNI_H__
#define __WAYLAND_JAVA_WAYLAND_JNI_H__

#include <jni.h>

extern JavaVM * java_vm;

JNIEnv * wl_jni_get_env();

int wl_jni_register_reference(JNIEnv * env, void * native_ptr, jobject jobj);
int wl_jni_register_weak_reference(JNIEnv * env, void * native_ptr,
        jobject jobj);
void wl_jni_unregister_reference(JNIEnv * env, void * native_ptr);
jobject wl_jni_find_reference(JNIEnv * env, void * native_ptr);

#endif /* ! defined __WAYLAND_JAVA_WAYLAND_JNI_H__ */

