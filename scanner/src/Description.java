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

