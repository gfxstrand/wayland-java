import java.util.ArrayList;

import java.io.Writer;
import java.io.IOException;

import org.w3c.dom.Node;
import org.w3c.dom.Element;

class Interface
{
    private final String SERVER_BASE_CLASS_NAME =
            "org.freedesktop.wayland.server.Resource";

    public Protocol protocol;
    public Scanner scanner;
    public String name;
    public String wl_name;
    private int version;
    private Description description;
    private ArrayList<Enum> enums;
    private ArrayList<Request> requests;
    private ArrayList<Event> events;

    public String getName()
    {
        return name;
    }

    public static String toClassName(String name)
    {
        if (name.startsWith("wl_")) {
            name = name.substring(3);
        }

        return Scanner.toUpperCamelCase(name);
    }
    
    public Interface(Scanner scanner, Element xmlElem)
    {
        this.scanner = scanner;

        enums = new ArrayList<Enum>();
        requests = new ArrayList<Request>();
        events = new ArrayList<Event>();

        wl_name = xmlElem.getAttribute("name");
        name = toClassName(wl_name);
        version = Integer.parseInt(xmlElem.getAttribute("version"));

        int eventID = 0;
        int requestID = 0;
        for (Node node = xmlElem.getFirstChild(); node != null;
                node = node.getNextSibling()) {
            if (node.getNodeType() != Node.ELEMENT_NODE)
                continue;

            Element child = (Element)node;
            String tagName = child.getTagName().toLowerCase();
            if (tagName.equals("description")) {
                description = new Description(child);
            } else if (tagName.equals("enum")) {
                enums.add(new Enum(this, child));
            } else if (tagName.equals("request")) {
                requests.add(new Request(this, requestID++, child));
            } else if (tagName.equals("event")) {
                events.add(new Event(this, eventID++, child));
            }
        }
    }

    public void writeServerJava(Writer writer) throws IOException
    {
        if (scanner.getJavaPackage() != null)
            writer.write("package " + scanner.getJavaPackage() + ";\n\n");

        // TODO: Import stuff here
        writer.write("import java.lang.String;\n");
        writer.write("import org.freedesktop.wayland.Fixed;\n");
        writer.write("import org.freedesktop.wayland.Interface;\n");
        writer.write("import org.freedesktop.wayland.server.Client;\n");
        writer.write("import org.freedesktop.wayland.server.Resource;\n");
        writer.write("import org.freedesktop.wayland.server.RequestError;\n");
        writer.write("\n");
        
        if (description != null)
            description.writeJavaDoc(writer, "");
        writer.write("public abstract class " + toClassName(name));
        writer.write(" extends " + SERVER_BASE_CLASS_NAME + "\n");
        writer.write("{\n");

        writer.write("\tprivate native void setWLInterfaces();\n");
        writer.write("\tprivate native static long getWLImplementation();\n");
        writer.write("\tprotected " + name + "(int id)\n");
        writer.write("\t{\n");
        writer.write("\t\tsuper(WAYLAND_INTERFACE, id);\n");
        writer.write("\t\tsetWLInterfaces();\n");
        writer.write("\t}\n");

        writer.write("\n");
        writer.write("\tpublic static final Interface WAYLAND_INTERFACE = ");
        writer.write("new Interface(\n");
        writer.write("\t\t\"" + wl_name + "\", ");
        writer.write(name + ".class, " + version + ",\n");
        writer.write("\t\tnew Interface.Message[]{\n");
        for (Request request : requests) {
            request.writeJavaWaylandMessageInfo(writer);
        }
        writer.write("\t\t},\n");
        writer.write("\t\tnew Interface.Message[]{\n");
        for (Event event : events) {
            event.writeJavaWaylandMessageInfo(writer);
        }
        writer.write("\t\t},\n");
        writer.write("\t\tgetWLImplementation()\n");
        writer.write("\t);\n");

        for (Enum enm : enums) {
            writer.write("\n");
            enm.writeJavaDeclaration(writer);
        }

        for (Request request : requests) {
            writer.write("\n");
            request.writeJavaServerMethod(writer);
        }

        for (Event event : events) {
            writer.write("\n");
            event.writeJavaServerMethod(writer);
        }

        writer.write("}\n");
    }

    public void writeServerC(Writer writer) throws IOException
    {
        for (Request request : requests) {
            writer.write("\n");
            request.writeCServerWrapper(writer);
        }

        writer.write("\nstatic const __void_function ");
        writer.write(name + "_wl_implementation[] = {\n");
        for (Request request : requests) {
            writer.write("\t(__void_function)&");
            writer.write(request.getCServerWrapperName() + ",\n");
        }
        writer.write("};\n");

        writer.write("\nJNIEXPORT void JNICALL\n");
        writer.write("Java");
        String pkg = scanner.getJavaPackage();
        if (pkg != null)
            writer.write("_" + pkg.replace(".", "_"));
        writer.write("_" + name + "_setWLInterfaces(");
        writer.write("\n\t\tJNIEnv * __env, jobject __jobj)\n");
        writer.write("{\n");
        writer.write("\tstruct wl_resource * resource;\n");
        writer.write("\tresource = wl_jni_resource_from_java(__env, __jobj);\n");
        writer.write("\tresource->object.interface = ");
        writer.write("&" + wl_name + "_interface;\n");
        writer.write("\tresource->object.implementation = ");
        writer.write(name + "_wl_implementation;\n");
        writer.write("}\n");

        writer.write("\nJNIEXPORT long JNICALL\n");
        writer.write("Java");
        if (pkg != null)
            writer.write("_" + pkg.replace(".", "_"));
        writer.write("_" + name + "_getWLImplementation(");
        writer.write("\n\t\tJNIEnv * __env, jclass __cls)\n");
        writer.write("{\n");
        writer.write("\treturn (long)");
        writer.write(name + "_wl_implementation;\n");
        writer.write("}\n");

        for (Event event : events) {
            writer.write("\n");
            event.writeCServerNativeMethod(writer);
        }
    }
}

