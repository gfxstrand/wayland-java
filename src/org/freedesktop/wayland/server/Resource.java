package org.freedesktop.wayland.server;

import org.freedesktop.wayland.Object;

public class Resource
{
    private long resource_ptr;

    protected Resource(int id)
    {
        _create(id);
    }

    private final native void _create(int id);

    public native void destroy(Client client);

    @Override
    public void finalize() throws Throwable
    {
        destroy(null);
        super.finalize();
    }

    private static native void initializeJNI();

    static {
        System.loadLibrary("libwayland-java-server");
        initializeJNI();
    }
}

