#include "common/error.h"
#include "material/PhongEmittanceDistributionFunction.h"
#include "scene/RadianceMethod.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/bidirectionalRaytracing/FlagChain.h"

void
FlagChain::init(const int paramLength, bool const paramSubtract) {
    if ( chain ) {
        delete[] chain;
    }

    if ( paramLength > 0 ) {
        chain = new BSDF_FLAGS[paramLength];
    } else {
        chain = nullptr;
    }

    this->length = paramLength;
    this->subtract = paramSubtract;

    for ( int i = 0; i < paramLength; i++ ) {
        if ( chain != nullptr ) {
            chain[i] = 0;
        }
    }
}

FlagChain::FlagChain(const int paramLength, const bool paramSubtract): length(), subtract() {
    chain = nullptr;

    init(paramLength, paramSubtract);
}

FlagChain::FlagChain(const FlagChain &c): length(), subtract() {
    chain = nullptr;
    init(c.length, c.subtract);

    for ( int i = 0; i < length; i++ ) {
        if ( chain != nullptr ) {
            chain[i] = c.chain[i];
        }
    }
}

FlagChain::~FlagChain() {
    delete[] chain;
}

bool
FlagChainCompare(const FlagChain *c1,
                      const FlagChain *c2) {
    // Determine if equal

    int nrDifferent = 0;
    int i;

    if ( (c1->length != c2->length) || (c1->subtract != c2->subtract) ) {
        return false;
    }

    for ( i = 0; (i < c1->length) && (nrDifferent == 0); i++ ) {
        if ( c1->chain[i] != c2->chain[i] ) {
            nrDifferent++;
        }
    }

    // combine into new chain

    if ( nrDifferent == 0 ) {
        // flag chains identical
        return true;
    }

    // Not combinable

    return false;
}

FlagChain *
FlagChainCombine(const FlagChain *c1,
                             const FlagChain *c2) {
    // Determine if combinable
    int nrDifferent = 0;
    int diffIndex = 0;

    if ( (c1->length != c2->length) || (c1->subtract != c2->subtract) ) {
        return nullptr;
    }

    for ( int i = 0; (i < c1->length) && (nrDifferent <= 1); i++ ) {
        if ( c1->chain[i] != c2->chain[i] ) {
            nrDifferent++;
            diffIndex = i;
        }
    }

    // Combine into new chain
    if ( nrDifferent == 0 ) {
        // Flag chains identical - maybe dangerous if someone wants to
        // count one contribution twice...
        return new FlagChain(*c1);
    }

    if ( nrDifferent == 1 ) {
        // Combinable
        FlagChain *newFlagChain = new FlagChain(*c1);
        newFlagChain->chain[diffIndex] = static_cast<BSDF_FLAGS>(c1->chain[diffIndex] | c2->chain[diffIndex]);
        return newFlagChain;
    }

    // Not combinable
    return nullptr;
}

/**
Compute : calculate the product of bsdf components defined
by the chain. Eye and light node ARE INCLUDED
*/
ColorRgb
FlagChain::compute(CBiPath *path) const {
    ColorRgb result, tmpCol;
    result.setMonochrome(1.0);
    int eyeSize = path->m_eyeSize;
    int lightSize = path->m_lightSize;

    SimpleRaytracingPathNode *node;

    if ( lightSize + eyeSize != length ) {
        logError("FlagChain::Compute", "Wrong path length");
        return result;
    }

    // Flag chain start at the light node and end at the eye node
    node = path->m_lightPath;

    for ( int i = 0; i < lightSize; i++ ) {
        tmpCol = node->m_bsdfComp.Sum(chain[i]);
        result.selfScalarProduct(tmpCol);
        node = node->next();
    }

    node = path->m_eyePath;

    for ( int i = 0; i < eyeSize; i++ ) {
        tmpCol = node->m_bsdfComp.Sum(chain[length - 1 - i]);
        result.selfScalarProduct(tmpCol);
        node = node->next();
    }

    if ( subtract ) {
        result.scale(-1);
    }

    return result;
}

ChainList::ChainList() {
    count = 0;
    length = 0;
}

ChainList::~ChainList() {
    removeAll();
}

void
ChainList::add(ChainList *list) {
    // Add all chains in 'list'
    FlagChainIterator iter(*list);
    FlagChain *tmpChain;

    while ( (tmpChain = iter.nextOnSequence()) != nullptr ) {
        add(*tmpChain);
    }
}

void
ChainList::add(const FlagChain &chain) {
    if ( count > 0 ) {
        if ( chain.length != length ) {
            logError("CChainList::add", "Wrong length flag chain inserted!");
            return;
        }
    } else {
        // first element
        length = chain.length;
    }

    count++;
    append(chain);
}

void
ChainList::addDisjoint(const FlagChain &chain) {
    if ( count > 0 ) {
        if ( chain.length != length ) {
            logError("CChainList::add", "Wrong length flag chain inserted!");
            return;
        }
    } else {
        // first element
        length = chain.length;
    }

    FlagChainIterator iter(*this);
    FlagChain *tmpChain;
    bool found = false;

    while ((tmpChain = iter.nextOnSequence()) && !found ) {
        found = FlagChainCompare(tmpChain, &chain);
    }

    if ( !found ) {
        count++;
        append(chain);
    }
}

ColorRgb
ChainList::compute(CBiPath *path) {
    ColorRgb result;
    ColorRgb tmpCol;

    result.clear();

    FlagChainIterator iter(*this);
    FlagChain *chain;

    while ( (chain = iter.nextOnSequence()) != nullptr ) {
        tmpCol = chain->compute(path);

        result.add(tmpCol, result);
    }

    return result;
}

/**
simplify the chain list returning the equivalent
simplified chain list. Equal entries MAY be reduced to a
single entry! (So in fact no equal entries is advisable)
*/
ChainList *
ChainList::simplify() {
    // Try a simple simplification scheme, just comparing pair wise chains
    ChainList *newList = new ChainList;
    FlagChain *c1;
    FlagChain *c2;
    FlagChain *cCombined;
    FlagChainIterator iter(*this);

    c1 = iter.nextOnSequence();

    if ( c1 ) {
        while ( (c2 = iter.nextOnSequence()) != nullptr ) {
            cCombined = FlagChainCombine(c1, c2);
            if ( cCombined ) {
                c1 = cCombined; // Combined
            } else {
                newList->add(*c1);
                c1 = c2;
            }
        }

        // Add final chain still in c1
        newList->add(*c1);
    }

    return newList;
}

ContribHandler::ContribHandler() {
    array = nullptr;
    maxLength = 0;
}

void
ContribHandler::init(int paramMaxLength) {
    this->maxLength = paramMaxLength;

    if ( array ) {
        delete[] array;
    }

    // For each length we need a chain list
    array = new ChainList[paramMaxLength + 1]; // 0 <= length <= maxlength !!
}

ContribHandler::~ContribHandler() {
    delete[] array;
}

ColorRgb
ContribHandler::compute(CBiPath *path) {
    ColorRgb result;
    int length;

    result.clear();

    length = path->m_eyeSize + path->m_lightSize;

    if ( length > maxLength ) {
        logError("CContribHandler::Compute", "Path too long !!");
        return result;
    }

    return array[length].compute(path);
}

void
ContribHandler::doRegExp(char *regExp, bool subtract) {
    doRegExpGeneral(regExp, subtract);
}

/**
add a group of paths
regExp indicates the regular expression covered by the sampling strategy
The class of covered paths covered by the contrib handler is : (regSPaR)(regPath)
regSPar is not needed here. The regExp must ensure disjoint paths!
*/
void
ContribHandler::addRegExp(char *regExp) {
    if ( regExp[0] == '-' ) {
        doRegExp(regExp + 1, true);
    } else {
        doRegExp(regExp, false);
    }
}

void
ContribHandler::doSyntaxError(const char *errString) {
    logError("Flag chain Syntax Error", errString);
    init(maxLength);
}

bool
ContribHandler::getFlags(const char *regExp, int *pos, BSDF_FLAGS *flags) {
    char c;
    int p = *pos;

    *flags = 0;

    if ( regExp[p++] != '(' ) {
        doSyntaxError("getFlags: '(' expected");
        return false;
    }

    while ((c = regExp[p++]) != ')' ) {
        //printf("Char '%c'\n", c);
        switch ( c ) {
            case 'S':
                switch ( regExp[p] ) {
                    case 'T':
                        p++;
                        *flags |= BTDF_SPECULAR_COMPONENT;
                        break;
                    case 'R':
                        p++;
                        *flags |= BRDF_SPECULAR_COMPONENT;
                        break;
                    default:
                        *flags |= BTDF_SPECULAR_COMPONENT | BRDF_SPECULAR_COMPONENT;
                        break;
                }
                break;
            case 'G':
                switch ( regExp[p] ) {
                    case 'T':
                        p++;
                        *flags |= BTDF_GLOSSY_COMPONENT;
                        break;
                    case 'R':
                        p++;
                        *flags |= BRDF_GLOSSY_COMPONENT;
                        break;
                    default:
                        *flags |= BTDF_GLOSSY_COMPONENT | BRDF_GLOSSY_COMPONENT;
                        break;
                }
                break;
            case 'D':
                switch ( regExp[p] ) {
                    case 'T':
                        p++;
                        *flags |= BTDF_DIFFUSE_COMPONENT;
                        break;
                    case 'R':
                        p++;
                        *flags |= BRDF_DIFFUSE_COMPONENT;
                        break;
                    default:
                        *flags |= BTDF_DIFFUSE_COMPONENT | BRDF_DIFFUSE_COMPONENT;
                        break;
                }
                break;
            case 'X':
                switch ( regExp[p] ) {
                    case 'T':
                        p++;
                        *flags |= (BTDF_DIFFUSE_COMPONENT | BTDF_GLOSSY_COMPONENT |
                                   BTDF_SPECULAR_COMPONENT);
                        break;
                    case 'R':
                        p++;
                        *flags |= (BRDF_DIFFUSE_COMPONENT | BRDF_GLOSSY_COMPONENT |
                                   BRDF_SPECULAR_COMPONENT);
                        break;
                    default:
                        *flags |= BSDF_ALL_COMPONENTS;
                        break;
                }
                break;
            case 'L':
                if ( regExp[p] != 'X' ) {
                    doSyntaxError("getFlags: No 'X' after 'L'. Only LX supported");
                    return false;
                }
                p++;
                *flags = BSDF_ALL_COMPONENTS;
                break;
            case 'E':
                if ( regExp[p] != 'X' ) {
                    doSyntaxError("getFlags: No 'X' after 'E'. Only EX supported");
                    return false;
                }
                p++;
                *flags = BSDF_ALL_COMPONENTS;
                break;
            case '|':
                break;  // Do Nothing because we don't support other operators
            default:
                doSyntaxError("getFlags: Unexpected character in token");
                return false;
        }
    }

    *pos = p;
    return true;
}

bool
ContribHandler::getToken(char *regExp, int *pos, char *token,
                         BSDF_FLAGS *flags) {
    switch ( regExp[*pos] ) {
        case '\0':
            return false;
            // break;
        case '+':
            *token = '+';
            (*pos)++;
            break;
        case '*':
            *token = '*';
            (*pos)++;
            break;
        case '(':
            *token = 'F';
            return getFlags(regExp, pos, flags);
            // break;
        default:
            doSyntaxError("Unknown token");
            return false;
    }

    return true;
}

void
ContribHandler::doRegExpGeneral(char *regExp, bool subtract) {
    FlagChain c;


    // Build iteration arrays (not tree so no nested brackets!)
    const int MAX_REGEXP_ITEMS = 15;

    BSDF_FLAGS flagArray[MAX_REGEXP_ITEMS];
    char typeArray[MAX_REGEXP_ITEMS];
    int countArray[MAX_REGEXP_ITEMS];
    int pos = 0, tokenCount = -1, iteratorCount = 0;
    char token;
    BSDF_FLAGS data;

    while ( getToken(regExp, &pos, &token, &data) ) {
        if ( token == 'F' ) {
            // A flag was read

            if ( tokenCount == MAX_REGEXP_ITEMS - 1 ) {
                doSyntaxError("Too many tokens in regexp");
                return;
            }

            tokenCount++;
            flagArray[tokenCount] = data;
            typeArray[tokenCount] = ' ';
            countArray[tokenCount] = 0;
        } else {
            // An iteration token was read
            if ( tokenCount == -1 ) {
                doSyntaxError("Initial iteration token");
                return;
            }

            if ( token == '+' ) {
                // Transform '+' in ' *'
                if ( tokenCount == MAX_REGEXP_ITEMS - 1 ) {
                    doSyntaxError("Too many tokens in regexp");
                    return;
                }

                flagArray[tokenCount + 1] = flagArray[tokenCount];
                tokenCount++;
                token = '*';
            }

            typeArray[tokenCount] = token;
            countArray[tokenCount] = 0;

            if ( (token == '*') || (token == '+') ) {
                iteratorCount++;
            }
        }
    }

    if ( tokenCount == -1 ) {
        // No tokens read ?!
        doSyntaxError("No tokens in regexp");
        return;
    }

    tokenCount++;
    typeArray[tokenCount] = 0;


    // Iterate all possible lengths
    int beginLength = tokenCount - iteratorCount;
    int endLength = maxLength;
    int iteratorsFound, remember, maxIteration;
    int iterationsDone, nextIterationsDone, num;
    bool done;

    if ( iteratorCount == 0 ) {
        // No iterators, we need just one chain length

        endLength = beginLength;
    }

    for ( int length = beginLength; length <= endLength; length++ ) {
        ChainList tmpList;
        c.init(length, subtract);

        maxIteration = length - tokenCount + iteratorCount;

        //printf("maxIteration %i\n", maxIteration);

        done = false;

        iterationsDone = 0;
        nextIterationsDone = 0;

        while ( !done ) {
            iteratorsFound = 0;
            remember = 0;
            pos = 0; // Number of flags filled in

            for ( int i = 0; i < tokenCount; i++ ) {
                if ( typeArray[i] == ' ' ) {
                    c[pos++] = flagArray[i];
                } else {
                    // typeArray[i] == '*' !  Choose a number

                    iteratorsFound++;
                    if ( iteratorsFound == iteratorCount ) {
                        // Last iterator : fill in
                        num = maxIteration - iterationsDone;
                        if ( iteratorCount == 1 || remember ) {
                            done = true;  // Only one possible combination or all tried
                        }
                    } else {
                        num = countArray[i];
                        if ( iteratorsFound == 1 ) {
                            // First Iterator
                            countArray[i]++;
                            nextIterationsDone++;
                            if ( nextIterationsDone > maxIteration ) {
                                // Too many
                                nextIterationsDone -= countArray[i];
                                countArray[i] = 0;
                                remember = true;
                            } else {
                                remember = false;
                            }
                        } else {
                            // In between iterator
                            if ( remember ) {
                                // Overflow for next iteration
                                countArray[i]++;
                                nextIterationsDone++;
                                if ( nextIterationsDone > maxIteration ) {
                                    // Too many
                                    nextIterationsDone -= countArray[i];
                                    countArray[i] = 0;
                                    remember = true;
                                } else {
                                    remember = false;
                                }
                            }
                        }
                    }

                    // Set num flags
                    for ( int j = 0; j < num; j++ ) {
                        c[pos++] = flagArray[i];
                    }
                }
            }

            iterationsDone = nextIterationsDone;
            tmpList.addDisjoint(c);

            if ( iteratorCount == 0 ) {
                done = true;
            }
        }

        array[length].add(tmpList.simplify()); // Add all chains
    }
}
