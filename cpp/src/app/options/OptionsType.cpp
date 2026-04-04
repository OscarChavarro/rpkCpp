/**
Command line options type registry
*/

#include "app/options/EnumDesc.h"
#include "app/options/Options.h"
#include "app/options/OptionsType.h"

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
    intType = {Options::optionsGetInt, Options::optionsPrintInt, OptionValueWrapper(static_cast<void *>(&dummyInt), OptionKind::INT), nullptr};
    boolType = {Options::optionsEnumGet, Options::optionsEnumPrint, OptionValueWrapper(static_cast<void *>(&dummyInt), OptionKind::BOOL), static_cast<void *>(OptionsType::boolTable)};
    setTrueType = {Options::optionsSetTrue, Options::optionsPrintOther, OptionValueWrapper(static_cast<void *>(&dummyTrue), OptionKind::BOOL), nullptr};
    setFalseType = {Options::optionsSetFalse, Options::optionsPrintOther, OptionValueWrapper(static_cast<void *>(&dummyFalse), OptionKind::BOOL), nullptr};
    stringType = {Options::optionsGetString, Options::optionsPrintString, OptionValueWrapper(static_cast<void *>(&dummyString), OptionKind::STRING), nullptr};
    floatType = {Options::optionsGetfloat, Options::optionsPrintFloat, OptionValueWrapper(static_cast<void *>(&dummyFloat), OptionKind::FLOAT), nullptr};
    vectorType = {Options::optionsGetVector, Options::optionsPrintVector, OptionValueWrapper(static_cast<void *>(&dummyVector3D), OptionKind::VECTOR3D), nullptr};
    rgbType = {Options::optionsGetRgb, Options::optionsPrintRgb, OptionValueWrapper(static_cast<void *>(&dummyRgb), OptionKind::COLORRGB), nullptr};
    xyType = {Options::optionsGetCieXy, Options::optionsPrintCieXyCallBack, OptionValueWrapper(static_cast<void *>(dummyCieXy), OptionKind::FLOAT), nullptr};
}
