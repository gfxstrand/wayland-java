import org.freedesktop.wayland.client.*;
import org.freedesktop.wayland.ShmPool;
import org.freedesktop.wayland.protocol.*;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

public class Window
{
    public class Buffer
    {
        boolean busy;
        private wl_buffer.Proxy buffer;
        private ShmPool shm_pool;

        public Buffer()
        {
            try {
                shm_pool = new ShmPool(width * height * 4);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }

        public ByteBuffer getByteBuffer()
        {
            return shm_pool.asByteBuffer();
        }

        public wl_buffer.Proxy getProxy()
        {
            if (buffer == null) {
                wl_shm_pool.Proxy pool = display.shm.createPool(
                        shm_pool.getFileDescriptor(), width * height * 4);
                buffer = pool.createBuffer(0, width, height, width * 4,
                        wl_shm.FORMAT_XRGB8888);
                buffer.addListener(new wl_buffer.Events() {
                    public void release(wl_buffer.Proxy proxy)
                    {
                        busy = false;
                    }
                }, null);
                pool.destroy();
            }

            return buffer;
        }
    }

    public final Display display;

    public final int width;
    public final int height;

    public wl_surface.Proxy surface;
    public wl_shell_surface.Proxy shell_surface;
    public wl_callback.Proxy callback;
    Buffer buffers[];
    Buffer prev_buffer;

    public Window(Display display, int width, int height)
    {
        this.display = display;
        this.width = width;
        this.height = height;

        buffers = new Buffer[2];
        buffers[0] = new Buffer();
        buffers[1] = new Buffer();

        surface = display.compositor.createSurface();
        surface.damage(0, 0, width, height);

        shell_surface = display.shell.getShellSurface(surface);
        shell_surface.addListener(new wl_shell_surface.Events() {
            @Override
            public void ping(wl_shell_surface.Proxy shell_surface, int serial)
            {
                shell_surface.pong(serial);
            }

            @Override
            public void configure(wl_shell_surface.Proxy p, int edges,
                    int width, int height)
            { }

            @Override
            public void popupDone(wl_shell_surface.Proxy p)
            { }
        }, null);

        shell_surface.setTitle("simple-shm");
        shell_surface.setToplevel();
    }

    public void destroy()
    {
        if (callback != null)
            callback.destroy();

        shell_surface.destroy();
        surface.destroy();
    }

    public Buffer nextBuffer()
    {
        if (!buffers[0].busy)
            return buffers[0];
        else if (!buffers[1].busy)
            return buffers[1];
        else
            return null;
    }

    private int abs(int i)
    {
        return i < 0 ? -i : i;
    }

    private void paintPixels(ByteBuffer buffer, int padding, int time)
    {
        final int halfh = padding + (height - padding * 2) / 2;
        final int halfw = padding + (width  - padding * 2) / 2;
        int ir;
        int or;
        IntBuffer image = buffer.asIntBuffer();
        image.clear();
        for (int i = 0; i < width * height; ++i)
            image.put(0xffffffff);
        image.clear();

        /* squared radii thresholds */
        or = (halfw < halfh ? halfw : halfh) - 8;
        ir = or - 32;
        or = or * or;
        ir = ir * ir;

        image.position(padding * width);
        for (int y = padding; y < height - padding; y++) {
            int y2 = (y - halfh) * (y - halfh);

            image.position(image.position() + padding);
            for (int x = padding; x < width - padding; x++) {
                int v;

                int r2 = (x - halfw) * (x - halfw) + y2;

                if (r2 < ir)
                    v = (r2 / 32 + time / 64) * 0x0080401;
                else if (r2 < or)
                    v = (y + time / 32) * 0x0080401;
                else
                    v = (x + time / 16) * 0x0080401;
                v &= 0x00ffffff;

                if (abs(x - y) > 6 && abs(x + y - height) > 6)
                    v |= 0xff000000;

                image.put(v);
            }
            image.position(image.position() + padding);
        }
    }

    public void redraw(int time)
    {
        Buffer buffer = nextBuffer();
        
        paintPixels(buffer.getByteBuffer(), 20, time);
        surface.attach(buffer.getProxy(), 0, 0);
        surface.damage(20, 20, height - 40, height - 40);

        if (callback != null)
            callback.destroy();
        callback = surface.frame();
        callback.addListener(new wl_callback.Events() {
            @Override
            public void done(wl_callback.Proxy p, int serial)
            {
                redraw(serial);
            }
        }, null);
        buffer.busy = true;
        surface.commit();
    }
}

