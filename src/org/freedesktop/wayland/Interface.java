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
package org.freedesktop.wayland;

import java.io.StringWriter;

public class Interface
{
    public static class Message
    {
        private final String name;
        private final String signature;
        private final String javaSignature;
        private final Interface[] types;

        public Message(String name, String signature, Interface[] types)
        {
            this.name = name;
            this.signature = signature;
            this.types = types;

            StringWriter writer = new StringWriter();

            int tpos = 0;
            for (int spos = 0; spos < signature.length(); ++spos) {
                switch(signature.charAt(spos)) {
                case '?':
                    // Skip '?' characters
                    continue;
                case 'i':
                    writer.write("I");
                    break;
                case 'u':
                    writer.write("I");
                    break;
                case 'f':
                    writer.write("Lorg/freedesktop/wayland/Fixed;");
                    break;
                case 's':
                    writer.write("Ljava/lang/String;");
                    break;
                case 'o':
                    writer.write("Lorg/freedesktop/wayland/server/Resource;");
                    break;
                case 'n':
                    writer.write("I");
                    break;
                case 'a':
                    writer.write("Ljava.lang.ByteBuffer;");
                    break;
                case 'h':
                    writer.write("I");
                    break;
                default:
                    throw new IllegalArgumentException("Invalid signature");
                }
                ++tpos;
            }

            this.javaSignature = writer.toString();
        }
    }

    private String name;
    private Class<?> clazz;
    private int version;
    private Message[] requests;
    private Message[] events;
    private long interface_ptr;

    public Interface(String name, Class<?> clazz, int version,
            Message[] requests, Message[] events)
    {
        this(name, clazz, version, requests, events, 0);
    }

    public Interface(String name, Class<?> clazz, int version,  
            Message[] requests, Message[] events, long implementation_ptr)
    {
        this.name = name;
        this.clazz = clazz;
        this.version = version;
        this.requests = requests;
        this.events = events;
        this.interface_ptr = 0;

        createNative(implementation_ptr);
    }

    private native void createNative(long implementation_ptr);
    private native void destroyNative();

    public String getName()
    {
        return name;
    }

    public Class<?> getJavaClass()
    {
        return clazz;
    }

    @Override
    public void finalize() throws Throwable
    {
        destroyNative();
        super.finalize();
    }

    private static native void initializeJNI();
    static {
        System.loadLibrary("wayland-java-server");
        initializeJNI();
    }
}

