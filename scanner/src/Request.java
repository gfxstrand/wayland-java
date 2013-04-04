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
import java.util.Iterator;
import java.util.ListIterator;

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
            description.writeJavaDoc(writer, "\t\t");

        String new_proxy_type = null;
        boolean create_new_proxy = false;
        for (Argument arg : args) {
            if (arg.type == Argument.Type.NEW_ID) {
                create_new_proxy = true;
                new_proxy_type = arg.ifaceName;
                break;
            }
        }

        writer.write("\t\tpublic ");
        if (create_new_proxy) {
            if (new_proxy_type != null)
                writer.write(new_proxy_type + ".Proxy");
            else
                writer.write("org.freedesktop.wayland.client.Proxy");
        } else {
            writer.write("void");
        }
        writer.write(" " + StringUtil.toLowerCamelCase(name) + "(");

        boolean needs_comma = false;
        for (Iterator<Argument> iter = args.iterator(); iter.hasNext();) {
            final Argument arg = iter.next();
            
            if (arg.type == Argument.Type.NEW_ID && new_proxy_type != null)
                continue;

            if (needs_comma)
                writer.write(", ");

            if (arg.type == Argument.Type.NEW_ID) {
                writer.write("Interface iface, int version");
                needs_comma = true;
                continue;
            }

            writer.write(arg.getJavaType("org.freedesktop.wayland.client.Proxy"));
            writer.write(" " + arg.name);
            needs_comma = true;
        }

        writer.write(")\n");
        writer.write("\t\t{\n");

        if (create_new_proxy) {
            if (new_proxy_type != null) {
                writer.write("\t\t\tfinal " + new_proxy_type + ".Proxy");
                writer.write(" _new_proxy = new ");
                writer.write(new_proxy_type + ".Proxy(this);\n");
            } else {
                writer.write("\t\t\tfinal org.freedesktop.wayland.client");
                writer.write(".Proxy _new_proxy;\n");
                writer.write("\t\t\ttry {\n");
                writer.write("\t\t\t\t_new_proxy = ");
                writer.write("org.freedesktop.wayland.client.Proxy.create");
                writer.write("(this, iface);\n");
                writer.write("\t\t\t} catch(Exception e) {\n");
                writer.write("\t\t\t\tthrow new RuntimeException(e);\n");
                writer.write("\t\t\t}\n");
            }
        }

        writer.write("\t\t\tmarshal(" + id);
        for (Argument arg : args) {
            if (arg.type == Argument.Type.NEW_ID) {
                if (new_proxy_type != null) {
                    writer.write(", _new_proxy");
                } else {
                    writer.write(", iface.getName(), version, _new_proxy");
                }
            } else {
                writer.write(", " + arg.name);
            }
        }
        writer.write(");\n");

        if (create_new_proxy)
            writer.write("\t\t\treturn _new_proxy;\n");

        writer.write("\t\t}\n");
    }
}

