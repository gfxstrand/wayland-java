#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <wayland-server.h>

#include "server/server-jni.h"

struct {
    jclass class;
    jmethodID init_long;
    jmethodID init_display_int;
    jfieldID client_ptr;
} Client;

struct wl_jni_client {
    struct wl_listener destroy_listener;
    struct wl_client * client;
};

struct wl_client *
wl_jni_client_from_java(JNIEnv * env, jobject jclient)
{
    struct wl_jni_client * jni_client;

    if (jclient == NULL)
        return NULL;

    jni_client = (struct wl_jni_client *)(intptr_t)
            (*env)->GetLongField(env, jclient, Client.client_ptr);
    if (jni_client == NULL)
        return NULL;

    return jni_client->client;
}

static void
client_destroy_listener_func(struct wl_listener * listener, void * data)
{
    JNIEnv * env;
    struct wl_jni_client * jni_client;
    jobject jclient;

    env = wl_jni_get_env();

    /* we can do a direct cast here */
    jni_client = (struct wl_jni_client *)listener;

    jclient = wl_jni_find_reference(env, jni_client->client);
    if (jclient == NULL)
        return;

    /* Remove the global reference and replace it with a weak reference */
    wl_jni_unregister_reference(env, jni_client->client);
    wl_jni_register_weak_reference(env, jni_client->client, jclient);

    (*env)->DeleteLocalRef(env, jclient);
}

jobject
wl_jni_client_to_java(JNIEnv * env, struct wl_client * client)
{
    jobject jclient;
    struct wl_jni_client * jni_client;

    jclient = wl_jni_find_reference(env, client);
    if (jclient != NULL)
        return jclient;
    
    /*
     * If we get to this point, we've never seen the client so we need to make
     * a new wrapper object.
     */

    jni_client = malloc(sizeof(struct wl_jni_client));
    if (jni_client == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    jclient = (*env)->NewObject(env, Client.class, Client.init_long,
            (long)(intptr_t)jni_client);
    if (jclient == NULL) {
        free(jni_client);
        return; /* Exception Thrown */
    }

    wl_jni_register_reference(env, client, jclient);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        (*env)->DeleteLocalRef(env, jclient);
        free(jni_client);
        return; /* Exception Thrown */
    }

    jni_client->client = client;
    jni_client->destroy_listener.notify = client_destroy_listener_func;
    wl_client_add_destroy_listener(client, &jni_client->destroy_listener);

    return jclient;
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_Client_startClient(JNIEnv * env,
        jclass cls, jobject jdisplay, jobject jfile, jarray jargs)
{
    jmethodID mid;
    jstring jexec_path, jarg;
    jobject jclient;
    int nargs, arg;
    char * exec_path;
    char fd_str[16];
    char ** args;
    pid_t pid;
    int sockets[2];
    int flags;

    jclient = NULL;
    
    /* Get the string for the executable path */
    cls = (*env)->GetObjectClass(env, jfile);
    if (cls == NULL) return NULL; /* Exception Thrown */
    mid = (*env)->GetMethodID(env, cls, "getPath", "()Ljava/lang/String;");
    if (mid == NULL) return NULL; /* Exception Thrown */
    jexec_path = (*env)->CallObjectMethod(env, jfile, mid);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) return NULL;
    exec_path = wl_jni_string_to_default(env, jexec_path);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) return NULL;

    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, jexec_path);

    nargs = (*env)->GetArrayLength(env, jargs);
    args = malloc((nargs + 1) * sizeof(char *));
    if (args == NULL) {
        free(exec_path);
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return NULL;
    }

    args[nargs] = NULL;
    for (arg = 0; arg < nargs; ++arg) {
        jarg = (*env)->GetObjectArrayElement(env, jargs, arg);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
            --arg;
            goto cleanup_arguments;
        }

        args[arg] = wl_jni_string_to_utf8(env, jarg);
        if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
            goto cleanup_arguments;
        }
        (*env)->DeleteLocalRef(env, jarg);
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets)) {
        (*env)->ThrowNew(env,
            (*env)->FindClass(env, "java/lang/io/IOException"),
            "socketpair failed");
        goto cleanup_arguments;
    }

    pid = fork();
    if (pid == -1) {
        (*env)->ThrowNew(env,
                (*env)->FindClass(env, "java/lang/io/IOException"),
                "Fork Failed");
    } else if (pid == 0) {
        // We are the child
        
        // Close the parent socket
        close(sockets[0]);

        snprintf(fd_str, 16, "%d", sockets[1]);
        setenv("WAYLAND_SOCKET", fd_str, 1);
        setenv("XDG_RUNTIME_DIR", "/data/data/net.jlekstrand.wayland/", 1);

        execv(exec_path, args);
        // This is bad
        exit(-1);
    } else {
        close(sockets[1]);
        flags = fcntl(sockets[0], F_GETFD);
        flags |= FD_CLOEXEC;
        if (fcntl(sockets[0], F_SETFD, flags) == -1) {
            (*env)->ThrowNew(env,
                (*env)->FindClass(env, "java/lang/io/IOException"),
                "fnctl failed");
            goto cleanup_arguments;
        }

        jclient = (*env)->NewObject(env, Client.class, Client.init_display_int,
                jdisplay, sockets[0]);
    }

cleanup_arguments:
    for (; arg >= 0; --arg)
        free(args[arg]);
    free(args);
    free(exec_path);

    return jclient;
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Client_create(JNIEnv * env, jobject jclient,
        jobject jdisplay, int fd)
{
    struct wl_jni_client * jni_client;
    struct wl_display * display;

    display = wl_jni_display_from_java(env, jdisplay);

    jni_client = malloc(sizeof(struct wl_jni_client));
    if (jni_client == NULL) {
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }

    jni_client->client = wl_client_create(display, fd);

    (*env)->SetLongField(env, jclient, Client.client_ptr,
            (long)(intptr_t)jni_client);

    wl_jni_register_reference(env, jni_client->client, jclient);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE) {
        free(jni_client);
        return; /* Exception Thrown */
    }

    jni_client->destroy_listener.notify = client_destroy_listener_func;
    wl_client_add_destroy_listener(jni_client->client,
            &jni_client->destroy_listener);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Client_flush(JNIEnv * env, jobject jclient)
{
    wl_client_flush(wl_jni_client_from_java(env, jclient));
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_Client_newObject(JNIEnv * env,
        jobject jclient, jobject jiface, int id, jobject jdata)
{
    struct wl_client * client;
    struct wl_resource * resource;
    struct wl_object tmp_obj;
    jobject jresource;

    client = wl_jni_client_from_java(env, jclient);
    if (client == NULL)
        return NULL; /* Exception Thrown */

    wl_jni_interface_init_object(env, jiface, &tmp_obj);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return NULL; /* Exception Thrown */

    resource = wl_client_new_object(client, tmp_obj.interface,
            tmp_obj.implementation, NULL);
    if (resource == NULL)
        return NULL; /* Error */

    jresource = wl_jni_resource_create_from_native(env, resource, jdata);
    if (jresource == NULL) {
        wl_resource_destroy(resource);
        return NULL; /* Exception Thrown */
    }

    return jresource;
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_Client_addObject(JNIEnv * env,
        jobject jclient, jobject jiface, int id, jobject jdata)
{
    struct wl_client * client;
    struct wl_resource * resource;
    struct wl_object tmp_obj;
    jobject jresource;

    client = wl_jni_client_from_java(env, jclient);
    if (client == NULL)
        return NULL; /* Exception Thrown */

    wl_jni_interface_init_object(env, jiface, &tmp_obj);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return NULL; /* Exception Thrown */

    resource = wl_client_add_object(client, tmp_obj.interface,
            tmp_obj.implementation, id, NULL);
    if (resource == NULL)
        return NULL; /* Error */

    jresource = wl_jni_resource_create_from_native(env, resource, jdata);
    if (jresource == NULL) {
        wl_resource_destroy(resource);
        return NULL; /* Exception Thrown */
    }

    return jresource;
}

JNIEXPORT int JNICALL
Java_org_freedesktop_wayland_server_Client_addResource(JNIEnv * env,
        jobject jclient, jobject jresource)
{
    /* TODO: Handle the case of already attached resources */
    struct wl_client * client;
    struct wl_resource * resource;
    jobject global_ref;
    int id;

    if (jresource == NULL) {
        wl_jni_throw_NullPointerException(env, NULL);
        return;
    }

    client = wl_jni_client_from_java(env, jclient);
    resource = wl_jni_resource_from_java(env, jresource);
    if (client == NULL || resource == NULL)
        return; /* Exception Thrown */

    /*
     * This will handle making sure garbage collection happens properly. Notice
     * that this function call is before wl_client_add_resource. This is
     * because wl_client_add_resource will modify the client parameter of the
     * resource and might break garbage collection
     */
    wl_jni_resource_set_client(env, resource, client);
    if ((*env)->ExceptionCheck(env) == JNI_TRUE)
        return; /* Exception Thrown */

    return (int)wl_client_add_resource(client, resource);
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_Client_getDisplay(JNIEnv * env,
        jobject jclient)
{
    struct wl_client * client = wl_jni_client_from_java(env, jclient);
    return wl_jni_display_to_java(env, wl_client_get_display(client));
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Client_destroy(JNIEnv * env,
        jobject jclient)
{
    struct wl_client * client = wl_jni_client_from_java(env, jclient);
    if (client) {
        wl_client_destroy(client);

        wl_jni_unregister_reference(env, client);

        (*env)->SetLongField(env, jclient, Client.client_ptr, 0);
    }
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Client_initializeJNI(JNIEnv * env,
        jclass cls)
{
    Client.class = (*env)->NewGlobalRef(env, cls);
    if (Client.class == NULL)
        return; /* Exception Thrown */

    Client.init_long = (*env)->GetMethodID(env, cls, "<init>", "J");
    if (Client.init_long == NULL)
        return; /* Exception Thrown */

    Client.init_display_int = (*env)->GetMethodID(env, cls,
            "<init>", "(Lorg/freedesktop/wayland/server/Display;I)V");
    if (Client.init_display_int == NULL)
        return; /* Exception Thrown */

    Client.client_ptr = (*env)->GetFieldID(env, cls, "client_ptr", "J");
    if (Client.client_ptr == NULL)
        return; /* Exception Thrown */
}

