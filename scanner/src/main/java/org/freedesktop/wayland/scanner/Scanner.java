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

import java.io.File;

import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.ParserConfigurationException;

import org.apache.tools.ant.Task;
import org.apache.tools.ant.BuildException;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.Element;

import org.xml.sax.SAXException;

public class Scanner extends Task
{
    private DocumentBuilder xmlBuilder;

    private File src;
    public void setSrc(File src)
    {
        this.src = src;
    }

    private File dest;
    public void setDest(File dest)
    {
        this.dest = dest;
    }

    private String javaPackage;
    public void setJavapackage(String javaPackage)
    {
        this.javaPackage = javaPackage;
    }
    public String getJavaPackage()
    {
        if (javaPackage == null || javaPackage.isEmpty())
            return null;
        else
            return javaPackage;
    }

    @Override
    public void execute()
    {
        if (src == null)
            throw new BuildException("No source specified for protocol scanner.");
        if (dest == null)
            throw new BuildException("No destination specified for protocol scanner.");

        try {
            DocumentBuilderFactory factory =
                    DocumentBuilderFactory.newInstance();
            xmlBuilder = factory.newDocumentBuilder();
        } catch (ParserConfigurationException e) {
            throw new BuildException(e.getMessage());
        }

        if (src.isFile()) {
            scanFile(src);
        } else if (src.isDirectory()) {
            scanDirectory(src);
        }
    }

    private void scanDirectory(File dir)
    {
        File[] files = dir.listFiles();
        for (int i = 0; i < files.length; ++i) {
            if (files[i].isDirectory()) {
                scanDirectory(files[i]);
            } else if (files[i].isFile()) {
                if (files[i].getName().toLowerCase().endsWith(".xml")) {
                    scanFile(files[i]);
                }
            }
        }
    }

    private void scanFile(File file)
    {
        log("Scanning protocol file " + file);
        Element xmlElem;

        try {
            Document xmlDoc = xmlBuilder.parse(file);
            xmlElem = xmlDoc.getDocumentElement();
        } catch (SAXException e) {
            throw new BuildException(e.getMessage());
        } catch (java.io.IOException e) {
            throw new BuildException(e.getMessage());
        }

        // Very quick sanity check
        if (! xmlElem.getTagName().toLowerCase().equals("protocol"))
            throw new BuildException("Source file \"" + file +
                    "\" is not a protocol definition file");

        Protocol protocol = new Protocol(this, xmlElem);

        protocol.writeJava(dest);
    }
}

