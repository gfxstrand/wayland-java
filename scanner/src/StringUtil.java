import java.io.StringWriter;

class StringUtil
{
    public static String toJNIName(String name)
    {
        StringWriter writer = new StringWriter();
        for (int i = 0; i < name.length(); ++i) {
            char c = name.charAt(i);
            switch (c) {
            case '/':
                writer.write("_");
                break;
            case '_':
                writer.write("_1");
                break;
            case ';':
                writer.write("_2");
                break;
            case '[':
                writer.write("_3");
                break;
            default:
                writer.write(c);
            }
        }
        return writer.toString();
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
}

