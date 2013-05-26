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
package org.freedesktop.wayland.client;

import java.lang.Class;
import java.lang.reflect.Constructor;

import org.freedesktop.wayland.Interface;
import org.freedesktop.wayland.protocol.wl_display;

public class Proxy
{
    long proxy_ptr;
    private Object userData;
    private Object listener;
    private Interface iface;

    protected Proxy(Proxy factory, Interface iface)
    {
        this.proxy_ptr = 0;
        this.userData= null;
        this.listener = null;
        this.iface = iface;

        // Creating a wl_display proxy is a special case.  The actual display
        // object will be created in Display.connect and this serves only as a
        // wrapper.
        if (iface != wl_display.WAYLAND_INTERFACE)
            createNative(factory, iface);
    }

    public static Proxy create(Proxy factory, Interface iface)
            throws NoSuchMethodException, InstantiationException,
                   IllegalAccessException,
                   java.lang.reflect.InvocationTargetException
    {
        Class<?> proxyClass = iface.getProxyClass();

        Constructor<?> ctor = proxyClass.getConstructor(Proxy.class);
        return (Proxy)ctor.newInstance(factory);
    }

    private native void createNative(Proxy factory, Interface iface);

    public native void marshal(int opcode, Object...args);
    public native void destroy();

    protected void addListener(Object listener, Object userData)
    {
        if (this.listener != null)
            throw new IllegalStateException("listener already set");

        this.listener = listener;
        this.userData = userData;
    }

    public Object getUserData()
    {
        return userData;
    }

    public void setUserData(Object userData)
    {
        this.userData = userData;
    }

    public native int getID();

    public String getInterfaceName()
    {
        return iface.getName();
    }

    public native void setQueue(EventQueue queue);

    private static native void initializeJNI();
    static {
        System.loadLibrary("wayland-java-util");
        System.loadLibrary("wayland-java-client");
        initializeJNI();
    }
}

