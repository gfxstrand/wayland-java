import java.util.ArrayList;

import java.io.Writer;
import java.io.IOException;

import org.w3c.dom.Node;
import org.w3c.dom.Element;

class Event
{
    private Interface iface;
    private int id;
    private String name;
    private Description description;
    private ArrayList<Argument> args;

    public Event(Interface iface, int id, Element xmlElem)
    {
        this.iface = iface;
        this.id = id;

        name = "post" + Scanner.toUpperCamelCase(xmlElem.getAttribute("name"));
        args = new ArrayList<Argument>();

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
        writer.write("\tpublic static native void " + name + "(");
        writer.write("Resource resource");

        for (int i = 0; i < args.size(); ++i) {
            writer.write(", ");
            args.get(i).writeJavaDeclaration(writer);
        }

        writer.write(");\n");
    }

    public void writeCServerNativeMethod(Writer writer)
            throws IOException
    {
        writer.write("JNIEXPORT void JNICALL\n");
        writer.write("Java");

        String pkg = iface.scanner.getJavaPackage();
        if (pkg != null)
            writer.write("_" + pkg.replace(".", "_"));

        writer.write("_" + iface.name);
        writer.write("_" + name + "(");
        writer.write("\n\t\tJNIEnv * __env, jclass __cls, jobject __jres");
        for (Argument arg : args) {
            writer.write(",\n\t\t");
            writer.write(arg.getJNIType() + " " + arg.getName());
        }
        writer.write(")\n{\n");
        for (Argument arg : args) {
            writer.write("\t" + arg.getCType());
            writer.write(" __jni_" + arg.getName() + ";\n");
        }
        writer.write("\n");
        for (Argument arg : args) {
            writer.write("\t__jni_" + arg.getName() + " = ");
            arg.writeJNIToCConversion(writer);
            writer.write(";\n");
        }
        writer.write("\n");

        writer.write("\twl_resource_post_event(");
        writer.write("wl_jni_resource_from_java(__env, __jres), " + id);
        for (Argument arg : args) {
            writer.write(",\n\t\t\t__jni_" + arg.getName());
        }
        writer.write(");\n\n");
        for (Argument arg : args) {
            writer.write("\t");
            arg.writeJNICleanup(writer, "__jni_" + arg.getName());
            writer.write("\n");
        }
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

