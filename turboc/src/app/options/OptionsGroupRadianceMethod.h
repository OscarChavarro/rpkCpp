
#include "common/VSDK.h"
#ifndef CMMND_LINE_RDNC_MTHD_OPTNS_GRP
#define CMMND_LINE_RDNC_MTHD_OPTNS_GRP

class OptionsGroupRadianceMethod{ public:
    static void radianceMethodParseOptions( int *argc, char **argv, char *radianceMethodsStringOut);
};

#endif
