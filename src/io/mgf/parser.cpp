/**
Parse an mgf file, converting or discarding unsupported entities.
*/

#include <cctype>
#include <cstring>

#include "common/error.h"
#include "io/FileUncompressWrapper.h"
#include "io/mgf/lookup.h"
#include "io/mgf/messages.h"
#include "io/mgf/parser.h"
#include "io/mgf/MgfTransformContext.h"
#include "io/mgf/words.h"
#include "io/mgf/mgfGeometry.h"

// Entity names
char GLOBAL_mgf_entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH] = MG_NAMELIST;

// Number of divisions per quarter circle
int GLOBAL_mgf_divisionsPerQuarterCircle = MGF_DEFAULT_NUMBER_OF_DIVISIONS;

/**
The idea with this parser is to compensate for any missing entries in
GLOBAL_mgf_handleCallbacks with alternate handlers that express these entities in terms
of others that the calling program can handle.

In some cases, no alternate handler is possible because the entity
has no approximate equivalent.  These entities are simply discarded.

Certain entities are dependent on others, and mgfAlternativeInit() will fail
if the supported entities are not consistent.

Some alternate entity handlers require that earlier entities be
noted in some fashion, and we therefore keep another array of
parallel support handlers to assist in this effort.
*/

// Alternate handler support functions
static char globalFloatFormat[] = "%.12g";
static int warpconends; // Hack for generating good normals

/**
Discard unneeded/unwanted entity
*/
static int
mgfDiscardUnNeededEntity(int /*ac*/, char ** /*av*/, RadianceMethod * /*context*/) {
    return MGF_OK;
}

/**
Put out current xy chromaticities
*/
static int
mgfPutCxy(RadianceMethod *context)
{
    static char xbuf[24];
    static char ybuf[24];
    static char *ccom[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_CXY], xbuf, ybuf};

    snprintf(xbuf, 24, "%.4f", GLOBAL_mgf_currentColor->cx);
    snprintf(ybuf, 24, "%.4f", GLOBAL_mgf_currentColor->cy);
    return mgfHandle(MGF_ENTITY_CXY, 3, ccom, context);
}

/**
Put out current color spectrum
*/
static int
mgfPutCSpec(RadianceMethod *context)
{
    char wl[2][6];
    char vbuf[NUMBER_OF_SPECTRAL_SAMPLES][24];
    char *newav[NUMBER_OF_SPECTRAL_SAMPLES + 4];
    double sf;
    int i;

    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_C_SPEC] != handleColorEntity ) {
        snprintf(wl[0], 6, "%d", C_CMINWL);
        snprintf(wl[1], 6, "%d", C_CMAXWL);
        newav[0] = GLOBAL_mgf_entityNames[MGF_ENTITY_C_SPEC];
        newav[1] = wl[0];
        newav[2] = wl[1];
        sf = (double)NUMBER_OF_SPECTRAL_SAMPLES / (double)GLOBAL_mgf_currentColor->spectralStraightSum;
        for ( i = 0; i < NUMBER_OF_SPECTRAL_SAMPLES; i++ ) {
            snprintf(vbuf[i], 24, "%.4f", sf * GLOBAL_mgf_currentColor->straightSamples[i]);
            newav[i + 3] = vbuf[i];
        }
        newav[NUMBER_OF_SPECTRAL_SAMPLES + 3] = nullptr;
        if ((i = mgfHandle(MGF_ENTITY_C_SPEC, NUMBER_OF_SPECTRAL_SAMPLES + 3, newav, context)) != MGF_OK ) {
            return i;
        }
    }
    return MGF_OK;
}

/**
Handle spectral color
*/
static int
mgfECSpec(int /*ac*/, char ** /*av*/, RadianceMethod *context) {
    // Convert to xy chromaticity
    mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, C_CSXY);
    // If it's really their handler, use it
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

/**
Handle mixing of colors
Contorted logic works as follows:
1. the colors are already mixed in c_hcolor() support function
2. if we would handle a spectral result, make sure it's not
3. if handleColorEntity() would handle a spectral result, don't bother
4. otherwise, make cspec entity and pass it to their handler
5. if we have only xy results, handle it as c_spec() would
*/
static int
mgfECmix(int /*ac*/, char ** /*av*/, RadianceMethod *context) {
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_C_SPEC] == mgfECSpec ) {
        mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, C_CSXY);
    } else if ( GLOBAL_mgf_currentColor->flags & C_CDSPEC ) {
        return mgfPutCSpec(context);
    }
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

/**
Handle color temperature
*/
static int
mgfColorTemperature(int /*ac*/, char ** /*av*/, RadianceMethod *context)
{
    // Logic is similar to mgfECmix here.  Support handler has already
    // converted temperature to spectral color.  Put it out as such
    // if they support it, otherwise convert to xy chromaticity and
    // put it out if they handle it
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_C_SPEC] != mgfECSpec ) {
        return mgfPutCSpec(context);
    }
    mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, C_CSXY);
    if ( GLOBAL_mgf_handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        return mgfPutCxy(context);
    }
    return MGF_OK;
}

/**
rayCasterInitialize alternate entity handlers
*/
void
mgfAlternativeInit(int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int, char **, RadianceMethod *)) {
    unsigned long ineed = 0;
    unsigned long uneed = 0;
    int i;

    // Pick up slack
    if ( handleCallbacks[MG_E_IES] == nullptr) {
        handleCallbacks[MG_E_IES] = mgfDiscardUnNeededEntity;
    }
    if ( handleCallbacks[MG_E_INCLUDE] == nullptr) {
        handleCallbacks[MG_E_INCLUDE] = handleIncludedFile;
    }
    if ( handleCallbacks[MGF_ENTITY_SPHERE] == nullptr) {
        handleCallbacks[MGF_ENTITY_SPHERE] = mgfEntitySphere;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_CYLINDER] == nullptr) {
        handleCallbacks[MGF_ENTITY_CYLINDER] = mgfEntityCylinder;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_CONE] == nullptr) {
        handleCallbacks[MGF_ENTITY_CONE] = mgfEntityCone;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_RING] == nullptr) {
        handleCallbacks[MGF_ENTITY_RING] = mgfEntityRing;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_PRISM] == nullptr) {
        handleCallbacks[MGF_ENTITY_PRISM] = mgfEntityPrism;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_TORUS] == nullptr) {
        handleCallbacks[MGF_ENTITY_TORUS] = mgfEntityTorus;
        ineed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX;
    } else {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_NORMAL | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_FACE] == nullptr) {
        handleCallbacks[MGF_ENTITY_FACE] = handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES];
    } else if ( handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES] == nullptr) {
        handleCallbacks[MGF_ENTITY_FACE_WITH_HOLES] = mgfEntityFaceWithHoles;
    }
    if ( handleCallbacks[MGF_ENTITY_COLOR] != nullptr) {
        if ( handleCallbacks[MGF_ENTITY_C_MIX] == nullptr) {
            handleCallbacks[MGF_ENTITY_C_MIX] = mgfECmix;
            ineed |= 1L << MGF_ENTITY_COLOR | 1L << MGF_ENTITY_CXY | 1L << MGF_ENTITY_C_SPEC | 1L << MGF_ENTITY_C_MIX | 1L << MGF_ENTITY_CCT;
        }
        if ( handleCallbacks[MGF_ENTITY_C_SPEC] == nullptr) {
            handleCallbacks[MGF_ENTITY_C_SPEC] = mgfECSpec;
            ineed |= 1L << MGF_ENTITY_COLOR | 1L << MGF_ENTITY_CXY | 1L << MGF_ENTITY_C_SPEC | 1L << MGF_ENTITY_C_MIX | 1L << MGF_ENTITY_CCT;
        }
        if ( handleCallbacks[MGF_ENTITY_CCT] == nullptr) {
            handleCallbacks[MGF_ENTITY_CCT] = mgfColorTemperature;
            ineed |= 1L << MGF_ENTITY_COLOR | 1L << MGF_ENTITY_CXY | 1L << MGF_ENTITY_C_SPEC | 1L << MGF_ENTITY_C_MIX | 1L << MGF_ENTITY_CCT;
        }
    }

    // Check for consistency
    if ( handleCallbacks[MGF_ENTITY_FACE] != nullptr) {
        uneed |= 1L << MGF_ENTITY_POINT | 1L << MGF_ENTITY_VERTEX | 1L << MGF_ENTITY_XF;
    }
    if ( handleCallbacks[MGF_ENTITY_CXY] != nullptr || handleCallbacks[MGF_ENTITY_C_SPEC] != nullptr ||
         handleCallbacks[MGF_ENTITY_C_MIX] != nullptr) {
        uneed |= 1L << MGF_ENTITY_COLOR;
    }
    if ( handleCallbacks[MGF_ENTITY_RD] != nullptr || handleCallbacks[MGF_ENTITY_TD] != nullptr ||
         handleCallbacks[MGF_ENTITY_IR] != nullptr ||
         handleCallbacks[MGF_ENTITY_ED] != nullptr ||
         handleCallbacks[MGF_ENTITY_RS] != nullptr ||
         handleCallbacks[MGF_ENTITY_TS] != nullptr ||
         handleCallbacks[MGF_ENTITY_SIDES] != nullptr) {
        uneed |= 1L << MGF_ENTITY_MATERIAL;
    }
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( uneed & 1L << i && handleCallbacks[i] == nullptr) {
            fprintf(stderr, "Missing support for \"%s\" entity\n",
                    GLOBAL_mgf_entityNames[i]);
            exit(1);
        }
    }

    // Add support as needed
    if ( ineed & 1L << MGF_ENTITY_VERTEX && handleCallbacks[MGF_ENTITY_VERTEX] != handleVertexEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_VERTEX] = handleVertexEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_POINT && handleCallbacks[MGF_ENTITY_POINT] != handleVertexEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_POINT] = handleVertexEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_NORMAL && handleCallbacks[MGF_ENTITY_NORMAL] != handleVertexEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_NORMAL] = handleVertexEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_COLOR && handleCallbacks[MGF_ENTITY_COLOR] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_COLOR] = handleColorEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_CXY && handleCallbacks[MGF_ENTITY_CXY] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_CXY] = handleColorEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_C_SPEC && handleCallbacks[MGF_ENTITY_C_SPEC] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_C_SPEC] = handleColorEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_C_MIX && handleCallbacks[MGF_ENTITY_C_MIX] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_C_MIX] = handleColorEntity;
    }
    if ( ineed & 1L << MGF_ENTITY_CCT && handleCallbacks[MGF_ENTITY_CCT] != handleColorEntity ) {
        GLOBAL_mgf_support[MGF_ENTITY_CCT] = handleColorEntity;
    }

    // Discard remaining entities
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( handleCallbacks[i] == nullptr) {
            handleCallbacks[i] = mgfDiscardUnNeededEntity;
        }
    }
}

/**
Close input file
*/
void
mgfClose()
{
    MgfReaderContext *ctx = GLOBAL_mgf_file;

    GLOBAL_mgf_file = ctx->prev; // Restore enclosing context
    if ( ctx->fp != stdin ) {
        // Close file if it's a file
        closeFile(ctx->fp, ctx->isPipe);
    }
}

/**
Read next line from file
*/
int
mgfReadNextLine()
{
    int len = 0;

    do {
        if ( fgets(GLOBAL_mgf_file->inputLine + len,
                   MGF_MAXIMUM_INPUT_LINE_LENGTH - len, GLOBAL_mgf_file->fp) == nullptr) {
            return len;
        }
        len += (int)strlen(GLOBAL_mgf_file->inputLine + len);
        if ( len >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
            return len;
        }
        GLOBAL_mgf_file->lineNumber++;
    } while ( len > 1 && GLOBAL_mgf_file->inputLine[len - 2] == '\\' );

    return len;
}

/**
Parse current input line
*/
int
mgfParseCurrentLine(RadianceMethod *context)
{
    char abuf[MGF_MAXIMUM_INPUT_LINE_LENGTH];
    char *argv[MGF_MAXIMUM_ARGUMENT_COUNT];
    char *cp;
    char *cp2;
    char **ap;

    // Copy line, removing escape chars
    cp = abuf;
    cp2 = GLOBAL_mgf_file->inputLine;
    while ((*cp++ = *cp2++)) {
        if ( cp2[0] == '\n' && cp2[-1] == '\\' ) {
            cp--;
        }
    }
    cp = abuf;
    ap = argv; // Break into words
    for ( ;; ) {
        while ( isspace(*cp)) {
            *cp++ = '\0';
        }
        if ( !*cp ) {
            break;
        }
        if ( ap - argv >= MGF_MAXIMUM_ARGUMENT_COUNT - 1 ) {
            return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        *ap++ = cp;
        while ( *++cp && !isspace(*cp) );
    }
    if ( ap == argv ) {
        // No words in line
        return MGF_OK;
    }
    *ap = nullptr;
    // Else handle it
    return mgfHandle(-1, (int)(ap - argv), argv, context);
}

/**
Clear parser history
*/
void
mgfClear()
{
    clearContextTables(); // Clear context tables
    while ( GLOBAL_mgf_file != nullptr) {
        // Reset our file context
        mgfClose();
    }
}

int
handleIncludedFile(int ac, char **av, RadianceMethod *context)
{
    char *xfarg[MGF_MAXIMUM_ARGUMENT_COUNT];
    MgfReaderContext ictx{};
    MgfTransformContext *xf_orig = GLOBAL_mgf_xfContext;

    if ( ac < 2 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    int rv = mgfOpen(&ictx, av[1]);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        xfarg[0] = GLOBAL_mgf_entityNames[MGF_ENTITY_XF];
        for ( int i = 1; i < ac - 1; i++ ) {
            xfarg[i] = av[i + 1];
        }
        xfarg[ac - 1] = nullptr;
        rv = mgfHandle(MGF_ENTITY_XF, ac - 1, xfarg, context);
        if ( rv != MGF_OK ) {
            mgfClose();
            return rv;
        }
    }
    do {
        while ( (rv = mgfReadNextLine()) > 0 ) {
            if ( rv >= MGF_MAXIMUM_INPUT_LINE_LENGTH - 1 ) {
                fprintf(stderr, "%s: %d: %s\n", ictx.fileName,
                        ictx.lineNumber, GLOBAL_mgf_errors[MGF_ERROR_LINE_TOO_LONG]);
                mgfClose();
                return MGF_ERROR_IN_INCLUDED_FILE;
            }
            rv = mgfParseCurrentLine(context);
            if ( rv != MGF_OK ) {
                fprintf(stderr, "%s: %d: %s:\n%s", ictx.fileName,
                        ictx.lineNumber, GLOBAL_mgf_errors[rv],
                        ictx.inputLine);
                mgfClose();
                return MGF_ERROR_IN_INCLUDED_FILE;
            }
        }
        if ( ac > 2 ) {
            rv = mgfHandle(MGF_ENTITY_XF, 1, xfarg, context);
            if ( rv != MGF_OK ) {
                mgfClose();
                return rv;
            }
        }
    } while ( GLOBAL_mgf_xfContext != xf_orig );
    mgfClose();
    return MGF_OK;
}

/**
Replace face+holes with single contour
*/
int
mgfEntityFaceWithHoles(int ac, char **av, RadianceMethod *context)
{
    char *newav[MGF_MAXIMUM_ARGUMENT_COUNT];
    int lastp = 0;

    newav[0] = GLOBAL_mgf_entityNames[MGF_ENTITY_FACE];
    int i;
    for ( i = 1; i < ac; i++ ) {
        int j;

        if ( av[i][0] == '-' ) {
            if ( i < 4 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            if ( i >= ac - 1 ) {
                break;
            }
            if ( !lastp ) {
                lastp = i - 1;
            }
            for ( j = i + 1; j < ac - 1 && av[j + 1][0] != '-'; j++ ) {
            }
            if ( j - i < 3 ) {
                return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
            }
            newav[i] = av[j]; // Connect hole loop
        } else {
            // Hole or perimeter vertex
            newav[i] = av[i];
        }
    }
    if ( lastp ) {
        // Finish seam to outside
        newav[i++] = av[lastp];
    }
    newav[i] = nullptr;
    return mgfHandle(MGF_ENTITY_FACE, i, newav, context);
}

/**
Expand a sphere into cones
*/
int
mgfEntitySphere(int ac, char **av, RadianceMethod *context)
{
    static char p2x[24];
    static char p2y[24];
    static char p2z[24];
    static char r1[24];
    static char r2[24];
    static char *v1ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_sv1", (char *)"=", (char *)"_sv2"};
    static char *v2ent[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_sv2", (char *)"="};
    static char *p2ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p2x, p2y, p2z};
    static char *conent[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_CONE], (char *)"_sv1", r1, (char *)"_sv2", r2};
    MgfVertexContext *cv;
    int i;
    int rval;
    double rad;
    double theta;

    if ( ac != 3 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    cv = getNamedVertex(av[1]);
    if ( cv == nullptr) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( !isFloatWords(av[2]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    rad = strtod(av[2], nullptr);

    // Initialize
    warpconends = 1;
    rval = mgfHandle(MGF_ENTITY_VERTEX, 3, v2ent, context);
    if ( rval != MGF_OK ) {
        return rval;
    }
    snprintf(p2x, 24, globalFloatFormat, cv->p[0]);
    snprintf(p2y, 24, globalFloatFormat, cv->p[1]);
    snprintf(p2z, 24, globalFloatFormat, cv->p[2] + rad);
    rval = mgfHandle(MGF_ENTITY_POINT, 4, p2ent, context);
    if ( rval != MGF_OK ) {
        return rval;
    }
    r2[0] = '0';
    r2[1] = '\0';
    for ( i = 1; i <= 2 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        rval = mgfHandle(MGF_ENTITY_VERTEX, 4, v1ent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
        snprintf(p2z, 24, globalFloatFormat, cv->p[2] + rad * std::cos(theta));
        rval = mgfHandle(MGF_ENTITY_VERTEX, 2, v2ent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
        rval = mgfHandle(MGF_ENTITY_POINT, 4, p2ent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, rad * std::sin(theta));
        rval = mgfHandle(MGF_ENTITY_CONE, 5, conent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
    }
    warpconends = 0;
    return MGF_OK;
}

/**
Expand a torus into cones
*/
int
mgfEntityTorus(int ac, char **av, RadianceMethod *context)
{
    static char p2[3][24];
    static char r1[24];
    static char r2[24];
    static char *v1ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_tv1", (char *)"=", (char *)"_tv2"};
    static char *v2ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_tv2", (char *)"="};
    static char *p2ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p2[0], p2[1], p2[2]};
    static char *conent[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_CONE], (char *)"_tv1", r1, (char *)"_tv2", r2};
    MgfVertexContext *cv;
    int i;
    int j;
    int rval;
    int sgn;
    double minrad;
    double maxrad;
    double avgrad;
    double theta;

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( (cv = getNamedVertex(av[1])) == nullptr ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( is0Vector(cv->n) ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !isFloatWords(av[2]) || !isFloatWords(av[3]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    minrad = strtod(av[2], nullptr);
    round0(minrad);
    maxrad = strtod(av[3], nullptr);

    // Check orientation
    if ( minrad > 0.0 ) {
        sgn = 1;
    } else if ( minrad < 0.0 ) {
        sgn = -1;
    } else {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( sgn * (maxrad - minrad) <= 0.0 ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    warpconends = 1;
    v2ent[3] = av[1];
    for ( j = 0; j < 3; j++ ) {
        snprintf(p2[j], 24, globalFloatFormat, cv->p[j] + 0.5 * sgn * (maxrad - minrad) * cv->n[j]);
    }
    rval = mgfHandle(MGF_ENTITY_VERTEX, 4, v2ent, context);
    if ( rval != MGF_OK ) {
        return rval;
    }
    rval = mgfHandle(MGF_ENTITY_POINT, 4, p2ent, context);
    if ( rval != MGF_OK ) {
        return rval;
    }
    snprintf(r2, 24, globalFloatFormat, avgrad = 0.5 * (minrad + maxrad));

    // Run outer section
    for ( i = 1; i <= 2 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        rval = mgfHandle(MGF_ENTITY_VERTEX, 4, v1ent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
        for ( j = 0; j < 3; j++ ) {
            snprintf(p2[j], 24, globalFloatFormat, cv->p[j] +
                                   0.5 * sgn * (maxrad - minrad) * std::cos(theta) * cv->n[j]);
        }
        rval = mgfHandle(MGF_ENTITY_VERTEX, 2, v2ent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
        rval = mgfHandle(MGF_ENTITY_POINT, 4, p2ent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, avgrad + 0.5 * (maxrad - minrad) * std::sin(theta));
        rval = mgfHandle(MGF_ENTITY_CONE, 5, conent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
    }

    // Run inner section
    snprintf(r2, 24, globalFloatFormat, -0.5 * (minrad + maxrad));
    for ( ; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        for ( j = 0; j < 3; j++ ) {
            snprintf(p2[j], 24, globalFloatFormat, cv->p[j] +
                                   0.5 * sgn * (maxrad - minrad) * std::cos(theta) * cv->n[j]);
        }
        rval = mgfHandle(MGF_ENTITY_VERTEX, 4, v1ent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
        rval = mgfHandle(MGF_ENTITY_VERTEX, 2, v2ent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
        rval = mgfHandle(MGF_ENTITY_POINT, 4, p2ent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, -avgrad - .5 * (maxrad - minrad) * std::sin(theta));
        rval = mgfHandle(MGF_ENTITY_CONE, 5, conent, context);
        if ( rval != MGF_OK ) {
            return rval;
        }
    }
    warpconends = 0;
    return MGF_OK;
}

/**
Replace a cylinder with equivalent cone
*/
int
mgfEntityCylinder(int ac, char **av, RadianceMethod *context)
{
    static char *avnew[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_CONE]};

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    avnew[1] = av[1];
    avnew[2] = av[2];
    avnew[3] = av[3];
    avnew[4] = av[2];
    return mgfHandle(MGF_ENTITY_CONE, 5, avnew, context);
}

/**
Turn a ring into polygons
*/
int
mgfEntityRing(int ac, char **av, RadianceMethod *context)
{
    static char p3[3][24];
    static char p4[3][24];
    static char *nzent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_NORMAL], (char *)"0", (char *)"0", (char *)"0"};
    static char *v1ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_rv1", (char *)"="};
    static char *v2ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_rv2", (char *)"=", (char *)"_rv3"};
    static char *v3ent[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_rv3", (char *)"="};
    static char *p3ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p3[0], p3[1], p3[2]};
    static char *v4ent[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_rv4", (char *)"="};
    static char *p4ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p4[0], p4[1], p4[2]};
    static char *fent[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_FACE], (char *)"_rv1", (char *)"_rv2", (char *)"_rv3", (char *)"_rv4"};
    MgfVertexContext *cv;
    int i;
    int j;
    double minRad;
    double maxRad;
    int rv;
    double theta;
    double d;

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    cv = getNamedVertex(av[1]);
    if ( cv == nullptr) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    if ( is0Vector(cv->n) ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    if ( !isFloatWords(av[2]) || !isFloatWords(av[3]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    minRad = strtod(av[2], nullptr);
    round0(minRad);
    maxRad = strtod(av[3], nullptr);
    if ( minRad < 0.0 || maxRad <= minRad ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    VECTOR3Dd u;
    VECTOR3Dd v;

    mgfMakeAxes(u, v, cv->n);
    for ( j = 0; j < 3; j++ ) {
        snprintf(p3[j], 24, globalFloatFormat, cv->p[j] + maxRad * u[j]);
    }
    rv = mgfHandle(MGF_ENTITY_VERTEX, 3, v3ent, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MGF_ENTITY_POINT, 4, p3ent, context);
    if ( rv != MGF_OK ) {
        return rv;
    }

    if ( minRad == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        // Closed
        v1ent[3] = av[1];
        rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v1ent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MGF_ENTITY_NORMAL, 4, nzent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v2ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                snprintf(p3[j], 24, globalFloatFormat, cv->p[j] +
                                            maxRad * u[j] * std::cos(theta) +
                                            maxRad * v[j] * std::sin(theta));
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v3ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p3ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_FACE, 4, fent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    } else {
        // Open
        rv = mgfHandle(MGF_ENTITY_VERTEX, 3, v4ent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( j = 0; j < 3; j++ ) {
            snprintf(p4[j], 24, globalFloatFormat, cv->p[j] + minRad * u[j]);
        }
        rv = mgfHandle(MGF_ENTITY_POINT, 4, p4ent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        v1ent[3] = (char *)"_rv4";
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v1ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v2ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                d = u[j] * std::cos(theta) + v[j] * std::sin(theta);
                snprintf(p3[j], 24, globalFloatFormat, cv->p[j] + maxRad * d);
                snprintf(p4[j], 24, globalFloatFormat, cv->p[j] + minRad * d);
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v3ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p3ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v4ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p4ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_FACE, 5, fent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    }
    return MGF_OK;
}

/**
Turn a cone into polygons
*/
int
mgfEntityCone(int ac, char **av, RadianceMethod *context)
{
    static char p3[3][24];
    static char p4[3][24];
    static char n3[3][24];
    static char n4[3][24];
    static char *v1ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_cv1", (char *)"="};
    static char *v2ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_cv2", (char *)"=", (char *)"_cv3"};
    static char *v3ent[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_cv3", (char *)"="};
    static char *p3ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p3[0], p3[1], p3[2]};
    static char *n3ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_NORMAL], n3[0], n3[1], n3[2]};
    static char *v4ent[4] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], (char *)"_cv4", (char *)"="};
    static char *p4ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p4[0], p4[1], p4[2]};
    static char *n4ent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_NORMAL], n4[0], n4[1], n4[2]};
    static char *fent[6] = {GLOBAL_mgf_entityNames[MGF_ENTITY_FACE], (char *)"_cv1", (char *)"_cv2", (char *)"_cv3", (char *)"_cv4"};
    char *v1n;
    MgfVertexContext *cv1;
    MgfVertexContext *cv2;
    double rad1;
    double rad2;
    int sgn;
    double n1off;
    double n2off;
    double d;
    int rv;
    double theta;

    if ( ac != 5 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    cv1 = getNamedVertex(av[1]);
    cv2 = getNamedVertex(av[3]);
    if ( cv1 == nullptr || cv2 == nullptr) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    v1n = av[1];
    if ( !isFloatWords(av[2]) || !isFloatWords(av[4]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    rad1 = strtod(av[2], nullptr);
    round0(rad1);
    rad2 = strtod(av[4], nullptr);
    round0(rad2);
    if ( rad1 == 0.0 ) {
        if ( rad2 == 0.0 ) {
            return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else if ( rad2 != 0.0 ) {
        if ((rad1 < 0.0) ^ (rad2 < 0.0)) {
            return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
        }
    } else {
        // Swap
        MgfVertexContext *cv;

        cv = cv1;
        cv1 = cv2;
        cv2 = cv;
        v1n = av[3];
        d = rad1;
        rad1 = rad2;
        rad2 = d;
    }
    sgn = rad2 < 0.0 ? -1 : 1;

    // Initialize
    VECTOR3Dd w;

    for ( int j = 0; j < 3; j++ ) {
        w[j] = cv1->p[j] - cv2->p[j];
    }
    d = normalize(w);
    if ( d == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }
    n1off = n2off = (rad2 - rad1) / d;
    if ( warpconends != 0 ) {
        // Hack for mgfEntitySphere and mgfEntityTorus
        d = std::atan(n2off) - (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle;
        if ( d <= -M_PI / 2 + EPSILON) {
            n2off = -FLOAT_HUGE;
        } else {
            n2off = std::tan(d);
        }
    }

    VECTOR3Dd u;
    VECTOR3Dd v;
    mgfMakeAxes(u, v, w);
    for ( int j = 0; j < 3; j++ ) {
        snprintf(p3[j], 24, globalFloatFormat, cv2->p[j] + rad2 * u[j]);
        if ( n2off <= -FLOAT_HUGE) {
            snprintf(n3[j], 24, globalFloatFormat, -w[j]);
        } else {
            snprintf(n3[j], 24, globalFloatFormat, u[j] + w[j] * n2off);
        }
    }
    rv = mgfHandle(MGF_ENTITY_VERTEX, 3, v3ent, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MGF_ENTITY_POINT, 4, p3ent, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n3ent, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( rad1 == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        // Triangles
        v1ent[3] = v1n;
        rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v1ent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int j = 0; j < 3; j++ ) {
            snprintf(n4[j], 24, globalFloatFormat, w[j]);
        }
        rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n4ent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = sgn * i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v2ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( int j = 0; j < 3; j++ ) {
                d = u[j] * std::cos(theta) + v[j] * std::sin(theta);
                snprintf(p3[j], 24, globalFloatFormat, cv2->p[j] + rad2 * d);
                if ( n2off > -FLOAT_HUGE) {
                    snprintf(n3[j], 24, globalFloatFormat, d + w[j] * n2off);
                }
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v3ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p3ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n3ent, context);
            if ( n2off > -FLOAT_HUGE && rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_FACE, 4, fent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    } else {
        // Quads
        v1ent[3] = (char *)"_cv4";
        if ( warpconends ) {
            // Hack for mgfEntitySphere and mgfEntityTorus
            d = std::atan(n1off) + (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle;
            if ( d >= M_PI / 2 - EPSILON) {
                n1off = FLOAT_HUGE;
            } else {
                n1off = std::tan(std::atan(n1off) + (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle);
            }
        }
        for ( int j = 0; j < 3; j++ ) {
            snprintf(p4[j], 24, globalFloatFormat, cv1->p[j] + rad1 * u[j]);
            if ( n1off >= FLOAT_HUGE) {
                snprintf(n4[j], 24, globalFloatFormat, w[j]);
            } else {
                snprintf(n4[j], 24, globalFloatFormat, u[j] + w[j] * n1off);
            }
        }
        rv = mgfHandle(MGF_ENTITY_VERTEX, 3, v4ent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MGF_ENTITY_POINT, 4, p4ent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n4ent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( int i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = sgn * i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v1ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, v2ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( int j = 0; j < 3; j++ ) {
                d = u[j] * std::cos(theta) + v[j] * std::sin(theta);
                snprintf(p3[j], 24, globalFloatFormat, cv2->p[j] + rad2 * d);
                if ( n2off > -FLOAT_HUGE) {
                    snprintf(n3[j], 24, globalFloatFormat, d + w[j] * n2off);
                }
                snprintf(p4[j], 24, globalFloatFormat, cv1->p[j] + rad1 * d);
                if ( n1off < FLOAT_HUGE) {
                    snprintf(n4[j], 24, globalFloatFormat, d + w[j] * n1off);
                }
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v3ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p3ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n3ent, context);
            if ( n2off > -FLOAT_HUGE && rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, v4ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_POINT, 4, p4ent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, n4ent, context);
            if ( n1off < FLOAT_HUGE &&
                 rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_FACE, 5, fent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    }
    return MGF_OK;
}

/**
Turn a prism into polygons
*/
int
mgfEntityPrism(int ac, char **av, RadianceMethod *context)
{
    static char p[3][24];
    static char *vent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_VERTEX], nullptr, (char *)"="};
    static char *pent[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_POINT], p[0], p[1], p[2]};
    static char *znorm[5] = {GLOBAL_mgf_entityNames[MGF_ENTITY_NORMAL], (char *)"0", (char *)"0", (char *)"0"};
    char *newav[MGF_MAXIMUM_ARGUMENT_COUNT];
    char nvn[MGF_MAXIMUM_ARGUMENT_COUNT - 1][8];
    double length;
    int hasnorm;
    MgfVertexContext *cv;
    MgfVertexContext *cv0;
    int rv;
    int i;
    int j;

    // Check arguments
    if ( ac < 5 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    if ( !isFloatWords(av[ac - 1]) ) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    length = strtod(av[ac - 1], nullptr);
    if ( length <= EPSILON && length >= -EPSILON ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Compute face normal
    cv0 = getNamedVertex(av[1]);
    if ( cv0 == nullptr ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    hasnorm = 0;

    VECTOR3Dd norm;

    norm[0] = 0.0;
    norm[1] = 0.0;
    norm[2] = 0.0;

    VECTOR3Dd v1;
    v1[0] = 0.0;
    v1[1] = 0.0;
    v1[2] = 0.0;



    for ( i = 2; i < ac - 1; i++ ) {
        cv = getNamedVertex(av[i]);
        if ( cv == nullptr) {
            return MGF_ERROR_UNDEFINED_REFERENCE;
        }
        hasnorm += !is0Vector(cv->n);

        VECTOR3Dd v2;
        VECTOR3Dd v3;

        v2[0] = cv->p[0] - cv0->p[0];
        v2[1] = cv->p[1] - cv0->p[1];
        v2[2] = cv->p[2] - cv0->p[2];
        floatCrossProduct(v3, v1, v2);
        norm[0] += v3[0];
        norm[1] += v3[1];
        norm[2] += v3[2];
        mgfVertexCopy(v1, v2);
    }
    if ( normalize(norm) == 0.0 ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Create moved vertices
    for ( i = 1; i < ac - 1; i++ ) {
        snprintf(nvn[i - 1], MGF_MAXIMUM_ARGUMENT_COUNT, "_pv%d", i);
        vent[1] = nvn[i - 1];
        vent[3] = av[i];
        rv = mgfHandle(MGF_ENTITY_VERTEX, 4, vent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        cv = getNamedVertex(av[i]); // Checked above
        for ( j = 0; j < 3; j++ ) {
            snprintf(p[j], 24, globalFloatFormat, cv->p[j] - length * norm[j]);
        }
        rv = mgfHandle(MGF_ENTITY_POINT, 4, pent, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
    }

    // Make faces
    newav[0] = GLOBAL_mgf_entityNames[MGF_ENTITY_FACE];
    // Do the side faces
    newav[5] = nullptr;
    newav[3] = av[ac - 2];
    newav[4] = nvn[ac - 3];
    for ( i = 1; i < ac - 1; i++ ) {
        newav[1] = nvn[i - 1];
        newav[2] = av[i];
        rv = mgfHandle(MGF_ENTITY_FACE, 5, newav, context);
        if ( rv != MGF_OK ) {
            return rv;
        }
        newav[3] = newav[2];
        newav[4] = newav[1];
    }

    // Do top face
    for ( i = 1; i < ac - 1; i++ ) {
        if ( hasnorm ) {
            // Zero normals
            vent[1] = nvn[i - 1];
            rv = mgfHandle(MGF_ENTITY_VERTEX, 2, vent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, znorm, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
        newav[ac - 1 - i] = nvn[i - 1]; // Reverse
    }
    rv = mgfHandle(MGF_ENTITY_FACE, ac - 1, newav, context);
    if ( rv != MGF_OK ) {
        return rv;
    }

    // Do bottom face
    if ( hasnorm != 0 ) {
        for ( i = 1; i < ac - 1; i++ ) {
            vent[1] = nvn[i - 1];
            vent[3] = av[i];
            rv = mgfHandle(MGF_ENTITY_VERTEX, 4, vent, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MGF_ENTITY_NORMAL, 4, znorm, context);
            if ( rv != MGF_OK ) {
                return rv;
            }
            newav[i] = nvn[i - 1];
        }
    } else {
        for ( i = 1; i < ac - 1; i++ ) {
            newav[i] = av[i];
        }
    }
    newav[i] = nullptr;
    rv = mgfHandle(MGF_ENTITY_FACE, i, newav, context);
    if ( rv != MGF_OK ) {
        return rv;
    }
    return MGF_OK;
}
