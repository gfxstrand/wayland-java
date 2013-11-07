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
package org.freedesktop.wayland.scanner;

import java.util.List;
import java.util.Iterator;
import java.util.ArrayList;

import java.io.Writer;
import java.io.IOException;

import org.w3c.dom.Node;
import org.w3c.dom.Element;

class Interface
{
    public final Scanner scanner;
    public final String name;
    public final String wl_name;
    private final int version;
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

    private String versionedIfaceName(String ifaceBaseName, int version)
    {
        if (version == 1)
            return ifaceBaseName;
        else
            return ifaceBaseName + version;
    }

    private void writeInterfaceClassList(Writer writer,
            List<Message> messages, String ifaceBaseName) throws IOException
    {
        Iterator<Message> iter = messages.iterator();
        Message msg = iter.hasNext() ? iter.next() : null;

        int lastVersion = 1;
        for (int i = 1; i <= version; ++i) {
            if (msg != null && msg.since <= i) {
                lastVersion = msg.since;

                while (msg != null && msg.since <= i)
                    msg = iter.hasNext() ? iter.next() : null;
            }

            writer.write("\t\t\t");
            writer.write(versionedIfaceName(ifaceBaseName, lastVersion));
            writer.write(".class, \n");
        }
    }

    private void writeVersionedInterfaces(Writer writer,
            List<Message> messages, String ifaceBaseName) throws IOException
    {
        int version = 1;
        writer.write("\n");
        writer.write("\tpublic interface ");
        writer.write(versionedIfaceName(ifaceBaseName, 1));
        writer.write("\n\t{");
        for (Message msg : messages) {
            if (msg.since < version)
                throw new RuntimeException(ifaceBaseName +
                        " version order mismatch for "
                        + toClassName(name) + "." + msg.name);

            if (msg.since > version) {
                writer.write("\t}\n\n");
                writer.write("\tpublic interface ");
                writer.write(versionedIfaceName(ifaceBaseName, msg.since));
                writer.write(" extends ");
                writer.write(versionedIfaceName(ifaceBaseName, version));
                writer.write("\n\t{");

                version = msg.since;
            }

            writer.write("\n");
            msg.writeInterfaceMethod(writer);
        }
        writer.write("\t}\n");
    }

    public void writeJava(Writer writer) throws IOException
    {
        if (scanner.getJavaPackage() != null)
            writer.write("package " + scanner.getJavaPackage() + ";\n\n");

        // TODO: Import stuff here
        writer.write("import java.lang.String;\n");
        writer.write("import org.freedesktop.wayland.Fixed;\n");
        writer.write("import org.freedesktop.wayland.Interface;\n");
        writer.write("import org.freedesktop.wayland.server.Client;\n");
        writer.write("import org.freedesktop.wayland.server.RequestError;\n");
        writer.write("\n");
        
        if (description != null)
            description.writeJavaDoc(writer, "");
        writer.write("public final class " + toClassName(name) + "\n");
        writer.write("{\n");

        writer.write("\tpublic static final Interface WAYLAND_INTERFACE = ");
        writer.write("new Interface(\n");
        writer.write("\t\t\"" + wl_name + "\", ");
        writer.write(version + ",\n");
        writer.write("\t\tnew Interface.Message[]{\n");
        for (Message request : requests) {
            request.writeMessageInfo(writer);
        }
        writer.write("\t\t},\n");
        writer.write("\t\tnew Class<?>[]{\n");
        writeInterfaceClassList(writer, requests, "Requests");
        writer.write("\t\t},\n");

        writer.write("\t\tnew Interface.Message[]{\n");
        for (Message event : events) {
            event.writeMessageInfo(writer);
        }
        writer.write("\t\t},\n");
        writer.write("\t\tnew Class<?>[]{\n");
        writeInterfaceClassList(writer, events, "Events");
        writer.write("\t\t},\n");
        writer.write("\t\tProxy.class,\n");
        writer.write("\t\tResource.class\n");
        writer.write("\t);\n");

        for (Enum enm : enums) {
            writer.write("\n");
            enm.writeJavaDeclaration(writer);
        }

        writeVersionedInterfaces(writer, requests, "Requests");

        writeVersionedInterfaces(writer, events, "Events");

        writer.write("\n");
        writer.write("\tpublic static class Proxy");
        writer.write(" extends org.freedesktop.wayland.client.Proxy\n");
        writer.write("\t{\n");

        if (name.equals("wl_display"))
            // wl_display is special.  We need to create it throug Display
            writer.write("\t\tprotected Proxy(");
        else
            writer.write("\t\tpublic Proxy(");

        writer.write("org.freedesktop.wayland.client.Proxy factory)\n");
        writer.write("\t\t{\n");
        writer.write("\t\t\tsuper(factory, WAYLAND_INTERFACE);\n");
        writer.write("\t\t}\n");

        writer.write("\n");
        writer.write("\t\tpublic void addListener(Events listener, Object data)\n");
        writer.write("\t\t{\n");
        writer.write("\t\t\tsuper.addListener(listener, data);\n");
        writer.write("\t\t}\n");

        for (Message request : requests) {
            writer.write("\n");
            request.writePostMethod(writer);
        }
        writer.write("\t}\n");

        writer.write("\n");
        writer.write("\tpublic static class Resource");
        writer.write(" extends org.freedesktop.wayland.server.Resource\n");
        writer.write("\t{\n");

        writer.write("\t\tpublic Resource(");
        writer.write("Client client, int version, int id)\n");
        writer.write("\t\t{\n");
        writer.write("\t\t\tsuper(client, WAYLAND_INTERFACE, version, id);\n");
        writer.write("\t\t}\n");

        writer.write("\t\tpublic Resource(");
        writer.write("Client client, int version)\n");
        writer.write("\t\t{\n");
        writer.write("\t\t\tsuper(client, WAYLAND_INTERFACE, version);\n");
        writer.write("\t\t}\n");

        for (Message event : events) {
            writer.write("\n");
            event.writePostMethod(writer);
        }

        writer.write("\t}\n");

        writer.write("}\n");
    }
}

