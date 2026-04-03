package vsdk.toolkit.render.sgl;

// A BOX (TYPICALLY IN SCREEN SPACE)
public class PolygonBox {
    public double x0; // Left and right
    public double x1;
    public double y0; // Top and bottom
    public double y1;
    public double z0; // Near and far
    public double z1;

    public PolygonBox() {
    }

    public PolygonBox(double x0, double x1, double y0, double y1, double z0, double z1) {
        this.x0 = x0;
        this.x1 = x1;
        this.y0 = y0;
        this.y1 = y1;
        this.z0 = z0;
        this.z1 = z1;
    }
}
