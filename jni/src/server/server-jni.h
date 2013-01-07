#ifndef __WAYLAND_JAVA_SERVER_JNI_H__
#define __WAYLAND_JAVA_SERVER_JNI_H__

#include "wayland-jni.h"

#include "wayland-server.h"

struct wl_client * wl_jni_client_from_java(JNIEnv * env, jobject client);
jobject wl_jni_client_to_java(JNIEnv * env, struct wl_client * client);

struct wl_display * wl_jni_display_from_java(JNIEnv * env, jobject display);
jobject wl_jni_display_to_java(JNIEnv * env, struct wl_display * display);

struct wl_global * wl_jni_global_from_java(JNIEnv * env, jobject jglobal);
jobject wl_jni_global_get_interface(JNIEnv * env, jobject jglobal);
void wl_jni_global_set_data(JNIEnv * env, jobject jglobal, jobject self_ref,
        struct wl_global * global);
void wl_jni_global_bind_func(struct wl_client * client, void * data,
        uint32_t version, uint32_t id);
void wl_jni_global_release(JNIEnv * env, jobject jglobal);

struct wl_event_loop * wl_jni_event_loop_from_java(JNIEnv * env, jobject event_loop);
jobject wl_jni_event_loop_to_java(JNIEnv * env, struct wl_event_loop * event_loop);

jobject wl_jni_resource_create_from_native(JNIEnv * env,
        struct wl_resource * resource, jobject jdata);
struct wl_resource * wl_jni_resource_from_java(JNIEnv * env, jobject resource);
jobject wl_jni_resource_to_java(JNIEnv * env, struct wl_resource * resource);
void wl_jni_resource_set_client(JNIEnv * env, struct wl_resource * resource,
        struct wl_client * client);

void wl_jni_resource_call_request(struct wl_client * client,
        struct wl_resource * resource,
        const char * method_name, const char * prototype,
        const char * java_prototype, ...);

struct wl_jni_listener {
    struct wl_listener listener;
    struct wl_listener destroy_listener;
    jobject self_ref;
};

struct wl_jni_listener * wl_jni_listener_from_java(JNIEnv * env,
        jobject jlistener);

void wl_jni_listener_added_to_signal(JNIEnv * env, jobject jlistener);

#endif /* ! defined __WAYLAND_JAVA_SERVER_JNI_H__ */

