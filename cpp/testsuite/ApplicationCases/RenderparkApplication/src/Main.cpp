#include "RpkApplication.h"

int
main(int argc, char *argv[]) {
    RpkApplication * const application = new RpkApplication();
    const int result = application->entryPoint(argc, argv);
    delete application;
    return result;
}
