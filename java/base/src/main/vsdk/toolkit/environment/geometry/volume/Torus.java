//= References:                                                             =
//= [WAGN2004] Wagner, Max. "Ray/Torus Intersection". CS400 course homework =
//= report for CS400 class                                                  =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume;
import java.io.Serial;
import java.util.Arrays;

// VSDK classes
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolid;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidEulerConstructions;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidEulerOperators;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidTopologyEditing;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidValidationEngine;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.element.RayHit;

/**
Current implementation is based on [WAGN2004].
*/
public class Torus extends Solid
{
    @SuppressWarnings("FieldNameHidesFieldInSuperclass")
    @Serial private static final long serialVersionUID = 20131024L;
    
    /// The roots the quartic solver gives for a ray are accepted only if the
    /// point they define is on the surface of the torus, within this distance
    /// (relative to the major radius): verified roots have errors around 1e-11,
    /// and spurious ones (the solver gives some) errors of the order of 0.1
    private static final double ROOT_VALIDATION_TOLERANCE = 1.0e-6;

    /// Default number of sections of the boundary representation along the
    /// major circle (around the Z axis)
    private static final int DEFAULT_MAJOR_DIVISIONS = 24;
    /// Default number of sides of each section (around the tube)
    private static final int DEFAULT_MINOR_DIVISIONS = 12;
    private static final int MIN_MAJOR_DIVISIONS = 3;
    private static final int MIN_MINOR_DIVISIONS = 3;

    private double majorRadius;
    private double minorRadius;

    /**
    @param inMajorRadius
    @param inMinorRadius 
    */
    public Torus(final double inMajorRadius, final double inMinorRadius)
    {
        majorRadius = inMajorRadius;
        minorRadius = inMinorRadius;
        getControlSpecifications().add("double;majorRadius;(0, INFINITE)");
        getControlSpecifications().add("double;minorRadius;(0, INFINITE)");
    }

    /**
    @return the majorRadius
    */
    public double getMajorRadius() 
    {
        return majorRadius;
    }

    /**
     * @param rMajor the majorRadius to set
     */
    public void setMajorRadius(double rMajor) 
    {
        this.majorRadius = rMajor;
    }

    /**
    @return the minorRadius
    */
    public double getMinorRadius() 
    {
        return minorRadius;
    }

    /**
    @param rMinor the minorRadius to set
    */
    public void setMinorRadius(double rMinor) 
    {
        this.minorRadius = rMinor;
    }

    /**
    Intersects a ray with the torus, from the equation of [WAGN2004]. The
    quartic equation is solved with the ray origin moved to the point of the
    ray nearest to the center of the torus, so its coefficients stay small
    however far the origin is; its real roots are found in a deterministic
    way (see `findRealRoots`), as the iterative solver previously used gave
    real roots that did not belong to the torus, and lost ones that did.
    @param inOut_ray ray to intersect, in the coordinates of the torus
    @return the ray with the distance to the nearest hit at its `t`, or null if
    it does not hit the torus ahead of its origin
    */
    public Ray doIntersectionFirstHit(Ray inOut_ray) 
    {
        Vector3Dd origin = inOut_ray.getOrigin();

        inOut_ray = inOut_ray.withDirection(inOut_ray.getDirection().normalized());
        Vector3Dd d = inOut_ray.getDirection();

        // Distance from the original origin to the point nearest to the center
        double shift = -origin.dotProduct(d);
        Vector3Dd p = origin.add(d.multiply(shift));

        double alpha, beta, gama;

        alpha = d.dotProduct(d);
        beta = 2 * p.dotProduct(d);
        gama = p.dotProduct(p) - (minorRadius * minorRadius) - (majorRadius * majorRadius);

        double a4, a3, a2, a1, a0;

        a4 = alpha * alpha;
        a3 = 2 * alpha * beta;
        a2 = (beta * beta) + 2 * alpha * gama + 4 * (majorRadius * majorRadius) * (d.z() * d.z());
        a1 = 2 * beta * gama + 8 * (majorRadius * majorRadius) * p.z() * d.z();
        a0 = (gama * gama) + 4 * (majorRadius * majorRadius) * (p.z() * p.z()) - (4 * (majorRadius * majorRadius) * (minorRadius * minorRadius));

        double[] roots = findRealRoots(new double[] {a0, a1, a2, a3, a4});
        double mRoot = 0;
        int count = 0;

        for ( double root : roots ) {
            // Distance along the ray that was given
            double t = shift + root;

            if ( t > 0 && isOnSurface(origin, d, t) ) {
                if ( count == 0 || t < mRoot ) {
                    mRoot = t;
                    count++;
                }
            }
        }

        if (count == 0) {
            return null;
        } 
        else {
            return inOut_ray.withT(mRoot);
        }
    }

    /**
    Finds the real roots of a polynomial, in ascending order, isolating them
    with the roots of its derivative (that separate the intervals where the
    polynomial is monotonic) and bisecting the intervals where it changes its
    sign. Roots of even multiplicity (tangencies) are found only if they are
    a root of the derivative too.
    @param polynomial coefficients, in ascending order of the powers
    @return the real roots
    */
    private static double[] findRealRoots(double[] polynomial)
    {
        int degree = polynomial.length - 1;

        while ( degree > 0 && polynomial[degree] == 0.0 ) {
            degree--;
        }
        if ( degree <= 0 ) {
            return new double[0];
        }
        if ( degree == 1 ) {
            return new double[] {-polynomial[0] / polynomial[1]};
        }

        double[] derivative = new double[degree];

        for ( int k = 1; k <= degree; k++ ) {
            derivative[k - 1] = k * polynomial[k];
        }
        double[] critical = findRealRoots(derivative);

        // Cauchy bound: every root is inside it
        double bound = 0;

        for ( int k = 0; k < degree; k++ ) {
            bound = Math.max(bound, Math.abs(polynomial[k] / polynomial[degree]));
        }
        bound += 1;

        double[] limits = new double[critical.length + 2];

        limits[0] = -bound;
        for ( int k = 0; k < critical.length; k++ ) {
            limits[k + 1] = Math.max(-bound, Math.min(bound, critical[k]));
        }
        limits[limits.length - 1] = bound;

        double[] found = new double[degree];
        int count = 0;

        for ( int k = 0; k < limits.length - 1 && count < degree; k++ ) {
            double low = limits[k];
            double high = limits[k + 1];
            double valueAtLow = evaluate(polynomial, degree, low);
            double valueAtHigh = evaluate(polynomial, degree, high);

            if ( valueAtLow == 0.0 ) {
                if ( count == 0 || found[count - 1] != low ) {
                    found[count++] = low;
                }
            }
            else if ( valueAtHigh != 0.0 && (valueAtLow < 0.0) != (valueAtHigh < 0.0) ) {
                for ( int iteration = 0; iteration < 200; iteration++ ) {
                    double middle = 0.5 * (low + high);

                    if ( middle <= low || middle >= high ) {
                        break;
                    }
                    if ( (evaluate(polynomial, degree, middle) < 0.0) == (valueAtLow < 0.0) ) {
                        low = middle;
                    }
                    else {
                        high = middle;
                    }
                }
                found[count++] = 0.5 * (low + high);
            }
        }
        if ( count < degree && evaluate(polynomial, degree, bound) == 0.0 ) {
            found[count++] = bound;
        }
        return Arrays.copyOf(found, count);
    }

    private static double evaluate(double[] polynomial, int degree, double x)
    {
        double value = 0;

        for ( int k = degree; k >= 0; k-- ) {
            value = value * x + polynomial[k];
        }
        return value;
    }

    /**
    Checks a root of the intersection equation: the solver of the equation can
    give real roots that do not correspond to any point of the torus.
    @param origin origin of the ray
    @param direction unit direction of the ray
    @param t distance along the ray to check
    @return true if the point of the ray at the distance is on the surface of
    the torus
    */
    private boolean isOnSurface(Vector3Dd origin, Vector3Dd direction, double t)
    {
        Vector3Dd hit = origin.add(direction.multiply(t));
        double distanceToCircle = Math.hypot(Math.hypot(hit.x(), hit.y()) - majorRadius, hit.z());
        double tolerance = ROOT_VALIDATION_TOLERANCE * Math.max(majorRadius, minorRadius);

        return Math.abs(distanceToCircle - minorRadius) <= tolerance;
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inRay, RayHit outHit)
    {
        Ray hit = doIntersectionFirstHit(inRay);
        if ( hit == null ) {
            return false;
        }
        if ( outHit != null ) {
            outHit.setRay(hit);
            doExtraInformation(hit, hit.getT(), outHit);
            outHit.setRay(hit);
        }
        return true;
    }

    /*
    Unused and not working yet method. An old try to find roots from analytical
    cubic equation solution.
    @param a4
    @param a3
    @param a2
    @param a1
    @param a0
    @param ray
    @return true if current quartic equation has at least one real root
    */    
    /*
    private boolean
    calculateRoot(double a4, double a3,double a2, double a1, double a0, Ray ray)
    {
        int iRoot[];
        iRoot = new int[4];
        double Root[];
        Root = new double[4];
        
        //Root 1
        //--------------------------------------------------------------------
        double x;
        double x1;
        double x2;
        double x3;
        
        //----------------------------------------------------
        //1. Part 1 
        double p1=-a3/(4*a4);
        System.out.println("Parte 1: "+p1);
       
        
        //--------------------------------------------------------- 
        //2. Part 2
        
        //System.out.println("\nParte 2: p2_1 * Math.sqrt( p2_2  - p2_3 + p2_4 + p2_5)"); 
        
        // 2.1
        double p2_1=1/2d;
        System.out.println("p2_1: "+p2_1);
        
        // 2.2
        double p2_2=Math.pow(a3, 2)/(4 *  Math.pow(a4, 2));
        System.out.println("p2_2: "+p2_2);
        
        // 2.3
        double p2_3=(2 * a2)/(3 * a4);
        System.out.println("p2_3: "+p2_3);
       
        // 2.4
        System.out.println("p2_4= p2_4_n/p2_4_d ");
       
        // 2.4 nominador
        double p2_4_n;
        p2_4_n = Math.pow(2d, (1/3d))*(Math.pow(a2, 2) - 3 * a3  * a1 + 12 * a4 * a0);
        //System.out.println("p2_4_n: "+p2_4_n);
        
        
        // 2.4 denominador
        //System.out.println("p2_4_d=3 * a * Math.pow((  p2_4_d_1 - p2_4_d_2 + p2_4_d_3 + p2_4_d_4 - p2_4_d_5  + Math.sqrt(p2_4_d_6)),(1/3d)); ");
        
        // 2.4.1
        double p2_4_d_1= 2 * Math.pow(a2, 3);
        System.out.println("p2_4_d_1: "+p2_4_d_1);
        // 2.4.2
        double p2_4_d_2= 9 * a3 * a2 * a1;
        System.out.println("p2_4_d_2: "+p2_4_d_2);
        // 2.4.3
        double p2_4_d_3= 27 * a4 * Math.pow(a1, 2) ;
        System.out.println("p2_4_d_3: "+p2_4_d_3);
        // 2.4.4
        double p2_4_d_4= 27 * Math.pow(a3, 2) * a0;
        System.out.println("p2_4_d_4: "+p2_4_d_4);
        // 2.4.5
        double p2_4_d_5= 72 * a4 * a2 * a0;
        System.out.println("p2_4_d_5: "+p2_4_d_5);
        // 2.4.6        
        System.out.println("p2_4_d_6=p2_4_d_6_1 + p2_4_d_6_2");
        
        double p2_4_d_6_1=4 * Math.pow(
                                        (Math.pow(a2, 2) 
                                        - 3 * a3 * a1 
                                        + 12 * a4 * a0),3
                                       );
        double p2_4_d_6_2=Math.pow(
                                      (2 * Math.pow(a2, 3) 
                                       - 9 * a3 * a2 * a1 
                                       + 27 * a4 * Math.pow(a1, 2) 
                                       + 27 * Math.pow(a3, 2) * a0 
                                       - 72 * a4 * a2 * a0),2
                                     );
        
        double p2_4_d_6 = - p2_4_d_6_1 + p2_4_d_6_2;
        
        System.out.println("p2_4_d_6: "+p2_4_d_6);
        
        double p2_4_d=3 * a4 * Math.pow(
                                        (  p2_4_d_1 
                                         - p2_4_d_2 
                                         + p2_4_d_3 
                                         + p2_4_d_4 
                                         - p2_4_d_5 
                                         + Math.sqrt(p2_4_d_6)
                                        )
                                       ,(1/3d));
        
        System.out.println("p2_4_d: "+p2_4_d);
        
        double p2_4= p2_4_n/p2_4_d;
        
        // 2.5
        
        double p2_5= 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow(
                                                                  (2 * Math.pow(a2, 3) 
                                                                   - 9 * a3 * a2 * a1 
                                                                   + 27 * a4 * Math.pow(a1, 2) 
                                                                   + 27 * Math.pow(a3, 2) * a0 
                                                                   - 72 * a4 * a2 * a0 
                                                                   + Math.sqrt(
                                                                               -4 * Math.pow(
                                                                                               (Math.pow(a2, 2)
                                                                                                - 3 * a3 * a1 
                                                                                                + 12 * a4 * a0
                                                                                                )
                                                                                                ,3
                                                                                             ) 
                                                                               + Math.pow(
                                                                                           (2 * Math.pow(a2, 3) 
                                                                                            - 9 * a3 * a2 * a1 
                                                                                            + 27 * a4 * Math.pow(a1, 2) 
                                                                                            + 27 * Math.pow(a3, 2) * a0 
                                                                                            - 72 * a4 * a2 * a0
                                                                                            ),2
                                                                                           )
                                                                               )
                                                                  )
                                                                 ,(1/3d)
                                                                 );
        
        double p2 = p2_1 * Math.sqrt( p2_2  - p2_3 + p2_4 + p2_5);
        
        System.out.println("p2: "+p2);
       
        //--------------------------------------------------------------------------------------------------------------
        //3. Part 3
        
        // 3.1
        double p3_1= 1/2d;
       
        //3.2
        double p3_2=Math.pow(a3, 2)/(2 * Math.pow(a4, 2));
        
        // 3.3
        double p3_3=(4 * a2)/(3 * a4);
        
        // 3.4
        double p3_4=( Math.pow(2, (1/3d))*(Math.pow(a2, 2) 
                                            - 3 * a3 * a1 
                                            + 12 * a4 * a0
                                           )
                    )/
                    (3 * a4 * Math.pow(
                                       (2 * Math.pow(a2, 3) 
                                        - 9 * a3 * a2 * a1 
                                        + 27 * a4 * Math.pow(a1, 2) 
                                        + 27 * Math.pow(a3, 2) * a0 
                                        - 72 * a4 * a2 * a0 
                                        + Math.sqrt(
                                                    -4 * Math.pow(
                                                                   (Math.pow(a2, 2) 
                                                                    - 3 * a3 * a1 
                                                                    + 12 * a4 * a0
                                                                    ),3
                                                                   ) 
                                                    + Math.pow(
                                                                 (2 * Math.pow(a2, 3) 
                                                                  - 9 * a3 * a2 * a1 
                                                                  + 27 * a4 * Math.pow(a1, 2) 
                                                                  + 27 * Math.pow(a3, 2) * a0 
                                                                  - 72 * a4 * a2 * a0
                                                                  ),2
                                                                 )
                                                      )
                                        ),(1/3d)
                                      )
                     );
        
        // 3.5
        double p3_5= 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow(
                                                                  (2 * Math.pow(a2, 3) 
                                                                   - 9 * a3 * a2 * a1 
                                                                   + 27 * a4 * Math.pow(a1, 2) 
                                                                   + 27 * Math.pow(a3, 2) * a0 
                                                                   - 72 * a4 * a2 * a0 
                                                                   + Math.sqrt(
                                                                                -4 * Math.pow(
                                                                                               (Math.pow(a2, 2) 
                                                                                                - 3 * a3 * a1 
                                                                                                + 12 * a4 * a0
                                                                                                ),3
                                                                                              ) 
                                                                                + Math.pow(
                                                                                            (2 * Math.pow(a2, 3)
                                                                                             - 9 * a3 * a2 * a1 
                                                                                             + 27 * a4 * Math.pow(a1, 2) 
                                                                                             + 27 * Math.pow(a3, 2) * a0 
                                                                                             - 72 * a4 * a2 * a0
                                                                                             ),2
                                                                                            )
                                                                               )
                                                                   ),(1/3)
                                                                 );
        
        //3.6
        double p3_6=(-(Math.pow(a3, 3)/Math.pow(a4, 3)) 
                     + (4 * a3 * a2)/Math.pow(a4, 2) 
                     - (8 * a1)/a4
                     )/
                     (  4 * Math.sqrt(
                                        Math.pow(a3, 2)/(4 * Math.pow(a4, 2)) 
                                        - (2 * a2)/(3 * a4) 
                                        + (Math.pow(2, (1/3d))* (Math.pow(a2, 2) - 3 * a3 * a1 + 12 * a4 * a0))
                                           /(3 * a4 * Math.pow(
                                                               (2 * Math.pow(a2, 3) 
                                                                - 9 * a3 * a2 * a1
                                                                + 27 * a4 * Math.pow(a1, 2) 
                                                                + 27 * Math.pow(a3, 2) * a0 
                                                                - 72 * a4 * a2 * a0 
                                                                + Math.sqrt(
                                                                            -4 * Math.pow(
                                                                                           (Math.pow(a2, 2) 
                                                                                            - 3 * a3 * a1 
                                                                                            + 12 * a4 * a0
                                                                                            ),3
                                                                                           ) 
                                                                             + Math.pow(
                                                                                          (2 * Math.pow(a2, 3) 
                                                                                           - 9 * a3 * a2 * a1 
                                                                                           + 27 * a4 * Math.pow(a1, 2) 
                                                                                           + 27 * Math.pow(a3, 2) * a0 
                                                                                           - 72 * a4 * a2 * a0
                                                                                           ),2
                                                                                          )
                                                                              )
                                                               ),(1/3d)
                                                              )
                                            ) 
                                        + 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow( 
                                                                                        (2 * Math.pow(a2, 3)
                                                                                         - 9 * a3 * a2 * a1 
                                                                                         + 27 * a4 * Math.pow(a1, 2) 
                                                                                         + 27 * Math.pow(a3, 2) * a0 
                                                                                         - 72 * a4 * a2 * a0 
                                                                                         + Math.sqrt(
                                                                                                      -4 * Math.pow(
                                                                                                                     (Math.pow(a2, 2) 
                                                                                                                      - 3 * a3 * a1 
                                                                                                                      + 12 * a4 * a0
                                                                                                                      ),3
                                                                                                                     ) 
                                                                                                       + Math.pow(
                                                                                                                     (2 * Math.pow(a2, 3) 
                                                                                                                      - 9 * a3 * a2 * a1 
                                                                                                                      + 27 * a4 * Math.pow(a1, 2) 
                                                                                                                      + 27 * Math.pow(a3, 2) * a0 
                                                                                                                      - 72 * a4 * a2 * a0
                                                                                                                      ),2
                                                                                                                   )
                                                                                                      )
                                                                                         ),(1/3d)
                                                                                     )
                                      )
                      );
        
        double p3= p3_1 * Math.sqrt(p3_2 - p3_3 - p3_4 - p3_5 - p3_6);
        
   //     System.out.println("p3_1: "+p3_1+"\np3_2: "+p3_2+" \np3_3: "+p3_3+" \np3_4: "+p3_4+" \np3_5: "+p3_5+" \np3_6: "+p3_6);
       
        
          x=p1-p2-p3;
   //     System.out.println("P1: "+p1+" P2: "+p2+" P3: "+p3+" \nX:"+x);
          System.out.println("\nX:"+x);
          
          if(Double.isNaN(x))
          {
              iRoot[0]=0;
              Root[0]=0;
          }
          else
          {
              iRoot[0]=1;
              Root[0]=x;
          }
        
        //Root 2
        //------------------------------------------------------------------------
        //----------------------------------------------------
        //----------------------------------------------------
        //1. Part 1 
        p1=-a3/(4*a4);
        //System.out.println("Parte 1: "+p1);

        //--------------------------------------------------------- 
        //2. Part 2        
        //System.out.println("\nParte 2: p2_1 * Math.sqrt( p2_2  - p2_3 + p2_4 + p2_5)"); 
        
        // 2.1        
        p2_1=1/2d;
        //System.out.println("p2_1: "+p2_1);
        
        // 2.2
        
        p2_2=Math.pow(a3, 2)/(4 *  Math.pow(a4, 2));
        //System.out.println("p2_2: "+p2_2);
        
        // 2.3
        
        p2_3=(2 * a2)/(3 * a4);
        //System.out.println("p2_3: "+p2_3);
       
        // 2.4
        //System.out.println("p2_4= p2_4_n/p2_4_d ");
       
        // 2.4 nominador
        p2_4_n=Math.pow(2d, (1/3d))*(Math.pow(a2, 2) - 3 * a3  * a1 + 12 * a4 * a0);
        //System.out.println("p2_4_n: "+p2_4_n);
        
        // 2.4 denominador
        //System.out.println("p2_4_d=3 * a * Math.pow((  p2_4_d_1 - p2_4_d_2 + p2_4_d_3 + p2_4_d_4 - p2_4_d_5  + Math.sqrt(p2_4_d_6)),(1/3d)); ");
        
        // 2.4.1
        p2_4_d_1= 2 * Math.pow(a2, 3);
        //System.out.println("p2_4_d_1: "+p2_4_d_1);
        // 2.4.2
        p2_4_d_2= 9 * a3 * a2 * a1;
        //System.out.println("p2_4_d_2: "+p2_4_d_2);
        // 2.4.3
        p2_4_d_3= 27 * a4 * Math.pow(a1, 2) ;
        //System.out.println("p2_4_d_3: "+p2_4_d_3);
        // 2.4.4
        p2_4_d_4= 27 * Math.pow(a3, 2) * a0;
        //System.out.println("p2_4_d_4: "+p2_4_d_4);
        // 2.4.5
        p2_4_d_5= 72 * a4 * a2 * a0;
        //System.out.println("p2_4_d_5: "+p2_4_d_5);
        // 2.4.6        
        //System.out.println("p2_4_d_6=p2_4_d_6_1 + p2_4_d_6_2");
        
        p2_4_d_6_1=4 * Math.pow(
                                        (Math.pow(a2, 2) 
                                        - 3 * a3 * a1 
                                        + 12 * a4 * a0),3
                                       );
        p2_4_d_6_2=Math.pow(
                                      (2 * Math.pow(a2, 3) 
                                       - 9 * a3 * a2 * a1 
                                       + 27 * a4 * Math.pow(a1, 2) 
                                       + 27 * Math.pow(a3, 2) * a0 
                                       - 72 * a4 * a2 * a0),2
                                     );
        
        p2_4_d_6 = - p2_4_d_6_1 + p2_4_d_6_2;
        
        //System.out.println("p2_4_d_6: "+p2_4_d_6);
        
        p2_4_d=3 * a4 * Math.pow(
                                        (  p2_4_d_1 
                                         - p2_4_d_2 
                                         + p2_4_d_3 
                                         + p2_4_d_4 
                                         - p2_4_d_5 
                                         + Math.sqrt(p2_4_d_6)
                                        )
                                       ,(1/3d));
        
        //System.out.println(p2_4_d);
        
        p2_4= p2_4_n/p2_4_d;
        
        // 2.5
        
        p2_5= 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow(
                                                                  (2 * Math.pow(a2, 3) 
                                                                   - 9 * a3 * a2 * a1 
                                                                   + 27 * a4 * Math.pow(a1, 2) 
                                                                   + 27 * Math.pow(a3, 2) * a0 
                                                                   - 72 * a4 * a2 * a0 
                                                                   + Math.sqrt(
                                                                               -4 * Math.pow(
                                                                                               (Math.pow(a2, 2)
                                                                                                - 3 * a3 * a1 
                                                                                                + 12 * a4 * a0
                                                                                                )
                                                                                                ,3
                                                                                             ) 
                                                                               + Math.pow(
                                                                                           (2 * Math.pow(a2, 3) 
                                                                                            - 9 * a3 * a2 * a1 
                                                                                            + 27 * a4 * Math.pow(a1, 2) 
                                                                                            + 27 * Math.pow(a3, 2) * a0 
                                                                                            - 72 * a4 * a2 * a0
                                                                                            ),2
                                                                                           )
                                                                               )
                                                                  )
                                                                 ,(1/3d)
                                                                 );
        
        p2 = p2_1 * Math.sqrt( p2_2  - p2_3 + p2_4 + p2_5);
        
        
       
        //--------------------------------------------------------------------------------------------------------------
        //3. Part 3
        
        // 3.1
        p3_1= 1/2d;
       
        //3.2
        p3_2=Math.pow(a3, 2)/(2 * Math.pow(a4, 2));
        
        // 3.3
        p3_3=(4 * a2)/(3 * a4);
        
        // 3.4
        p3_4=( Math.pow(2, (1/3d))*(Math.pow(a2, 2) 
                                            - 3 * a3 * a1 
                                            + 12 * a4 * a0
                                           )
                    )/
                    (3 * a4 * Math.pow(
                                       (2 * Math.pow(a2, 3) 
                                        - 9 * a3 * a2 * a1 
                                        + 27 * a4 * Math.pow(a1, 2) 
                                        + 27 * Math.pow(a3, 2) * a0 
                                        - 72 * a4 * a2 * a0 
                                        + Math.sqrt(
                                                    -4 * Math.pow(
                                                                   (Math.pow(a2, 2) 
                                                                    - 3 * a3 * a1 
                                                                    + 12 * a4 * a0
                                                                    ),3
                                                                   ) 
                                                    + Math.pow(
                                                                 (2 * Math.pow(a2, 3) 
                                                                  - 9 * a3 * a2 * a1 
                                                                  + 27 * a4 * Math.pow(a1, 2) 
                                                                  + 27 * Math.pow(a3, 2) * a0 
                                                                  - 72 * a4 * a2 * a0
                                                                  ),2
                                                                 )
                                                      )
                                        ),(1/3d)
                                      )
                     );
        
        // 3.5
        p3_5= 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow(
                                                                  (2 * Math.pow(a2, 3) 
                                                                   - 9 * a3 * a2 * a1 
                                                                   + 27 * a4 * Math.pow(a1, 2) 
                                                                   + 27 * Math.pow(a3, 2) * a0 
                                                                   - 72 * a4 * a2 * a0 
                                                                   + Math.sqrt(
                                                                                -4 * Math.pow(
                                                                                               (Math.pow(a2, 2) 
                                                                                                - 3 * a3 * a1 
                                                                                                + 12 * a4 * a0
                                                                                                ),3
                                                                                              ) 
                                                                                + Math.pow(
                                                                                            (2 * Math.pow(a2, 3)
                                                                                             - 9 * a3 * a2 * a1 
                                                                                             + 27 * a4 * Math.pow(a1, 2) 
                                                                                             + 27 * Math.pow(a3, 2) * a0 
                                                                                             - 72 * a4 * a2 * a0
                                                                                             ),2
                                                                                            )
                                                                               )
                                                                   ),(1/3)
                                                                 );
        
        //3.6
        p3_6=(-(Math.pow(a3, 3)/Math.pow(a4, 3)) 
                     + (4 * a3 * a2)/Math.pow(a4, 2) 
                     - (8 * a1)/a4
                     )/
                     (  4 * Math.sqrt(
                                        Math.pow(a3, 2)/(4 * Math.pow(a4, 2)) 
                                        - (2 * a2)/(3 * a4) 
                                        + (Math.pow(2, (1/3d))* (Math.pow(a2, 2) - 3 * a3 * a1 + 12 * a4 * a0))
                                           /(3 * a4 * Math.pow(
                                                               (2 * Math.pow(a2, 3) 
                                                                - 9 * a3 * a2 * a1
                                                                + 27 * a4 * Math.pow(a1, 2) 
                                                                + 27 * Math.pow(a3, 2) * a0 
                                                                - 72 * a4 * a2 * a0 
                                                                + Math.sqrt(
                                                                            -4 * Math.pow(
                                                                                           (Math.pow(a2, 2) 
                                                                                            - 3 * a3 * a1 
                                                                                            + 12 * a4 * a0
                                                                                            ),3
                                                                                           ) 
                                                                             + Math.pow(
                                                                                          (2 * Math.pow(a2, 3) 
                                                                                           - 9 * a3 * a2 * a1 
                                                                                           + 27 * a4 * Math.pow(a1, 2) 
                                                                                           + 27 * Math.pow(a3, 2) * a0 
                                                                                           - 72 * a4 * a2 * a0
                                                                                           ),2
                                                                                          )
                                                                              )
                                                               ),(1/3d)
                                                              )
                                            ) 
                                        + 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow( 
                                                                                        (2 * Math.pow(a2, 3)
                                                                                         - 9 * a3 * a2 * a1 
                                                                                         + 27 * a4 * Math.pow(a1, 2) 
                                                                                         + 27 * Math.pow(a3, 2) * a0 
                                                                                         - 72 * a4 * a2 * a0 
                                                                                         + Math.sqrt(
                                                                                                      -4 * Math.pow(
                                                                                                                     (Math.pow(a2, 2) 
                                                                                                                      - 3 * a3 * a1 
                                                                                                                      + 12 * a4 * a0
                                                                                                                      ),3
                                                                                                                     ) 
                                                                                                       + Math.pow(
                                                                                                                     (2 * Math.pow(a2, 3) 
                                                                                                                      - 9 * a3 * a2 * a1 
                                                                                                                      + 27 * a4 * Math.pow(a1, 2) 
                                                                                                                      + 27 * Math.pow(a3, 2) * a0 
                                                                                                                      - 72 * a4 * a2 * a0
                                                                                                                      ),2
                                                                                                                   )
                                                                                                      )
                                                                                         ),(1/3d)
                                                                                     )
                                      )
                      );
        
        p3= p3_1 * Math.sqrt(p3_2 - p3_3 - p3_4 - p3_5 - p3_6);
        
        //System.out.println("p3_1: "+p3_1+"\np3_2: "+p3_2+" \np3_3: "+p3_3+" \np3_4: "+p3_4+" \np3_5: "+p3_5+" \np3_6: "+p3_6);
        
        x1=p1+p2-p3;
        //System.out.println("P1: "+p1+" P2: "+p2+" P3: "+p3+" \nX1:"+x1);
        
        //System.out.println("X1: "+x1);
       
        if( Double.isNaN(x1) ) {
            iRoot[1] = 0;
            Root[1] = 0;
        } 
        else {
            iRoot[1] = 1;
            Root[1] = x1;
        }
        
        // Root 3
        //----------------------------------------------------------------------
        
        //1. Part 1 
        p1=-a3/(4*a4);
        //        System.out.println("Parte 1: "+p1);
       
        
        //--------------------------------------------------------- 
       //2. Part 2
        
//       System.out.println("\nParte 2: p2_1 * Math.sqrt( p2_2  - p2_3 + p2_4 + p2_5)"); 
        
        // 2.1
        
        p2_1=1/2d;
 //       System.out.println("p2_1: "+p2_1);
        
        // 2.2
        
        p2_2=Math.pow(a3, 2)/(4 *  Math.pow(a4, 2));
//        System.out.println("p2_2: "+p2_2);
        
        // 2.3
        
        p2_3=(2 * a2)/(3 * a4);
//        System.out.println("p2_3: "+p2_3);
       
        // 2.4
 //       System.out.println("p2_4= p2_4_n/p2_4_d ");
       
        // 2.4 nominador
        p2_4_n=Math.pow(2d, (1/3d))*(Math.pow(a2, 2) - 3 * a3  * a1 + 12 * a4 * a0);
 //       System.out.println("p2_4_n: "+p2_4_n);
        
        
        // 2.4 denominador
 //       System.out.println("p2_4_d=3 * a * Math.pow((  p2_4_d_1 - p2_4_d_2 + p2_4_d_3 + p2_4_d_4 - p2_4_d_5  + Math.sqrt(p2_4_d_6)),(1/3d)); ");
        
        // 2.4.1
        p2_4_d_1= 2 * Math.pow(a2, 3);
 //       System.out.println("p2_4_d_1: "+p2_4_d_1);
        // 2.4.2
        p2_4_d_2= 9 * a3 * a2 * a1;
 //       System.out.println("p2_4_d_2: "+p2_4_d_2);
        // 2.4.3
        p2_4_d_3= 27 * a4 * Math.pow(a1, 2) ;
 //       System.out.println("p2_4_d_3: "+p2_4_d_3);
        // 2.4.4
        p2_4_d_4= 27 * Math.pow(a3, 2) * a0;
 //       System.out.println("p2_4_d_4: "+p2_4_d_4);
        // 2.4.5
        p2_4_d_5= 72 * a4 * a2 * a0;
 //       System.out.println("p2_4_d_5: "+p2_4_d_5);
        // 2.4.6        
 //       System.out.println("p2_4_d_6=p2_4_d_6_1 + p2_4_d_6_2");
        
        p2_4_d_6_1=4 * Math.pow(
                                        (Math.pow(a2, 2) 
                                        - 3 * a3 * a1 
                                        + 12 * a4 * a0),3
                                       );
         p2_4_d_6_2=Math.pow(
                                      (2 * Math.pow(a2, 3) 
                                       - 9 * a3 * a2 * a1 
                                       + 27 * a4 * Math.pow(a1, 2) 
                                       + 27 * Math.pow(a3, 2) * a0 
                                       - 72 * a4 * a2 * a0),2
                                     );
        
        p2_4_d_6 = - p2_4_d_6_1 + p2_4_d_6_2;
        
//        System.out.println("p2_4_d_6: "+p2_4_d_6);
        
        p2_4_d=3 * a4 * Math.pow(
                                        (  p2_4_d_1 
                                         - p2_4_d_2 
                                         + p2_4_d_3 
                                         + p2_4_d_4 
                                         - p2_4_d_5 
                                         + Math.sqrt(p2_4_d_6)
                                        )
                                       ,(1/3d));
        
 //       System.out.println(p2_4_d);
        
        p2_4= p2_4_n/p2_4_d;
        
        // 2.5
        
        p2_5= 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow(
                                                                  (2 * Math.pow(a2, 3) 
                                                                   - 9 * a3 * a2 * a1 
                                                                   + 27 * a4 * Math.pow(a1, 2) 
                                                                   + 27 * Math.pow(a3, 2) * a0 
                                                                   - 72 * a4 * a2 * a0 
                                                                   + Math.sqrt(
                                                                               -4 * Math.pow(
                                                                                               (Math.pow(a2, 2)
                                                                                                - 3 * a3 * a1 
                                                                                                + 12 * a4 * a0
                                                                                                )
                                                                                                ,3
                                                                                             ) 
                                                                               + Math.pow(
                                                                                           (2 * Math.pow(a2, 3) 
                                                                                            - 9 * a3 * a2 * a1 
                                                                                            + 27 * a4 * Math.pow(a1, 2) 
                                                                                            + 27 * Math.pow(a3, 2) * a0 
                                                                                            - 72 * a4 * a2 * a0
                                                                                            ),2
                                                                                           )
                                                                               )
                                                                  )
                                                                 ,(1/3d)
                                                                 );
        
        p2 = p2_1 * Math.sqrt( p2_2  + p2_3 + p2_4 + p2_5);
        
        
       
        //--------------------------------------------------------------------------------------------------------------
        //3. Part 3
        
        // 3.1
        p3_1= 1/2d;
       
        //3.2
        p3_2=Math.pow(a3, 2)/(2 * Math.pow(a4, 2));
        
        // 3.3
        p3_3=(4 * a2)/(3 * a4);
        
        // 3.4
        p3_4=( Math.pow(2, (1/3d))*(Math.pow(a2, 2) 
                                            - 3 * a3 * a1 
                                            + 12 * a4 * a0
                                           )
                    )/
                    (3 * a4 * Math.pow(
                                       (2 * Math.pow(a2, 3) 
                                        - 9 * a3 * a2 * a1 
                                        + 27 * a4 * Math.pow(a1, 2) 
                                        + 27 * Math.pow(a3, 2) * a0 
                                        - 72 * a4 * a2 * a0 
                                        + Math.sqrt(
                                                    -4 * Math.pow(
                                                                   (Math.pow(a2, 2) 
                                                                    - 3 * a3 * a1 
                                                                    + 12 * a4 * a0
                                                                    ),3
                                                                   ) 
                                                    + Math.pow(
                                                                 (2 * Math.pow(a2, 3) 
                                                                  - 9 * a3 * a2 * a1 
                                                                  + 27 * a4 * Math.pow(a1, 2) 
                                                                  + 27 * Math.pow(a3, 2) * a0 
                                                                  - 72 * a4 * a2 * a0
                                                                  ),2
                                                                 )
                                                      )
                                        ),(1/3d)
                                      )
                     );
        
        // 3.5
        p3_5= 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow(
                                                                  (2 * Math.pow(a2, 3) 
                                                                   - 9 * a3 * a2 * a1 
                                                                   + 27 * a4 * Math.pow(a1, 2) 
                                                                   + 27 * Math.pow(a3, 2) * a0 
                                                                   - 72 * a4 * a2 * a0 
                                                                   + Math.sqrt(
                                                                                -4 * Math.pow(
                                                                                               (Math.pow(a2, 2) 
                                                                                                - 3 * a3 * a1 
                                                                                                + 12 * a4 * a0
                                                                                                ),3
                                                                                              ) 
                                                                                + Math.pow(
                                                                                            (2 * Math.pow(a2, 3)
                                                                                             - 9 * a3 * a2 * a1 
                                                                                             + 27 * a4 * Math.pow(a1, 2) 
                                                                                             + 27 * Math.pow(a3, 2) * a0 
                                                                                             - 72 * a4 * a2 * a0
                                                                                             ),2
                                                                                            )
                                                                               )
                                                                   ),(1/3)
                                                                 );
        
        //3.6
        p3_6=(-(Math.pow(a3, 3)/Math.pow(a4, 3)) 
                     + (4 * a3 * a2)/Math.pow(a4, 2) 
                     - (8 * a1)/a4
                     )/
                     (  4 * Math.sqrt(
                                        Math.pow(a3, 2)/(4 * Math.pow(a4, 2)) 
                                        - (2 * a2)/(3 * a4) 
                                        + (Math.pow(2, (1/3d))* (Math.pow(a2, 2) - 3 * a3 * a1 + 12 * a4 * a0))
                                           /(3 * a4 * Math.pow(
                                                               (2 * Math.pow(a2, 3) 
                                                                - 9 * a3 * a2 * a1
                                                                + 27 * a4 * Math.pow(a1, 2) 
                                                                + 27 * Math.pow(a3, 2) * a0 
                                                                - 72 * a4 * a2 * a0 
                                                                + Math.sqrt(
                                                                            -4 * Math.pow(
                                                                                           (Math.pow(a2, 2) 
                                                                                            - 3 * a3 * a1 
                                                                                            + 12 * a4 * a0
                                                                                            ),3
                                                                                           ) 
                                                                             + Math.pow(
                                                                                          (2 * Math.pow(a2, 3) 
                                                                                           - 9 * a3 * a2 * a1 
                                                                                           + 27 * a4 * Math.pow(a1, 2) 
                                                                                           + 27 * Math.pow(a3, 2) * a0 
                                                                                           - 72 * a4 * a2 * a0
                                                                                           ),2
                                                                                          )
                                                                              )
                                                               ),(1/3d)
                                                              )
                                            ) 
                                        + 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow( 
                                                                                        (2 * Math.pow(a2, 3)
                                                                                         - 9 * a3 * a2 * a1 
                                                                                         + 27 * a4 * Math.pow(a1, 2) 
                                                                                         + 27 * Math.pow(a3, 2) * a0 
                                                                                         - 72 * a4 * a2 * a0 
                                                                                         + Math.sqrt(
                                                                                                      -4 * Math.pow(
                                                                                                                     (Math.pow(a2, 2) 
                                                                                                                      - 3 * a3 * a1 
                                                                                                                      + 12 * a4 * a0
                                                                                                                      ),3
                                                                                                                     ) 
                                                                                                       + Math.pow(
                                                                                                                     (2 * Math.pow(a2, 3) 
                                                                                                                      - 9 * a3 * a2 * a1 
                                                                                                                      + 27 * a4 * Math.pow(a1, 2) 
                                                                                                                      + 27 * Math.pow(a3, 2) * a0 
                                                                                                                      - 72 * a4 * a2 * a0
                                                                                                                      ),2
                                                                                                                   )
                                                                                                      )
                                                                                         ),(1/3d)
                                                                                     )
                                      )
                      );
        
        p3= p3_1 * Math.sqrt(p3_2 - p3_3 - p3_4 - p3_5 + p3_6);
        
//        System.out.println("p3_1: "+p3_1+"\np3_2: "+p3_2+" \np3_3: "+p3_3+" \np3_4: "+p3_4+" \np3_5: "+p3_5+" \np3_6: "+p3_6);
        
        x2 = p1 + p2 - p3;
     //   System.out.println("P1: "+p1+" P2: "+p2+" P3: "+p3+" \nX2:"+x2);
        System.out.println("X2: "+x2);
        
        if(Double.isNaN(x2))
          {
              iRoot[2]=0;
              Root[2]=0;
          }
          else
          {
              iRoot[2]=1;
              Root[2]=x2;
          }
              
        // Root 4
        //----------------------------------------------------------------------
        
       //1. Part 1 
        p1=-a3/(4*a4);
 //       System.out.println("Parte 1: "+p1);
       
        
        //--------------------------------------------------------- 
       //2. Part 2
        
  //     System.out.println("\nParte 2: p2_1 * Math.sqrt( p2_2  - p2_3 + p2_4 + p2_5)"); 
        
        // 2.1
        
        p2_1=1/2d;
 //       System.out.println("p2_1: "+p2_1);
        
        // 2.2
        
        p2_2=Math.pow(a3, 2)/(4 *  Math.pow(a4, 2));
 //       System.out.println("p2_2: "+p2_2);
        
        // 2.3
        
        p2_3=(2 * a2)/(3 * a4);
 //       System.out.println("p2_3: "+p2_3);
       
        // 2.4
//        System.out.println("p2_4= p2_4_n/p2_4_d ");
       
        // 2.4 nominador
        p2_4_n=Math.pow(2d, (1/3d))*(Math.pow(a2, 2) - 3 * a3  * a1 + 12 * a4 * a0);
//        System.out.println("p2_4_n: "+p2_4_n);
        
        
        // 2.4 denominador
 //       System.out.println("p2_4_d=3 * a * Math.pow((  p2_4_d_1 - p2_4_d_2 + p2_4_d_3 + p2_4_d_4 - p2_4_d_5  + Math.sqrt(p2_4_d_6)),(1/3d)); ");
        
        // 2.4.1
        p2_4_d_1= 2 * Math.pow(a2, 3);
 //       System.out.println("p2_4_d_1: "+p2_4_d_1);
        // 2.4.2
        p2_4_d_2= 9 * a3 * a2 * a1;
 //       System.out.println("p2_4_d_2: "+p2_4_d_2);
        // 2.4.3
        p2_4_d_3= 27 * a4 * Math.pow(a1, 2) ;
 //       System.out.println("p2_4_d_3: "+p2_4_d_3);
        // 2.4.4
        p2_4_d_4= 27 * Math.pow(a3, 2) * a0;
 //       System.out.println("p2_4_d_4: "+p2_4_d_4);
        // 2.4.5
        p2_4_d_5= 72 * a4 * a2 * a0;
 //       System.out.println("p2_4_d_5: "+p2_4_d_5);
        // 2.4.6        
 //       System.out.println("p2_4_d_6=p2_4_d_6_1 + p2_4_d_6_2");
        
        p2_4_d_6_1=4 * Math.pow(
                                        (Math.pow(a2, 2) 
                                        - 3 * a3 * a1 
                                        + 12 * a4 * a0),3
                                       );
         p2_4_d_6_2=Math.pow(
                                      (2 * Math.pow(a2, 3) 
                                       - 9 * a3 * a2 * a1 
                                       + 27 * a4 * Math.pow(a1, 2) 
                                       + 27 * Math.pow(a3, 2) * a0 
                                       - 72 * a4 * a2 * a0),2
                                     );
        
        p2_4_d_6 = - p2_4_d_6_1 + p2_4_d_6_2;
        
//        System.out.println("p2_4_d_6: "+p2_4_d_6);
        
        p2_4_d=3 * a4 * Math.pow(
                                        (  p2_4_d_1 
                                         - p2_4_d_2 
                                         + p2_4_d_3 
                                         + p2_4_d_4 
                                         - p2_4_d_5 
                                         + Math.sqrt(p2_4_d_6)
                                        )
                                       ,(1/3d));
        
 //       System.out.println(p2_4_d);
        
        p2_4= p2_4_n/p2_4_d;
        
        // 2.5
        
        p2_5= 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow(
                                                                  (2 * Math.pow(a2, 3) 
                                                                   - 9 * a3 * a2 * a1 
                                                                   + 27 * a4 * Math.pow(a1, 2) 
                                                                   + 27 * Math.pow(a3, 2) * a0 
                                                                   - 72 * a4 * a2 * a0 
                                                                   + Math.sqrt(
                                                                               -4 * Math.pow(
                                                                                               (Math.pow(a2, 2)
                                                                                                - 3 * a3 * a1 
                                                                                                + 12 * a4 * a0
                                                                                                )
                                                                                                ,3
                                                                                             ) 
                                                                               + Math.pow(
                                                                                           (2 * Math.pow(a2, 3) 
                                                                                            - 9 * a3 * a2 * a1 
                                                                                            + 27 * a4 * Math.pow(a1, 2) 
                                                                                            + 27 * Math.pow(a3, 2) * a0 
                                                                                            - 72 * a4 * a2 * a0
                                                                                            ),2
                                                                                           )
                                                                               )
                                                                  )
                                                                 ,(1/3d)
                                                                 );
        
        p2 = p2_1 * Math.sqrt( p2_2  + p2_3 + p2_4 + p2_5);
        
        
       
        //--------------------------------------------------------------------------------------------------------------
        //3. Part 3
        
        // 3.1
        p3_1= 1/2d;
       
        //3.2
        p3_2=Math.pow(a3, 2)/(2 * Math.pow(a4, 2));
        
        // 3.3
        p3_3=(4 * a2)/(3 * a4);
        
        // 3.4
        p3_4=( Math.pow(2, (1/3d))*(Math.pow(a2, 2) 
                                            - 3 * a3 * a1 
                                            + 12 * a4 * a0
                                           )
                    )/
                    (3 * a4 * Math.pow(
                                       (2 * Math.pow(a2, 3) 
                                        - 9 * a3 * a2 * a1 
                                        + 27 * a4 * Math.pow(a1, 2) 
                                        + 27 * Math.pow(a3, 2) * a0 
                                        - 72 * a4 * a2 * a0 
                                        + Math.sqrt(
                                                    -4 * Math.pow(
                                                                   (Math.pow(a2, 2) 
                                                                    - 3 * a3 * a1 
                                                                    + 12 * a4 * a0
                                                                    ),3
                                                                   ) 
                                                    + Math.pow(
                                                                 (2 * Math.pow(a2, 3) 
                                                                  - 9 * a3 * a2 * a1 
                                                                  + 27 * a4 * Math.pow(a1, 2) 
                                                                  + 27 * Math.pow(a3, 2) * a0 
                                                                  - 72 * a4 * a2 * a0
                                                                  ),2
                                                                 )
                                                      )
                                        ),(1/3d)
                                      )
                     );
        
        // 3.5
        p3_5= 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow(
                                                                  (2 * Math.pow(a2, 3) 
                                                                   - 9 * a3 * a2 * a1 
                                                                   + 27 * a4 * Math.pow(a1, 2) 
                                                                   + 27 * Math.pow(a3, 2) * a0 
                                                                   - 72 * a4 * a2 * a0 
                                                                   + Math.sqrt(
                                                                                -4 * Math.pow(
                                                                                               (Math.pow(a2, 2) 
                                                                                                - 3 * a3 * a1 
                                                                                                + 12 * a4 * a0
                                                                                                ),3
                                                                                              ) 
                                                                                + Math.pow(
                                                                                            (2 * Math.pow(a2, 3)
                                                                                             - 9 * a3 * a2 * a1 
                                                                                             + 27 * a4 * Math.pow(a1, 2) 
                                                                                             + 27 * Math.pow(a3, 2) * a0 
                                                                                             - 72 * a4 * a2 * a0
                                                                                             ),2
                                                                                            )
                                                                               )
                                                                   ),(1/3)
                                                                 );
        
        //3.6
        p3_6=(-(Math.pow(a3, 3)/Math.pow(a4, 3)) 
                     + (4 * a3 * a2)/Math.pow(a4, 2) 
                     - (8 * a1)/a4
                     )/
                     (  4 * Math.sqrt(
                                        Math.pow(a3, 2)/(4 * Math.pow(a4, 2)) 
                                        - (2 * a2)/(3 * a4) 
                                        + (Math.pow(2, (1/3d))* (Math.pow(a2, 2) - 3 * a3 * a1 + 12 * a4 * a0))
                                           /(3 * a4 * Math.pow(
                                                               (2 * Math.pow(a2, 3) 
                                                                - 9 * a3 * a2 * a1
                                                                + 27 * a4 * Math.pow(a1, 2) 
                                                                + 27 * Math.pow(a3, 2) * a0 
                                                                - 72 * a4 * a2 * a0 
                                                                + Math.sqrt(
                                                                            -4 * Math.pow(
                                                                                           (Math.pow(a2, 2) 
                                                                                            - 3 * a3 * a1 
                                                                                            + 12 * a4 * a0
                                                                                            ),3
                                                                                           ) 
                                                                             + Math.pow(
                                                                                          (2 * Math.pow(a2, 3) 
                                                                                           - 9 * a3 * a2 * a1 
                                                                                           + 27 * a4 * Math.pow(a1, 2) 
                                                                                           + 27 * Math.pow(a3, 2) * a0 
                                                                                           - 72 * a4 * a2 * a0
                                                                                           ),2
                                                                                          )
                                                                              )
                                                               ),(1/3d)
                                                              )
                                            ) 
                                        + 1/(3 * Math.pow(2, (1/3d)) * a4) * Math.pow( 
                                                                                        (2 * Math.pow(a2, 3)
                                                                                         - 9 * a3 * a2 * a1 
                                                                                         + 27 * a4 * Math.pow(a1, 2) 
                                                                                         + 27 * Math.pow(a3, 2) * a0 
                                                                                         - 72 * a4 * a2 * a0 
                                                                                         + Math.sqrt(
                                                                                                      -4 * Math.pow(
                                                                                                                     (Math.pow(a2, 2) 
                                                                                                                      - 3 * a3 * a1 
                                                                                                                      + 12 * a4 * a0
                                                                                                                      ),3
                                                                                                                     ) 
                                                                                                       + Math.pow(
                                                                                                                     (2 * Math.pow(a2, 3) 
                                                                                                                      - 9 * a3 * a2 * a1 
                                                                                                                      + 27 * a4 * Math.pow(a1, 2) 
                                                                                                                      + 27 * Math.pow(a3, 2) * a0 
                                                                                                                      - 72 * a4 * a2 * a0
                                                                                                                      ),2
                                                                                                                   )
                                                                                                      )
                                                                                         ),(1/3d)
                                                                                     )
                                      )
                      );
        
        p3= p3_1 * Math.sqrt(p3_2 - p3_3 - p3_4 - p3_5 + p3_6);
        
 //       System.out.println("p3_1: "+p3_1+"\np3_2: "+p3_2+" \np3_3: "+p3_3+" \np3_4: "+p3_4+" \np3_5: "+p3_5+" \np3_6: "+p3_6);
        
        x3 = p1 + p2 + p3;
        //System.out.println("P1: "+p1+" P2: "+p2+" P3: "+p3+" \nX3:"+x3);
        System.out.println("X3: "+x3);
        
        if ( Double.isNaN(x3) ) {
              iRoot[3]=0;
              Root[3]=0;
          }
          else {
              iRoot[3]=1;
              Root[3]=x3;
        }
        
        // Check which of the roots found is closest to the ray point
        
        double root = 0;
        int cont = 0;
        
        for ( int i = 0; i < 4; i++ ) {
            if ( iRoot[i] == 0 ) {
            }
            else {
                if ( cont == 0 ) {
                    root = Root[i];
                    cont++;
                }
                else {
                    if ( root < Root[i] ) {
                    }
                    else {
                        root = Root[i];
		    }
                }
                
                
            }
        }

        if ( cont == 0 ) {
            return false;
        }
        else {
                ray = ray.withT(root);
         //       System.out.println("Raiz: "+root);
                return true;
        }
    }
    */
    
    public void doExtraInformation(
        Ray inRay, double intT, RayHit outData) 
    {
        if ( outData == null ) {
            return;
        }
        Vector3Dd hitPoint = new Vector3Dd(
            inRay.getOrigin().x() + intT*inRay.getDirection().x(),
            inRay.getOrigin().y() + intT*inRay.getDirection().y(),
            inRay.getOrigin().z() + intT*inRay.getDirection().z());
        outData.point = hitPoint;
        double r2=minorRadius*minorRadius;
        double R2=majorRadius*majorRadius;
        double hitNormSquared =
            hitPoint.x()*hitPoint.x() +
            hitPoint.y()*hitPoint.y() +
            hitPoint.z()*hitPoint.z();
     
        outData.normal = new Vector3Dd(
            4 * hitPoint.x() * (hitNormSquared - r2 - R2),
            4 * hitPoint.y() * (hitNormSquared - r2 - R2),
            4 * hitPoint.z() * (hitNormSquared - r2 - R2) + 8 * R2 * hitPoint.z()
        ).normalized();
    }

    @Override
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid()
    {
        return buildPolyhedralBoundedSolid(DEFAULT_MAJOR_DIVISIONS,
            DEFAULT_MINOR_DIVISIONS);
    }

    /**
    @param majorDivisions number of sections along the major circle, around
    the Z axis (at least 3)
    @param minorDivisions number of sides of each section, around the tube
    (at least 3)
    @return a new polyhedral approximation of current torus
    */
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid(
        int majorDivisions, int minorDivisions)
    {
        int normalizedMajorDivisions = Math.max(MIN_MAJOR_DIVISIONS,
            majorDivisions);
        int normalizedMinorDivisions = Math.max(MIN_MINOR_DIVISIONS,
            minorDivisions);

        if ( normalizedMajorDivisions == DEFAULT_MAJOR_DIVISIONS &&
             normalizedMinorDivisions == DEFAULT_MINOR_DIVISIONS ) {
            return exportToPolyhedralBoundedSolid();
        }

        return buildPolyhedralBoundedSolid(normalizedMajorDivisions,
            normalizedMinorDivisions);
    }

    /**
    Builds a "polyhedral torus" approximation by a closed rotational sweep of
    a circular lamina, following the idea of section [MANT1988].12.5 and
    program [MANT1988].12.11, but expressed with the generalized sweep of
    `PolyhedralBoundedSolidEulerConstructions` (each step is a rotation about
    the Z axis instead of a translation):
      - A circular lamina of `minorDivisions` vertices and radius
        `minorRadius` is placed on the XZ plane, centered at
        (`majorRadius`, 0, 0), with its face 1 looking along the sweep (+Y).
      - Face 1 is swept `majorDivisions` times, each one rotated
        2*PI/`majorDivisions` about Z. After the last step its vertices
        coincide with the ones of the other face of the lamina.
      - The two coincident faces are merged as two loops of one face
        (`lkfmrh`) and glued (`loopGlue`, [MANT1988].12.4.2), closing the
        handle of the genus 1 solid.

    The side faces are planar: the four vertices of each one are two points
    of a section and their rotated copies, an isosceles trapezoid symmetric
    about the bisector plane of both sections.
    */
    private PolyhedralBoundedSolid buildPolyhedralBoundedSolid(
        int majorDivisions, int minorDivisions)
    {
        PolyhedralBoundedSolid solid;
        Matrix4x4d T;
        Matrix4x4d R;
        Matrix4x4d M;
        Matrix4x4d step;
        int i;

        //- Section lamina, from the XY plane to the XZ plane -------------
        solid = PolyhedralBoundedSolidEulerConstructions.createCircularLamina(
            0.0, 0.0, minorRadius, 0.0, minorDivisions);
        T = new Matrix4x4d();
        T = T.translation(majorRadius, 0.0, 0.0);
        R = new Matrix4x4d();
        R = R.axisRotation(-Math.PI/2.0, 1, 0, 0);
        M = T.multiply(R);
        for ( i = 0; i < solid.getVerticesList().size(); i++ ) {
            solid.getVerticesList().get(i).position =
                M.multiply(solid.getVerticesList().get(i).position);
        }

        //- Rotational sweep of face 1 about the Z axis -------------------
        step = new Matrix4x4d();
        step = step.axisRotation(2.0*Math.PI / ((double)majorDivisions),
            0, 0, 1);
        for ( i = 0; i < majorDivisions; i++ ) {
            PolyhedralBoundedSolidEulerConstructions.translationalSweepExtrudeFace(
                solid, solid.findFace(1), step);
        }

        //- Close the ring: glue the swept face to the starting one -------
        _PolyhedralBoundedSolidFace headFace = solid.findFace(2);
        _PolyhedralBoundedSolidFace tailFace = solid.findFace(1);
        PolyhedralBoundedSolidEulerOperators.lkfmrh(solid, headFace, tailFace);
        PolyhedralBoundedSolidTopologyEditing.loopGlue(solid, headFace.id);
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);

        return solid;
    }

    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    @Override
    public double[] getMinMax() {
        double [] minmax = new double[6];
        
        minmax[0] = -(majorRadius + minorRadius);
        minmax[1] = -(majorRadius+minorRadius);
        minmax[2] = -minorRadius;
        minmax[3] = majorRadius + minorRadius;
        minmax[4] = majorRadius + minorRadius;
        minmax[5] = minorRadius;

        return minmax;
    }    
}
