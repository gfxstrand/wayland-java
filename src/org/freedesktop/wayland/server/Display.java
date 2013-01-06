package org.freedesktop.wayland.server;

import java.io.File;
import java.util.LinkedList;

import org.freedesktop.wayland.Interface;

public class Display
{
    private long display_ptr;
    private LinkedList<Global> globals;

    public Display()
    {
        globals = new LinkedList<Global>();
        create();
    }

    public native EventLoop getEventLoop();
    public native int addSocket(File name);
    public native void terminate();
    public native void run();
    public native void flushClients();
    private native void doAddGlobal(Global global);
    private native void doRemoveGlobal(Global global);

    public void addGlobal(Global global)
    {
        globals.add(global);
        doAddGlobal(global);
    }

    public Global addGlobal(Interface iface, Global.BindHandler bindHandler)
    {
        Global global = new Global(iface, bindHandler);
        addGlobal(global);
        return global;
    }

    public void removeGlobal(Global global)
    {
        doRemoveGlobal(global);
        globals.remove(global);
    }

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

    private static native void initializeJNI();

    static {
        System.loadLibrary("wayland-java-server");
        initializeJNI();
    }
}

