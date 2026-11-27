package vsdk.toolkit.io.context;

import vsdk.toolkit.common.color.Cie;
import vsdk.toolkit.scene.RadianceMethod;

public class ParseOptionsContext {
    public RadianceMethod radianceMethod;
    public boolean singleSided;
    public int numberOfQuarterCircleDivisions;
    public boolean monochrome;
    private Cie colorSpace;

    public ParseOptionsContext() {
        radianceMethod = null;
        singleSided = false;
        numberOfQuarterCircleDivisions = 0;
        monochrome = false;
        colorSpace = new Cie();
    }

    public Cie getColorSpace() {
        return colorSpace;
    }

    public void setColorSpace(Cie inColorSpace) {
        colorSpace = inColorSpace;
    }
}
