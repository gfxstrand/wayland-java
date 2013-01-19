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

import org.junit.*;

import org.freedesktop.wayland.Interface;
import org.freedesktop.wayland.protocol.wl_callback;

public class ResourceTest
{
    Resource resource;
    Interface iface;
    boolean resourceDestroyed;

    public ResourceTest()
    { }

    @Before
    public void createDisplay()
    {
        // High enough the server won't think it's a global
        int id = 0xff000345;
        resource = new Resource(wl_callback.WAYLAND_INTERFACE,
                id, new Object());
    }

    @Test
    public void testDestroyListener()
    {
        resourceDestroyed = false;

        resource.addDestroyListener(new Listener() {
            public void onNotify()
            {
                resourceDestroyed = true;
            }
        });

        resource.destroy();
        resource = null;

        Assert.assertTrue(resourceDestroyed);
    }

    @After
    public void destroyDisplay()
    {
        if (resource != null) {
            resource.destroy();
            resource = null;
        }
    }
}

