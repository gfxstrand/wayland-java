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
                if (iface == "wl_compositor")
                    compositor = (wl_compositor.Proxy)registry.bind(name,
                            wl_compositor.WAYLAND_INTERFACE, 1);
                else if (iface == "wl_shell")
                    shell = (wl_shell.Proxy)registry.bind(name,
                            wl_shell.WAYLAND_INTERFACE, 1);
                else if (iface == "wl_shm")
                    shm = (wl_shm.Proxy)registry.bind(name,
                            wl_shell.WAYLAND_INTERFACE, 1);

                    shm.addListener(new wl_shm.Events() {
                        @Override
                        public void format(wl_shm.Proxy p, int format)
                        {
                            shm_formats |= (1 << format);
                        }
                    }, null);
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

