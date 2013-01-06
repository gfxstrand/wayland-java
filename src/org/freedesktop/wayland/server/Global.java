package org.freedesktop.wayland.server;

import org.freedesktop.wayland.Interface;

public class Global
{
    public static interface BindHandler
    {
        public abstract void bindClient(Client client, int version, int id);
    }

    private long global_ptr;
    private long self_ref;
    private Interface iface;
    private BindHandler handler;

    public Global(Interface iface, BindHandler hander)
    {
        if (iface == null)
            throw new NullPointerException("Interface cannot be null");
        if (handler == null)
            throw new NullPointerException("BindHandler cannot be null");

        this.global_ptr = 0;
        this.self_ref = 0;
        this.iface = iface;
        this.handler = handler;
    }

    private native void _destroy();

    @Override
    protected void finalize() throws Throwable
    {
        _destroy();
        super.finalize();
    }

    private static native void initializeJNI();

    static {
        System.loadLibrary("wayland-java-server");
        initializeJNI();
    }
}

