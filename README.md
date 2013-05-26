# Java bindings for libwayland

This project aims to provide Java bindings to the Wayland backend library.
It does so by a combination of auto-generated Java code and appropriate JNI
bindings.  The main purpose of wayland-java is to service the Android
wayland server app that I am currently developing.

## Current state of the library

The wayland-java library is useable, but I am not yet ready to make guarantees
about API stability.  There are probably a lot of bugs, but I welcome bug
reports! It is working well enough to backend the bits of my app that I have
working.

## Building wayland-java locally

Building wayland-java locally isn't too difficult assuming you have a
version of the wayland library with language bindings support.

1. Clone the dispatchers-internal branch of git://github.com/jekstrand/wayland.git
2. Build and install libwayland
3. Clone git://github.com/jekstrand/wayland-java.git
4. Run `git submodule init` and `git submodule update` (this is mainly to pull
   wayland.xml into the subtree because it is not yet distributed.)
5. Run maven

## Building wayland-java for Android

Building the library for Android requires a slight modification to the wayland
source code.  I hope to submitting patches to the wayland project soon so that
it can be built unaltered. If you're brave enough to try and build wayland-java
on Android, you need to do the following:

1. Install both the android SDK and NDK and get them configured
2. Clone git://github.com/jekstrand/wayland-java.git
3. Run `git submodule init` and `git submodule update`
4. In the protocol/src/main/native/external/wayland directory, you have to modify the wayland source
   code to remove calls to `signalfd` and `timerfd` and their respective header
   files
5. Make sure that the version of protocol/src/main/native/external/wayland is patched with this
   [patch series][1]
6. Use the Android tools to update the build paths in the `android`
   subdirectory to your version of the Android SDK
7. Run ant from inside the `android` subdirectory

[1]: http://lists.freedesktop.org/archives/wayland-devel/2013-February/007473.html

