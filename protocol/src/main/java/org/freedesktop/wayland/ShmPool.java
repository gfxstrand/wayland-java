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
package org.freedesktop.wayland;

import java.io.Closeable;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public final class ShmPool implements Closeable
{
    private int fd;
    private long size;
    private boolean readOnly;
    private ByteBuffer buffer;

    private ShmPool(int fd, long size, boolean dupFD, boolean readOnly)
            throws IOException
    {
        this.fd = fd;
        this.size = size;
        this.readOnly = readOnly;
        this.buffer = map(fd, size, dupFD, readOnly);
    }

    private static ByteBuffer map(int fd, long size, boolean dupFD,
            boolean readOnly) throws IOException
    {
        ByteBuffer tmpBuff = mapNative(fd, size, dupFD, readOnly);
        tmpBuff.order(ByteOrder.nativeOrder());
        return tmpBuff;
    }

    public ShmPool(long size) throws IOException
    {
        this.fd = createTmpFileNative();
        this.size = size;
        this.readOnly = false;
        try {
            truncateNative(this.fd, this.size);
            this.buffer = map(this.fd, this.size, false, false);
        } catch (IOException e) {
            closeNative(this.fd);
            throw e;
        }
    }

    public static ShmPool fromFileDescriptor(int fd, long size, boolean dupFD,
            boolean readOnly) throws IOException
    {
        return new ShmPool(fd, size, dupFD, readOnly);
    }

    public ByteBuffer asByteBuffer()
    {
        if (buffer == null)
            throw new IllegalStateException("ShmPool is closed");

        final ByteBuffer buffCopy;

        if (readOnly) {
            buffCopy = buffer.asReadOnlyBuffer();
        } else {
            buffCopy = buffer.duplicate();
        }

        return buffCopy.order(ByteOrder.nativeOrder());
    }

    public int getFileDescriptor()
    {
        return fd;
    }

    public long size()
    {
        return size;
    }

    public boolean isReadOnly()
    {
        return readOnly;
    }

	public void resize(long size, boolean truncate) throws IOException
    {
        if (buffer == null)
            throw new IllegalStateException("ShmPool is closed");

        unmapNative(buffer);

        this.size = size;
        if (truncate)
            truncateNative(fd, size);

        buffer = map(fd, size, false, readOnly);
    }

	public void resize(long size) throws IOException
    {
        resize(size, !readOnly);
    }

    @Override
	public void close() throws IOException
    {
        if (buffer != null) {
            unmapNative(buffer);
            this.fd = -1;
            this.size = 0;
            this.buffer = null;
        }
    }

    @Override
    public void finalize() throws Throwable
    {
        close();
    }

    private static native int createTmpFileNative()
            throws IOException;
    private static native ByteBuffer mapNative(int fd, long size, boolean dupFD,
            boolean readOnly) throws IOException;
    private static native void unmapNative(ByteBuffer buffer)
            throws IOException;
    private static native void truncateNative(int fd, long size)
            throws IOException;
    private static native void closeNative(int fd)
            throws IOException;

    static {
        System.loadLibrary("wayland-java-util");
    }
}

