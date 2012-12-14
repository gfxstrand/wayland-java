package org.freedesktop.wayland.server;

public class Client
{
    private long client_ptr;

    public Client(Display display, int fd)
    {
        create(display, fd);
    }

    private native void create(Display display, int fd);
    public native void flush();
    public native int addResource(Resource resource);
    public native Display getDisplay();
    public native void destroy();

    @Override
    protected void finalize() throws Throwable
    {
        destroy();
        super.finalize();
    }

    static {
        System.loadLibrary("wayland-java-server");
    }
}

