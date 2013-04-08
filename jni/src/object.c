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

/*
 * This file contains functions that apply to all objects regardless of whether
 * they are proxies or resources.  Specifically, it contains dispatchers and
 * functions for converting arguments for posting to the wayland protocol
 */

/* 
 * Copied from wayland source because it's useful.  The argument_details
 * structure and associated functions are Copyright © 2008 Kristian Høgsberg
 */
struct argument_details {
    char type;
    int nullable;
};

static const char *
get_next_argument(const char *signature, struct argument_details *details)
{
    if (*signature == '?') {
        details->nullable = 1;
        signature++;
    } else
        details->nullable = 0;

    details->type = *signature;
    return signature + 1;
}

static int
arg_count_for_signature(const char *signature)
{
    int count = 0;
    while (*signature) {
        if (*signature != '?')
            count++;
        signature++;
    }
    return count;
}

/*
 * Frees the memory allocated by wl_jni_arguments_from_java
 */
void
wl_jni_arguments_from_java_destroy(union wl_argument *args,
        const char *signature, int count)
{
    int i;
    struct argument_details arg;

    for (i = 0; i < count; ++i) {
        signature = get_next_argument(signature, &arg);

        switch(arg.type) {
        case 's':
            free((char *)args[i].s);
            break;
        case 'a':
            free(args[i].a);
            break;
        default:
            break;
        }
    }
}

/*
 * Converts the java array of java-formatted arguments to wayland-formatted
 * arguments and stores them in args.
 */
void wl_jni_arguments_from_java(JNIEnv *env, union wl_argument *args,
        jarray jargs, const char *signature, int count,
        struct wl_object *(* object_conversion)(JNIEnv *env, jobject))
{
    int i;
    const char *sig_tmp;
    struct argument_details arg;
    jobject jobj;

    sig_tmp = signature;
    for (i = 0; i < count; ++i) {
        sig_tmp = get_next_argument(sig_tmp, &arg);

        jobj = (*env)->GetObjectArrayElement(env, jargs, i);
        if ((*env)->ExceptionCheck(env))
            goto free_args;

        switch(arg.type) {
        case 'i':
            args[i].i = (int32_t)wl_jni_unbox_integer(env, jobj);
            break;
        case 'u':
            args[i].u = (uint32_t)wl_jni_unbox_integer(env, jobj);
            break;
        case 'f':
            args[i].f = wl_jni_fixed_from_java(env, jobj);
            break;
        case 's':
            args[i].s = wl_jni_string_to_utf8(env, jobj);
            break;
        case 'o':
            args[i].o = (*object_conversion)(env, jobj);
            break;
        case 'n':
            /* new_id types are actually expected to be passed in as objects */
            args[i].o = (*object_conversion)(env, jobj);
            break;
        case 'a':
            args[i].a = malloc(sizeof(struct wl_array));
            if (args[i].a == NULL) {
                wl_jni_throw_OutOfMemoryError(env, NULL);
                goto free_args;
            }

            args[i].a->alloc = 0;
            args[i].a->data = (*env)->GetDirectBufferAddress(env, jobj);
            args[i].a->size = (*env)->GetDirectBufferCapacity(env, jobj);
            break;
        case 'h':
            args[i].h = wl_jni_unbox_integer(env, jobj);
            break;
        }
        (*env)->DeleteLocalRef(env, jobj);
        if ((*env)->ExceptionCheck(env))
            goto free_args;
    }

    return;

free_args:
    wl_jni_arguments_from_java_destroy(args, signature, i);
}

/*
 * Converts the wayland-formatted arguments in args to java-formatted arguments
 * and stores them in the array given by jargs
 */
void
wl_jni_arguments_to_java(JNIEnv *env, union wl_argument *args, jvalue *jargs,
        const char *signature, int count, jboolean new_id_is_object,
        jobject (* object_conversion)(JNIEnv *env, struct wl_object *))
{
    int i;
    struct argument_details arg;

    for (i = 0; i < count; ++i) {
        signature = get_next_argument(signature, &arg);

        switch(arg.type) {
        case 'i':
            jargs[i].i = (jint)args[i].i;
            break;
        case 'u':
            jargs[i].i = (jint)args[i].u;
            break;
        case 'f':
            jargs[i].l = wl_jni_fixed_to_java(env, args[i].f);
            if (jargs[i].l == NULL)
                goto error;
            break;
        case 's':
            if (! arg.nullable && args[i].s == NULL) {
                wl_jni_throw_NullPointerException(env, NULL);
                goto error;
            }

            jargs[i].l = wl_jni_string_from_utf8(env, args[i].s);
            if ((*env)->ExceptionCheck(env))
                goto error;
            break;
        case 'n':
            if (! new_id_is_object) {
                jargs[i].l = (*object_conversion)(env, args[i].o);
                if ((*env)->ExceptionCheck(env))
                    goto error;
            } else {
                jargs[i].i = (jint)args[i].n;
            }
            break;
        case 'o':
            if (! arg.nullable && args[i].o == NULL) {
                wl_jni_throw_NullPointerException(env, NULL);
                goto error;
            }

            jargs[i].l = (*object_conversion)(env, args[i].o);
            if ((*env)->ExceptionCheck(env))
                goto error;
            break;
        case 'a':
            jargs[i].l = (*env)->NewDirectByteBuffer(env,
                    args[i].a->data, args[i].a->size);
            if (jargs[i].l == NULL)
                goto error; /* Exception Thrown */
            break;
        case 'h':
            jargs[i].i = (jint)args[i].h;
            break;
        case '?':
            continue;
        default:
            wl_jni_throw_IllegalArgumentException(env,
                    "Invalid wayland request prototype");
            goto error; /* Exception Thrown */
        }
    }

error:
    return;
}

