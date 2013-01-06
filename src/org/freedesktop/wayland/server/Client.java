package org.freedesktop.wayland.server;

import java.io.File;

import org.freedesktop.wayland.Interface;

public class Client
{
    private long client_ptr;

    private Client(long client_ptr)
    {
        this.client_ptr = client_ptr;
    }

    public Client(Display display, int fd)
    {
        create(display, fd);
    }

    public static native Client startClient(Display display, File executable,
            String[] args);

    private native void create(Display display, int fd);
    public native void flush();
    public native int addResource(Resource resource);
    public native Resource addObject(Interface iface, int id, Object data);
    public native Resource newObject(Interface iface, Object data);
    public native Display getDisplay();
    public native void destroy();

    @Override
    protected void finalize() throws Throwable
    {
        destroy();
        super.finalize();
    }

    private static native void initializeJNI();

    static {
        System.loadLibrary("wayland-java-server");
        initializeJNI();
    }
}

