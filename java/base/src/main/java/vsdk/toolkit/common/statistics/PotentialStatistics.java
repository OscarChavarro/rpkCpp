package vsdk.toolkit.common.statistics;

// Potential times area

public class PotentialStatistics {
    public double averageDirectPotential;
    public double maxDirectPotential;
    public double maxDirectImportance;
    public double totalDirectPotential;

    public PotentialStatistics() {
        averageDirectPotential = 0.0;
        maxDirectPotential = 0.0;
        maxDirectImportance = 0.0;
        totalDirectPotential = 0.0;
    }

    public void reset() {
        averageDirectPotential = 0.0;
        maxDirectPotential = 0.0;
        maxDirectImportance = 0.0;
        totalDirectPotential = 0.0;
    }
}
