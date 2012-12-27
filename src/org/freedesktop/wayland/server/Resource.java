package org.freedesktop.wayland.server;

import org.freedesktop.wayland.Interface;

public class Resource
{
    private long resource_ptr;

    protected Resource(int id)
    {
        _create(id);
    }

    // public Resource(Object obj, int id, Interface iface);

    private final native void _create(int id);

    public native void destroy(Client client);

    @Override
    public void finalize() throws Throwable
    {
        destroy(null);
        super.finalize();
    }

    private static native void initializeJNI();

    // public static native void postEvent(int opcode, Object...args);

    static {
        System.loadLibrary("wayland-java-server");
        initializeJNI();
    }
}

