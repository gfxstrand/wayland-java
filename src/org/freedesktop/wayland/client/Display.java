package org.freedesktop.wayland.client;

import org.freedesktop.wayland.protocol.wl_display;

public class Display extends wl_display.Proxy
{
    Display(long native_ptr)
    {
        super(null);
        ((Proxy)this).proxy_ptr = native_ptr;
    }

    public native static Display connect(String name);
    public native static Display connect(int fd);

    public native void disconnectNative();
    public void disconnect()
    {
        disconnectNative();
        ((Proxy)this).proxy_ptr = 0;
    }

    public native int getFD();
    public native int dispatch();
    public native int dispatchPending();
    public native int dispatchQueue(EventQueue queue);
    public native int dispatchQueuePending(EventQueue queue);
    public native int flush();
    public native int roundtrip();
}

