public class SimpleShm
{
    public static void main(String args[])
    {
        Display display = new Display();
        Window window = new Window(display, 250, 250);

        try {
            while(true)
                display.display.dispatch();
        } catch (Exception e) {
            window.destroy();
            display.destroy();
        }
    }
}

