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

class Request extends Message
{
    public Request(Interface iface, int id, Element xmlElem)
    {
        super(iface, id, xmlElem);
    }

    @Override
    public void writeInterfaceMethod(Writer writer) throws IOException
    {
        if (description != null)
            description.writeJavaDoc(writer, "\t\t");
        writer.write("\t\tpublic abstract void ");
        writer.write(StringUtil.toLowerCamelCase(name));
        writer.write("(Resource resource");

        for (Argument arg : args) {
            writer.write(", " + arg.getJavaType("Resource") + " " + arg.name);
        }

        writer.write(");\n");
    }

    @Override
    public void writePostMethod(Writer writer) throws IOException
    {
        if (description != null)
            description.writeJavaDoc(writer, "\t");
        writer.write("\tpublic static void ");
        writer.write("post" + StringUtil.toUpperCamelCase(name) + "(");
        writer.write("Proxy proxy");

        for (Argument arg : args) {
            writer.write(", " + arg.getJavaType("Proxy") + " " + arg.name);
        }

        writer.write(")\n");
        writer.write("\t{\n");
        writer.write("\t\tproxy.marshal(" + id);
        for (Argument arg : args)
            writer.write(", " + arg.name);
        writer.write(");\n");
        writer.write("\t}\n");
    }
}

