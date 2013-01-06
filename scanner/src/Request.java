import java.util.ArrayList;

import java.io.Writer;
import java.io.IOException;

import org.w3c.dom.Node;
import org.w3c.dom.Element;

class Request
{
    private Interface iface;
    private int id;
    private String name;
    private Description description;
    private ArrayList<Argument> args;

    public Request(Interface iface, int id, Element xmlElem)
    {
        this.iface = iface;
        this.id = id;
        args = new ArrayList<Argument>();

        name = StringUtil.toLowerCamelCase(xmlElem.getAttribute("name"));

        for (Node node = xmlElem.getFirstChild(); node != null;
                node = node.getNextSibling()) {
            if (node.getNodeType() != Node.ELEMENT_NODE)
                continue;

            Element child = (Element)node;
            String tagName = child.getTagName().toLowerCase();
            if (tagName.equals("description")) {
                description = new Description(child);
            } else if (tagName.equals("arg")) {
                args.add(new Argument(child));
            }
        }
    }

    public void writeJavaServerMethod(Writer writer) throws IOException
    {
        // We need not create headers for an empyt destroy event. One already
        // exists in the parent class.
        if (name.equals("destroy") && args.isEmpty()) {
            return;
        }

        if (description != null)
            description.writeJavaDoc(writer, "\t\t");
        writer.write("\t\tpublic abstract void " + name + "(");
        writer.write("Client client");

        for (Argument arg : args) {
            writer.write(", ");
            arg.writeJavaDeclaration(writer);
        }

        writer.write(");\n");
    }

    public String getCServerWrapperName()
    {
        String pkg = iface.scanner.getJavaPackage();
        if (pkg == null)
            pkg = "";
        else
            pkg = pkg.replace(".", "_") + "_";
        return pkg + iface.name + "_" + name;
    }

    public void writeCServerWrapper(Writer writer)
            throws IOException
    {
        writer.write("static void\n");

        writer.write(getCServerWrapperName() + "(");
        writer.write("\n\t\tstruct wl_client * __client");
        writer.write(",\n\t\tstruct wl_resource * __resource");
        for (Argument arg : args) {
            writer.write(",\n\t\t");
            arg.writeCDeclaration(writer);
        }
        writer.write(")\n{\n");
        writer.write("\twl_jni_resource_call_request(__client, __resource");
        writer.write(",\n\t\t\t\"" + name + "\"");
        writer.write(",\n\t\t\t\"");
        for (Argument arg : args) {
            writer.write(arg.getWLPrototype());
        }
        writer.write("\"");
        writer.write(",\n\t\t\t\"(");
        for (Argument arg : args) {
            writer.write(arg.getJavaPrototype());
        }
        writer.write(")V\"");
        for (Argument arg : args) {
            writer.write(",\n\t\t\t" + arg.getName());
        }
        writer.write(");\n");
        writer.write("}\n");
    }

    public void writeJavaWaylandMessageInfo(Writer writer)
            throws IOException
    {
        writer.write("\t\t\tnew Interface.Message(\"" + name + "\", ");
        writer.write("\"");
        for (Argument arg : args) {
            writer.write(arg.getWLPrototype());
        }
        writer.write("\", new Interface[]{\n");
        for (Argument arg : args) {
            if (arg.type == Argument.Type.OBJECT && arg.ifaceName != null) {
                writer.write("\t\t\t\t");
                writer.write("org.freedesktop.wayland.server.protocol.");
                writer.write(arg.ifaceName + ".WAYLAND_INTERFACE");
                writer.write(",\n");
            } else {
                writer.write("\t\t\t\t");
                writer.write("null,\n");
            }
        }
        writer.write("\t\t\t}),\n");
    }
}

