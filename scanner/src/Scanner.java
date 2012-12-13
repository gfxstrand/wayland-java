import java.io.File;

import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.ParserConfigurationException;

import org.apache.tools.ant.Task;
import org.apache.tools.ant.BuildException;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.Element;

import org.xml.sax.SAXException;

public class Scanner extends Task
{
    private DocumentBuilder xmlBuilder;

    private File src;
    public void setSrc(File src)
    {
        this.src = src;
    }

    private File dest;
    public void setDest(File dest)
    {
        this.dest = dest;
    }

    public enum CodeType {
        java,
        c
    }
    private CodeType type;
    public void setType(CodeType type)
    {
        this.type = type;
    }

    public enum Component {
        server,
        client
    }
    private Component component;
    public void setComponent(Component comp)
    {
        this.component = comp;
    }

    private String javaPackage;
    public void setJavapackage(String javaPackage)
    {
        this.javaPackage = javaPackage;
    }
    public String getJavaPackage()
    {
        if (javaPackage == null || javaPackage.isEmpty())
            return null;
        else
            return javaPackage;
    }

    public static String toUpperCamelCase(String string)
    {
        String[] words = string.split("_");
        string = "";
        for (int i = 0; i < words.length; ++i) {
            String firstChar = words[i].substring(0, 1);
            string = string + firstChar.toUpperCase() + words[i].substring(1);
        }
        return string; 
    }

    public static String toLowerCamelCase(String string)
    {
        String[] words = string.split("_");
        string = words[0];
        for (int i = 1; i < words.length; ++i) {
            String firstChar = words[i].substring(0, 1);
            string = string + firstChar.toUpperCase() + words[i].substring(1);
        }
        return string; 
    }

    @Override
    public void execute()
    {
        if (src == null)
            throw new BuildException("No source specified for protocol scanner.");
        if (dest == null)
            throw new BuildException("No destination specified for protocol scanner.");

        try {
            DocumentBuilderFactory factory =
                    DocumentBuilderFactory.newInstance();
            xmlBuilder = factory.newDocumentBuilder();
        } catch (ParserConfigurationException e) {
            throw new BuildException(e.getMessage());
        }

        if (src.isFile()) {
            scanFile(src);
        } else if (src.isDirectory()) {
            scanDirectory(src);
        }
    }

    private void scanDirectory(File dir)
    {
        File[] files = dir.listFiles();
        for (int i = 0; i < files.length; ++i) {
            if (files[i].isDirectory()) {
                scanDirectory(files[i]);
            } else if (files[i].isFile()) {
                if (files[i].getName().toLowerCase().endsWith(".xml")) {
                    scanFile(files[i]);
                }
            }
        }
    }

    private void scanFile(File file)
    {
        log("Scanning protocol file " + file);
        Element xmlElem;

        try {
            Document xmlDoc = xmlBuilder.parse(file);
            xmlElem = xmlDoc.getDocumentElement();
        } catch (SAXException e) {
            throw new BuildException(e.getMessage());
        } catch (java.io.IOException e) {
            throw new BuildException(e.getMessage());
        }

        // Very quick sanity check
        if (! xmlElem.getTagName().toLowerCase().equals("protocol"))
            throw new BuildException("Source file \"" + file +
                    "\" is not a protocol definition file");

        Protocol protocol = new Protocol(this, xmlElem);

        if (component == Component.server && type == CodeType.java) {
            protocol.writeServerJava(dest);
        } else if (component == Component.client && type == CodeType.java) {
        } else if (component == Component.server && type == CodeType.c) {
            protocol.writeServerC(dest);
        } else if (component == Component.client && type == CodeType.c) {
        }
    }
}

