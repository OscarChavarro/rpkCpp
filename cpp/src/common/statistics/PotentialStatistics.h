#ifndef POTENTIAL_STATISTICS__
#define POTENTIAL_STATISTICS__

class PotentialStatistics {
  public:
    double averageDirectPotential;
    double maxDirectPotential;
    double maxDirectImportance; // Potential times area
    double totalDirectPotential;

    PotentialStatistics();
    void reset();
};

#endif
