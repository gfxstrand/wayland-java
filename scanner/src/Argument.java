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
import org.apache.tools.ant.BuildException;

import java.io.Writer;
import java.io.IOException;

import org.w3c.dom.Element;

class Argument 
{
    public enum Type {
        INT,
        UINT,
        FIXED,
        STRING,
        OBJECT,
        NEW_ID,
        ARRAY,
        FD
    }

    public String name;
    public Type type;
    public String ifaceName;
    public boolean allowNull;
    private Scanner scanner;

    public Argument(Element xmlElem, Scanner scanner)
    {
        this.scanner = scanner;
        name = xmlElem.getAttribute("name");

        // Protect some java keywords
        if (name.equals("class")) {
            name = "clazz";
        } else if (name.equals("interface")) {
            name = "iface";
        } else if (name.equals("public")) {
            name = "publik";
        } else if (name.equals("abstract")) {
            name = "abstrct";
        } else if (name.equals("native")) {
            name = "nativ";
        }

        String typeStr = xmlElem.getAttribute("type");
        if (typeStr.equals("int")) {
            type = Type.INT;
        } else if (typeStr.equals("uint")) {
            type = Type.UINT;
        } else if (typeStr.equals("fixed")) {
            type = Type.FIXED;
        } else if (typeStr.equals("string")) {
            type = Type.STRING;
        } else if (typeStr.equals("object")) {
            type = Type.OBJECT;
        } else if (typeStr.equals("new_id")) {
            type = Type.NEW_ID;
        } else if (typeStr.equals("array")) {
            type = Type.ARRAY;
        } else if (typeStr.equals("fd")) {
            type = Type.FD;
        } else {
            throw new BuildException("Invalid type \"" + typeStr + "\"");
        }
        ifaceName = xmlElem.getAttribute("interface");
        if (ifaceName.isEmpty())
            ifaceName = null;
        else
            ifaceName = Interface.toClassName(ifaceName);

        String nullstr = xmlElem.getAttribute("allow-null").toLowerCase();
        allowNull = nullstr.equals("true") || nullstr.equals("yes")
                || nullstr.equals("1");
    }

    public String getName()
    {
        return name;
    }

    public String getCType()
    {
        switch (type) {
        case INT:
            return "int32_t";
        case UINT:
            return "uint32_t";
        case FIXED:
            return "wl_fixed_t";
        case STRING:
            return "char *";
        case OBJECT:
            return "struct wl_resource *";
        case NEW_ID:
            return "uint32_t";
        case ARRAY:
            return "void *";
        case FD:
            return "int32_t";
        default:
            return null;
        }
    }

    public String getJavaType()
    {
        switch (type) {
        case INT:
            return "int";
        case UINT:
            return "int";
        case FIXED:
            return "Fixed";
        case STRING:
            return "String";
        case OBJECT:
            if (ifaceName != null && ! ifaceName.isEmpty())
                return Interface.toClassName(ifaceName) + ".Requests";
            else
                return "Resource";
        case NEW_ID:
            return "int";
        case ARRAY:
            return "java.nio.ByteBuffer";
        case FD:
            return "int";
        default:
            return null;
        }
    }

    public String getJNIType()
    {
        switch (type) {
        case INT:
            return "jint";
        case UINT:
            return "jint";
        case FIXED:
            return "jobject";
        case STRING:
            return "jstring";
        case OBJECT:
            return "jobject";
        case NEW_ID:
            return "jint";
        case ARRAY:
            return "jarray";
        case FD:
            return "jint";
        default:
            return null;
        }
    }

    public String getWLPrototype()
    {
        switch (type) {
        case INT:
            return "i";
        case UINT:
            return "u";
        case FIXED:
            return "f";
        case STRING:
            return allowNull ? "?s" : "s";
        case OBJECT:
            return allowNull ? "?o" : "o";
        case NEW_ID:
            return "n";
        case ARRAY:
            return "a";
        case FD:
            return "h";
        default:
            return null;
        }
    }

    public String getJavaPrototype()
    {
        switch (type) {
        case INT:
            return "I";
        case UINT:
            return "I";
        case FIXED:
            return "Lorg/freedesktop/wayland/Fixed;";
        case STRING:
            return "Ljava/lang/String;";
        case OBJECT:
            return "L" + scanner.getJavaPackage().replaceAll("\\.", "/")
                    + ifaceName + "$Requests;";
        case NEW_ID:
            return "I";
        case ARRAY:
            return "[B";
        case FD:
            return "I";
        default:
            return null;
        }
    }

    public void writeJavaDeclaration(Writer writer) throws IOException
    {
        writer.write(getJavaType() + " " + name);
    }

    public void writeCDeclaration(Writer writer) throws IOException
    {
        writer.write(getCType() + " " + name);
    }

    public void writeJNIToCConversion(Writer writer) throws IOException
    {
        switch (type) {
        case INT:
            writer.write("(int32_t)" + name);
            break;
        case UINT:
            writer.write("(uint32_t)" + name);
            break;
        case FIXED:
            writer.write("wl_jni_fixed_from_java(__env, " + name + ")");
            break;
        case STRING:
            writer.write("wl_jni_string_to_utf8(__env, " + name + ")");
            break;
        case OBJECT:
            writer.write("wl_jni_resource_from_java(__env, " + name + ")");
            break;
        case NEW_ID:
            writer.write("(uint32_t)" + name);
            break;
        case ARRAY:
            writer.write("(*__env)->GetByteArrayElements(__env,");
            writer.write(" " + name + ", NULL)");
            break;
        case FD:
            writer.write("(int32_t)" + name);
            break;
        }
    }

    public void writeJNICleanup(Writer writer, String jni_name)
            throws IOException
    {
        switch (type) {
        case INT:
            break;
        case UINT:
            break;
        case FIXED:
            break;
        case STRING:
            writer.write("free(" + jni_name + ");");
            break;
        case OBJECT:
            break;
        case NEW_ID:
            break;
        case ARRAY:
            writer.write("(*__env)->ReleaseByteArrayElements(__env,");
            writer.write(" " + name + ", " + jni_name + ", JNI_ABORT);");
            break;
        case FD:
            break;
        }
        
    }
}

