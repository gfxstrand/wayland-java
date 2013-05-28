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

import org.freedesktop.wayland.arch.Native;
import org.freedesktop.wayland.Interface;

public class Resource
{
    long resource_ptr;
    private Object data;

    protected
    Resource(Client client, Interface iface, int id, Object data)
    {
        this.data = data;
        this.resource_ptr = client.addResourceNative(this, iface, id);
    }

    protected
    Resource(Client client, Interface iface, Object data)
    {
        this(client, iface, 0, data);
    }

    public Object
    getData()
    {
        return data;
    }

    public native Client getClient();
    public native int getId();
    public native void addDestroyListener(DestroyListener listener);
    public native void destroy();

    public native void postEvent(int opcode, Object...args);
    public native void postError(int code, String msg);

    private static native void initializeJNI();

    @Override
    public int hashCode()
    {
        return (int)resource_ptr;
    }

    // public static native void postEvent(int opcode, Object...args);

    static {
        Native.loadLibrary("wayland-java-util");
        Native.loadLibrary("wayland-java-server");
        initializeJNI();
    }
}

