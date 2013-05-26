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

import org.apache.tools.ant.BuildException;

import org.w3c.dom.Node;
import org.w3c.dom.Element;

class Enum
{
    public class Entry
    {
        private String name;
        private String summary;
        private String value;

        private Entry(Element xmlElem)
        {
            name = xmlElem.getAttribute("name");
            summary = xmlElem.getAttribute("summary");
            value = xmlElem.getAttribute("value");
        }
    }

    private Interface iFace;
    private String name;
    private Description description;
    private ArrayList<Entry> entries;

    public Enum(Interface iFace, Element xmlElem)
    {
        this.iFace = iFace;
        name = xmlElem.getAttribute("name");
        entries = new ArrayList<Entry>();

        for (Node node = xmlElem.getFirstChild(); node != null;
                node = node.getNextSibling()) {
            if (node.getNodeType() != Node.ELEMENT_NODE)
                continue;

            Element child = (Element)node;
            String tagName = child.getTagName().toLowerCase();
            if (tagName.equals("description")) {
                description = new Description(child);
            } else if (tagName.equals("entry")) {
                entries.add(new Entry(child));
            }
        }
    }

    public void writeJavaDeclaration(Writer writer) throws IOException
    {
        if (description != null)
            description.writeJavaDoc(writer, "\t");

        for (Entry entry : entries) {
            if (! entry.summary.isEmpty())
                writer.write("\t/** " + entry.summary + " */\n");
            writer.write("\tpublic static final int " + name.toUpperCase() + "_");
            writer.write(entry.name.toUpperCase());
            writer.write(" = " + entry.value + ";\n");
        }
        
        if (name.toLowerCase().equals("error")) {
            for (Entry entry : entries) {
                String errorName = StringUtil.toUpperCamelCase(entry.name)
                        + "Error";

                writer.write("\n");
                writer.write("\tprotected class " + errorName);
                writer.write(" extends RequestError\n");
                writer.write("\t{\n");
                writer.write("\t\t" + errorName + "(String message)\n");
                writer.write("\t\t{\n");
                writer.write("\t\t\tsuper(message, ");
                writer.write(name.toUpperCase() + "_");
                writer.write(entry.name.toUpperCase() + ");\n");
                writer.write("\t\t}\n");
                writer.write("\t}\n");
            }
        }
    }
}

