#include "common/error.h"
#include "material/statistics.h"
#include "skin/RadianceMethod.h"
#include "raycasting/bidirectionalRaytracing/spar.h"

Spar::Spar() {
    m_contrib = new CContribHandler[MAX_PATH_GROUPS];
    m_sparList = new CSparList[MAX_PATH_GROUPS];
}

Spar::~Spar() {
    delete[] m_contrib;
    delete[] m_sparList;
}

void
Spar::init(SparConfig *config) {
    for ( int i = 0; i < MAX_PATH_GROUPS; i++ ) {
        m_contrib[i].init(config->baseConfig->maximumPathDepth);
        m_sparList[i].removeAll();
    }
}

/**
MainInit spar with a comma separated list of regular expressions
*/
void
Spar::parseAndInit(int group, char *regExp) {
    int beginPos = 0, endPos = 0;
    char tmpChar;

    while ( regExp[endPos] != '\0' ) {
        if ( regExp[endPos] == ',' ) {
            // Next RegExp
            tmpChar = regExp[endPos];
            regExp[endPos] = '\0';

            m_contrib[group].addRegExp(regExp + beginPos);

            regExp[endPos] = tmpChar; // Restore
            beginPos = endPos + 1; // Begin next regexp
        }

        endPos++;
    }

    // Still parse last regexp in list
    if ( beginPos != endPos ) {
        m_contrib[group].addRegExp(regExp + beginPos);
    }
}

/**
Handles a bidirectional path. Image contribution
is returned. Normally this is a contribution for the pixel
affected by the path
*/
COLOR
Spar::handlePath(SparConfig *config, CBiPath *path) {
    COLOR result;

    colorClear(result);

    return result;
}

void
LeSpar::init(SparConfig *sparConfig) {
    Spar::init(sparConfig);

    // Disjoint path group for BPT
    if ( sparConfig->baseConfig->doLe ) {
        parseAndInit(DISJOINT_GROUP, sparConfig->baseConfig->leRegExp);
    }

    if ( sparConfig->baseConfig->doWeighted ) {
        parseAndInit(LD_GROUP, sparConfig->baseConfig->wleRegExp);
        m_sparList[LD_GROUP].add(sparConfig->ldSpar);
    }
}

void
LDSpar::init(SparConfig *sparConfig) {
    Spar::init(sparConfig);

    if ( !(sparConfig->baseConfig->doLD || sparConfig->baseConfig->doWeighted)) {
        return;
    }

    if ( !GLOBAL_radiance_currentRadianceMethodHandle ) {
        logError("CLDSpar::mainInit", "Galerkin Radiance method not active !");
    }

    // Overlap group
    if ( sparConfig->baseConfig->doLD ) {
        parseAndInit(DISJOINT_GROUP, sparConfig->baseConfig->ldRegExp);
    }

    if ( sparConfig->baseConfig->doWeighted ) {
        parseAndInit(LD_GROUP, sparConfig->baseConfig->wldRegExp);
        m_sparList[LD_GROUP].add(sparConfig->leSpar);
    }
}

void
CSparList::handlePath(
    SparConfig *config,
    CBiPath *path,
    COLOR *fRad,
    COLOR *fBpt)
{
    CSparListIter iter(*this);
    Spar **spar;
    COLOR col;

    colorClear(*fBpt);
    colorClear(*fRad);

    while ( (spar = iter.nextOnSequence()) ) {
        col = (*spar)->handlePath(config, path);

        if ( *spar == config->leSpar ) {
            colorAdd(col, *fBpt, *fBpt);
        } else {
            colorAdd(col, *fRad, *fRad);
        }
    }
}
