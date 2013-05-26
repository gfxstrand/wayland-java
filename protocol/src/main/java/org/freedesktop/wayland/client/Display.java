/*
 * Copyright © 2012-2013 Jason Ekstrand.
 *  
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 * 
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
package org.freedesktop.wayland.client;

import org.freedesktop.wayland.protocol.wl_display;

public class Display extends wl_display.Proxy
{
    Display(long native_ptr)
    {
        super(null);
        ((Proxy)this).proxy_ptr = native_ptr;
    }

    public native static Display connect(String name);
    public native static Display connect(int fd);

    public native void disconnectNative();
    public void disconnect()
    {
        disconnectNative();
        ((Proxy)this).proxy_ptr = 0;
    }

    public native int getFD();
    public native int dispatch();
    public native int dispatchPending();
    public native int dispatchQueue(EventQueue queue);
    public native int dispatchQueuePending(EventQueue queue);
    public native int flush();
    public native int roundtrip();

    static {
        System.loadLibrary("wayland-java-util");
        System.loadLibrary("wayland-java-client");
    }
}

