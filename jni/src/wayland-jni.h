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
#ifndef __WAYLAND_JAVA_WAYLAND_JNI_H__
#define __WAYLAND_JAVA_WAYLAND_JNI_H__

#include <jni.h>

#include <stdint.h>

#ifdef ANDROID
#include <android/log.h>

#define LOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, __FILE__, __VA_ARGS__)
#else /* ! ANDROID */
#include <stdio.h>

#define LOG_DEBUG(...) printf(__VA_ARGS__)
#endif

#include "wayland-util.h"

wl_fixed_t wl_jni_fixed_from_java(JNIEnv * env, jobject jobj);
jobject wl_jni_fixed_to_java(JNIEnv * env, wl_fixed_t fixed);

struct wl_jni_interface
{
    struct wl_interface interface;
    jmethodID *requests;
    jmethodID *events;
};

struct wl_jni_interface * wl_jni_interface_from_java(JNIEnv * env,
        jobject jinterface);
void wl_jni_interface_init_object(JNIEnv * env, jobject jinterface,
        struct wl_object * obj);

void wl_jni_arguments_from_java(JNIEnv *env, union wl_argument *args,
        jarray jargs, const char *signature, int count,
        struct wl_object *(* object_conversion)(JNIEnv *env, jobject));
void wl_jni_arguments_to_java(JNIEnv *env, union wl_argument *args,
        jvalue *jargs, const char *signature, int count,
        jobject (* object_conversion)(JNIEnv *env, struct wl_object *));
void wl_jni_arguments_from_java_destroy(union wl_argument *args,
        const char *signature, int count);

void wl_jni_throw_OutOfMemoryError(JNIEnv * env, const char * message);
void wl_jni_throw_NullPointerException(JNIEnv * env, const char * message);
void wl_jni_throw_IllegalArgumentException(JNIEnv * env, const char * message);
void wl_jni_throw_IOException(JNIEnv * env, const char * message);
void wl_jni_throw_by_name(JNIEnv * env, const char *name, const char * message);
void wl_jni_throw_from_errno(JNIEnv * env, int err);

jint wl_jni_unbox_integer(JNIEnv *env, jobject integer);

extern JavaVM * java_vm;

JNIEnv * wl_jni_get_env();

jobject wl_jni_register_reference(JNIEnv * env, void * native_ptr,
        jobject jobj);
jobject wl_jni_register_weak_reference(JNIEnv * env, void * native_ptr,
        jobject jobj);
void wl_jni_unregister_reference(JNIEnv * env, void * native_ptr);
jobject wl_jni_find_reference(JNIEnv * env, void * native_ptr);

jstring wl_jni_string_from_utf8(JNIEnv * env, const char * str);
char * wl_jni_string_to_utf8(JNIEnv * env, jstring java_str);

char * wl_jni_string_to_default(JNIEnv * env, jstring java_str);

#endif /* ! defined __WAYLAND_JAVA_WAYLAND_JNI_H__ */

