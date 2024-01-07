/*
command line options and defaults
*/

#include <cstring>
#include <cstdlib>

#include "common/linealAlgebra/vectorMacros.h"
#include "material/color.h"
#include "shared/options.h"

static int *_argc;            /* pointer to argument count */
static char **_argv,            /* pointer to current arg value */
**first_arg;        /* pointer to first arg value */

/* initializes the global variables above */
#define init_arg(pargc, argv)    {_argc = pargc; _argv = first_arg = argv;}

/* tests whether arguments remain */
#define args_remain()        (_argv - first_arg < *_argc)

/* skips to next argument value */
#define next_arg()        {_argv++;}

/* consumes the current argument value, that is: removes it from the
 * list. */
static void consume_arg() {
    char **av = _argv;
    while ( av - first_arg < *_argc - 1 ) {
        *av = *(av + 1);
        av++;
    }
    *av = nullptr;
    (*_argc)--;
}

/* scans the current argument value for a value of given format */
#define get_argval(format, res)    (sscanf(*_argv, format, res) == 1)

/* ------------------- integer option values --------------------- */
static int getint(int *n, void *data) {
    if ( !get_argval("%d", n)) {
        fprintf(stderr, "'%s' is not a valid integer value\n", *_argv);
        return false;
    }
    return true;
}

static void printint(FILE *fp, int *n, void *data) {
    fprintf(fp, "%d", *n);
}

static int dummy_int = 0;

CMDLINEOPTTYPE intTypeStruct = {
        (int (*)(void *, void *)) getint,
        (void (*)(FILE *, void *, void *)) printint,
        (void *) &dummy_int,
        nullptr
};

/* ------------------- string option values --------------------- */
static int getstring(char **s, void *data) {
    *s = (char *)malloc(strlen(*_argv) + 1);
    sprintf(*s, "%s", *_argv);
    /*Free(*s, 0);*/
    return true;
}

static void printstring(FILE *fp, char **s, void *data) {
    fprintf(fp, "'%s'", *s ? *s : "");
}

static char *dummy_string = nullptr;

CMDLINEOPTTYPE stringTypeStruct = {
        (int (*)(void *, void *)) getstring,
        (void (*)(FILE *, void *, void *)) printstring,
        (void *) &dummy_string,
        nullptr
};

/* ------------------- copied string (maxlength n) option values --------------------- */
int Tnstring_get(char *s, int n) {
    if ( s ) {
        strncpy(s, *_argv, n);
        s[n - 1] = '\0';  /* ensure zero ending c-string */
    }

    return true;
}

void Tnstring_print(FILE *fp, char *s, int n) {
    fprintf(fp, "'%s'", s ? s : "");
}

char *Tnstring_dummy_val = nullptr;


/* ------------------- enumerated type option values --------------------- */
static void print_enum_vals(ENUMDESC *tab) {
    while ( tab && tab->name ) {
        fprintf(stderr, "\t%s\n", tab->name);
        tab++;
    }
}

int Tenum_get(int *v, ENUMDESC *tab) {
    ENUMDESC *tabsave = tab;
    while ( tab && tab->name ) {
        if ( strncasecmp(*_argv, tab->name, tab->abbrev) == 0 ) {
            *v = tab->value;
            return true;
        }
        tab++;
    }
    fprintf(stderr, "Invalid option argument '%s'. Should be one of:\n", *_argv);
    print_enum_vals(tabsave);
    return false;
}

void Tenum_print(FILE *fp, int *v, ENUMDESC *tab) {
    while ( tab && tab->name ) {
        if ( *v == tab->value ) {
            fprintf(fp, "%s", tab->name);
            return;
        }
        tab++;
    }
    fprintf(fp, "INVALID ENUM VALUE!!!");
}

int Tenum_dummy_val = 0;

/* ------------------- boolean (yes|no) option values-------------------- */
/* implemented as an enumeration type */

static ENUMDESC boolTable[] = {
        {true,  "yes",   1},
        {false, "no",    1},
        {true,  "true",  1},
        {false, "false", 1},
        {0, nullptr,        0}
};

CMDLINEOPTTYPE boolTypeStruct = {
        (int (*)(void *, void *)) Tenum_get,
        (void (*)(FILE *, void *, void *)) Tenum_print,
        (void *) &Tenum_dummy_val,
        (void *) boolTable
};

/* ------------------- set true/false option values --------------------- */

static int dummy_true = true;
static int dummy_false = false;

static int set_true(int *x, void *data) {
    /* No option expected on command line, nothing consumed */

    *x = true;
    return true;
}

static int set_false(int *x, void *data) {
    /* No option expected on command line, nothing consumed */

    *x = false;
    return true;
}

static void print_other(FILE *fp, void *x, void *data) {
    fprintf(fp, "other");
}


CMDLINEOPTTYPE setTrueTypeStruct = {
        (int (*)(void *, void *)) set_true,
        (void (*)(FILE *, void *, void *)) print_other,
        (void *) &dummy_true,
        (void *) nullptr
};

CMDLINEOPTTYPE setFalseTypeStruct = {
        (int (*)(void *, void *)) set_false,
        (void (*)(FILE *, void *, void *)) print_other,
        (void *) &dummy_false,
        (void *) nullptr
};

/* ------------------- float option values --------------------- */
static int getfloat(float *x, void *data) {
    if ( !get_argval("%f", x)) {
        fprintf(stderr, "'%s' is not a valid floating point value\n", *_argv);
        return false;
    }
    return true;
}

static void printfloat(FILE *fp, float *x, void *data) {
    fprintf(fp, "%g", *x);
}

static float dummy_float = 0.;

CMDLINEOPTTYPE floatTypeStruct = {
        (int (*)(void *, void *)) getfloat,
        (void (*)(FILE *, void *, void *)) printfloat,
        (void *) &dummy_float,
        nullptr
};

/* ------------------- Vector3D option values --------------------- */
static int getvector(Vector3D *v, void *data) {
    int ok = true;
    ok = get_argval("%f", &v->x);
    if ( ok ) {
        consume_arg();
        ok &= args_remain() && get_argval("%f", &v->y);
    }
    if ( ok ) {
        consume_arg();
        ok &= args_remain() && get_argval("%f", &v->z);
    }
    if ( !ok ) {
        fprintf(stderr, "invalid vector argument value");
    }

    return ok;
}

static void printvector(FILE *fp, Vector3D *v, void *data) {
    VectorPrint(fp, *v);
}

static Vector3D dummy_vector = {0., 0., 0.};

CMDLINEOPTTYPE vectorTypeStruct = {
        (int (*)(void *, void *)) getvector,
        (void (*)(FILE *, void *, void *)) printvector,
        (void *) &dummy_vector,
        nullptr
};

/* ------------------- RGB option values --------------------- */
static int getrgb(RGB *c, void *data) {
    int ok = true;
    ok = get_argval("%f", &c->r);
    if ( ok ) {
        consume_arg();
        ok &= args_remain() && get_argval("%f", &c->g);
    }
    if ( ok ) {
        consume_arg();
        ok &= args_remain() && get_argval("%f", &c->b);
    }
    if ( !ok ) {
        fprintf(stderr, "invalid RGB color argument value");
    }

    return ok;
}

static void printrgb(FILE *fp, RGB *v, void *data) {
    RGBPrint(fp, *v);
}

static RGB dummy_rgb = {0., 0., 0.};

CMDLINEOPTTYPE rgbTypeStruct = {
        (int (*)(void *, void *)) getrgb,
        (void (*)(FILE *, void *, void *)) printrgb,
        (void *) &dummy_rgb,
        nullptr
};

/* ------------------- CIE xy option values --------------------- */
static int getxy(float *c, void *data) {
    int ok = true;
    ok = get_argval("%f", &c[0]);
    if ( ok ) {
        consume_arg();
        ok &= args_remain() && get_argval("%f", &c[1]);
    }
    if ( !ok ) {
        fprintf(stderr, "invalid CIE xy color argument value");
    }

    return ok;
}

static void printxy(FILE *fp, float *c, void *data) {
    fprintf(fp, "%g %g", c[0], c[1]);
}

static float dummy_xy[2] = {0., 0.};

CMDLINEOPTTYPE xyTypeStruct = {
        (int (*)(void *, void *)) getxy,
        (void (*)(FILE *, void *, void *)) printxy,
        (void *) &dummy_xy,
        nullptr
};

/* ------------------- argument parsing --------------------------- */
static CMDLINEOPTDESC *LookupOption(char *s, CMDLINEOPTDESC *options) {
    CMDLINEOPTDESC *opt = options;
    while ( opt->name ) {
        if ( strncmp(s, opt->name, MAX(opt->abbrevlength > 0 ? opt->abbrevlength : strlen(opt->name), strlen(s))) ==
             0 ) {
            return opt;
        }
        opt++;
    }
    return (CMDLINEOPTDESC *) nullptr;
}

static void process_arg(CMDLINEOPTDESC *options) {
    CMDLINEOPTDESC *opt = LookupOption(*_argv, options);
    if ( opt ) {
        int ok = true;
        if ( opt->type ) {
            if ((opt->type == &setTrueTypeStruct) ||
                (opt->type == &setFalseTypeStruct)) {
                if ( !opt->type->get(opt->value ? opt->value : opt->type->dummy, opt->type->data)) {
                    ok = false;
                }
            } else {
                consume_arg();
                if ( args_remain()) {
                    if ( !opt->type->get(opt->value ? opt->value : opt->type->dummy, opt->type->data)) {
                        ok = false;
                    }
                } else {
                    fprintf(stderr, "Option argument missing.\n");
                    ok = false;
                }
            }
        }
        if ( ok && opt->action ) {
            opt->action(opt->value ? opt->value : (opt->type ? opt->type->dummy : nullptr));
        }
        consume_arg();
    } else next_arg();
}

void parseOptions(CMDLINEOPTDESC *options, int *argc, char **argv) {
    init_arg(argc, argv);
    while ( args_remain()) {
        process_arg(options);
    }
}
