#include "app/RpkApplication.h"

int
main(int argc, char *argv[]) {
    RpkApplication *application = new RpkApplication();
    int result = application->entryPoint(argc, argv);
    delete application;
    return result;
}
