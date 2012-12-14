package org.freedesktop.wayland;

public class Fixed
{
    private int data;

    private Fixed(int raw)
    {
        this.data = raw;
    }

    static {
        System.loadLibrary("wayland-java-server");
    }
}

