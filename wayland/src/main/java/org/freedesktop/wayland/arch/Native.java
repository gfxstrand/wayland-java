package org.freedesktop.wayland.arch;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashSet;
import java.util.Set;

public class Native
{
    private static final String IO_TEMP_DIR = System.getProperty("java.io.tmpdir");
    private static final Set<String> libsLoaded = new HashSet<String>();

    private Native()
    { }

    public static void loadLibrary(String libName)
    {
        libName = "lib" + libName + ".so";
        if (libsLoaded.contains(libName)) {
            // avoid loading native libs double.
            return;
        }

        final InputStream in = Native.class.getClassLoader().getResourceAsStream(libName);
        if (in == null) {
            throw new IllegalArgumentException("Native library not found on classloader classpath: " + libName);
        }

        final File temp = new File(new File(IO_TEMP_DIR), libName);

        try {
            final byte[] buffer = new byte[4096];
            int read = -1;
            FileOutputStream fos = new FileOutputStream(temp);
            while ((read = in.read(buffer)) != -1) {
                fos.write(buffer, 0, read);
            }
            fos.close();
            in.close();

            System.load(temp.getAbsolutePath());
            libsLoaded.add(libName);
        } catch (final FileNotFoundException e) {
            throw new Error(e);
        } catch (final IOException e) {
            throw new Error(e);
        }
    }
}

