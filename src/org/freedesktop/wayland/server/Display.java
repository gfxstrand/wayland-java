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
package org.freedesktop.wayland.server;

import java.io.File;
import java.util.LinkedList;

import org.freedesktop.wayland.Interface;

public class Display
{
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
    public native void addGlobal(Global global);
    public native void removeGlobal(Global global);

    public Global addGlobal(Interface iface, Global.BindHandler bindHandler)
    {
        Global global = new Global(iface, bindHandler);
        addGlobal(global);
        return global;
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

