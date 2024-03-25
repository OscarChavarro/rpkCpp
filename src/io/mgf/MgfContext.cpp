#include <cstring>

#include "io/mgf/MgfContext.h"

MgfContext::MgfContext():
    radianceMethod(),
    singleSided(),
    currentVertexName(),
    numberOfQuarterCircleDivisions(),
    monochrome(),
    entityNames(),
    errorCodeMessages(),
    readerContext(),
    handleCallbacks(),
    supportCallbacks()
{
    strcpy(entityNames[0], "#");
    strcpy(entityNames[1], "c");
    strcpy(entityNames[2], "cct");
    strcpy(entityNames[3], "cone");
    strcpy(entityNames[4], "cmix");
    strcpy(entityNames[5], "cspec");
    strcpy(entityNames[6], "cxy");
    strcpy(entityNames[7], "cyl");
    strcpy(entityNames[8], "ed");
    strcpy(entityNames[9], "f");
    strcpy(entityNames[10], "i");
    strcpy(entityNames[11], "ies");
    strcpy(entityNames[12], "ir");
    strcpy(entityNames[13], "m");
    strcpy(entityNames[14], "n");
    strcpy(entityNames[15], "o");
    strcpy(entityNames[16], "p");
    strcpy(entityNames[17], "prism");
    strcpy(entityNames[18], "rd");
    strcpy(entityNames[19], "ring");
    strcpy(entityNames[20], "rs");
    strcpy(entityNames[21], "sides");
    strcpy(entityNames[22], "sph");
    strcpy(entityNames[23], "td");
    strcpy(entityNames[24], "torus");
    strcpy(entityNames[25], "ts");
    strcpy(entityNames[26], "v");
    strcpy(entityNames[27], "xf");
    strcpy(entityNames[28], "fh");

	errorCodeMessages[0] = "No error";
	errorCodeMessages[1] =  "Unknown entity";
	errorCodeMessages[2] =  "Wrong number of arguments";
	errorCodeMessages[3] =  "Wrong argument type";
	errorCodeMessages[4] =  "Illegal argument value";
	errorCodeMessages[5] =  "Undefined reference";
	errorCodeMessages[6] =  "Cannot open input file";
	errorCodeMessages[7] =  "Error in included file";
	errorCodeMessages[8] =  "Out of memory";
	errorCodeMessages[9] =  "Seek failure";
	errorCodeMessages[10] =  "Illegal material specification";
	errorCodeMessages[11] =  "Input line too long";
	errorCodeMessages[12] =  "Unmatched context close";
}
