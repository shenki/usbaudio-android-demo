package au.id.jms.usbaudio;

public class UsbAudio {
    static {
        System.loadLibrary("usbaudio");
    }

    public native void start();
    public native void stop();
}
