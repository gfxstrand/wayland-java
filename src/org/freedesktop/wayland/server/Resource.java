package org.freedesktop.wayland.server;

import org.freedesktop.wayland.Object;

public class Resource extends Object
{
    protected Resource(int id)
    {
        super(0);
        create(id);
    }

    private final native void create(int id);
    public native void destroy();

    @Override
    public void finalize() throws Throwable
    {
        destroy();
        super.finalize();
    }
}

