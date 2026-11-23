package vsdk.toolkit.app.options;

public class EnumAppOptions {
    public int width;
    public int height;
    public int nqcdivs;
    public int yesValue;
    public int noValue;
    public int debug;
    public boolean nativeRNG;
    public EnumAppOptions(int width, int height, int nqcdivs, int yesValue,
                          int noValue, int debug, boolean nativeRNG) {
        this.width = width;
        this.height = height;
        this.nqcdivs = nqcdivs;
        this.yesValue = yesValue;
        this.noValue = noValue;
        this.debug = debug;
        this.nativeRNG = nativeRNG;
    }
}
