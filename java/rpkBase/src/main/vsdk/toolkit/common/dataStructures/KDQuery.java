package vsdk.toolkit.common.dataStructures;

public class KDQuery {
    public float[] point;
    public int wantedN;
    public int foundN;
    public boolean notFilled;
    public Object[] results;
    public float[] distances;
    public float maximumDistance;
    public float sqrRadius;
    public short excludeFlags;

    public KDQuery() {
    }

    public void print() {
        System.out.printf("Point X %g, Y %g, Z %g%n", point[0], point[1], point[2]);
        System.out.printf("Wanted N: %d, found N: %d%n", wantedN, foundN);
        System.out.printf("maximumDistance %g%n", maximumDistance);
        System.out.printf("sqrRadius %g%n", sqrRadius);
        System.out.printf("excludeFlags %x%n", (int)excludeFlags);
    }
}
