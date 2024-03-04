/**
Parse an mgf file, converting or discarding unsupported entities.
*/

#include <cctype>
#include <cstring>

#include "io/FileUncompressWrapper.h"
#include "io/mgf/lookup.h"
#include "io/mgf/messages.h"
#include "io/mgf/parser.h"

/*
 * Global definitions of variables declared in parser.h
 */

// Entity names
char GLOBAL_mgf_entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES][MGF_MAXIMUM_ENTITY_NAME_LENGTH] = MG_NAMELIST;

// Handler routines for each entity
int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv);

// Handler routine for unknown entities
int (*GLOBAL_mgf_unknownEntityHandleCallback)(int argc, char **argv) = mgfDefaultHandlerForUnknownEntities;

// Count of unknown entities
unsigned GLOBAL_mgf_unknownEntitiesCounter;

// Error messages
char *GLOBAL_mgf_errors[MGF_NUMBER_OF_ERRORS] = MG_ERROR_LIST;

// Current file context pointer
MgfReaderContext *GLOBAL_mgf_file;

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
static int (*e_supp[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv);
static char globalFloatFormat[] = "%.12g";
static int warpconends; // Hack for generating good normals

/**
Discard unneeded/unwanted entity
*/
static int
mgfDiscardUnNeededEntity(int /*ac*/, char ** /*av*/) {
    return MGF_OK;
}

/**
Compute u and v given w (normalized)
*/
static void
mgfMakeAxes(double *u, double *v, double *w)
{
    int i;

    v[0] = v[1] = v[2] = 0.0;
    for ( i = 0; i < 3; i++ ) {
        if ( w[i] < 0.6 && w[i] > -0.6 ) {
            break;
        }
    }
    v[i] = 1.0;
    floatCrossProduct(u, v, w);
    normalize(u);
    floatCrossProduct(v, w, u);
}

/**
Put out current xy chromaticities
*/
static int
mgfPutCxy()
{
    static char xbuf[24];
    static char ybuf[24];
    static char *ccom[4] = {GLOBAL_mgf_entityNames[MG_E_CXY], xbuf, ybuf};

    snprintf(xbuf, 24, "%.4f", GLOBAL_mgf_currentColor->cx);
    snprintf(ybuf, 24, "%.4f", GLOBAL_mgf_currentColor->cy);
    return mgfHandle(MG_E_CXY, 3, ccom);
}

/**
Put out current color spectrum
*/
static int
mgfPutCSpec()
{
    char wl[2][6];
    char vbuf[C_CNSS][24];
    char *newav[C_CNSS + 4];
    double sf;
    int i;

    if ( GLOBAL_mgf_handleCallbacks[MG_E_CSPEC] != handleColorEntity ) {
        snprintf(wl[0], 6, "%d", C_CMINWL);
        snprintf(wl[1], 6, "%d", C_CMAXWL);
        newav[0] = GLOBAL_mgf_entityNames[MG_E_CSPEC];
        newav[1] = wl[0];
        newav[2] = wl[1];
        sf = (double)C_CNSS / (double)GLOBAL_mgf_currentColor->ssum;
        for ( i = 0; i < C_CNSS; i++ ) {
            snprintf(vbuf[i], 24, "%.4f", sf * GLOBAL_mgf_currentColor->ssamp[i]);
            newav[i + 3] = vbuf[i];
        }
        newav[C_CNSS + 3] = nullptr;
        if ( (i = mgfHandle(MG_E_CSPEC, C_CNSS + 3, newav)) != MGF_OK ) {
            return i;
        }
    }
    return MGF_OK;
}

/**
Handle spectral color
*/
static int
mgfECSpec(int /*ac*/, char ** /*av*/) {
    // Convert to xy chromaticity
    mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, C_CSXY);
    // If it's really their handler, use it
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CXY] != handleColorEntity ) {
        return mgfPutCxy();
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
mgfECmix(int /*ac*/, char ** /*av*/) {
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CSPEC] == mgfECSpec ) {
        mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, C_CSXY);
    } else if ( GLOBAL_mgf_currentColor->flags & C_CDSPEC ) {
        return mgfPutCSpec();
    }
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CXY] != handleColorEntity ) {
        return mgfPutCxy();
    }
    return MGF_OK;
}

/**
Handle color temperature
*/
static int
mgfColorTemperature(int /*ac*/, char ** /*av*/)
{
    // Logic is similar to mgfECmix here.  Support handler has already
    // converted temperature to spectral color.  Put it out as such
    // if they support it, otherwise convert to xy chromaticity and
    // put it out if they handle it
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CSPEC] != mgfECSpec ) {
        return mgfPutCSpec();
    }
    mgfContextFixColorRepresentation(GLOBAL_mgf_currentColor, C_CSXY);
    if ( GLOBAL_mgf_handleCallbacks[MG_E_CXY] != handleColorEntity ) {
        return mgfPutCxy();
    }
    return MGF_OK;
}

/**
rayCasterInitialize alternate entity handlers
*/
void
mgfAlternativeInit(int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int, char **)) {
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
    if ( handleCallbacks[MGF_ERROR_SPHERE] == nullptr) {
        handleCallbacks[MGF_ERROR_SPHERE] = mgfEntitySphere;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_CYLINDER] == nullptr) {
        handleCallbacks[MGF_ERROR_CYLINDER] = mgfEntityCylinder;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_CONE] == nullptr) {
        handleCallbacks[MGF_ERROR_CONE] = mgfEntityCone;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_RING] == nullptr) {
        handleCallbacks[MGF_ERROR_RING] = mgfEntityRing;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_PRISM] == nullptr) {
        handleCallbacks[MGF_ERROR_PRISM] = mgfEntityPrism;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MGF_ERROR_TORUS] == nullptr) {
        handleCallbacks[MGF_ERROR_TORUS] = mgfEntityTorus;
        ineed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX;
    } else {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_NORMAL | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MG_E_FACE] == nullptr) {
        handleCallbacks[MG_E_FACE] = handleCallbacks[MG_E_FACEH];
    } else if ( handleCallbacks[MG_E_FACEH] == nullptr) {
        handleCallbacks[MG_E_FACEH] = mgfEntityFaceWithHoles;
    }
    if ( handleCallbacks[MG_E_COLOR] != nullptr) {
        if ( handleCallbacks[MG_E_CMIX] == nullptr) {
            handleCallbacks[MG_E_CMIX] = mgfECmix;
            ineed |= 1L << MG_E_COLOR | 1L << MG_E_CXY | 1L << MG_E_CSPEC | 1L << MG_E_CMIX | 1L << MG_E_CCT;
        }
        if ( handleCallbacks[MG_E_CSPEC] == nullptr) {
            handleCallbacks[MG_E_CSPEC] = mgfECSpec;
            ineed |= 1L << MG_E_COLOR | 1L << MG_E_CXY | 1L << MG_E_CSPEC | 1L << MG_E_CMIX | 1L << MG_E_CCT;
        }
        if ( handleCallbacks[MG_E_CCT] == nullptr) {
            handleCallbacks[MG_E_CCT] = mgfColorTemperature;
            ineed |= 1L << MG_E_COLOR | 1L << MG_E_CXY | 1L << MG_E_CSPEC | 1L << MG_E_CMIX | 1L << MG_E_CCT;
        }
    }

    // Check for consistency
    if ( handleCallbacks[MG_E_FACE] != nullptr) {
        uneed |= 1L << MG_E_POINT | 1L << MG_E_VERTEX | 1L << MG_E_XF;
    }
    if ( handleCallbacks[MG_E_CXY] != nullptr || handleCallbacks[MG_E_CSPEC] != nullptr ||
         handleCallbacks[MG_E_CMIX] != nullptr) {
        uneed |= 1L << MG_E_COLOR;
    }
    if ( handleCallbacks[MG_E_RD] != nullptr || handleCallbacks[MG_E_TD] != nullptr ||
         handleCallbacks[MG_E_IR] != nullptr ||
         handleCallbacks[MG_E_ED] != nullptr ||
         handleCallbacks[MG_E_RS] != nullptr ||
         handleCallbacks[MG_E_TS] != nullptr ||
         handleCallbacks[MG_E_SIDES] != nullptr) {
        uneed |= 1L << MG_E_MATERIAL;
    }
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( uneed & 1L << i && handleCallbacks[i] == nullptr) {
            fprintf(stderr, "Missing support for \"%s\" entity\n",
                    GLOBAL_mgf_entityNames[i]);
            exit(1);
        }
    }

    // Add support as needed
    if ( ineed & 1L << MG_E_VERTEX && handleCallbacks[MG_E_VERTEX] != handleVertexEntity ) {
        e_supp[MG_E_VERTEX] = handleVertexEntity;
    }
    if ( ineed & 1L << MG_E_POINT && handleCallbacks[MG_E_POINT] != handleVertexEntity ) {
        e_supp[MG_E_POINT] = handleVertexEntity;
    }
    if ( ineed & 1L << MG_E_NORMAL && handleCallbacks[MG_E_NORMAL] != handleVertexEntity ) {
        e_supp[MG_E_NORMAL] = handleVertexEntity;
    }
    if ( ineed & 1L << MG_E_COLOR && handleCallbacks[MG_E_COLOR] != handleColorEntity ) {
        e_supp[MG_E_COLOR] = handleColorEntity;
    }
    if ( ineed & 1L << MG_E_CXY && handleCallbacks[MG_E_CXY] != handleColorEntity ) {
        e_supp[MG_E_CXY] = handleColorEntity;
    }
    if ( ineed & 1L << MG_E_CSPEC && handleCallbacks[MG_E_CSPEC] != handleColorEntity ) {
        e_supp[MG_E_CSPEC] = handleColorEntity;
    }
    if ( ineed & 1L << MG_E_CMIX && handleCallbacks[MG_E_CMIX] != handleColorEntity ) {
        e_supp[MG_E_CMIX] = handleColorEntity;
    }
    if ( ineed & 1L << MG_E_CCT && handleCallbacks[MG_E_CCT] != handleColorEntity ) {
        e_supp[MG_E_CCT] = handleColorEntity;
    }

    // Discard remaining entities
    for ( i = 0; i < MGF_TOTAL_NUMBER_OF_ENTITIES; i++ ) {
        if ( handleCallbacks[i] == nullptr) {
            handleCallbacks[i] = mgfDiscardUnNeededEntity;
        }
    }
}

/**
Get entity number from its name
*/
int
mgfEntity(char *name)
{
    static LUTAB ent_tab = LU_SINIT(nullptr, nullptr); // Lookup table
    char *cp;

    if ( !ent_tab.tsiz ) {
        // Initialize hash table
        if ( !lookUpInit(&ent_tab, MGF_TOTAL_NUMBER_OF_ENTITIES)) {
            return -1;
        }

        // What to do?
        for ( cp = GLOBAL_mgf_entityNames[MGF_TOTAL_NUMBER_OF_ENTITIES - 1];
              cp >= GLOBAL_mgf_entityNames[0];
              cp -= sizeof(GLOBAL_mgf_entityNames[0]) ) {
            lookUpFind(&ent_tab, cp)->key = cp;
        }
    }
    cp = lookUpFind(&ent_tab, name)->key;
    if ( cp == nullptr) {
        return -1;
    }
    return (int)((cp - GLOBAL_mgf_entityNames[0]) / sizeof(GLOBAL_mgf_entityNames[0]));
}

/**
Pass entity to appropriate handler
*/
int
mgfHandle(int en, int ac, char **av)
{
    int rv;

    if ( en < 0 && (en = mgfEntity(av[0])) < 0 ) {
        // Unknown entity
        if ( GLOBAL_mgf_unknownEntityHandleCallback != nullptr) {
            return (*GLOBAL_mgf_unknownEntityHandleCallback)(ac, av);
        }
        return MGF_ERROR_UNKNOWN_ENTITY;
    }
    if ( e_supp[en] != nullptr) {
        // Support handler
        // TODO SITHMASTER: Check number of arguments here
        rv = (*e_supp[en])(ac, av);
        if ( rv != MGF_OK ) {
            return rv;
        }
    }
    return (*GLOBAL_mgf_handleCallbacks[en])(ac, av); // Assigned handler
}

/**
shaftCullOpen new input file
*/
int
mgfOpen(MgfReaderContext *ctx, char *fn)
{
    static int nfids;
    char *cp;
    int isPipe;

    ctx->fileContextId = ++nfids;
    ctx->lineNumber = 0;
    ctx->isPipe = 0;
    if ( fn == nullptr) {
        strcpy(ctx->fileName, "<stdin>");
        ctx->fp = stdin;
        ctx->prev = GLOBAL_mgf_file;
        GLOBAL_mgf_file = ctx;
        return MGF_OK;
    }

    // Get name relative to this context
    if ( GLOBAL_mgf_file != nullptr && (cp = strrchr(GLOBAL_mgf_file->fileName, '/')) != nullptr) {
        strcpy(ctx->fileName, GLOBAL_mgf_file->fileName);
        strcpy(ctx->fileName + (cp - GLOBAL_mgf_file->fileName + 1), fn);
    } else {
        strcpy(ctx->fileName, fn);
    }

    ctx->fp = openFile(ctx->fileName, "r", &isPipe);
    ctx->isPipe = (char)isPipe;

    if ( ctx->fp == nullptr) {
        return MGF_ERROR_CAN_NOT_OPEN_INPUT_FILE;
    }

    ctx->prev = GLOBAL_mgf_file; // Establish new context
    GLOBAL_mgf_file = ctx;
    return MGF_OK;
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
Get current position in input file
*/
void
mgfGetFilePosition(MgdReaderFilePosition *pos)
{
    pos->fid = GLOBAL_mgf_file->fileContextId;
    pos->lineno = GLOBAL_mgf_file->lineNumber;
    pos->offset = ftell(GLOBAL_mgf_file->fp);
}

/**
Reposition input file pointer
*/
int
mgfGoToFilePosition(MgdReaderFilePosition *pos)
{
    if ( pos->fid != GLOBAL_mgf_file->fileContextId ) {
        return MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( pos->lineno == GLOBAL_mgf_file->lineNumber ) {
        return MGF_OK;
    }
    if ( GLOBAL_mgf_file->fp == stdin || GLOBAL_mgf_file->isPipe ) {
        // Cannot seek on standard input
        return MGF_ERROR_FILE_SEEK_ERROR;
    }
    if ( fseek(GLOBAL_mgf_file->fp, pos->offset, 0) == EOF) {
        return MGF_ERROR_FILE_SEEK_ERROR;
    }
    GLOBAL_mgf_file->lineNumber = pos->lineno;
    return MGF_OK;
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
mgfParseCurrentLine()
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
    return mgfHandle(-1, (int)(ap - argv), argv);
}

/**
Default handler for unknown entities
*/
int
mgfDefaultHandlerForUnknownEntities(int /*ac*/, char **av)
{
    if ( GLOBAL_mgf_unknownEntitiesCounter++ == 0 ) {
        // Report first incident
        fprintf(stderr, "%s: %d: %s: %s\n", GLOBAL_mgf_file->fileName,
                GLOBAL_mgf_file->lineNumber, GLOBAL_mgf_errors[MGF_ERROR_UNKNOWN_ENTITY], av[0]);
    }
    return MGF_OK;
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
handleIncludedFile(int ac, char **av)
{
    char *xfarg[MGF_MAXIMUM_ARGUMENT_COUNT];
    MgfReaderContext ictx{};
    XfSpec *xf_orig = GLOBAL_mgf_xfContext;

    if ( ac < 2 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }

    int rv = mgfOpen(&ictx, av[1]);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( ac > 2 ) {
        int i;

        xfarg[0] = GLOBAL_mgf_entityNames[MG_E_XF];
        for ( i = 1; i < ac - 1; i++ ) {
            xfarg[i] = av[i + 1];
        }
        xfarg[ac - 1] = nullptr;
        rv = mgfHandle(MG_E_XF, ac - 1, xfarg);
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
            rv = mgfParseCurrentLine();
            if ( rv != MGF_OK ) {
                fprintf(stderr, "%s: %d: %s:\n%s", ictx.fileName,
                        ictx.lineNumber, GLOBAL_mgf_errors[rv],
                        ictx.inputLine);
                mgfClose();
                return MGF_ERROR_IN_INCLUDED_FILE;
            }
        }
        if ( ac > 2 ) {
            rv = mgfHandle(MG_E_XF, 1, xfarg);
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
mgfEntityFaceWithHoles(int ac, char **av)
{
    char *newav[MGF_MAXIMUM_ARGUMENT_COUNT];
    int lastp = 0;

    newav[0] = GLOBAL_mgf_entityNames[MG_E_FACE];
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
    return mgfHandle(MG_E_FACE, i, newav);
}

/**
Expand a sphere into cones
*/
int
mgfEntitySphere(int ac, char **av)
{
    static char p2x[24];
    static char p2y[24];
    static char p2z[24];
    static char r1[24];
    static char r2[24];
    static char *v1ent[5] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_sv1", (char *)"=", (char *)"_sv2"};
    static char *v2ent[4] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_sv2", (char *)"="};
    static char *p2ent[5] = {GLOBAL_mgf_entityNames[MG_E_POINT], p2x, p2y, p2z};
    static char *conent[6] = {GLOBAL_mgf_entityNames[MGF_ERROR_CONE], (char *)"_sv1", r1, (char *)"_sv2", r2};
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
    if ( !isFloatWords(av[2])) {
        return MGF_ERROR_ARGUMENT_TYPE;
    }
    rad = strtod(av[2], nullptr);

    // Initialize
    warpconends = 1;
    rval = mgfHandle(MG_E_VERTEX, 3, v2ent);
    if ( rval != MGF_OK ) {
        return rval;
    }
    snprintf(p2x, 24, globalFloatFormat, cv->p[0]);
    snprintf(p2y, 24, globalFloatFormat, cv->p[1]);
    snprintf(p2z, 24, globalFloatFormat, cv->p[2] + rad);
    rval = mgfHandle(MG_E_POINT, 4, p2ent);
    if ( rval != MGF_OK ) {
        return rval;
    }
    r2[0] = '0';
    r2[1] = '\0';
    for ( i = 1; i <= 2 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        if ( (rval = mgfHandle(MG_E_VERTEX, 4, v1ent)) != MGF_OK ) {
            return rval;
        }
        snprintf(p2z, 24, globalFloatFormat, cv->p[2] + rad * std::cos(theta));
        if ( (rval = mgfHandle(MG_E_VERTEX, 2, v2ent)) != MGF_OK ) {
            return rval;
        }
        if ((rval = mgfHandle(MG_E_POINT, 4, p2ent)) != MGF_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, rad * std::sin(theta));
        if ((rval = mgfHandle(MGF_ERROR_CONE, 5, conent)) != MGF_OK ) {
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
mgfEntityTorus(int ac, char **av)
{
    static char p2[3][24];
    static char r1[24];
    static char r2[24];
    static char *v1ent[5] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_tv1", (char *)"=", (char *)"_tv2"};
    static char *v2ent[5] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_tv2", (char *)"="};
    static char *p2ent[5] = {GLOBAL_mgf_entityNames[MG_E_POINT], p2[0], p2[1], p2[2]};
    static char *conent[6] = {GLOBAL_mgf_entityNames[MGF_ERROR_CONE], (char *)"_tv1", r1, (char *)"_tv2", r2};
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
    if ( sgn * (maxrad - minrad) <= 0. ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    warpconends = 1;
    v2ent[3] = av[1];
    for ( j = 0; j < 3; j++ ) {
        snprintf(p2[j], 24, globalFloatFormat, cv->p[j] + 0.5 * sgn * (maxrad - minrad) * cv->n[j]);
    }
    rval = mgfHandle(MG_E_VERTEX, 4, v2ent);
    if ( rval != MGF_OK ) {
        return rval;
    }
    rval = mgfHandle(MG_E_POINT, 4, p2ent);
    if ( rval != MGF_OK ) {
        return rval;
    }
    snprintf(r2, 24, globalFloatFormat, avgrad = 0.5 * (minrad + maxrad));

    // Run outer section
    for ( i = 1; i <= 2 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
        theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
        if ((rval = mgfHandle(MG_E_VERTEX, 4, v1ent)) != MGF_OK ) {
            return rval;
        }
        for ( j = 0; j < 3; j++ ) {
            snprintf(p2[j], 24, globalFloatFormat, cv->p[j] +
                                   0.5 * sgn * (maxrad - minrad) * std::cos(theta) * cv->n[j]);
        }
        rval = mgfHandle(MG_E_VERTEX, 2, v2ent);
        if ( rval != MGF_OK ) {
            return rval;
        }
        rval = mgfHandle(MG_E_POINT, 4, p2ent);
        if ( rval != MGF_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, avgrad + 0.5 * (maxrad - minrad) * std::sin(theta));
        rval = mgfHandle(MGF_ERROR_CONE, 5, conent);
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
        rval = mgfHandle(MG_E_VERTEX, 4, v1ent);
        if ( rval != MGF_OK ) {
            return rval;
        }
        rval = mgfHandle(MG_E_VERTEX, 2, v2ent);
        if ( rval != MGF_OK ) {
            return rval;
        }
        rval = mgfHandle(MG_E_POINT, 4, p2ent);
        if ( rval != MGF_OK ) {
            return rval;
        }
        strcpy(r1, r2);
        snprintf(r2, 24, globalFloatFormat, -avgrad - .5 * (maxrad - minrad) * std::sin(theta));
        rval = mgfHandle(MGF_ERROR_CONE, 5, conent);
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
mgfEntityCylinder(int ac, char **av)
{
    static char *avnew[6] = {GLOBAL_mgf_entityNames[MGF_ERROR_CONE]};

    if ( ac != 4 ) {
        return MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    avnew[1] = av[1];
    avnew[2] = av[2];
    avnew[3] = av[3];
    avnew[4] = av[2];
    return mgfHandle(MGF_ERROR_CONE, 5, avnew);
}

/**
Turn a ring into polygons
*/
int
mgfEntityRing(int ac, char **av)
{
    static char p3[3][24];
    static char p4[3][24];
    static char *nzent[5] = {GLOBAL_mgf_entityNames[MG_E_NORMAL], (char *)"0", (char *)"0", (char *)"0"};
    static char *v1ent[5] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_rv1", (char *)"="};
    static char *v2ent[5] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_rv2", (char *)"=", (char *)"_rv3"};
    static char *v3ent[4] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_rv3", (char *)"="};
    static char *p3ent[5] = {GLOBAL_mgf_entityNames[MG_E_POINT], p3[0], p3[1], p3[2]};
    static char *v4ent[4] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_rv4", (char *)"="};
    static char *p4ent[5] = {GLOBAL_mgf_entityNames[MG_E_POINT], p4[0], p4[1], p4[2]};
    static char *fent[6] = {GLOBAL_mgf_entityNames[MG_E_FACE], (char *)"_rv1", (char *)"_rv2", (char *)"_rv3", (char *)"_rv4"};
    MgfVertexContext *cv;
    int i;
    int j;
    FVECT u;
    FVECT v;
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
    if ( minRad < 0. || maxRad <= minRad ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Initialize
    mgfMakeAxes(u, v, cv->n);
    for ( j = 0; j < 3; j++ ) {
        snprintf(p3[j], 24, globalFloatFormat, cv->p[j] + maxRad * u[j]);
    }
    rv = mgfHandle(MG_E_VERTEX, 3, v3ent);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MG_E_POINT, 4, p3ent);
    if ( rv != MGF_OK ) {
        return rv;
    }

    if ( minRad == 0.0 ) {
        // TODO: Review floating point comparisons vs EPSILON
        // Closed
        v1ent[3] = av[1];
        rv = mgfHandle(MG_E_VERTEX, 4, v1ent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MG_E_NORMAL, 4, nzent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MG_E_VERTEX, 4, v2ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                snprintf(p3[j], 24, globalFloatFormat, cv->p[j] +
                                            maxRad * u[j] * std::cos(theta) +
                                            maxRad * v[j] * std::sin(theta));
            }
            rv = mgfHandle(MG_E_VERTEX, 2, v3ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_POINT, 4, p3ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_FACE, 4, fent);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
    } else {
        // Open
        rv = mgfHandle(MG_E_VERTEX, 3, v4ent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( j = 0; j < 3; j++ ) {
            snprintf(p4[j], 24, globalFloatFormat, cv->p[j] + minRad * u[j]);
        }
        rv = mgfHandle(MG_E_POINT, 4, p4ent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        v1ent[3] = (char *)"_rv4";
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MG_E_VERTEX, 4, v1ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_VERTEX, 4, v2ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                d = u[j] * std::cos(theta) + v[j] * std::sin(theta);
                snprintf(p3[j], 24, globalFloatFormat, cv->p[j] + maxRad * d);
                snprintf(p4[j], 24, globalFloatFormat, cv->p[j] + minRad * d);
            }
            rv = mgfHandle(MG_E_VERTEX, 2, v3ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_POINT, 4, p3ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_VERTEX, 2, v4ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_POINT, 4, p4ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_FACE, 5, fent);
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
mgfEntityCone(int ac, char **av)
{
    static char p3[3][24];
    static char p4[3][24];
    static char n3[3][24];
    static char n4[3][24];
    static char *v1ent[5] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_cv1", (char *)"="};
    static char *v2ent[5] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_cv2", (char *)"=", (char *)"_cv3"};
    static char *v3ent[4] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_cv3", (char *)"="};
    static char *p3ent[5] = {GLOBAL_mgf_entityNames[MG_E_POINT], p3[0], p3[1], p3[2]};
    static char *n3ent[5] = {GLOBAL_mgf_entityNames[MG_E_NORMAL], n3[0], n3[1], n3[2]};
    static char *v4ent[4] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], (char *)"_cv4", (char *)"="};
    static char *p4ent[5] = {GLOBAL_mgf_entityNames[MG_E_POINT], p4[0], p4[1], p4[2]};
    static char *n4ent[5] = {GLOBAL_mgf_entityNames[MG_E_NORMAL], n4[0], n4[1], n4[2]};
    static char *fent[6] = {GLOBAL_mgf_entityNames[MG_E_FACE], (char *)"_cv1", (char *)"_cv2", (char *)"_cv3", (char *)"_cv4"};
    char *v1n;
    MgfVertexContext *cv1;
    MgfVertexContext *cv2;
    int i, j;
    FVECT u;
    FVECT v;
    FVECT w;
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
    for ( j = 0; j < 3; j++ ) {
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
        if ( d <= -M_PI / 2 + FLOAT_TINY) {
            n2off = -FLOAT_HUGE;
        } else {
            n2off = std::tan(d);
        }
    }
    mgfMakeAxes(u, v, w);
    for ( j = 0; j < 3; j++ ) {
        snprintf(p3[j], 24, globalFloatFormat, cv2->p[j] + rad2 * u[j]);
        if ( n2off <= -FLOAT_HUGE) {
            snprintf(n3[j], 24, globalFloatFormat, -w[j]);
        } else {
            snprintf(n3[j], 24, globalFloatFormat, u[j] + w[j] * n2off);
        }
    }
    rv = mgfHandle(MG_E_VERTEX, 3, v3ent);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MG_E_POINT, 4, p3ent);
    if ( rv != MGF_OK ) {
        return rv;
    }
    rv = mgfHandle(MG_E_NORMAL, 4, n3ent);
    if ( rv != MGF_OK ) {
        return rv;
    }
    if ( rad1 == 0.0 ) { // TODO: Review floating point comparisons vs EPSILON
        // Triangles
        v1ent[3] = v1n;
        rv = mgfHandle(MG_E_VERTEX, 4, v1ent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( j = 0; j < 3; j++ ) {
            snprintf(n4[j], 24, globalFloatFormat, w[j]);
        }
        rv = mgfHandle(MG_E_NORMAL, 4, n4ent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = sgn * i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MG_E_VERTEX, 4, v2ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
                d = u[j] * std::cos(theta) + v[j] * std::sin(theta);
                snprintf(p3[j], 24, globalFloatFormat, cv2->p[j] + rad2 * d);
                if ( n2off > -FLOAT_HUGE) {
                    snprintf(n3[j], 24, globalFloatFormat, d + w[j] * n2off);
                }
            }
            rv = mgfHandle(MG_E_VERTEX, 2, v3ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_POINT, 4, p3ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_NORMAL, 4, n3ent);
            if ( n2off > -FLOAT_HUGE && rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_FACE, 4, fent);
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
            if ( d >= M_PI / 2 - FLOAT_TINY) {
                n1off = FLOAT_HUGE;
            } else {
                n1off = std::tan(std::atan(n1off) + (M_PI / 4) / GLOBAL_mgf_divisionsPerQuarterCircle);
            }
        }
        for ( j = 0; j < 3; j++ ) {
            snprintf(p4[j], 24, globalFloatFormat, cv1->p[j] + rad1 * u[j]);
            if ( n1off >= FLOAT_HUGE) {
                snprintf(n4[j], 24, globalFloatFormat, w[j]);
            } else {
                snprintf(n4[j], 24, globalFloatFormat, u[j] + w[j] * n1off);
            }
        }
        rv = mgfHandle(MG_E_VERTEX, 3, v4ent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MG_E_POINT, 4, p4ent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        rv = mgfHandle(MG_E_NORMAL, 4, n4ent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        for ( i = 1; i <= 4 * GLOBAL_mgf_divisionsPerQuarterCircle; i++ ) {
            theta = sgn * i * (M_PI / 2) / GLOBAL_mgf_divisionsPerQuarterCircle;
            rv = mgfHandle(MG_E_VERTEX, 4, v1ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_VERTEX, 4, v2ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            for ( j = 0; j < 3; j++ ) {
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
            rv = mgfHandle(MG_E_VERTEX, 2, v3ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_POINT, 4, p3ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_NORMAL, 4, n3ent);
            if ( n2off > -FLOAT_HUGE && rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_VERTEX, 2, v4ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_POINT, 4, p4ent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_NORMAL, 4, n4ent);
            if ( n1off < FLOAT_HUGE &&
                 rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_FACE, 5, fent);
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
mgfEntityPrism(int ac, char **av)
{
    static char p[3][24];
    static char *vent[5] = {GLOBAL_mgf_entityNames[MG_E_VERTEX], nullptr, (char *)"="};
    static char *pent[5] = {GLOBAL_mgf_entityNames[MG_E_POINT], p[0], p[1], p[2]};
    static char *znorm[5] = {GLOBAL_mgf_entityNames[MG_E_NORMAL], (char *)"0", (char *)"0", (char *)"0"};
    char *newav[MGF_MAXIMUM_ARGUMENT_COUNT];
    char nvn[MGF_MAXIMUM_ARGUMENT_COUNT - 1][8];
    double length;
    int hasnorm;
    FVECT v1;
    FVECT v2;
    FVECT v3;
    FVECT norm;
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
    if ( length <= FLOAT_TINY && length >= -FLOAT_TINY ) {
        return MGF_ERROR_ILLEGAL_ARGUMENT_VALUE;
    }

    // Compute face normal
    cv0 = getNamedVertex(av[1]);
    if ( cv0 == nullptr ) {
        return MGF_ERROR_UNDEFINED_REFERENCE;
    }
    hasnorm = 0;
    norm[0] = norm[1] = norm[2] = 0.;
    v1[0] = v1[1] = v1[2] = 0.;
    for ( i = 2; i < ac - 1; i++ ) {
        cv = getNamedVertex(av[i]);
        if ( cv == nullptr) {
            return MGF_ERROR_UNDEFINED_REFERENCE;
        }
        hasnorm += !is0Vector(cv->n);
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
        rv = mgfHandle(MG_E_VERTEX, 4, vent);
        if ( rv != MGF_OK ) {
            return rv;
        }
        cv = getNamedVertex(av[i]); // Checked above
        for ( j = 0; j < 3; j++ ) {
            snprintf(p[j], 24, globalFloatFormat, cv->p[j] - length * norm[j]);
        }
        rv = mgfHandle(MG_E_POINT, 4, pent);
        if ( rv != MGF_OK ) {
            return rv;
        }
    }

    // Make faces
    newav[0] = GLOBAL_mgf_entityNames[MG_E_FACE];
    // Do the side faces
    newav[5] = nullptr;
    newav[3] = av[ac - 2];
    newav[4] = nvn[ac - 3];
    for ( i = 1; i < ac - 1; i++ ) {
        newav[1] = nvn[i - 1];
        newav[2] = av[i];
        rv = mgfHandle(MG_E_FACE, 5, newav);
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
            rv = mgfHandle(MG_E_VERTEX, 2, vent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_NORMAL, 4, znorm);
            if ( rv != MGF_OK ) {
                return rv;
            }
        }
        newav[ac - 1 - i] = nvn[i - 1]; // Reverse
    }
    rv = mgfHandle(MG_E_FACE, ac - 1, newav);
    if ( rv != MGF_OK ) {
        return rv;
    }

    // Do bottom face
    if ( hasnorm != 0 ) {
        for ( i = 1; i < ac - 1; i++ ) {
            vent[1] = nvn[i - 1];
            vent[3] = av[i];
            rv = mgfHandle(MG_E_VERTEX, 4, vent);
            if ( rv != MGF_OK ) {
                return rv;
            }
            rv = mgfHandle(MG_E_NORMAL, 4, znorm);
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
    rv = mgfHandle(MG_E_FACE, i, newav);
    if ( rv != MGF_OK ) {
        return rv;
    }
    return MGF_OK;
}
