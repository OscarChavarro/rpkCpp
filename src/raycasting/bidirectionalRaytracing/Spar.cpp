#include "common/error.h"
#include "common/Statistics.h"
#include "scene/RadianceMethod.h"
#include "raycasting/bidirectionalRaytracing/Spar.h"

Spar::Spar() {
    m_contrib = new ContribHandler[MAX_PATH_GROUPS];
    m_sparList = new SparList[MAX_PATH_GROUPS];
}

Spar::~Spar() {
    delete[] m_contrib;
    delete[] m_sparList;
}

void
Spar::init(SparConfig *config, RadianceMethod *radianceMethod) {
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
    int beginPos = 0;
    int endPos = 0;
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
ColorRgb
Spar::handlePath(SparConfig *config, CBiPath *path) {
    ColorRgb result;

    result.clear();

    return result;
}

void
LeSpar::init(SparConfig *sparConfig, RadianceMethod *radianceMethod) {
    Spar::init(sparConfig, radianceMethod);

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
LDSpar::init(SparConfig *sparConfig, RadianceMethod *radianceMethod) {
    Spar::init(sparConfig, radianceMethod);

    if ( !(sparConfig->baseConfig->doLD || sparConfig->baseConfig->doWeighted) ) {
        return;
    }

    if ( radianceMethod == nullptr ) {
        logError("CLDSpar::mainInitApplication", "Galerkin Radiance method not active !");
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
SparList::handlePath(
    SparConfig *config,
    CBiPath *path,
    ColorRgb *fRad,
    ColorRgb *fBpt)
{
    CSparListIter iter(*this);
    Spar **spar;
    ColorRgb col;

    fBpt->clear();
    fRad->clear();

    while ( (spar = iter.nextOnSequence()) ) {
        col = (*spar)->handlePath(config, path);

        if ( *spar == config->leSpar ) {
            fBpt->add(col, *fBpt);
        } else {
            fRad->add(col, *fRad);
        }
    }
}

SparList::~SparList() {
}
