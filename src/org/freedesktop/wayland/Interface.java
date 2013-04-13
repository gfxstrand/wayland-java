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
        private final Interface[] types;

        public Message(String name, String signature, Interface[] types)
        {
            this.name = name;
            this.signature = signature;
            this.types = types;
        }
    }

    private long interface_ptr;

    private String name;
    private int version;
    private Message[] requests;
    private Class<?> requestsIface;
    private Message[] events;
    private Class<?> eventsIface;
    private Class<?> proxyClass;
    private Class<?> resourceClass;

    public Interface(String name, int version, Message[] requests,
            Class<?> requestsIface, Message[] events, Class<?> eventsIface,
            Class<?> proxyClass, Class<?> resourceClass)
    {
        this.interface_ptr = 0;

        this.name = name;
        this.version = version;
        this.requests = requests;
        this.requestsIface = requestsIface;
        this.events = events;
        this.eventsIface = eventsIface;
        this.proxyClass = proxyClass;
        this.resourceClass = resourceClass;
    }

    private native void destroyNative();

    public String getName()
    {
        return name;
    }

    public Class<?> getRequestsInterface()
    {
        return requestsIface;
    }

    public Class<?> getEventsInterface()
    {
        return eventsIface;
    }

    public Class<?> getProxyClass()
    {
        return proxyClass;
    }

    public Class<?> getResourceClass()
    {
        return resourceClass;
    }

    @Override
    public void finalize() throws Throwable
    {
        if (interface_ptr != 0)
            destroyNative();
        super.finalize();
    }

    private static native void initializeJNI();
    static {
        initializeJNI();
        System.loadLibrary("wayland-java-util");
    }
}

