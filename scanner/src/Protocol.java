import java.util.ArrayList;

import java.io.File;
import java.io.Writer;
import java.io.FileWriter;
import java.io.IOException;
import java.io.StringReader;
import java.io.BufferedReader;

import org.apache.tools.ant.BuildException;

import org.w3c.dom.Node;
import org.w3c.dom.Element;

public class Protocol
{
    private Scanner scanner;
    private String name;
    private String copyright;
    private ArrayList<Interface> interfaces;

    public Protocol(Scanner scanner, Element xmlElem)
    {
        this.scanner = scanner;
        interfaces = new ArrayList<Interface>();

        name = xmlElem.getAttribute("name");

        for (Node node = xmlElem.getFirstChild(); node != null;
                node = node.getNextSibling()) {
            if (node.getNodeType() != Node.ELEMENT_NODE)
                continue;

            Element child = (Element)node;
            String tagName = child.getTagName().toLowerCase();
            if (tagName.equals("copyright")) {
                copyright = child.getTextContent().trim();
            } else if (tagName.equals("interface")) {
                interfaces.add(new Interface(scanner, child));
            }
        }
    }

    public void writeCopyright(Writer writer) throws IOException
    {
        writer.write("/*\n");

        BufferedReader reader = new BufferedReader(
                new StringReader(copyright));

        for (String line = reader.readLine(); line != null;
                line = reader.readLine()) {
            writer.write(" * " + line.trim() + "\n");
        }

        writer.write(" */\n");
    }

    public void writeServerJava(File dest)
    {
        if (scanner.getJavaPackage() != null) {
            String pkg = scanner.getJavaPackage();
            String[] parts = pkg.split("\\.");
            for (int i = 0; i < parts.length; ++i) {
                dest = new File(dest, parts[i]);
                if (! dest.exists())
                    dest.mkdir();
            }
        }

        for (Interface iFace : interfaces) {
            File destFile = new File(dest, iFace.getName() + ".java");

            try {
                scanner.log("Generating " + destFile);
                Writer writer = new FileWriter(destFile);
                writeCopyright(writer);
                iFace.writeServerJava(writer);
                writer.close();
            } catch (IOException e) {
                throw new BuildException(e.getMessage());
            }
        }
    }

    public void writeServerC(File dest)
    {
        try {
            scanner.log("Generating " + dest);
            Writer writer = new FileWriter(dest);

            writeCopyright(writer);
            writer.write("\n");
            writer.write("#include <stdlib.h>\n");
            writer.write("#include \"server-jni.h\"\n");
            writer.write("#include \"wayland-server.h\"\n\n");
            writer.write("typedef void (* __void_function)(void);\n");

            for (Interface iFace : interfaces)
                iFace.writeServerC(writer);

            writer.close();
        } catch (IOException e) {
            throw new BuildException(e.getMessage());
        }
    }
}

