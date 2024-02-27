/**
Specification of the Stored Partial Radiance class
*/

#ifndef _SPAR_H_
#define _SPAR_H_

#include "common/color.h"

#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/bipath.h"
#include "raycasting/raytracing/flagchain.h"

#define MAXPATHGROUPS 2

// Some global path groups

#define DISJUNCTGROUP 0
#define LDGROUP 1

typedef int TPathGroupID;

class CSpar;

// Spar Config stores handy config params
class CSparConfig {
public:
    BP_BASECONFIG *m_bcfg;

    // Needed in weighted multi-pass methods
    CSpar *m_leSpar;
    CSpar *m_ldSpar;
};


class CSparList : public CTSList<CSpar *> {
public:
    virtual void handlePath(CSparConfig *sconfig,
                            CBiPath *path, COLOR *frad, COLOR *fbpt);
    virtual ~CSparList() {};
};

// iterator

typedef CTSList_Iter<CSpar *> CSparListIter;

class CSpar {
public:
    CContribHandler *m_contrib;
    CSparList *m_sparList;

public:
    CSpar();

    virtual void init(CSparConfig *config);

    // mainInit spar with a comma separated list of regular expressions
    virtual void parseAndInit(int group, char *regExp);

    // Destructor
    virtual ~CSpar();

    // photonMapHandlePath : Handles a bidirectional path. Image contribution
    // is returned. Normally this is a contribution for the pixel
    // affected by the path

    virtual COLOR handlePath(CSparConfig *config, CBiPath *path);

    // Compute weight terms : computes the sum of partial weight terms Ci.
    // The number of terms included depends on the number of PDF's used
    // for path sampling in this spar (== number of accepted paths in photonMapHandlePath)
    // ID indicates the path group under consideration.
    // returns the weight Ci for the pdf generating the supplied path
    // 'sum' is filled with the sum Ci for all pdf's that sample this spar
    virtual double computeWeightTerms(TPathGroupID ID, CSparConfig *config,
                                      CBiPath *path, double *sum);

    // Evaluate radiance function with a certain contrib handler
    virtual COLOR evalFunction(CContribHandler *contrib,
                               CBiPath *path);

    // return weight/pdf
    virtual double evalPdfAndWeight(CSparConfig *config,
                                    CBiPath *path);

    // GetStoredRadiance : get the stored radiance
    virtual void getStoredRadiance(CPathNode *node);
};

/**
Le Spar : Uses emission ase stored radiance. Allows sampling of
all bidirectional paths
*/
class CLeSpar : public CSpar {
public:
    virtual void init(CSparConfig *config);
    virtual COLOR handlePath(CSparConfig *config, CBiPath *path);
    virtual double computeWeightTerms(TPathGroupID ID, CSparConfig *config,
                                      CBiPath *path, double *sum);

    // standard bpt weights, no other spars
    virtual double evalPdfAndWeight(CSparConfig *config, CBiPath *path);
    virtual double evalPdfAndMpWeight(CSparConfig *config, CBiPath *path);
    virtual void getStoredRadiance(CPathNode *node);
};

/**
LD Spar : Uses direct diffuse as stored radiance. Allows sampling of
of eye paths. GetDirectRadiance is used as a readout function
*/
class CLDSpar : public CSpar {
public:
    virtual void init(CSparConfig *config);

    virtual COLOR handlePath(CSparConfig *config, CBiPath *path);

    virtual double computeWeightTerms(TPathGroupID ID, CSparConfig *config,
                                      CBiPath *path, double *sum);

    virtual double EvalPDFAndMPWeight(CSparConfig *config,
                                      CBiPath *path);

    // Standard bpt weights, no other spars
    virtual double evalPdfAndWeight(CSparConfig *config, CBiPath *path);
    virtual void getStoredRadiance(CPathNode *node);
};

#endif
