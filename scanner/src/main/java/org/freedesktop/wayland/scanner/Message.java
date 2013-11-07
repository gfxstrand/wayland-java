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

import java.util.ArrayList;

import java.io.Writer;
import java.io.IOException;

import org.w3c.dom.Node;
import org.w3c.dom.Element;

abstract class Message
{
    protected Interface iface;
    protected int id;
    public final String name;
    public final int since;
    protected Description description;
    protected ArrayList<Argument> args;

    public Message(Interface iface, int id, Element xmlElem)
    {
        this.iface = iface;
        this.id = id;
        args = new ArrayList<Argument>();

        name = xmlElem.getAttribute("name");

        String sinceVal = xmlElem.getAttribute("since");
        if (sinceVal == null || sinceVal.isEmpty())
            since = 1;
        else
            since = Integer.parseInt(sinceVal);

        for (Node node = xmlElem.getFirstChild(); node != null;
                node = node.getNextSibling()) {
            if (node.getNodeType() != Node.ELEMENT_NODE)
                continue;

            Element child = (Element)node;
            String tagName = child.getTagName().toLowerCase();
            if (tagName.equals("description")) {
                description = new Description(child);
            } else if (tagName.equals("arg")) {
                args.add(new Argument(child, iface.scanner));
            }
        }
    }

    public void writeMessageInfo(Writer writer) throws IOException
    {
        writer.write("\t\t\tnew Interface.Message(\"");
        writer.write(StringUtil.toLowerCamelCase(name) + "\", ");
        writer.write("\"");
        if (since != 1)
            writer.write(String.valueOf(since));
        for (Argument arg : args) {
            if (arg.type == Argument.Type.NEW_ID && arg.ifaceName == null)
                writer.write("su");
            writer.write(arg.getWLPrototype());
        }
        writer.write("\", new Interface[]{\n");
        for (Argument arg : args) {
            if (arg.type == Argument.Type.OBJECT && arg.ifaceName != null) {
                writer.write("\t\t\t\t");
                writer.write(arg.ifaceName + ".WAYLAND_INTERFACE");
                writer.write(",\n");
            } else {
                writer.write("\t\t\t\t");
                writer.write("null,\n");
            }
        }
        writer.write("\t\t\t}),\n");
    }

    public abstract void writeInterfaceMethod(Writer writer) throws IOException;
    public abstract void writePostMethod(Writer writer) throws IOException;
}

