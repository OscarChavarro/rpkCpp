package vsdk.toolkit.io.context;

import vsdk.toolkit.scene.RadianceMethod;

public class ParseOptionsContext {
    public RadianceMethod radianceMethod;
    public boolean singleSided;
    public int numberOfQuarterCircleDivisions;
    public boolean monochrome;

    public ParseOptionsContext() {
        radianceMethod = null;
        singleSided = false;
        numberOfQuarterCircleDivisions = 0;
        monochrome = false;
    }
}
