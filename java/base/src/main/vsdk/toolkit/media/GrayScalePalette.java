package vsdk.toolkit.media;
import java.io.Serial;

import vsdk.toolkit.common.color.ColorRgb;

/**
Represents a linear scale gray palette.
*/
public class GrayScalePalette extends RGBProceduralColorPalette {
    @Serial private static final long serialVersionUID = 20061218L;

    public GrayScalePalette() {
        super();
    }

    @Override
    public int selectNearestIndexToRgb(ColorRgb c)
    {
        if ( !pure ) {
            return super.selectNearestIndexToRgb(c);
        }

        double gray = (c.r() + c.g() + c.b()) / 3;

        if ( gray < 0.0 ) gray = 0.0;
        if ( gray > 1.0 ) gray = 1.0;

        return (int)(gray*(colors.size()-1));
    }
}
