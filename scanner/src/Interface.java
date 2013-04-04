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
import java.util.ArrayList;

import java.io.Writer;
import java.io.IOException;

import org.w3c.dom.Node;
import org.w3c.dom.Element;

class Interface
{
    public Protocol protocol;
    public Scanner scanner;
    public String name;
    public String wl_name;
    private int version;
    private Description description;
    private ArrayList<Enum> enums;
    private ArrayList<Message> requests;
    private ArrayList<Message> events;

    public String getName()
    {
        return name;
    }

    public static String toClassName(String name)
    {
        return name;
    }
    
    public Interface(Scanner scanner, Element xmlElem)
    {
        this.scanner = scanner;

        enums = new ArrayList<Enum>();
        requests = new ArrayList<Message>();
        events = new ArrayList<Message>();

        wl_name = xmlElem.getAttribute("name");
        name = toClassName(wl_name);
        version = Integer.parseInt(xmlElem.getAttribute("version"));

        int eventID = 0;
        int requestID = 0;
        for (Node node = xmlElem.getFirstChild(); node != null;
                node = node.getNextSibling()) {
            if (node.getNodeType() != Node.ELEMENT_NODE)
                continue;

            Element child = (Element)node;
            String tagName = child.getTagName().toLowerCase();
            if (tagName.equals("description")) {
                description = new Description(child);
            } else if (tagName.equals("enum")) {
                enums.add(new Enum(this, child));
            } else if (tagName.equals("request")) {
                requests.add(new Request(this, requestID++, child));
            } else if (tagName.equals("event")) {
                events.add(new Event(this, eventID++, child));
            }
        }
    }

    public void writeServerJava(Writer writer) throws IOException
    {
        if (scanner.getJavaPackage() != null)
            writer.write("package " + scanner.getJavaPackage() + ";\n\n");

        // TODO: Import stuff here
        writer.write("import java.lang.String;\n");
        writer.write("import org.freedesktop.wayland.Fixed;\n");
        writer.write("import org.freedesktop.wayland.Interface;\n");
        writer.write("import org.freedesktop.wayland.server.Resource;\n");
        writer.write("import org.freedesktop.wayland.server.RequestError;\n");
        writer.write("\n");
        
        if (description != null)
            description.writeJavaDoc(writer, "");
        writer.write("public final class " + toClassName(name) + "\n");
        writer.write("{\n");

        writer.write("\tpublic static final Interface WAYLAND_INTERFACE = ");
        writer.write("new Interface(\n");
        writer.write("\t\t\"" + wl_name + "\", ");
        writer.write(name + ".Requests.class, " + version + ",\n");
        writer.write("\t\tnew Interface.Message[]{\n");
        for (Message request : requests) {
            request.writeMessageInfo(writer);
        }
        writer.write("\t\t},\n");
        writer.write("\t\tnew Interface.Message[]{\n");
        for (Message event : events) {
            event.writeMessageInfo(writer);
        }
        writer.write("\t\t}\n");
        writer.write("\t);\n");

        for (Enum enm : enums) {
            writer.write("\n");
            enm.writeJavaDeclaration(writer);
        }

        writer.write("\n");
        writer.write("\tpublic interface Requests\n");
        writer.write("\t{");
        for (Message request : requests) {
            writer.write("\n");
            request.writeInterfaceMethod(writer);
        }
        writer.write("\t}\n");

        writer.write("\n");
        writer.write("\tpublic interface Events\n");
        writer.write("\t{");
        for (Message event : events) {
            writer.write("\n");
            event.writeInterfaceMethod(writer);
        }
        writer.write("\t}\n");

        writer.write("\n");
        writer.write("\tpublic static class Proxy");
        writer.write(" extends org.freedesktop.wayland.client.Proxy\n");
        writer.write("\t{\n");

        writer.write("\t\tpublic Proxy(");
        writer.write("org.freedesktop.wayland.client.Proxy factory)\n");
        writer.write("\t\t{\n");
        writer.write("\t\t\tsuper(factory, WAYLAND_INTERFACE);\n");
        writer.write("\t\t}\n");

        for (Message request : requests) {
            writer.write("\n");
            request.writePostMethod(writer);
        }
        writer.write("\t}\n");

        for (Message event : events) {
            writer.write("\n");
            event.writePostMethod(writer);
        }

        writer.write("}\n");
    }
}

