//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.processing;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;

/**
This class contains common computational geometry operations (mostly
geometrical querys over existing geometries). This is a companion class
for the `GeometricModeler`, which holds creation and modification operations
over gemetries.
*/
public class ComputationalGeometry extends ProcessingElement
{
    // For 2D line clipping an 4 bit outcode is used
    private static final int COHEN_SUTHERLAND_2D_INSIDE = 0; // 0000
    private static final int COHEN_SUTHERLAND_2D_LEFT = 1;  // 0001
    private static final int COHEN_SUTHERLAND_2D_RIGHT = 2; // 0010
    private static final int COHEN_SUTHERLAND_2D_BOTTOM = 4;// 0100
    private static final int COHEN_SUTHERLAND_2D_TOP = 8;   // 1000

    /**
    Given a line from p0 to p1 and a point p, this method gives the minimum
    distance between line and point.
    
    Following implementation from http://geomalgorithms.com/a02-_lines.html
    @param p0
    @param p1
    @param p
    @return 
    */
    public static double lineToPointDistance(
        Vector3Dd p0, 
        Vector3Dd p1,
        Vector3Dd p)
    {
        //----------------------------------------------------------------------
        Vector3Dd lineVector = p1.subtract(p0);

        double denominator = lineVector.length();
        if ( denominator < VSDK.EPSILON ) {
            return Double.NaN;
        }

        //----------------------------------------------------------------------
        Vector3Dd v, w;

        v = p1.subtract(p0);
        w = p.subtract(p0);
        
        double c1;
        double c2;

        c1 = w.dotProduct(v);
        if ( c1 <= 0.0 ) {
            return Vector3Dd.distance(p, p0);
        }
        
        c2 = v.dotProduct(v);
        if ( c2 <= c1 ) {
            return Vector3Dd.distance(p, p1);
        }
        
        double b;
        b = c1 / c2;
        Vector3Dd pb;
        pb = p0.add(v.multiply(b));
           
        return Vector3Dd.distance(p, pb);    
    }
    
    /**
    Given a line that passes between points `p0` and `p1`, this method 
    determines if point `p` falls under `distanceTolerance` in such line.
    @param p0
    @param p1
    @param p
    @param distanceTolerance
    @return 
    */
    public static int lineContainmentTest(
        Vector3Dd p0, 
        Vector3Dd p1,
        Vector3Dd p, 
        double distanceTolerance) 
    {
        double d;

        d = lineToPointDistance(p0, p1, p);
        
        if ( d <= distanceTolerance ) {
            return Containment.LIMIT.value();
        }
        return Containment.OUTSIDE.value();
    }

    /**
    Given a line that passes between points `p0` and `p1`, this method 
    determines if point `p` falls under `distanceTolerance` in such line.

    This method is functionaly equivalent to procedures `contev` and
    `intrev` from program [MANT1988].13.3. and section [MANT1988].13.2.2.
    @param p0
    @param p1
    @param p
    @param distanceTolerance
    @return 
    */
    public static int lineSegmentContainmentTest(
        Vector3Dd p0, 
        Vector3Dd p1,
        Vector3Dd p, 
        double distanceTolerance) 
    {
        double d;
        Vector3Dd a, b;

        a = p1.subtract(p0);
        b = p.subtract(p0);

        double denominator = a.length();
        if ( denominator < VSDK.EPSILON ) return Containment.OUTSIDE.value();

        double numerator = a.crossProduct(b).length();
        d = numerator / denominator;

        if ( d <= distanceTolerance ) {
            double t = a.dotProduct(b) / a.dotProduct(a);
            if ( t < -VSDK.EPSILON || t > 1+VSDK.EPSILON ) return Containment.OUTSIDE.value();

            return Containment.LIMIT.value();
        }
        return Containment.OUTSIDE.value();
    }


    /**
    Implementation of the Cohen-Sutherland line clipping algorithm on two
    Dimensions
    @param p0 first point of the line to be clipped
    @param p1 second point of the line to be clipped
    @param min coordinate with the minimum (x,y) values of the clipping
    rectangle
    @param max coordinate with the maximum (x,y) values of the clipping
    rectangle
    @param tolerance could be 0, or an small value. Positive makes area smaller,
    negative makes area bigger.
    @return <code> true </code> when there is at least a portion of the line
    inside the clipping rectangle and <code> false </code> when the line is
    outside the clipping rectangle.
    */
    public static boolean cohenSutherlandLineClipping2D(
        Vector2Dd p0, 
        Vector2Dd p1,
        Vector2Dd min, 
        Vector2Dd max,
        double tolerance)
    {
        return cohenSutherlandLineClipping2DResult(p0, p1, min, max, tolerance).accepted();
    }

    public static ClippedLine2DResult cohenSutherlandLineClipping2DResult(
        Vector2Dd p0,
        Vector2Dd p1,
        Vector2Dd min,
        Vector2Dd max,
        double tolerance)
    {
        Vector2Dd clipped0 = p0;
        Vector2Dd clipped1 = p1;
        double minX = Math.min(min.x(), max.x());
        double maxX = Math.max(min.x(), max.x());
        double minY = Math.min(min.y(), max.y());
        double maxY = Math.max(min.y(), max.y());
        Vector2Dd clipMin = new Vector2Dd(minX, minY);
        Vector2Dd clipMax = new Vector2Dd(maxX, maxY);
        
        if ( maxX - minX < VSDK.EPSILON ||
             maxY - minY < VSDK.EPSILON ) {
            //System.out.println("Error: to small area");
            return new ClippedLine2DResult(false, p0, p1);
        }

        int outCode0 = computeCohenSutherland2DCode(clipped0, clipMin, clipMax, tolerance);
        int outCode1 = computeCohenSutherland2DCode(clipped1, clipMin, clipMax, tolerance);
        int outCodeOut;
        boolean accept = false;
        double x = 0.0;
        double y = 0.0;
        
        while ( !accept ) {
            if ( (outCode0 | outCode1) == 0 ) {
                // Both endpoints are inside clip rectangle
                accept = true;
                break;
            }
            else if ( (outCode0 & outCode1) != 0 ) {
                // Both endpoints are outside one of the 4 limit lines
                accept = false;
                break;
            } 
            else {
                // Line should be trimmed
                double e = 2*tolerance; //2*VSDK.EPSILON;
                outCodeOut = (outCode0 != 0) ? outCode0 : outCode1;
                if ( (outCodeOut & COHEN_SUTHERLAND_2D_TOP) == 
                     COHEN_SUTHERLAND_2D_TOP) {
                    x = clipped0.x() +
                        (clipped1.x() - clipped0.x()) * ((clipMax.y()-e) - clipped0.y()) /
                        (clipped1.y() - clipped0.y());
                    y = clipMax.y() - e;
                }
                else if ( (outCodeOut & COHEN_SUTHERLAND_2D_BOTTOM) == 
                    COHEN_SUTHERLAND_2D_BOTTOM) {
                    x = clipped0.x() + (clipped1.x() - clipped0.x()) * (clipMin.y()+e - clipped0.y()) /
                        (clipped1.y() - clipped0.y());
                    y = clipMin.y()+e;
                }
                else if ( (outCodeOut & COHEN_SUTHERLAND_2D_RIGHT) == 
                    COHEN_SUTHERLAND_2D_RIGHT) {
                    y = clipped0.y() + (clipped1.y() - clipped0.y()) * (clipMax.x()-e - clipped0.x()) /
                        (clipped1.x() - clipped0.x());
                    x = clipMax.x()-e;
                }
                else if ( (outCodeOut & COHEN_SUTHERLAND_2D_LEFT) == 
                    COHEN_SUTHERLAND_2D_LEFT ) {
                    y = clipped0.y() + (clipped1.y() - clipped0.y()) * (clipMin.x()+e - clipped0.x()) /
                        (clipped1.x() - clipped0.x());
                    x = clipMin.x()+e;
                }
            }
            if ( outCodeOut == outCode0 ) {
                clipped0 = new Vector2Dd(x, y);
                outCode0 = computeCohenSutherland2DCode(clipped0, clipMin, clipMax, tolerance);
            }
            else {
                clipped1 = new Vector2Dd(x, y);
                outCode1 = computeCohenSutherland2DCode(clipped1, clipMin, clipMax, tolerance);
            }            
        }
        return new ClippedLine2DResult(accept, clipped0, clipped1);
    }

    public static final class ClippedLine2DResult
    {
        private final boolean accepted;
        private final Vector2Dd clipped0;
        private final Vector2Dd clipped1;

        public ClippedLine2DResult(boolean accepted, Vector2Dd clipped0, Vector2Dd clipped1) {
            this.accepted = accepted;
            this.clipped0 = clipped0;
            this.clipped1 = clipped1;
        }

        public boolean accepted() {
            return accepted;
        }

        public Vector2Dd clipped0() {
            return clipped0;
        }

        public Vector2Dd clipped1() {
            return clipped1;
        }
    }

    /**
    Compute a code of the position of a point relative to the clipping
    rectangle
    @param p point to be located
    @param min
    @param max
    @return the code in four bits representing the location of the point
    */
    private static int computeCohenSutherland2DCode(
        Vector2Dd p, Vector2Dd min, Vector2Dd max, double tolerance){
        int outCode;
        outCode = COHEN_SUTHERLAND_2D_INSIDE;
        double e = tolerance;

        if ( p.x() < min.x() + e ) {
            outCode |= COHEN_SUTHERLAND_2D_LEFT;
        }
        if ( p.x() > max.x() - e ) {
            outCode |= COHEN_SUTHERLAND_2D_RIGHT;
        }
        if ( p.y() < min.y() + e ) {
            outCode |= COHEN_SUTHERLAND_2D_BOTTOM;
        }
        if ( p.y() > max.y() - e ) {
            outCode |= COHEN_SUTHERLAND_2D_TOP;
        }
        return outCode;
    }
}
