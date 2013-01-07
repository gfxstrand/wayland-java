package org.freedesktop.wayland.server;

public abstract class Listener
{
    private long listener_ptr;

    public
    Listener()
    {
        _create();
    }

    public native void detach();

    public abstract void onNotify();

    private native void _create();
    private native void _destroy();

    @Override
    public void finalize() throws Throwable
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

