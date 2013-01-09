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

public class EventLoop
{
    public class EventSource
    {
        private long event_src_ptr;
        
        private EventSource(long event_src_ptr)
        {
            this.event_src_ptr = event_src_ptr;
        }
    }

    public interface FileDescriptorEventHandler
    {
        public abstract int handleFileDestriptorEvent(int fd, int mask);
    }

    public interface TimerEventHandler
    {
        public abstract int handleTimerEvent();
    }

    public interface SignalEventHandler
    {
        public abstract int handleSignalEvent(int signalNumber);
    }

    public interface IdleHandler
    {
        public abstract void handleIdle();
    }

    private long event_loop_ptr;
    private final boolean isWrapper;

    private EventLoop(long native_ptr)
    {
        isWrapper = true;
        create(native_ptr);
    }

    public EventLoop()
    {
        isWrapper = false;
        create(0);
    }

    public native EventSource addFileDescriptor(int fd, int mask,
            FileDescriptorEventHandler handler);
    public native int updateFileDescriptor(EventSource source, int mask);
    public native EventSource addTimer(TimerEventHandler handler);
    public native int updateTimer(EventSource source, int milliseconds);
    public native EventSource addSignal(int signalNumber, SignalEventHandler handler);
    public native EventSource addIdle(IdleHandler handler);
    public native int remove(EventSource source);
    public native void check(EventSource source);

    public native int dispatch(int timeout);
    public native void dispatchIdle();

    private native void create(long native_ptr);
    private native void destroy();

    @Override
    public void finalize() throws Throwable
    {
        destroy();
        super.finalize();
    }

    static {
        System.loadLibrary("wayland-java-server");
    }
}

