package vsdk.toolkit.environment.geometry.surface.polygon;

import java.io.Serial;
import java.util.ArrayList;
import java.util.List;

import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.environment.geometry.element.Vertex2D;

public class _Polygon2DContour extends FundamentalEntity implements Comparable<_Polygon2DContour>
{
    @Serial
    private static final long serialVersionUID = 20260419L;
    public List<Vertex2D> vertices;
    // If this contour is a hole, exteriorContour is the contour that contains it.
    private _Polygon2DContour exteriorContour;
    public boolean fleetingFlag; //Caution, not a long term flag.

    public _Polygon2DContour()
    {
        vertices = new ArrayList<>();
        exteriorContour = null;
    }

    public void addVertex(double x, double y, double r, double g, double b)
    {
        vertices.add(new Vertex2D(x, y, r, g, b));
    }

    public void addVertex(double x, double y)
    {
        vertices.add(new Vertex2D(x, y));
    }
    
    public void pushVertex(double x, double y)
    {
        vertices.add(0, new Vertex2D(x, y));
    }
    
    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    public double[] getMinMax()
    {
        double minMax[];
        Vertex2D v;
        int i, size;
        double minX;
        double minY;
        double maxX;
        double maxY;
        

        minMax = new double[6];
        size = vertices.size();
        if(size>0) {
            v = vertices.get(0);
            minX = v.x;
            minY = v.y;
            maxX = v.x;
            maxY = v.y;
        } else {// To operate in the same way that others getMinMax() functions.
            minX = Double.MAX_VALUE;
            minY = Double.MAX_VALUE;
            maxX = -Double.MAX_VALUE;
            maxY = -Double.MAX_VALUE;
        }
        for ( i = 1; i < size; ++i ) {
            v = vertices.get(i);
            if ( v.x > maxX ) {
                maxX = v.x;
            }
            if ( v.x < minX ) {
                minX = v.x;
            }
            if ( v.y > maxY ) {
                maxY = v.y;
            }
            if ( v.y < minY ) {
                minY = v.y;
            }
        }

        minMax[0] = minX;
        minMax[1] = minY;
        minMax[2] = 0;
        minMax[3] = maxX;
        minMax[4] = maxY;
        minMax[5] = 0;

        return minMax;
    }
    /**
     * The variable modifyState is kept for source compatibility.
     * @param modifyState
     * @return 
     */
    public double calcMinMaxArea(boolean modifyState)
    {
        double minMax[];
        
        minMax = getMinMax();
        return (minMax[3]-minMax[0])*(minMax[4]-minMax[1]);
    }
    
//    /**
//     * @return the hole
//     */
//    public boolean isHole() {
//        return hole;
//    }
//
//    /**
//     * @param hole the hole to set
//     */
//    public void setHole(boolean hole) {
//        this.hole = hole;
//    }

    /**
     * @return the exteriorContour
     */
    public _Polygon2DContour getExteriorContour() {
        return exteriorContour;
    }

    /**
     * @param exteriorContour the exteriorContour to set
     */
    public void setExteriorContour(_Polygon2DContour exteriorContour) {
        this.exteriorContour = exteriorContour;
    }

    /**
     * To use it efficiently(specialy, ordering a list of objects of this class),
     * call before CalcMinMaxArea() in both objects.
     * @param obj
     * @return 
     */
    @Override
    public int compareTo(_Polygon2DContour obj)
    {
        //if ( ! (obj instanceof _Polygon2DContour) ) {
        //    return -1;
        //}
        _Polygon2DContour other;
        other = /*(_Polygon2DContour)*/ obj;

        double area = calcMinMaxArea(false);
        double otherArea = other.calcMinMaxArea(false);
        
        if(area == otherArea)
            return 0;
        if(area < otherArea)
            return -1;
        else
            return 1;
    }
}
