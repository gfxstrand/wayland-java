/*
 * Copyright © 2011 Benjamin Franzke
 * Copyright © 2010 Intel Corporation
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
package org.freedesktop.wayland.examples.simpleshm;

import org.freedesktop.wayland.client.*;
import org.freedesktop.wayland.ShmPool;
import org.freedesktop.wayland.protocol.*;

import java.io.IOException;

public class Display
{
    public final org.freedesktop.wayland.client.Display display;
    public wl_registry.Proxy registry;
    public wl_compositor.Proxy compositor;
    public wl_shell.Proxy shell;
    public wl_shm.Proxy shm;
    public int shm_formats;

    public Display()
    {
        display = org.freedesktop.wayland.client.Display.connect(null);
        registry = display.getRegistry();
        shm_formats = 0;
        registry.addListener(new wl_registry.Events() {
            @Override
            public void global(wl_registry.Proxy registry, int name,
                    String iface, int version)
            {
                if ("wl_compositor".equals(iface)) {
                    compositor = (wl_compositor.Proxy)registry.bind(name,
                            wl_compositor.WAYLAND_INTERFACE, 1);
                } else if ("wl_shell".equals(iface)) {
                    shell = (wl_shell.Proxy)registry.bind(name,
                            wl_shell.WAYLAND_INTERFACE, 1);
                } else if ("wl_shm".equals(iface)) {
                    shm = (wl_shm.Proxy)registry.bind(name,
                            wl_shm.WAYLAND_INTERFACE, 1);

                    shm.addListener(new wl_shm.Events() {
                        @Override
                        public void format(wl_shm.Proxy p, int format)
                        {
                            shm_formats |= (1 << format);
                        }
                    }, null);
                }
            }

            @Override
		    public void globalRemove(wl_registry.Proxy registry, int name)
            { }
        }, null);
        display.roundtrip();

        if (shm == null)
            throw new NullPointerException("wl_shm not found!");

        display.roundtrip();
    }

    public void destroy()
    {
        if (shm != null)
            shm.destroy();
        if (shell != null)
            shell.destroy();

        compositor.destroy();
        registry.destroy();
        display.flush();
        display.disconnect();
    }
}

