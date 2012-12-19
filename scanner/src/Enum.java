import java.util.ArrayList;

import java.io.Writer;
import java.io.IOException;

import org.apache.tools.ant.BuildException;

import org.w3c.dom.Node;
import org.w3c.dom.Element;

class Enum
{
    public class Entry
    {
        private String name;
        private String summary;
        private String value;

        private Entry(Element xmlElem)
        {
            name = xmlElem.getAttribute("name");
            summary = xmlElem.getAttribute("summary");
            value = xmlElem.getAttribute("value");
        }
    }

    private Interface iFace;
    private String name;
    private Description description;
    private ArrayList<Entry> entries;

    public Enum(Interface iFace, Element xmlElem)
    {
        this.iFace = iFace;
        name = xmlElem.getAttribute("name");
        entries = new ArrayList<Entry>();

        for (Node node = xmlElem.getFirstChild(); node != null;
                node = node.getNextSibling()) {
            if (node.getNodeType() != Node.ELEMENT_NODE)
                continue;

            Element child = (Element)node;
            String tagName = child.getTagName().toLowerCase();
            if (tagName.equals("description")) {
                description = new Description(child);
            } else if (tagName.equals("entry")) {
                entries.add(new Entry(child));
            }
        }
    }

    public void writeJavaDeclaration(Writer writer) throws IOException
    {
        if (description != null)
            description.writeJavaDoc(writer, "\t");

        for (Entry entry : entries) {
            if (! entry.summary.isEmpty())
                writer.write("\t/** " + entry.summary + " */\n");
            writer.write("\tpublic final int " + name.toUpperCase() + "_");
            writer.write(entry.name.toUpperCase());
            writer.write(" = " + entry.value + ";\n");
        }
        
        if (name.toLowerCase().equals("error")) {
            for (Entry entry : entries) {
                String errorName = Scanner.toUpperCamelCase(entry.name)
                        + "Error";

                writer.write("\n");
                writer.write("\tprotected class " + errorName);
                writer.write(" extends RequestError\n");
                writer.write("\t{\n");
                writer.write("\t\t" + errorName + "(String message)\n");
                writer.write("\t\t{\n");
                writer.write("\t\t\tsuper(message, ");
                writer.write(iFace.getName() + ".this, ");
                writer.write(name.toUpperCase() + "_");
                writer.write(entry.name.toUpperCase() + ");\n");
                writer.write("\t\t}\n");
                writer.write("\t}\n");
            }
        }
    }
}

