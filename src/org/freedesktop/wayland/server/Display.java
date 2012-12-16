package org.freedesktop.wayland.server;

import java.io.File;

public class Display
{
    public static class Global
    {
        private long global_ptr;

        private Global(long global_ptr)
        {
            this.global_ptr = global_ptr;
        }
    }

    private long display_ptr;

    public Display()
    {
        create();
    }

    public native EventLoop getEventLoop();
    public native int addSocket(File name);
    public native void terminate();
    public native void run();
    public native void flushClients();
    public native Global addGlobal(Resource resource);
    // public native void removeGlobal(Global global);
    public native int getSerial();
    public native int nextSerial();

    private native void create();
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

