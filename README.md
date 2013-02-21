# Java bindings for libwayland

This project aims to provide Java bindings to the Wayland backend library.
It does so by a combination of auto-generated Java code and appropriate JNI
bindings.  The main purpose of wayland-java is to service the Android
wayland server app that I am currently developing.

## Current state of the library

Right now, wayland-java is nowhere close to production code; I would call it an
early alpha at best.  Very little is commented and it probably has a plethora
of bugs.  However, it is working well enough to backend the bits of my app that
I have working.

## Building wayland-java locally

Building wayland-java locally isn't too difficult assuming you have a
sufficiently patched version of the wayland library.

1. Clone wayland master and apply this [patch series][1]
2. Build and install libwayland
3. Clone git://github.com/jekstrand/wayland-java.git
4. Run ant

## Building wayland-java for Android

Building the library for Android requires a slight modification to the wayland
source code.  I hope to submitting patches to the wayland project soon so that
it can be built unaltered. If you're brave enough to try and build wayland-java
on Android, you need to do the following:

1. Install both the android SDK and NDK and get them configured
2. Clone git://github.com/jekstrand/wayland-java.git
3. Run `git submodule init` and `git submodule update`
4. In the jni/external/wayland directory, you have to modify the wayland source
   code to remove calls to `signalfd` and `timerfd` and their respective header
   files
5. Make sure that the version of jni/external/wayland is patched with this
   [patch series][1]
6. Use the Android tools to update the build paths in the `android`
   subdirectory to your version of the Android SDK
7. Run ant

[1]: http://lists.freedesktop.org/archives/wayland-devel/2013-February/007473.html

