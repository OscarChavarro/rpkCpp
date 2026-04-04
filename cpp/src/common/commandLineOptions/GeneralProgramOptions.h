#ifndef __APP_GENERAL_PROGRAM_OPTIONS__
#define __APP_GENERAL_PROGRAM_OPTIONS__

#include "common/commandLineOptions/TypedOption.h"

class GeneralProgramOptionsRegistry {
  public:
    GeneralProgramOptionsRegistry(
        int &outputImageWidth,
        int &outputImageHeight,
        int &numberOfQuarterCircleDivisions,
        int &yesValue,
        int &noValue,
        int &glutDebugEnabled,
        void (*mainForceOneSidedOption)(int &),
        void (*mainMonochromeOption)(int &),
        void (*setIntTrue)(int &))
        : widthOpt{"-width", &outputImageWidth, 1, nullptr, nullptr},
          heightOpt{"-height", &outputImageHeight, 1, nullptr, nullptr},
          nqcdivsOpt{"-nqcdivs", &numberOfQuarterCircleDivisions, 1, nullptr, nullptr},
          forceOneSidedOpt{"-force-onesided", &yesValue, 0, mainForceOneSidedOption, nullptr},
          dontForceOneSidedOpt{"-dont-force-onesided", &noValue, 0, mainForceOneSidedOption, nullptr},
          monochromaticOpt{"-monochromatic", &yesValue, 0, mainMonochromeOption, nullptr},
          glutDebugOpt{"-glutDebug", &glutDebugEnabled, 0, setIntTrue, nullptr},
          registry{
              REGISTER_OPTION(int, widthOpt, 5),
              REGISTER_OPTION(int, heightOpt, 6),
              REGISTER_OPTION(int, nqcdivsOpt, 3),
              REGISTER_OPTION(int, forceOneSidedOpt, 10),
              REGISTER_OPTION(int, dontForceOneSidedOpt, 14),
              REGISTER_OPTION(int, monochromaticOpt, 5),
              REGISTER_OPTION(int, glutDebugOpt, 6)
          }
    {
    }

    int count() const {
        return 7;
    }

    OptionBase *entries() {
        return registry;
    }

  private:
    TypedOption<int> widthOpt;
    TypedOption<int> heightOpt;
    TypedOption<int> nqcdivsOpt;
    TypedOption<int> forceOneSidedOpt;
    TypedOption<int> dontForceOneSidedOpt;
    TypedOption<int> monochromaticOpt;
    TypedOption<int> glutDebugOpt;
    OptionBase registry[7];
};

#endif
