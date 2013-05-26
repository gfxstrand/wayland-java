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

import org.apache.tools.ant.BuildException;

import java.io.Writer;
import java.io.IOException;

import org.w3c.dom.Element;

class Argument 
{
    public enum Type {
        INT,
        UINT,
        FIXED,
        STRING,
        OBJECT,
        NEW_ID,
        ARRAY,
        FD
    }

    final String name;
    final Type type;
    final String ifaceName;
    final boolean allowNull;
    private Scanner scanner;

    public Argument(Element xmlElem, Scanner scanner)
    {
        this.scanner = scanner;
        String tmpname = xmlElem.getAttribute("name");

        // Protect some java keywords
        if (tmpname.equals("class")) {
            name = "clazz";
        } else if (tmpname.equals("interface")) {
            name = "iface";
        } else if (tmpname.equals("public")) {
            name = "publik";
        } else if (tmpname.equals("abstract")) {
            name = "abstrct";
        } else if (tmpname.equals("native")) {
            name = "nativ";
        } else {
            name = tmpname;
        }

        String typeStr = xmlElem.getAttribute("type");
        if (typeStr.equals("int")) {
            type = Type.INT;
        } else if (typeStr.equals("uint")) {
            type = Type.UINT;
        } else if (typeStr.equals("fixed")) {
            type = Type.FIXED;
        } else if (typeStr.equals("string")) {
            type = Type.STRING;
        } else if (typeStr.equals("object")) {
            type = Type.OBJECT;
        } else if (typeStr.equals("new_id")) {
            type = Type.NEW_ID;
        } else if (typeStr.equals("array")) {
            type = Type.ARRAY;
        } else if (typeStr.equals("fd")) {
            type = Type.FD;
        } else {
            throw new BuildException("Invalid type \"" + typeStr + "\"");
        }

        tmpname = xmlElem.getAttribute("interface");
        if (tmpname.isEmpty())
            ifaceName = null;
        else
            ifaceName = Interface.toClassName(tmpname);

        String nullstr = xmlElem.getAttribute("allow-null").toLowerCase();
        allowNull = nullstr.equals("true") || nullstr.equals("yes")
                || nullstr.equals("1");
    }

    public String getCType()
    {
        switch (type) {
        case INT:
            return "int32_t";
        case UINT:
            return "uint32_t";
        case FIXED:
            return "wl_fixed_t";
        case STRING:
            return "char *";
        case OBJECT:
            return "struct wl_resource *";
        case NEW_ID:
            return "uint32_t";
        case ARRAY:
            return "void *";
        case FD:
            return "int32_t";
        default:
            return null;
        }
    }

    public String getJavaType(String objectType)
    {
        switch (type) {
        case INT:
            return "int";
        case UINT:
            return "int";
        case FIXED:
            return "Fixed";
        case STRING:
            return "String";
        case OBJECT:
            return objectType;
        case NEW_ID:
            return "int";
        case ARRAY:
            return "java.nio.ByteBuffer";
        case FD:
            return "int";
        default:
            return null;
        }
    }

    public String getWLPrototype()
    {
        switch (type) {
        case INT:
            return "i";
        case UINT:
            return "u";
        case FIXED:
            return "f";
        case STRING:
            return allowNull ? "?s" : "s";
        case OBJECT:
            return allowNull ? "?o" : "o";
        case NEW_ID:
            return "n";
        case ARRAY:
            return "a";
        case FD:
            return "h";
        default:
            return null;
        }
    }

    public String getJavaPrototype()
    {
        switch (type) {
        case INT:
            return "I";
        case UINT:
            return "I";
        case FIXED:
            return "Lorg/freedesktop/wayland/Fixed;";
        case STRING:
            return "Ljava/lang/String;";
        case OBJECT:
            return "Lorg/freedesktop/wayland/server/Resource;";
        case NEW_ID:
            return "I";
        case ARRAY:
            return "[B";
        case FD:
            return "I";
        default:
            return null;
        }
    }
}

