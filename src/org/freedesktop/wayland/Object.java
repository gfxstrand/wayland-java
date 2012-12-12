package org.freedesktop.wayland;

public class Object
{
    private long object_ptr;

    protected Object(long object_ptr)
    {
        this.object_ptr = object_ptr;
    }
}

