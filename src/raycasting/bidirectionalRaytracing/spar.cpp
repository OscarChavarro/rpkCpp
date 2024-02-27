#include "common/error.h"
#include "material/statistics.h"
#include "GALERKIN/GalerkinRadiosity.h"
#include "raycasting/bidirectionalRaytracing/spar.h"

CSpar::CSpar() {
    m_contrib = new CContribHandler[MAXPATHGROUPS];
    m_sparList = new CSparList[MAXPATHGROUPS];
}

CSpar::~CSpar() {
    delete[] m_contrib;
    delete[] m_sparList;
}

void
CSpar::init(CSparConfig *config) {
    for ( int i = 0; i < MAXPATHGROUPS; i++ ) {
        m_contrib[i].Init(config->m_bcfg->maximumPathDepth);
        m_sparList[i].RemoveAll();
    }
}

void
CSpar::parseAndInit(int group, char *regExp) {
    int beginPos = 0, endPos = 0;
    char tmpChar;

    while ( regExp[endPos] != '\0' ) {
        if ( regExp[endPos] == ',' ) {
            // Next RegExp
            tmpChar = regExp[endPos];
            regExp[endPos] = '\0';

            m_contrib[group].AddRegExp(regExp + beginPos);

            regExp[endPos] = tmpChar; // Restore
            beginPos = endPos + 1; // Begin next regexp
        }

        endPos++;
    }

    // Still parse last regexp in list
    if ( beginPos != endPos ) {
        m_contrib[group].AddRegExp(regExp + beginPos);
    }
}

COLOR
CSpar::handlePath(CSparConfig *config, CBiPath *path) {
    COLOR result;

    colorClear(result);

    return result;
}

void
CLeSpar::init(CSparConfig *sconfig) {
    CSpar::init(sconfig);

    // Disjunct path group for BPT
    if ( sconfig->m_bcfg->doLe ) {
        parseAndInit(DISJUNCTGROUP, sconfig->m_bcfg->leRegExp);
    }

    if ( sconfig->m_bcfg->doWeighted ) {
        parseAndInit(LDGROUP, sconfig->m_bcfg->wleRegExp);
        m_sparList[LDGROUP].Add(sconfig->m_ldSpar);
    }
}

void
CLDSpar::init(CSparConfig *sconfig) {
    CSpar::init(sconfig);

    if ( !(sconfig->m_bcfg->doLD || sconfig->m_bcfg->doWeighted)) {
        return;
    }

    if ( !GLOBAL_radiance_currentRadianceMethodHandle ) {
        logError("CLDSpar::mainInit", "Galerkin Radiance method not active !");
    }

    // Overlap group
    if ( sconfig->m_bcfg->doLD ) {
        parseAndInit(DISJUNCTGROUP, sconfig->m_bcfg->ldRegExp);
    }

    if ( sconfig->m_bcfg->doWeighted ) {
        parseAndInit(LDGROUP, sconfig->m_bcfg->wldRegExp);
        m_sparList[LDGROUP].Add(sconfig->m_leSpar);
    }
}

void
CSparList::handlePath(
    CSparConfig *sconfig,
    CBiPath *path,
    COLOR *frad,
    COLOR *fbpt)
{
    CSparListIter iter(*this);
    CSpar **pspar;
    COLOR col;

    colorClear(*fbpt);
    colorClear(*frad);

    while ( (pspar = iter.Next()) ) {
        col = (*pspar)->handlePath(sconfig, path);

        if ( *pspar == sconfig->m_leSpar ) {
            colorAdd(col, *fbpt, *fbpt);
        } else {
            colorAdd(col, *frad, *frad);
        }
    }
}
