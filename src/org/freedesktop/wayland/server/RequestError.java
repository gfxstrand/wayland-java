package org.freedesktop.wayland.server;

public class RequestError extends Error
{
    private int errorCode;

    protected RequestError(String message, Resource resource, int errorCode)
    {
        super(message);
        this.errorCode = errorCode;
    }
}

