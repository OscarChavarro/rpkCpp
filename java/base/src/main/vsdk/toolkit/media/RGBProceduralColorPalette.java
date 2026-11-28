package vsdk.toolkit.media;
import java.io.Serial;

import vsdk.toolkit.common.color.ColorRgb;

/**
The RGBProceduralColorPalette abstract class provides an interface for
procedural color palette classes. This serves two purposes:
  - To help in design level organization of procedural color palettes
    (this eases the study of the class hierarchy)
  - To provide a place to locate operations and methods, common to all
    procedural color palette classes.

Note that any procedural color palette is a normal color palette. Its
difference is that some operations (most notably the selectNearestIndexToRgb
method) are more rapidly calculated in some procedural defined palettes that
its normal method.
*/
public abstract class RGBProceduralColorPalette extends RGBColorPalette {
    @Serial private static final long serialVersionUID = 20061218L;

    /// A procedural palette is "pure" if its contents are the original ones
    /// defined by its filling procedure method "init", and are not pure
    /// if they are modified in any other way. A pure procedural palette may
    /// have a propierty that allows for faster calculation of some operation,
    /// whereas a non-pure equivalent will work equally, but slower.
    protected boolean pure = true;

    public RGBProceduralColorPalette()
    {
        super();
        pure = true;
    }

    @Override
    public int selectNearestIndexToRgb(ColorRgb c)
    {
        return super.selectNearestIndexToRgb(c);
    }

    @Override
    public void setColorAt(int i, ColorRgb c)
    {
        pure = false;
        super.setColorAt(i, c);
    }

    @Override
    public void setColorAt(int i, double r, double g, double b)
    {
        pure = false;
        super.setColorAt(i, r, g, b);
    }

    @Override
    public void addColor(ColorRgb c)
    {
        pure = false;
        super.addColor(c);
    }

    @Override
    public void addColor(double r, double g, double b)
    {
        pure = false;
        super.addColor(r, g, b);
    }
}
