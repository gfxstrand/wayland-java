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
    jvalue args[1];
    args[0].i = fixed;
    jobject jfixed = (*env)->NewObjectA(env, cls, cid, args);
    (*env)->DeleteLocalRef(env, cls);
    return jfixed;
}

