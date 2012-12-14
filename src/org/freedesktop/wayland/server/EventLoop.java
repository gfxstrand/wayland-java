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

