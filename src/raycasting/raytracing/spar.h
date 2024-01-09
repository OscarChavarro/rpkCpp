/*
 * spar.H : specification of the Stored Partial Radiance class
 */

#ifndef _SPAR_H_
#define _SPAR_H_

#include "material/color.h"

#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/bipath.h"
#include "raycasting/raytracing/flagchain.h"

#define MAXPATHGROUPS 2

// Some global path groups

#define DISJUNCTGROUP 0
#define LDGROUP 1

typedef int TPathGroupID;

class CSpar;


// Spar Config stores handy config params.

class CSparConfig {
public:
    BP_BASECONFIG *m_bcfg;

    // Needed in weighted multipass methods
    CSpar *m_leSpar;
    CSpar *m_ldSpar;
};


class CSparList : public CTSList<CSpar *> {
public:
    virtual COLOR HandlePath(CSparConfig *config,
                             CBiPath *path);

    virtual void HandlePath(CSparConfig *sconfig,
                            CBiPath *path, COLOR *frad, COLOR *fbpt);
    virtual ~CSparList() {};
};

// iterator

typedef CTSList_Iter<CSpar *> CSparListIter;


class CSpar {
    // members
public:
    CContribHandler *m_contrib;
    CSparList *m_sparList;

    // methods
public:

    // Constructor
    CSpar();

    virtual void Init(CSparConfig *config);

    // mainInit spar with a comma separated list of regular expressions
    virtual void ParseAndInit(int group, char *regExp);

    // Destructor
    virtual ~CSpar();

    // photonMapHandlePath : Handles a bidirectional path. Image contribution
    // is returned. Normally this is a contribution for the pixel
    // affected by the path

    virtual COLOR HandlePath(CSparConfig *config,
                             CBiPath *path);


    // Compute weight terms : computes the sum of partial weight terms Ci.
    // The number of terms included depends on the number of PDF's used
    // for path sampling in this spar (== number of accepted paths in photonMapHandlePath)
    // ID indicates the path group under consideration.
    // returns the weight Ci for the pdf generating the supplied path
    // 'sum' is filled with the sum Ci for all pdf's that sample this spar
    virtual double ComputeWeightTerms(TPathGroupID ID, CSparConfig *config,
                                      CBiPath *path, double *sum);

    // IsCoveredID : Test if the handled paths by this spar cover the path group
    // defined by the ID

    virtual bool IsCoveredID(TPathGroupID ID);

    // Evaluate radiance function with a certain contrib handler
    virtual COLOR EvalFunction(CContribHandler *contrib,
                               CBiPath *path);

    // return weight/pdf
    virtual double EvalPDFAndWeight(CSparConfig *config,
                                    CBiPath *path);

    // GetStoredRadiance : get the stored radiance
    virtual void GetStoredRadiance(CPathNode *node);
};

/* Le Spar : Uses emission ase stored radiance. Allows sampling of
 *   of all bidirectional paths
 */

class CLeSpar : public CSpar {
public:
    virtual void Init(CSparConfig *config);

    virtual COLOR HandlePath(CSparConfig *config,
                             CBiPath *path);

    virtual double ComputeWeightTerms(TPathGroupID ID, CSparConfig *config,
                                      CBiPath *path, double *sum);

    // standard bpt weights, no other spars
    virtual double EvalPDFAndWeight(CSparConfig *config,
                                    CBiPath *path);

    virtual double EvalPDFAndMPWeight(CSparConfig *config,
                                      CBiPath *path);

    virtual void GetStoredRadiance(CPathNode *node);
};


/* LD Spar : Uses direct diffuse as stored radiance. Allows sampling of
 *   of eye paths. GetDirectRadiance is used as a readout function
 */

class CLDSpar : public CSpar {
public:
    virtual void Init(CSparConfig *config);

    virtual COLOR HandlePath(CSparConfig *config,
                             CBiPath *path);

    virtual double ComputeWeightTerms(TPathGroupID ID, CSparConfig *config,
                                      CBiPath *path, double *sum);

    virtual double EvalPDFAndMPWeight(CSparConfig *config,
                                      CBiPath *path);

    // standard bpt weights, no other spars
    virtual double EvalPDFAndWeight(CSparConfig *config,
                                    CBiPath *path);

    virtual void GetStoredRadiance(CPathNode *node);
};


/* ID Spar : Uses indirect direct diffuse as stored radiance. Allows
 * sampling of of eye paths. GetDirectRadiance is used as a readout
 * function 
 */

class CIDSpar : public CSpar {
public:
    virtual void Init(CSparConfig *config);

    virtual COLOR HandlePath(CSparConfig *config,
                             CBiPath *path);

    // standard bpt weights, no other spars
    //virtual double EvalPDFAndWeight(CSparConfig *config,
    //		    CBiPath *path);

    virtual void GetStoredRadiance(CPathNode *node);
};

#endif /* _SPAR_H_ */
