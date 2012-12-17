#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <wayland-server.h>

#include "server/server-jni.h"

struct wl_client *
wl_jni_client_from_java(JNIEnv * env, jobject jclient)
{
    jclass cls = (*env)->GetObjectClass(env, jclient);
    jfieldID fid = (*env)->GetFieldID(env, cls, "client_ptr", "J");
    return (struct wl_client *)(*env)->GetLongField(env, jclient, fid);
}

jobject
wl_jni_client_to_java(JNIEnv * env, struct wl_client * client)
{
    return wl_jni_find_reference(env, client);
}

JNIEXPORT jobject JNICALL
Java_org_freedesktop_wayland_server_Client_startClient(JNIEnv * env,
        jclass Client, jobject jdisplay, jobject jfile, jarray jargs)
{
    jclass cls;
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

        cls = (*env)->FindClass(env, "org/freedesktop/wayland/server/Client");
        if (cls == NULL) goto cleanup_arguments;
        mid = (*env)->GetMethodID(env, cls, "<init>",
                "(Lorg/freedesktop/wayland/server/Display;I)V");
        if (mid == NULL) goto cleanup_arguments;
        jclient = (*env)->NewObject(env, cls, mid, jdisplay, sockets[0]);
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
    struct wl_display * display = wl_jni_display_from_java(env, jdisplay);
    struct wl_client * client = wl_client_create(display, fd);

    wl_jni_register_reference(env, client, jdisplay);

    jclass cls = (*env)->GetObjectClass(env, jclient);
    jfieldID fid = (*env)->GetFieldID(env, cls, "client_ptr", "J");
    (*env)->SetLongField(env, jclient, fid, (long)client);
}

JNIEXPORT void JNICALL
Java_org_freedesktop_wayland_server_Client_flush(JNIEnv * env, jobject jclient)
{
    wl_client_flush(wl_jni_client_from_java(env, jclient));
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

    /* We need to convert the weak global reference to a global reference for
     * proper garbage collection. See also: resource.c */
    global_ref = (*env)->NewGlobalRef(env, (jobject)resource->data);
    if (global_ref == NULL) {
        /* The only way this can happen is an out-of-memory error */
        wl_jni_throw_OutOfMemoryError(env, NULL);
        return;
    }
    (*env)->DeleteWeakGlobalRef(env, (jobject)resource->data);
    resource->data = global_ref;

    id = (int)wl_client_add_resource(client, resource);
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

        jclass cls = (*env)->GetObjectClass(env, jclient);
        jfieldID fid = (*env)->GetFieldID(env, cls, "client_ptr", "J");
        (*env)->SetLongField(env, jclient, fid, 0);
    }
}

