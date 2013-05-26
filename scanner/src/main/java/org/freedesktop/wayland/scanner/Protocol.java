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
}

