#include "common/error.h"
#include "material/edf.h"
#include "skin/radianceinterfaces.h"
#include "raycasting/common/pathnode.h"
#include "raycasting/raytracing/flagchain.h"

void CFlagChain::Init(const int length, const bool subtract) {
    if ( m_chain ) {
        delete[] m_chain;
    }

    if ( length > 0 ) {
        m_chain = new BSDFFLAGS[length];
    } else {
        m_chain = nullptr;
    }

    m_length = length;
    m_subtract = subtract;

    for ( int i = 0; i < length; i++ ) {
        m_chain[i] = 0;
    }
}

CFlagChain::CFlagChain(const int length, const bool subtract) {
    m_chain = nullptr;

    Init(length, subtract);
}

CFlagChain::CFlagChain(const CFlagChain &c) {
    m_chain = nullptr;
    Init(c.m_length, c.m_subtract);

    for ( int i = 0; i < m_length; i++ ) {
        m_chain[i] = c.m_chain[i];
    }
}

CFlagChain::~CFlagChain() {
    delete[] m_chain;
}

bool FlagChainCompare(const CFlagChain *c1,
                      const CFlagChain *c2) {
    // Determine if equal

    int nrDifferent = 0;
    int i;

    if ((c1->m_length != c2->m_length) || (c1->m_subtract != c2->m_subtract)) {
        return false;
    }

    for ( i = 0; (i < c1->m_length) && (nrDifferent == 0); i++ ) {
        if ( c1->m_chain[i] != c2->m_chain[i] ) {
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

CFlagChain *FlagChainCombine(const CFlagChain *c1,
                             const CFlagChain *c2) {
    // Determine if combinable

    int nrDifferent = 0;
    int diffIndex = 0, i;

    if ((c1->m_length != c2->m_length) || (c1->m_subtract != c2->m_subtract)) {
        return nullptr;
    }

    for ( i = 0; (i < c1->m_length) && (nrDifferent <= 1); i++ ) {
        if ( c1->m_chain[i] != c2->m_chain[i] ) {
            nrDifferent++;
            diffIndex = i;
        }
    }

    // combine into new chain

    if ( nrDifferent == 0 ) {
        // flag chains identical - maybe dangerous if someone wants to
        // count one contribution twice...
        return new CFlagChain(*c1);
    }

    if ( nrDifferent == 1 ) {
        // Combinable !

        CFlagChain *cnew = new CFlagChain(*c1);

        cnew->m_chain[diffIndex] = c1->m_chain[diffIndex] | c2->m_chain[diffIndex];

        return cnew;
    }

    // Not combinable

    return nullptr;
}


COLOR CFlagChain::Compute(CBiPath *path) {
    COLOR result, tmpCol;
    colorSetMonochrome(result, 1.0);
    int i;
    int eyeSize = path->m_eyeSize;
    int lightSize = path->m_lightSize;

    CPathNode *node;

    if ( lightSize + eyeSize != m_length ) {
        Error("FlagChain::Compute", "Wrong path length");
        return result;
    }

    // Flagchain start at the lightnode and end at the eyenode

    node = path->m_lightPath;

    for ( i = 0; i < lightSize; i++ ) {
        tmpCol = node->m_bsdfComp.Sum(m_chain[i]);
        colorProduct(tmpCol, result, result);
        node = node->Next();
    }

    node = path->m_eyePath;

    for ( i = 0; i < eyeSize; i++ ) {
        tmpCol = node->m_bsdfComp.Sum(m_chain[m_length - 1 - i]);
        colorProduct(tmpCol, result, result);
        node = node->Next();
    }

    if ( m_subtract ) {
        colorScale(-1, result, result);
    }

    return result;
}


void CFlagChain::Print() {
    if ( m_subtract ) {
        printf("-");
    }

    for ( int i = 0; i < m_length; i++ ) {
        XXDFFLAGS brdfFlags, btdfFlags;
        bool line = false;

        brdfFlags = GETBRDFFLAGS(m_chain[i]);
        btdfFlags = GETBTDFFLAGS(m_chain[i]);


        printf("(");

        if ( brdfFlags == ALL_COMPONENTS) {
            if ( line ) {
                printf("|");
            }
            printf("XR");
            line = true;
        } else {
            if ( brdfFlags & DIFFUSE_COMPONENT ) {
                if ( line ) {
                    printf("|");
                }
                printf("DR");
                line = true;
            }
            if ( brdfFlags & GLOSSY_COMPONENT ) {
                if ( line ) {
                    printf("|");
                }
                printf("GR");
                line = true;
            }
            if ( brdfFlags & SPECULAR_COMPONENT ) {
                if ( line ) {
                    printf("|");
                }
                printf("SR");
                line = true;
            }
        }

        if ( btdfFlags == ALL_COMPONENTS) {
            if ( line ) {
                printf("|");
            }
            printf("XT");
            line = true;
        } else {
            if ( btdfFlags & DIFFUSE_COMPONENT ) {
                if ( line ) {
                    printf("|");
                }
                printf("DT");
                line = true;
            }
            if ( btdfFlags & GLOSSY_COMPONENT ) {
                if ( line ) {
                    printf("|");
                }
                printf("GT");
                line = true;
            }
            if ( btdfFlags & SPECULAR_COMPONENT ) {
                if ( line ) {
                    printf("|");
                }
                printf("ST");
                line = true;
            }
        }
        printf(")");
    }
}


/********* Chain List ***********/

CChainList::CChainList() {
    m_count = 0;
    m_length = 0;
}

CChainList::~CChainList() {
    RemoveAll();
}

void CChainList::Add(CChainList *list) {
    // Add all chains in 'list'

    CFlagChainIter iter(*list);
    CFlagChain *tmpChain;

    while ((tmpChain = iter.Next())) {
        Add(*tmpChain);
    }
}

void CChainList::Add(const CFlagChain &chain) {
    if ( m_count > 0 ) {
        if ( chain.m_length != m_length ) {
            Error("CChainList::Add", "Wrong length flagchain inserted!");
            return;
        }
    } else {
        // first element
        m_length = chain.m_length;
    }

    m_count++;
    Append(chain);
}

void CChainList::AddDisjunct(const CFlagChain &chain) {
    if ( m_count > 0 ) {
        if ( chain.m_length != m_length ) {
            Error("CChainList::Add", "Wrong length flagchain inserted!");
            return;
        }
    } else {
        // first element
        m_length = chain.m_length;
    }


    CFlagChainIter iter(*this);
    CFlagChain *tmpChain;
    bool found = false;

    while ((tmpChain = iter.Next()) && !found ) {
        found = FlagChainCompare(tmpChain, &chain);
    }

    if ( !found ) {
        m_count++;
        Append(chain);
    }
}

void CChainList::Print() {
    CFlagChainIter iter(*this);
    CFlagChain *chain;

    printf("Chainlist, length %i, entries %i\n", m_length, m_count);

    while ((chain = iter.Next())) {
        printf("  ");
        chain->Print();
        printf("\n");
    }
}


COLOR CChainList::Compute(CBiPath *path) {
    COLOR result, tmpCol;

    colorClear(result);

    CFlagChainIter iter(*this);
    CFlagChain *chain;

    while ((chain = iter.Next())) {
        tmpCol = chain->Compute(path);

        colorAdd(tmpCol, result, result);
    }

    return (result);
}

CChainList *CChainList::Simplify() {
    // Try a simple simplifaction scheme, just comparing pair wise chains

    CChainList *newList = new CChainList;
    CFlagChain *c1, *c2, *ccomb;
    CFlagChainIter iter(*this);

    c1 = iter.Next();

    if ( c1 ) {
        while ((c2 = iter.Next())) {
            ccomb = FlagChainCombine(c1, c2);
            if ( ccomb ) {
                c1 = ccomb; // Combined
            } else {
                newList->Add(*c1);
                c1 = c2;
            }
        }

        // Add final chain still in c1
        newList->Add(*c1);
    }

    return newList;
}


/* ----------------------------------------------------------------------------- */


/*** Contribution Handler Implementation ***/

CContribHandler::CContribHandler() {
    m_array = nullptr;
    m_maxLength = 0;
}

void CContribHandler::Init(int maxLength) {
    m_maxLength = maxLength;

    if ( m_array ) {
        delete[] m_array;
    }

    // For each length we need a chainlist
    m_array = new CChainList[m_maxLength + 1]; // 0 <= length <= maxlength !!
}

CContribHandler::~CContribHandler() {
    delete[] m_array;
}

COLOR CContribHandler::Compute(CBiPath *path) {
    COLOR result;
    int length;

    colorClear(result);

    length = path->m_eyeSize + path->m_lightSize;

    if ( length > m_maxLength ) {
        Error("CContribHandler::Compute", "Path too long !!");
        return result;
    }

    //  if(length < 1)
    //{
    //  Warning("CContribHandler::Compute", "Path too short !!");
    //}

    result = m_array[length].Compute(path);

    return result;
}

void CContribHandler::Print() {
    int i;

    printf("ContribHandler\n");

    for ( i = 0; i <= m_maxLength; i++ ) {
        printf("Length %i\n", i);
        printf("=============\n");

        m_array[i].Print();

        printf("=============\n");
    }
}

void CContribHandler::DoRegExp(char *regExp, bool subtract) {

    DoRegExp_General(regExp, subtract);
    return;
}

void CContribHandler::AddRegExp(char *regExp) {
    if ( regExp[0] == '-' ) {
        DoRegExp(regExp + 1, true);
    } else {
        DoRegExp(regExp, false);
    }
}

void CContribHandler::SubRegExp(char *regExp) {
    if ( regExp[0] == '-' ) {
        DoRegExp(regExp + 1, false);
    } else {
        DoRegExp(regExp, true);
    }
}

/*** General Reg Exp parser ***/

void CContribHandler::DoSyntaxError(const char *errString) {
    Error("Flagchain Syntax Error", errString);
    Init(m_maxLength);
}

bool CContribHandler::GetFlags(char *regExp, int *pos, BSDFFLAGS *flags) {
    char c;
    int p = *pos;

    *flags = 0;

    if ( regExp[p++] != '(' ) {
        DoSyntaxError("GetFlags: '(' expected");
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
                    DoSyntaxError("GetFlags: No 'X' after 'L'. Only LX supported");
                    return false;
                }
                p++;
                *flags = BSDF_ALL_COMPONENTS;
                break;
            case 'E':
                if ( regExp[p] != 'X' ) {
                    DoSyntaxError("GetFlags: No 'X' after 'E'. Only EX supported");
                    return false;
                }
                p++;
                *flags = BSDF_ALL_COMPONENTS;
                break;
            case '|':
                break;  // Do Nothing cause we don't support other operators
            default:
                DoSyntaxError("GetFlags: Unexpected character in token");
                return false;
        }
    }

    //printf("To parse '%s'\n", regExp + p);

    *pos = p;
    return true;
}

bool CContribHandler::GetToken(char *regExp, int *pos, char *token,
                               BSDFFLAGS *flags) {
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
            return GetFlags(regExp, pos, flags);
            // break;
        default:
            DoSyntaxError("Unknown token");
            return false;
    }

    return true;
}

void CContribHandler::DoRegExp_General(char *regExp, bool subtract) {
    CFlagChain c;

    //printf("Adding %s\n", regExp);


    // Build iteration arrays (not tree so no nested brackets!)

    const int MAX_REGEXP_ITEMS = 15;

    BSDFFLAGS flagArray[MAX_REGEXP_ITEMS];
    char typeArray[MAX_REGEXP_ITEMS];
    int countArray[MAX_REGEXP_ITEMS];
    int pos = 0, tokenCount = -1, iteratorCount = 0;
    char token;
    BSDFFLAGS data;

    while ( GetToken(regExp, &pos, &token, &data)) {
        if ( token == 'F' ) {
            // A flag was read

            if ( tokenCount == MAX_REGEXP_ITEMS - 1 ) {
                DoSyntaxError("Too many tokens in regexp");
                return;
            }

            tokenCount++;
            flagArray[tokenCount] = data;
            typeArray[tokenCount] = ' ';
            countArray[tokenCount] = 0;
        } else {
            // An iteration token was read
            if ( tokenCount == -1 ) {
                DoSyntaxError("Initial iteration token");
                return;
            }

            if ( token == '+' ) {
                // Transform '+' in ' *'
                if ( tokenCount == MAX_REGEXP_ITEMS - 1 ) {
                    DoSyntaxError("Too many tokens in regexp");
                    return;
                }

                flagArray[tokenCount + 1] = flagArray[tokenCount];
                tokenCount++;
                token = '*';
            }

            typeArray[tokenCount] = token;
            countArray[tokenCount] = 0;

            if ((token == '*') || (token == '+')) {
                iteratorCount++;
            }
        }
    }

    if ( tokenCount == -1 ) {
        // No tokens read ?!
        DoSyntaxError("No tokens in regexp");
        return;
    }

    tokenCount++;
    typeArray[tokenCount] = 0;

    //printf("Tokens '%s'\n", typeArray);

    // Iterate all possible lengths

    int beginLength = tokenCount - iteratorCount;
    int endLength = m_maxLength;
    int iteratorsFound, remember, maxIteration;
    int iterationsDone, nextIterationsDone, num;
    bool done;

    if ( iteratorCount == 0 ) {
        // No iterators, we need just one chain length

        endLength = beginLength;
    }

    for ( int length = beginLength; length <= endLength; length++ ) {
        CChainList tmpList;
        c.Init(length, subtract);

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
                } // else (type == iterator)
            } // for

            //printf("Pos %i, Length %i\n", pos, length);

            //c.Print();
            //printf("\n\n");

            iterationsDone = nextIterationsDone;
            tmpList.AddDisjunct(c);

            if ( iteratorCount == 0 ) {
                done = true;
            }
        }

        m_array[length].Add(tmpList.Simplify()); // Add all chains
    }
}
