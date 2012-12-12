#include "wayland-jni.h"

#include <stdlib.h>

#include "wayland-util.h"

JavaVM * java_vm;

struct wl_list ptr_jobject_list;

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

JNIEXPORT jint
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    java_vm = vm;
    wl_list_init(&ptr_jobject_list);
    return JNI_VERSION_1_2;
}

