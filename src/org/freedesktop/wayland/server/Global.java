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

import org.freedesktop.wayland.Interface;

public class Global extends NativeObjectWrapper
{
    public static interface BindHandler
    {
        public abstract void bindClient(Client client, int version, int id);
    }

    private Interface iface;
    private BindHandler handler;

    public Global(Interface iface, BindHandler handler)
    {
        if (iface == null)
            throw new NullPointerException("Interface cannot be null");
        if (handler == null)
            throw new NullPointerException("BindHandler cannot be null");

        this.iface = iface;
        this.handler = handler;
    }

    protected Global(Interface iface)
    {
        this.iface = iface;
        this.handler = null;
    }

    public void bindClient(Client client, int version, int id)
    {
    	if (handler == null)
            throw new IllegalStateException("Handler not set and bindClient not overridden");

        handler.bindClient(client, version, id);
    }

    private static native void initializeJNI();

    static {
        System.loadLibrary("wayland-java-util");
        System.loadLibrary("wayland-java-server");
        initializeJNI();
    }
}

