package org.freedesktop.wayland.client;

public class Display
{
    long display_ptr;

    Display(long native_ptr)
    {
        this.display_ptr = native_ptr;
    }

    public native static Display connect(String name);
    public native static Display connect(int fd);

    public native void disconnect();
    public native int getFD();
    public native int dispatch();
    public native int dispatchPending();
    public native int dispatchQueue(EventQueue queue);
    public native int dispatchQueuePending(EventQueue queue);
    public native int flush();
    public native int roundtrip();

    private static native void initializeJNI();
    static {
        System.loadLibrary("wayland-java-client");
        initializeJNI();
    }
}

