package vsdk.toolkit.common.color;
import java.io.Serial;
import java.util.Objects;
import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;

/**
Respect to data representation:

The `r`, `g`, and `b` class attributes represent red, green and blue 
components in a color specification, with values in the range [0, inf) when
used in High Dynamic Range Imaginary (HDRI). Note that no restriction as been
specified regarding units to be used, and as of this revision the units
must be application defined. When not used in HDRI, the values must be
application-clamped to the range [0, 1]. A value of 0 always will represent
'no contribution' or 'black', and a value of 1 will be 'white' in non HDRI
applications. Interpretation in HDRI applications is pending to be defined.
*/
public final class ColorRgb extends FundamentalEntity
{
    @Serial private static final long serialVersionUID = 20260506L;

    private final double r;
    private final double g;
    private final double b;

    /**
    Note that default assumed color in the toolkit is black. It is
    important to note that changing this default could impact some
    algorithms. Do not change it.
    */
    public ColorRgb()
    {
        this(0, 0, 0);
    }

    /**
    This constructor builds a ColorRgb from another one.
    @param c source color to copy
    */
    public ColorRgb(ColorRgb c)
    {
        this(Objects.requireNonNull(c, "ColorRgb to copy cannot be null").r,
             c.g,
             c.b);
    }

    /**
    This constructor builds a ColorRgb from individual component values.
    @param r red component
    @param g green component
    @param b blue component
    */
    public ColorRgb(double r, double g, double b)
    {
        this.r = r;
        this.g = g;
        this.b = b;
    }

    /** This method returns a copy of the value r. Note that this method does
    NOT constitute an encapsulation of the value, as the original attribute
    is public. This method is supplied for puritans that like to see a lot
    of long get/set code, and for testing the performance of different
    algorithms, as the access technique is changed between the direct access
    to the attribute and this intermediate use of get/set methods.
    @return red color component
    */
    public double getR()
    {
        return r;
    }

    /** This method returns a copy of the value g. Note that this method does
    NOT constitute an encapsulation of the value, as the original attribute
    is public. This method is supplied for puritans that like to see a lot
    of long get/set code, and for testing the performance of different
    algorithms, as the access technique is changed between the direct access
    to the attribute and this intermediate use of get/set methods.
    @return green color component
    */
    public double getG()
    {
        return g;
    }

    /** This method returns a copy of the value b. Note that this method does
    NOT constitute an encapsulation of the value, as the original attribute
    is public. This method is supplied for puritans that like to see a lot
    of long get/set code, and for testing the performance of different
    algorithms, as the access technique is changed between the direct access
    to the attribute and this intermediate use of get/set methods.
    @return blue color component
    */
    public double getB()
    {
        return b;
    }

    /**
    This method exports the color components to a static array of float
    values. It is supposed to help operations in APIs like OpenGL/JOGL where
    this representation form is commonly used.
    @return a three element sized single precision float array containing
    r, g and b data
    */
    public float[] exportToFloatArrayVector()
    {
        return new float[]{(float)r, (float)g, (float)b, 1};
    }

    /**
    This method return a String representation of current color. In its
    current implementation it is biased for human readability, not for
    precision, so the use of an approximation formating.
    @return human-readable String representation of current color
    */
    @Override
    public String toString()
    {
        return "<" + VSDK.formatDouble(r) + ", " +
                     VSDK.formatDouble(g) + ", " + 
                     VSDK.formatDouble(b) + ">";
    }

    /**
    Given current color space (RGB coordinates), this method returns the
    Euclidean distance between two points in such space: `this` and `other`.
    @param other reference color to compare against
    @return color distance in RGB color coordinate space
    */
    public double distance(ColorRgb other) {
        return Math.sqrt((this.r - other.r)*(this.r - other.r) +
                         (this.g - other.g)*(this.g - other.g) +
                         (this.b - other.b)*(this.b - other.b));
    }

    public static double distance(ColorRgb a, ColorRgb b) {
        return Math.sqrt((a.r - b.r)*(a.r - b.r) +
                         (a.g - b.g)*(a.g - b.g) +
                         (a.b - b.b)*(a.b - b.b));
    }

    public ColorRgb add(ColorRgb other)
    {
        return new ColorRgb(this.r + other.r, this.g + other.g, this.b + other.b);
    }

    public ColorRgb multiply(double scalar)
    {
        return new ColorRgb(this.r * scalar, this.g * scalar, this.b * scalar);
    }

    public double r()
    {
        return r;
    }

    public double g()
    {
        return g;
    }

    public double b()
    {
        return b;
    }

    /**
    Allows this color to be used inside a HashMap.
    */
    @Override
    public int hashCode() {
        int hash = 5;
        hash = 59 * hash + Long.hashCode(Double.doubleToLongBits(this.r));
        hash = 59 * hash + Long.hashCode(Double.doubleToLongBits(this.g));
        hash = 59 * hash + Long.hashCode(Double.doubleToLongBits(this.b));
        return hash;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        final ColorRgb other = (ColorRgb) obj;
        if (Double.doubleToLongBits(this.r) != Double.doubleToLongBits(other.r)) {
            return false;
        }
        if (Double.doubleToLongBits(this.g) != Double.doubleToLongBits(other.g)) {
            return false;
        }
        return Double.doubleToLongBits(this.b) == Double.doubleToLongBits(other.b);
    }
}
