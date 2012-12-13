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

        name = toClassName(xmlElem.getAttribute("name"));

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
                enums.add(new Enum(child));
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
        writer.write("import org.freedesktop.wayland.server.Client;\n");
        writer.write("import org.freedesktop.wayland.server.Resource;\n");
        writer.write("\n");
        
        if (description != null)
            description.writeJavaDoc(writer, "");
        writer.write("public abstract class " + name);
        writer.write(" extends " + SERVER_BASE_CLASS_NAME + " \n{");

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

        for (Event event : events) {
            writer.write("\n");
            event.writeCServerNativeMethod(writer);
        }
    }
}

