package vsdk.toolkit.environment.geometry.element;

public class RayCrossingQuery {
    public double minT, maxT, minimumChordSquared, minimumExteriorApproach;
    public RayCrossingQuery(double minT, double maxT) { this(minT, maxT, 0, Double.NEGATIVE_INFINITY); }
    public RayCrossingQuery(double minT, double maxT, double minimumChordSquared) { this(minT, maxT, minimumChordSquared, Double.NEGATIVE_INFINITY); }
    public RayCrossingQuery(double minT, double maxT, double minimumChordSquared, double minimumExteriorApproach) {
        this.minT = minT; this.maxT = maxT; this.minimumChordSquared = minimumChordSquared; this.minimumExteriorApproach = minimumExteriorApproach;
    }
}
