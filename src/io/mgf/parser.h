/*
Header file for mgf interpreter
*/

#ifndef MG_VMAJOR

#include <cstdio>
#include <cstdlib>

#include "common/mymath.h"

#define MG_VMAJOR    2        /* major version number */

/* Entities (list is only appended, never modified) */
#define MG_E_COLOR    1        /* c		*/
#define MG_E_CCT    2        /* cct		*/
#define MGF_ERROR_CONE    3        /* cone		*/
#define MG_E_CMIX    4        /* cmix		*/
#define MG_E_CSPEC    5        /* cspec	*/
#define MG_E_CXY    6        /* cxy		*/
#define MGF_EROR_CYLINDER    7        /* cyl		*/
#define MG_E_ED        8        /* ed		*/
#define MG_E_FACE    9        /* f		*/
#define MG_E_INCLUDE    10        /* i		*/
#define MG_E_IES    11        /* ies		*/
#define MG_E_IR        12        /* ir		*/
#define MG_E_MATERIAL    13        /* m		*/
#define MG_E_NORMAL    14        /* n		*/
#define MG_E_OBJECT    15        /* o		*/
#define MG_E_POINT    16        /* p		*/
#define MGF_ERROR_PRISM    17        /* prism	*/
#define MG_E_RD        18        /* rd		*/
#define MGF_ERROR_RING    19        /* ring		*/
#define MG_E_RS        20        /* rs		*/
#define MG_E_SIDES    21        /* sides	*/
#define MGF_ERROR_SPHERE    22        /* sph		*/
#define MG_E_TD        23        /* td		*/
#define MGF_ERROR_TORUS    24        /* torus	*/
#define MG_E_TS        25        /* ts		*/
#define MG_E_VERTEX    26        /* v		*/
#define MG_E_XF        27        /* xf		*/
/* end of Version 1 entities */
#define MG_E_FACEH    28        /* fh		*/
/* end of Version 2 entities */

#define MGF_TOTAL_NUMBER_OF_ENTITIES    29        /* total # entities */

#define MG_NAMELIST    {"#","c","cct","cone","cmix","cspec","cxy","cyl","ed",\
            "f","i","ies","ir","m","n","o","p","prism","rd",\
            "ring","rs","sides","sph","td","torus","ts","v","xf",\
            "fh"}

#define MG_MAXELEN    6

extern char mg_ename[MGF_TOTAL_NUMBER_OF_ENTITIES][MG_MAXELEN];

/* Handler routines for each entity and unknown ones */

extern int (*GLOBAL_mgf_handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int argc, char **argv);

extern int (*GLOBAL_mgf_unknownEntityHandleCallback)(int argc, char **argv);

extern int mg_defuhand(int, char **);

extern unsigned mg_nunknown;        /* count of unknown entities */

/* Error codes */
#define MG_OK        0        /* normal return value */
#define MG_EUNK        1        /* unknown entity */
#define MG_EARGC    2        /* wrong number of arguments */
#define MG_ETYPE    3        /* argument type error */
#define MGF_ERROR_ILLEGAL_ARGUMENT_VALUE        4        /* illegal argument value */
#define MG_EUNDEF    5        /* undefined reference */
#define MG_ENOFILE    6        /* cannot open input file */
#define MG_EINCL    7        /* error in included file */
#define MG_EMEM        8        /* out of memory */
#define MG_ESEEK    9        /* file seek error */
#define MG_ELINE    11        /* input line too long */
#define MG_ECNTXT    12        /* unmatched context close */

#define MG_NERRS    13

extern char *GLOBAL_mgf_errors[MG_NERRS];    /* list of error messages */

/*
 * The general process for running the parser is to fill in the GLOBAL_mgf_handleCallbacks
 * array with handlers for each entity you know how to handle.
 * Then, call mgfAlternativeInit to fill in the rest.  This function will report
 * an error and quit if you try to support an inconsistent set of entities.
 * For each file you want to parse, call mg_load with the file name.
 * To read from standard input, use nullptr as the file name.
 * For additional control over error reporting and file management,
 * use mg_open, mg_read, mg_parse and mgfClose instead of mg_load.
 * To pass an entity of your own construction to the parser, use
 * the mg_handle function rather than the GLOBAL_mgf_handleCallbacks routines directly.
 * (The first argument to mg_handle is the entity #, or -1.)
 * To free any data structures and clear the parser, use mgfClear.
 * If there is an error, mg_load, mg_open, mg_parse, mg_handle and
 * mg_fgoto will return an error from the list above.  In addition,
 * mg_load will report the error to stderr.  The mgfReadNextLine routine
 * returns 0 when the end of file has been reached.
 */

#define MG_MAXLINE    4096        /* maximum input line length */
#define MG_MAXARGC    (MG_MAXLINE/4)    /* maximum argument count */

class MG_FCTXT {
  public:
    char fname[96];            /* file name */
    FILE *fp;                /* stream pointer */
    int fid;                /* unique file context id */
    char inpline[MG_MAXLINE];        /* input line */
    int lineno;                /* line number */
    char ispipe;                /* flag indicating whether input comes from a pipe or a real file */
    MG_FCTXT *prev;            /* previous context */
};

class MG_FPOS {
  public:
    int fid;                /* file this position is for */
    int lineno;                /* line number in file */
    long offset;                /* offset from beginning */
};

extern MG_FCTXT *mg_file;        /* current file context */

extern void mgfAlternativeInit(int (*handleCallbacks[MGF_TOTAL_NUMBER_OF_ENTITIES])(int, char **));        /* fill in GLOBAL_mgf_handleCallbacks array */
extern int mgfOpen(MG_FCTXT *, char *);    /* open new input file */
extern int mgfReadNextLine();        /* read next line */
extern int mg_parse();        /* parse current line */
extern void mg_fgetpos(MG_FPOS *);    /* get position on input file */
extern int mg_fgoto(MG_FPOS *);    /* go to position on input file */
extern void mgfClose();        /* close input file */
extern void mgfClear();        /* clear parser */
extern int mg_handle(int, int, char **);    /* handle an entity */

#ifndef MG_NQCD
#define MG_NQCD        5        /* default number of divisions */
#endif

extern int mg_nqcdivs;        /* divisions per quarter circle */

/*
 * The following library routines are included for your convenience:
 */

extern int mg_entity(char *name);
extern int isintWords(char *);
extern int isintdWords(char *, char *);
extern int isfltWords(char *);
extern int isfltdWords(char *, char *);
extern int isnameWords(char *);
extern int badarg(int, char **, char *);
extern int e_include(int ac, char **av);
extern int mgfEntitySphere(int ac, char **av);
extern int mgfEntityTorus(int ac, char **av);
extern int mgfEntityCylinder(int ac, char **av);
extern int mgfEntityRing(int ac, char **av);
extern int mgfEntityCone(int ac, char **av);
extern int mgfEntityPrism(int ac, char **av);
extern int e_faceh(int ac, char **av);

/************************************************************************
 *	Definitions for 3-d vector manipulation functions
 */

#define  FLOAT        double
#define  FTINY        (1e-6)
#define  FHUGE        (1e10)

typedef FLOAT FVECT[3];

#define  VCOPY(v1, v2)    ((v1)[0]=(v2)[0],(v1)[1]=(v2)[1],(v1)[2]=(v2)[2])
#define  DOT(v1, v2)    ((v1)[0]*(v2)[0]+(v1)[1]*(v2)[1]+(v1)[2]*(v2)[2])

#define is0vect(v)    (DOT(v,v) <= FTINY*FTINY)

#define round0(x)    if (x <= FTINY && x >= -FTINY) x = 0

extern double normalize(FVECT);    /* normalize a vector */
extern void fcross(FVECT, FVECT, FVECT);/* cross product of two vectors */

/************************************************************************
 *	Definitions for context handling routines
 *	(materials, colors, vectors)
 */

#define C_CMINWL    380        /* minimum wavelength */
#define C_CMAXWL    780        /* maximum wavelength */
#define C_CNSS        41        /* number of spectral samples */
#define C_CWLI        ((C_CMAXWL-C_CMINWL)/(C_CNSS-1))
#define C_CMAXV        10000        /* nominal maximum sample value */
#define C_CLPWM        (683./C_CMAXV)    /* peak lumens/watt multiplier */

#define C_CSSPEC    01        /* flag if spectrum is set */
#define C_CDSPEC    02        /* flag if defined w/ spectrum */
#define C_CSXY        04        /* flag if xy is set */
#define C_CDXY        010        /* flag if defined w/ xy */
#define C_CSEFF        020        /* flag if efficacy set */

class MgfColorContext {
public:
    int clock; // Incremented each change
    short flags; // What's been set
    short ssamp[C_CNSS]; // Spectral samples, min wl to max
    long ssum; // Straight sum of spectral values
    float cx; // Chromaticity X value
    float cy; // chromaticity Y value
    float eff; // efficacy (lumens/watt)
};

#define C_DEFCOLOR    { 1, C_CDXY|C_CSXY|C_CSSPEC|C_CSEFF,\
            {C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
            C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
            C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
            C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
            C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
            C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,\
            C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV,C_CMAXV},\
            (long)C_CNSS*C_CMAXV, 1./3., 1./3., 178.006 }

class MgfMaterialContext {
public:
    int clock; // Incremented each change -- resettable
    int sided; // 1 if surface is 1-sided, 0 for 2-sided
    float nr; // Index of refraction, real and imaginary
    float ni;
    float rd; // Diffuse reflectance
    MgfColorContext rd_c; // Diffuse reflectance color
    float td; // Diffuse transmittance
    MgfColorContext td_c; // Diffuse transmittance color
    float ed; // Diffuse emittance
    MgfColorContext ed_c; // Diffuse emittance color
    float rs; // Specular reflectance
    MgfColorContext rs_c; // Specular reflectance color
    float rs_a; // Specular reflectance roughness
    float ts; // Specular transmittance
    MgfColorContext ts_c; // Specular transmittance color
    float ts_a; // Specular transmittance roughness
};

class MgfVertexContext {
public:
    FVECT p; // Point
    FVECT n; // Normal
    long xid; // transform id of transform last time the vertex was modified (or created)
    int clock; // incremented each change -- resettable
    void *client_data; // client data -- initialized to nullptr by the parser
};

#define C_DEFMATERIAL    {1,0,1.,0.,0.,C_DEFCOLOR,0.,C_DEFCOLOR,0.,C_DEFCOLOR,\
                    0.,C_DEFCOLOR,0.,0.,C_DEFCOLOR,0.}
#define C_DEFVERTEX    {{0.,0.,0.},{0.,0.,0.},0,1,(void *)0}

extern MgfColorContext *c_ccolor;    /* the current color */
extern char *c_ccname;    /* current color name */
extern MgfMaterialContext *c_cmaterial;    /* the current material */
extern char *GLOBAL_mgf_currentMaterialName;    /* current material name */
extern MgfVertexContext *c_cvertex;    /* the current vertex */
extern char *c_cvname;    /* current vertex name */

extern int handleColorEntity(int ac, char **av);        /* handle color entity */
extern int handleMaterialEntity(int ac, char **av);    /* handle material entity */
extern int handleVertexEntity(int ac, char **av);    /* handle vertex entity */
extern void c_clearall();        /* clear context tables */
extern MgfVertexContext *c_getvert(char *name);        /* get a named vertex */
extern void mgfContextFixColorRepresentation(MgfColorContext *clr, int fl);        /* fix color representation */

/*************************************************************************
 *	Definitions for hierarchical object name handler
 */

extern int obj_nnames;        /* depth of name hierarchy */
extern char **obj_name;        /* names in hierarchy */

extern int obj_handler(int, char **);    /* handle an object entity */

/**************************************************************************
 *	Definitions for hierarchical transformation handler
 */

typedef FLOAT MAT4[4][4];

#define  copymat4(m4a, m4b)    memcpy((char *)m4a,(char *)m4b,sizeof(MAT4))

#define  MAT4IDENT        { {1.,0.,0.,0.}, {0.,1.,0.,0.}, \
                {0.,0.,1.,0.}, {0.,0.,0.,1.} }

extern MAT4 m4ident;

#define  setident4(m4)        copymat4(m4, m4ident)

/* regular transformation */
class XF {
  public:
    MAT4 xfm;                /* transform matrix */
    FLOAT sca;                /* scalefactor */
};

#define XF_MAXDIM    8        /* maximum array dimensions */

class XfArrayArg {
  public:
    short i, n;        /* current count and maximum */
    char arg[8];        /* string argument value */
};

class XfArray {
  public:
    MG_FPOS spos;            /* starting position on input */
    int ndim;            /* number of array dimensions */
    XfArrayArg aarg[XF_MAXDIM];
};

class XfSpec {
  public:
    long xid;            /* unique transform id */
    short xac;            /* context argument count */
    short rev;            /* boolean true if vertices reversed */
    XF xf;            /* cumulative transformation */
    XfArray *xarr;        /* transformation array pointer */
    XfSpec *prev;        /* previous transformation context */
};            /* followed by argument buffer */

extern XfSpec *xf_context;            /* current transform context */
extern char **xf_argend;            /* last transform argument */

#define xf_ac(xf)    ((xf)==nullptr ? 0 : (xf)->xac)
#define xf_av(xf)    (xf_argend - (xf)->xac)

#define xf_argc        xf_ac(xf_context)

#define xf_xid(xf)    ((xf)==nullptr ? 0 : (xf)->xid)

/*
 * The transformation handler should do most of the work that needs
 * doing.  Just pass it any xf entities, then use the associated
 * functions to transform and translate positions, transform vectors
 * (without translation), rotate vectors (without scaling) and scale
 * values appropriately.
 *
 * The routines xf_xfmpoint, xf_xfmvect and xf_rotvect take two
 * 3-D vectors (which may be identical), transforms the second and
 * puts the result into the first.
 */

extern int handleTransformationEntity(int ac, char **av);    /* handle xf entity */
extern void xf_xfmpoint(FVECT, FVECT);    /* transform point */
extern void xf_xfmvect(FVECT, FVECT);    /* transform vector */

/* The following are support routines you probably won't call directly */

XfSpec *new_xf(int, char **);        /* allocate new transform */
void free_xf(XfSpec *);        /* free a transform */
int xf_aname(XfArray *);    /* name this instance */
long comp_xfid(MAT4);        /* compute unique ID */
extern void multmat4(MAT4, MAT4, MAT4);    /* m4a = m4b X m4c */
extern void multv3(FVECT, FVECT, MAT4);    /* v3a = v3b X m4 (vectors) */
extern void multp3(FVECT, FVECT, MAT4);    /* p3a = p3b X m4 (positions) */
extern int xf(XF *, int, char **);        /* interpret transform spec. */

/************************************************************************
 *	Miscellaneous definitions
 */

#ifndef MEM_PTR
    #define MEM_PTR void *
#endif

#endif
