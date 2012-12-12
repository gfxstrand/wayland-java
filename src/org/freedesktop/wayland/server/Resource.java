package org.freedesktop.wayland.server;

import org.freedesktop.wayland.Object;

public class Resource extends Object
{
    protected Resource()
    {
        super(0);
        create();
    }

    private final native void create();
    public native void destroy();

    @Override
    public void finalize() throws Throwable
    {
        destroy();
        super.finalize();
    }
}

