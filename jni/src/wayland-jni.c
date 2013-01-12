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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>

JavaVM * java_vm;

/* Structure that stores a object reference pointer with instructions for the
 * type of reference and how it is to be deleted */
struct ptr_jobject_pair {
    void * ptr;
    jobject jobj;
    char is_weak;
    struct wl_list link;
};

/* A list of object reference pairs */
static struct wl_list ptr_jobject_list;

/**
 * The following stores an object cache that is filled by
 * wl_jni_ensure_object_cache. The cached objects are used for a variety of
 * helper functions to avoid addition JNI lookup calls
 */
static int jni_object_cache_loaded;
static struct {
    struct {
        struct {
            jclass class;
            jmethodID init_bytes_charset;
            jmethodID getBytes;
            jmethodID getBytes_charset;
        } String;

        struct {
            jclass class;
        } OutOfMemoryError;

        struct {
            jclass class;
        } NullPointerException;

        struct {
            jclass class;
        } IllegalArgumentException;

        struct {
            jclass class;
        } RuntimeException;
    } lang;

    struct {
        struct {
            jclass class;
        } IOException;
    } io;

    struct {
        struct {
            jobject utf8;
        } charset;
    } nio;
} java;

/**
 * This mutex is used to lock both the object cache and the object reference
 * pairs list
 */
static pthread_mutex_t object_cache_mutex;

static int
wl_jni_ensure_object_cache(JNIEnv * env)
{
    jclass cls;
    jmethodID mid;
    jobject jobj1, jobj2;

    if (jni_object_cache_loaded)
        return 0;

    pthread_mutex_lock(&object_cache_mutex);

    /* Don't load it twice */
    if (jni_object_cache_loaded) {
        pthread_mutex_unlock(&object_cache_mutex);
        return 0;
    }

    if ((*env)->EnsureLocalCapacity(env, 3) < 0)
        goto exception;

    /* we have to be a little careful because OutOfMemoryError is required to
     * throw an OutOfMemoryError */
    cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");
    if (cls == NULL) goto exception;
    java.lang.OutOfMemoryError.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (java.lang.OutOfMemoryError.class == NULL) {
        goto exception;
    }
    cls = NULL;

    cls = (*env)->FindClass(env, "java/lang/NullPointerException");
    if (cls == NULL) goto exception;
    java.lang.NullPointerException.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (java.lang.NullPointerException.class == NULL) {
        goto exception;
    }
    cls = NULL;

    cls = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
    if (cls == NULL) goto exception;
    java.lang.IllegalArgumentException.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (java.lang.IllegalArgumentException.class == NULL) {
        goto exception;
    }
    cls = NULL;

    cls = (*env)->FindClass(env, "java/lang/RuntimeException");
    if (cls == NULL) goto exception;
    java.lang.RuntimeException.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (java.lang.RuntimeException.class == NULL) {
        goto exception;
    }
    cls = NULL;

    cls = (*env)->FindClass(env, "java/io/IOException");
    if (cls == NULL) goto exception;
    java.io.IOException.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (java.io.IOException.class == NULL) {
        goto exception;
    }
    cls = NULL;

    cls = (*env)->FindClass(env, "java/lang/String");
    if (cls == NULL) goto exception; /* Exception Thrown */
    java.lang.String.class = (*env)->NewGlobalRef(env, cls);
    (*env)->DeleteLocalRef(env, cls);
    if (java.lang.String.class == NULL) goto exception;
    cls = NULL;

    java.lang.String.init_bytes_charset = (*env)->GetMethodID(env,
            java.lang.String.class, "<init>",
            "([BLjava/nio/charset/Charset;)V");
    if (java.lang.String.init_bytes_charset == NULL) goto exception;

    java.lang.String.getBytes = (*env)->GetMethodID(env,
            java.lang.String.class, "getBytes", "()[B");
    if (java.lang.String.getBytes == NULL) goto exception;
    java.lang.String.getBytes_charset = (*env)->GetMethodID(env,
            java.lang.String.class, "getBytes",
            "(Ljava/nio/charset/Charset;)[B");
    if (java.lang.String.getBytes_charset == NULL) goto exception;

    /* This code loades the UTF-8 charset using the
     * java.nio.charset.Charset.forName() method */
    cls = (*env)->FindClass(env, "java/nio/charset/Charset");
    if (cls == NULL) goto exception;
    mid = (*env)->GetStaticMethodID(env, cls, "forName",
            "(Ljava/lang/String;)Ljava/nio/charset/Charset;");
    if (mid == NULL) {
        (*env)->DeleteLocalRef(env, cls);
        goto exception;
    }
    jobj1 = (*env)->NewStringUTF(env, "UTF-8");
    if (jobj1 == NULL) {
        (*env)->DeleteLocalRef(env, cls);
        goto exception;
    }
    jobj2 = (*env)->CallStaticObjectMethod(env, cls, mid, jobj1);
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, jobj1);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        goto exception;
    java.nio.charset.utf8 = (*env)->NewGlobalRef(env, jobj2);
    if (java.nio.charset.utf8 == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        goto exception;
    }
    (*env)->DeleteLocalRef(env, jobj2); jobj2 = NULL;

    jni_object_cache_loaded = 1;

    pthread_mutex_unlock(&object_cache_mutex);

    return 0;

exception:
    pthread_mutex_unlock(&object_cache_mutex);
    return -1;
}

void
wl_jni_throw_OutOfMemoryError(JNIEnv * env, const char * message)
{
    if (java.lang.OutOfMemoryError.class == NULL) {
        if (wl_jni_ensure_object_cache(env) < 0)
            return;
    }

    (*env)->ThrowNew(env, java.lang.OutOfMemoryError.class, message);
}

void
wl_jni_throw_NullPointerException(JNIEnv * env, const char * message)
{
    if (wl_jni_ensure_object_cache(env) < 0)
        return;

    (*env)->ThrowNew(env, java.lang.NullPointerException.class, message);
}

void
wl_jni_throw_IllegalArgumentException(JNIEnv * env, const char * message)
{
    if (wl_jni_ensure_object_cache(env) < 0)
        return;

    (*env)->ThrowNew(env, java.lang.IllegalArgumentException.class, message);
}

void
wl_jni_throw_from_errno(JNIEnv * env, int err)
{
    switch (err) {
    case EINVAL:
        wl_jni_throw_IllegalArgumentException(env, strerror(err));
        return;
    case EIO:
    case EBADF:
        (*env)->ThrowNew(env, java.io.IOException.class, strerror(err));
        return;
    case ENOMEM:
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    default:
        (*env)->ThrowNew(env, java.lang.RuntimeException.class, strerror(err));
        return;
    }
}

JNIEnv *
wl_jni_get_env()
{
    JNIEnv * env;
    // If this fails, things have gone very badly 
    (*java_vm)->GetEnv(java_vm, (void **)&env, JNI_VERSION_1_2);
    return env;
}

jobject
wl_jni_register_reference(JNIEnv * env, void * native_ptr, jobject jobj)
{
    struct ptr_jobject_pair * pair;
    jobject local_ref;
    
    if ((*env)->EnsureLocalCapacity(env, 1) < 0)
        return NULL; /* Exception Thrown */

    local_ref = (*env)->NewLocalRef(env, jobj);
    if (local_ref == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);
        return NULL; /* Exception Thrown */
    }

    pair = malloc(sizeof(*pair));
    if (! pair) { // Memory Check
        wl_jni_throw_OutOfMemoryError(env, NULL);
        (*env)->DeleteLocalRef(env, local_ref);
        return NULL; /* Exception Thrown */
    }

    pair->ptr = native_ptr;
    pair->is_weak = 0;
    pair->jobj = (*env)->NewGlobalRef(env, local_ref);
    (*env)->DeleteLocalRef(env, local_ref);
    if (pair->jobj == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        free(pair);
        return NULL; /* Exception Thrown */
    }

    pthread_mutex_lock(&object_cache_mutex);
    wl_list_insert(&ptr_jobject_list, &pair->link);
    pthread_mutex_unlock(&object_cache_mutex);

    return pair->jobj;
}

jobject
wl_jni_register_weak_reference(JNIEnv * env, void * native_ptr, jobject jobj)
{
    struct ptr_jobject_pair * pair;
    pair = malloc(sizeof(*pair));
    if (pair == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL; /* Exception Thrown */
    }

    pair->ptr = native_ptr;
    pair->is_weak = 1;
    pair->jobj = (*env)->NewWeakGlobalRef(env, jobj);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        free(pair);
        return NULL; /* Exception Thrown */
    }

    pthread_mutex_lock(&object_cache_mutex);
    wl_list_insert(&ptr_jobject_list, &pair->link);
    pthread_mutex_unlock(&object_cache_mutex);

    return pair->jobj;
}

static void
delete_reference(JNIEnv * env, struct ptr_jobject_pair * pair)
{
    if (pair->is_weak) {
        (*env)->DeleteWeakGlobalRef(env, pair->jobj);
    } else {
        (*env)->DeleteGlobalRef(env, pair->jobj);
    }

    wl_list_remove(&pair->link);

    free(pair);
}

void
wl_jni_unregister_reference(JNIEnv * env, void * native_ptr)
{
    pthread_mutex_lock(&object_cache_mutex);

    struct ptr_jobject_pair * pair;
    wl_list_for_each(pair, &ptr_jobject_list, link) {
        if (pair->ptr == native_ptr) {
            delete_reference(env, pair);
            break;
        }
    }

    pthread_mutex_unlock(&object_cache_mutex);
}

jobject
wl_jni_find_reference(JNIEnv * env, void * native_ptr)
{
    struct ptr_jobject_pair * pair;
    jobject obj;

    if ((*env)->EnsureLocalCapacity(env, 1) < 0)
        return NULL; /* Exception Thrown */

    pthread_mutex_lock(&object_cache_mutex);

    obj = NULL;
    wl_list_for_each(pair, &ptr_jobject_list, link) {
        if (pair->ptr == native_ptr) {
            obj = (*env)->NewLocalRef(env, pair->jobj);

            /*
             * Clean up from deleted objects. there's no reason to leave them
             * around. Honestly, this shouldn't happen most of the time but we
             * should do it anyway just in case we have a leak.
             */
            if (obj == NULL)
                delete_reference(env, pair);

            break;
        }
    }

    pthread_mutex_unlock(&object_cache_mutex);

    return obj;
}

jstring
wl_jni_string_from_utf8(JNIEnv * env, const char * str)
{
    int len;
    jstring java_str;
    jarray bytes;
    jvalue args[2];

    if (str == NULL)
        return NULL;

    /* Set this to null so that NULL is return when an exception is thrown */
    java_str = NULL;

    if (wl_jni_ensure_object_cache(env) < 0)
        return NULL; /* Exception Thrown */

    if ((*env)->EnsureLocalCapacity(env, 2) < 0)
        return NULL; /* Exception Thrown */

    len = strlen(str);
    bytes = (*env)->NewByteArray(env, len);
    if (bytes == NULL) return NULL; /* Exception Thrown */

    (*env)->SetByteArrayRegion(env, bytes, 0, len, str);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        goto cleanup;

    args[0].l = bytes;
    args[1].l = java.nio.charset.utf8;
    java_str = (*env)->NewObjectA(env, java.lang.String.class,
            java.lang.String.init_bytes_charset, args);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        goto cleanup;

cleanup:
    (*env)->DeleteLocalRef(env, bytes);
    return java_str;
}

char *
wl_jni_string_to_utf8(JNIEnv * env, jstring java_str)
{
    int len;
    char * c_str;
    jarray bytes;

    if ((*env)->IsSameObject(env, java_str, NULL) == JNI_TRUE)
        return NULL;

    if (wl_jni_ensure_object_cache(env) < 0)
        return NULL; /* Exception Thrown */

    if ((*env)->EnsureLocalCapacity(env, 1) < 0)
        return NULL; /* Exception Thrown */

    bytes = (*env)->CallObjectMethod(env, java_str,
            java.lang.String.getBytes_charset, java.nio.charset.utf8);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return NULL; /* Exception Thrown */

    len = (*env)->GetArrayLength(env, bytes);
    c_str = malloc(len + 1);
    if (c_str == NULL) {
        (*env)->DeleteLocalRef(env, bytes);
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    (*env)->GetByteArrayRegion(env, bytes, 0, len, c_str);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        (*env)->DeleteLocalRef(env, bytes);
        free(c_str);
        return NULL;
    }

    (*env)->DeleteLocalRef(env, bytes);
    c_str[len] = '\0';
    return c_str;
}

char *
wl_jni_string_to_default(JNIEnv * env, jstring java_str)
{
    int len;
    char * c_str;
    jarray bytes;

    if ((*env)->IsSameObject(env, java_str, NULL) == JNI_TRUE)
        return NULL;

    if (wl_jni_ensure_object_cache(env) < 0)
        return NULL; /* Exception Thrown */

    if ((*env)->EnsureLocalCapacity(env, 1) < 0)
        return NULL; /* Exception Thrown */

    bytes = (*env)->CallObjectMethod(env, java_str,
            java.lang.String.getBytes);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return NULL; /* Exception Thrown */

    len = (*env)->GetArrayLength(env, bytes);
    c_str = malloc(len + 1);
    if (c_str == NULL) {
        (*env)->DeleteLocalRef(env, bytes);
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    (*env)->GetByteArrayRegion(env, bytes, 0, len, c_str);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        (*env)->DeleteLocalRef(env, bytes);
        free(c_str);
        return NULL;
    }

    (*env)->DeleteLocalRef(env, bytes);
    c_str[len] = '\0';
    return c_str;
}

JNIEXPORT jint
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    java_vm = vm;

    /* Mark the cache as not yet loaded */
    jni_object_cache_loaded = 0;
    /* This way we can detect this one individually */
    java.lang.OutOfMemoryError.class = NULL;

    pthread_mutex_init(&object_cache_mutex, NULL);

    /* Initialized the cached objects list */
    wl_list_init(&ptr_jobject_list);

    return JNI_VERSION_1_2;
}

