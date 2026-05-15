package vsdk.toolkit.render.sgl;

public class PolygonVertex {
    public double sx; // Screen space position (sometimes homo)
    public double sy;
    public double sz;
    public double sw;
    public double x; // World space position
    public double y;
    public double z;
    public double u; // Texture position (sometimes homogeneous)
    public double v;
    public double r; // (red,green,blue) color
    public double g;
    public double b;

    public double getCoord(int i) {
        switch (i) {
            case 0:
                return sx;
            case 1:
                return sy;
            case 2:
                return sz;
            case 3:
                return sw;
            case 4:
                return x;
            case 5:
                return y;
            case 6:
                return z;
            case 7:
                return u;
            case 8:
                return v;
            case 9:
                return r;
            case 10:
                return g;
            case 11:
                return b;
            default:
                return 0.0;
        }
    }
}
