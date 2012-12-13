import org.w3c.dom.Element;

import java.io.Writer;
import java.io.IOException;
import java.io.StringReader;
import java.io.BufferedReader;

class Description
{
    private String summary;
    private String fullText;

    public Description(Element xmlElem)
    {
        summary = xmlElem.getAttribute("summary");
        // Make sure it starts with a capitol letter and ends with a period.
        if (! summary.isEmpty()) {
            summary = summary.substring(0, 1).toUpperCase()
                    + summary.substring(1);
            if (! summary.endsWith("."))
                summary = summary + ".";
        }

        fullText = xmlElem.getTextContent().trim();
    }

    public void writeJavaDoc(Writer writer, String tab) throws IOException
    {
        writer.write(tab + "/**\n");
        writer.write(tab + " * " + summary + "\n");
        if (fullText != null && ! fullText.isEmpty()) {
            writer.write(tab + " *\n");

            BufferedReader reader = new BufferedReader(
                    new StringReader(fullText));

            for (String line = reader.readLine(); line != null;
                    line = reader.readLine()) {
                writer.write(tab + " * " + line.trim() + "\n");
            }
        }
        writer.write(tab + " */\n");
    }
}

