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

        name = Scanner.toLowerCamelCase(xmlElem.getAttribute("name"));

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
        if (description != null)
            description.writeJavaDoc(writer, "\t");
        writer.write("\tpublic abstract void " + name + "(");
        writer.write("Client client");

        for (Argument arg : args) {
            writer.write(", ");
            arg.writeJavaDeclaration(writer);
        }

        writer.write(");\n");
    }

    public void writeCServerWrapper(Writer writer)
            throws IOException
    {
        writer.write("static void\n");

        String pkg = iface.scanner.getJavaPackage();
        if (pkg != null)
            writer.write(pkg.replace(".", "_") + "_");

        writer.write(iface.name);
        writer.write("_" + name + "(");
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
}

