#include "wayland-jni.h"

#include <stdlib.h>
#include <string.h>

JavaVM * java_vm;

struct wl_list ptr_jobject_list;

int jni_object_cache_loaded;
struct {
    struct {
        struct {
            jclass class;
            jmethodID init_bytes_charset;
            jmethodID getBytes_charset;
        } String;
    } lang;

    struct {
        struct {
            jobject utf8;
        } charset;
    } nio;
} java;

static int
wl_jni_ensure_object_cache(JNIEnv * env)
{
    jclass cls;
    jmethodID mid;
    jobject jobj1, jobj2;

    if (jni_object_cache_loaded)
        return 0;

    if ((*env)->EnsureLocalCapacity(env, 3) < 0)
        return -1; /* Exception Thrown */

    cls = (*env)->FindClass(env, "java/lang/String");
    java.lang.String.class = (*env)->NewGlobalRef(env, cls);
    if (java.lang.String.class == NULL) return -1; /* Exception Thrown */
    (*env)->DeleteLocalRef(env, cls);
    cls = NULL;

    java.lang.String.init_bytes_charset = (*env)->GetMethodID(env,
            java.lang.String.class, "<init>",
            "([BLjava/nio/charset/Charset;)V");
    if (java.lang.String.init_bytes_charset == NULL) return -1;

    java.lang.String.init_bytes_charset = (*env)->GetMethodID(env,
            java.lang.String.class, "getBytes",
            "(Ljava/nio/charset/Charset;)[B");
    if (java.lang.String.getBytes_charset == NULL) return -1;

    /* This code loades the UTF-8 charset using the
     * java.nio.charset.Charset.forName() method */
    cls = (*env)->FindClass(env, "java/nio/charset/Charset");
    if (cls == NULL) return -1; /* Exception Thrown */
    mid = (*env)->GetStaticMethodID(env, cls, "forName",
            "(Ljava/lang/String;)Ljava/nio/charset/Charset;");
    if (mid == NULL) return -1; /* Exception Thrown */
    jobj1 = (*env)->NewStringUTF(env, "UTF-8");
    if (jobj1 == NULL) return -1; /* Exception Thrown */
    jobj2 = (*env)->CallStaticObjectMethod(env, cls, mid, jobj1);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) return -1;
    java.nio.charset.utf8 = (*env)->NewGlobalRef(env, jobj2);
    if (java.nio.charset.utf8 == NULL) return -1; /* Exception Thrown */
    (*env)->DeleteLocalRef(env, cls); cls = NULL;
    (*env)->DeleteLocalRef(env, jobj1); jobj1 = NULL;
    (*env)->DeleteLocalRef(env, jobj2); jobj2 = NULL;

    jni_object_cache_loaded = 1;
    return 0;
}

struct ptr_jobject_pair {
    void * ptr;
    jobject jobj;
    char is_weak;
    struct wl_list link;
};

JNIEnv *
wl_jni_get_env()
{
    JNIEnv * env;
    // If this fails, things have gone very badly 
    (*java_vm)->GetEnv(java_vm, (void **)&env, JNI_VERSION_1_2);
    return env;
}

int
wl_jni_register_reference(JNIEnv * env, void * native_ptr, jobject jobj)
{
    struct ptr_jobject_pair * pair;
    pair = malloc(sizeof(*pair));
    if (! pair) // Memory Check
        return -1;

    pair->ptr = native_ptr;
    pair->is_weak = 0;
    pair->jobj = (*env)->NewGlobalRef(env, jobj);
    if (! pair->jobj) {
        free(pair);
        return -1;
    }
    wl_list_insert(&ptr_jobject_list, &pair->link);
    return 0;
}

int
wl_jni_register_weak_reference(JNIEnv * env, void * native_ptr, jobject jobj)
{
    struct ptr_jobject_pair * pair;
    pair = malloc(sizeof(*pair));
    if (! pair) // Memory Check
        return -1;

    pair->ptr = native_ptr;
    pair->is_weak = 1;
    pair->jobj = (*env)->NewWeakGlobalRef(env, jobj);
    if (! pair->jobj) {
        free(pair);
        return -1;
    }
    wl_list_insert(&ptr_jobject_list, &pair->link);
    return 0;
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
    struct ptr_jobject_pair * pair;
    wl_list_for_each(pair, &ptr_jobject_list, link) {
        if (pair->ptr == native_ptr) {
            delete_reference(env, pair);
            return;
        }
    }
}

jobject
wl_jni_find_reference(JNIEnv * env, void * native_ptr)
{
    struct ptr_jobject_pair * pair;
    jobject obj;

    if ((*env)->EnsureLocalCapacity(env, 1) < 0)
        return NULL; /* Exception Thrown */

    wl_list_for_each(pair, &ptr_jobject_list, link) {
        if (pair->ptr == native_ptr) {
            obj = (*env)->NewLocalRef(env, pair->jobj);

            // Clean up from deleted objects. there's no reason to leave them
            // around. Honestly, this shouldn't happen most of the time but we
            // should do it anyway just in case we have a leak.
            if (obj == NULL)
                delete_reference(env, pair);

            return obj;
        }
    }
    return NULL;
}

jstring
wl_jni_string_from_utf8(JNIEnv * env, const char * str)
{
    int len;
    jstring java_str;
    jarray bytes;

    if (str == NULL)
        return NULL;

    if (wl_jni_ensure_object_cache(env) < 0)
        return NULL;

    if ((*env)->EnsureLocalCapacity(env, 2) < 0)
        return NULL; /* Exception Thrown */

    java_str = NULL;
    len = strlen(str);

    bytes = (*env)->NewByteArray(env, len);
    if (bytes == NULL) return NULL; /* Exception Thrown */

    (*env)->SetByteArrayRegion(env, bytes, 0, len, str);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        goto cleanup;

    java_str = (*env)->NewObject(env, java.lang.String.class,
            java.lang.String.init_bytes_charset, bytes, java.nio.charset.utf8);
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
        return NULL;

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
        (*env)->ThrowNew(env,
                (*env)->FindClass(env, "java/lang/OutOfMemoryError"), NULL);
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

void *
wl_array_append(struct wl_array * array, void * data, size_t size)
{
    size_t old_size;
    void * new_data;

    old_size = array->size;
    new_data = wl_array_add(array, size);

    if (new_data != NULL)
        memcpy(array->data + old_size, data, size);

    return new_data;
}

JNIEXPORT jint
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    java_vm = vm;
    jni_object_cache_loaded = 0;
    wl_list_init(&ptr_jobject_list);
    return JNI_VERSION_1_2;
}

