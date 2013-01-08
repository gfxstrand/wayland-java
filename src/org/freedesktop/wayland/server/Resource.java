package org.freedesktop.wayland.server;

import org.freedesktop.wayland.Interface;

public class Resource
{
    private long resource_ptr;
    private Object data;

    private
    Resource(long resource_ptr, Object data)
    {
        this.resource_ptr = resource_ptr;
        this.data = data;
    }

    public
    Resource(Interface iface, int id, Object data)
    {
        this.data = data;
        _create(id, iface);
    }

    protected
    Resource(Interface iface, int id)
    {
        this.data = this;
        _create(id, iface);
    }

    /**
     * Creates the native object.
     */
    private final native void _create(int id, Interface iface);

    /**
     * Destroys the native object.
     *
     * This function only destroys the native object. It is NOT safe to call
     * twice.
     */
    private final native void _destroy();

    public native void addDestroyListener(Listener listener);
    public native void destroy();

    @Override
    public void finalize() throws Throwable
    {
        _destroy();
        super.finalize();
    }

    private static native void initializeJNI();

    // public static native void postEvent(int opcode, Object...args);

    static {
        System.loadLibrary("wayland-java-server");
        initializeJNI();
    }
}

