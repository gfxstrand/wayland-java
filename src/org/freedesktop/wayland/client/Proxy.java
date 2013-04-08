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

