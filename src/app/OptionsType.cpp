/**
Command line options type registry
*/

#include "app/EnumDesc.h"
#include "app/Options.h"
#include "app/OptionsType.h"

/* ------------------- boolean (yes|no) option values-------------------- */
/* implemented as an enumeration type */
EnumDesc OptionsType::boolTable[5] = {
    {true,  "yes",   1},
    {false, "no",    1},
    {true,  "true",  1},
    {false, "false", 1},
    {0, nullptr,     0}
};

OptionsType::OptionsType():
    intType(),
    boolType(),
    setTrueType(),
    setFalseType(),
    stringType(),
    floatType(),
    vectorType(),
    rgbType(),
    xyType(),
    dummyInt(0),
    dummyString(nullptr),
    dummyTrue(true),
    dummyFalse(false),
    dummyFloat(0.0f),
    dummyVector3D(0.0f, 0.0f, 0.0f),
    dummyRgb(),
    dummyCieXy{0.0f, 0.0f}
{
    intType = {Options::optionsGetInt, Options::optionsPrintInt, static_cast<void *>(&dummyInt), nullptr};
    boolType = {Options::optionsEnumGet, Options::optionsEnumPrint, static_cast<void *>(&dummyInt), static_cast<void *>(OptionsType::boolTable)};
    setTrueType = {Options::optionsSetTrue, Options::optionsPrintOther, static_cast<void *>(&dummyTrue), nullptr};
    setFalseType = {Options::optionsSetFalse, Options::optionsPrintOther, static_cast<void *>(&dummyFalse), nullptr};
    stringType = {Options::optionsGetString, Options::optionsPrintString, static_cast<void *>(&dummyString), nullptr};
    floatType = {Options::optionsGetfloat, Options::optionsPrintFloat, static_cast<void *>(&dummyFloat), nullptr};
    vectorType = {Options::optionsGetVector, Options::optionsPrintVector, static_cast<void *>(&dummyVector3D), nullptr};
    rgbType = {Options::optionsGetRgb, Options::optionsPrintRgb, static_cast<void *>(&dummyRgb), nullptr};
    xyType = {Options::optionsGetCieXy, Options::optionsPrintCieXyCallBack, static_cast<void *>(dummyCieXy), nullptr};
}
