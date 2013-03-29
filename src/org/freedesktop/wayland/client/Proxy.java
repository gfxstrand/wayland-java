package org.freedesktop.wayland.client;

import org.freedesktop.wayland.Interface;

public class Proxy
{
    long proxy_ptr;
    private Object userData;
    private Object listener;
    private Interface iface;

    public Proxy(Proxy factory, Interface iface)
    {
        this.iface = iface;
        createNative(factory, iface);
    }

    private native void createNative(Proxy factory, Interface iface);

    public native void marshal(int opcode, Object...args);
    public native void destroy();

    public void addListener(Object listener, Object userData)
    {
        if (listener != null)
            throw new IllegalStateException("listener already set");

        if (iface.getJavaClass().isInstance(listener))
            throw new ClassCastException("listener does not implement "
                    + iface.getJavaClass().getName());

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
        System.loadLibrary("wayland-java-client");
        initializeJNI();
    }
}

