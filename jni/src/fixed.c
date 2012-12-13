#include "wayland-jni.h"

wl_fixed_t
wl_jni_fixed_from_java(JNIEnv * env, jobject jfixed)
{
    jclass cls = (*env)->GetObjectClass(env, jfixed);
    jfieldID fid = (*env)->GetFieldID(env, cls, "data", "I");
    wl_fixed_t fixed = (wl_fixed_t)(*env)->GetIntField(env, jfixed, fid);
    (*env)->DeleteLocalRef(env, cls);
    return fixed;
}

jobject
wl_jni_fixed_to_java(JNIEnv * env, wl_fixed_t fixed)
{
    jclass cls = (*env)->FindClass(env, "org/freedesktop/wayland/Fixed");
    jmethodID cid = (*env)->GetMethodID(env, cls, "<init>", "(I)V");
    jobject jfixed = (*env)->NewObject(env, cls, cid, (jint)fixed);
    (*env)->DeleteLocalRef(env, cls);
    return jfixed;
}

