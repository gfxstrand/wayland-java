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
            name = xmlElem.getAttribute("name").toUpperCase();
            summary = xmlElem.getAttribute("summary");
            value = xmlElem.getAttribute("value");
            /*
            try {
                String valueStr = xmlElem.getAttribute("value");
                if (valueStr.startsWith("0x")) {
                    value = Integer.parseInt(valueStr.substring(2), 16);
                } else {
                    value = Integer.parseInt(xmlElem.getAttribute("value"));
                }
            } catch (NumberFormatException e) {
                throw new BuildException("Invalid enumeration value: \"" +
                        xmlElem.getAttribute("value") + "\"");
            }
            */
        }

        public void writeEntry(Writer writer, String tab) throws IOException
        {
            if (summary != null && ! summary.isEmpty())
                writer.write(tab + "/** " + summary + " */\n");
            writer.write(tab + "ENUM_" + name + " (" + value + ")");
        }
    }

    private String name;
    private Description description;
    private ArrayList<Entry> entries;

    public Enum(Element xmlElem)
    {
        name = Scanner.toUpperCamelCase(xmlElem.getAttribute("name"));
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

        writer.write("\tenum " + name + " {\n");
        if (! entries.isEmpty())
            entries.get(0).writeEntry(writer, "\t\t");
        for (int i = 1; i < entries.size(); ++i) {
            writer.write(",\n");
            entries.get(i).writeEntry(writer, "\t\t");
        }
        writer.write(";\n\n");
        writer.write("\t\tpublic final int value;\n\n");
        writer.write("\t\t" + name + "(int value)\n");
        writer.write("\t\t{\n");
        writer.write("\t\t\tthis.value = value;\n");
        writer.write("\t\t}\n");
        writer.write("\t}\n");
    }
}

