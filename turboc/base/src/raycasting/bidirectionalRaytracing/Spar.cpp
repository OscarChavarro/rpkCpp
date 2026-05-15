#include "common/logging/Logger.h"
#include "common/statistics/Statistics.h"
#include "raycasting/bidirectionalRaytracing/Spar.h"

Spar::Spar() {
    m_contrib = new ContribHandler[SPAR_MAX_PATH_GROUPS];
    m_sparList = new SparList[SPAR_MAX_PATH_GROUPS];
}

Spar::~Spar() {
    delete[] m_contrib;
    delete[] m_sparList;
}

void
Spar::init(SparConfig *config, RadianceMethod */*radianceMethod*/) {
    for ( int i = 0; i < SPAR_MAX_PATH_GROUPS; i++ ) {
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
Spar::handlePath(SparConfig */*config*/, BiPath */*path*/) {
    ColorRgb result;

    result = ColorRgb(0.0f, 0.0f, 0.0f);

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

    if ( radianceMethod == NULL ) {
        Logger::error("CLDSpar::mainInitApplication", "Galerkin Radiance method not active !");
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
    BiPath *path,
    ColorRgb *fRad,
    ColorRgb *fBpt)
{
    CSparListIter iter(*this);
    Spar **spar;
    ColorRgb col;
    float fBptR = 0.0f;
    float fBptG = 0.0f;
    float fBptB = 0.0f;
    float fRadR = 0.0f;
    float fRadG = 0.0f;
    float fRadB = 0.0f;

    *fBpt = ColorRgb(0.0f, 0.0f, 0.0f);
    *fRad = ColorRgb(0.0f, 0.0f, 0.0f);

    while ( (spar = iter.nextOnSequence()) ) {
        col = (*spar)->handlePath(config, path);

        if ( *spar == config->leSpar ) {
            fBptR += col.getR();
            fBptG += col.getG();
            fBptB += col.getB();
        } else {
            fRadR += col.getR();
            fRadG += col.getG();
            fRadB += col.getB();
        }
    }

    *fBpt = ColorRgb(fBptR, fBptG, fBptB);
    *fRad = ColorRgb(fRadR, fRadG, fRadB);
}

SparList::~SparList() {
}
