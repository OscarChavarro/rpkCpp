#include <cstdlib>

#include "common/error.h"
#include "shared/render.h"
#include "shared/canvas.h"
#include "shared/options.h"
#include "app/ui.h"
#include "app/batch.h"

static int batch_processing = false;
static int batch_quit_at_end = false;
static int hres = 0;
static int vres = 0;

static void
BatchOption(void *value) {
    batch_processing = true;
}

static void
BatchQuitOption(void *value) {
    batch_quit_at_end = true;
}

static void
OffscreenOption(void *value) {
}

static CMDLINEOPTDESC interfaceOptions[] = {
    {"-batch",             3, TYPELESS, nullptr, BatchOption,
     "-batch         \t\t: non-interactive processing"},
    {"-batch-quit-at-end", 8, TYPELESS, nullptr, BatchQuitOption,
     "-batch-quit-at-end\t: (batch mode) quit program at end"},
    {"-offscreen",         3, TYPELESS, nullptr, OffscreenOption,
     "-offscreen     \t\t: render into offscreen window"},
    {nullptr, 0, TYPELESS, nullptr, DEFAULT_ACTION, nullptr}
};

/**
Work procedure for batch processing
*/
static void
StartBatchProcessing() {
    while ( !RenderInitialized() );
    RenderScene();

    Batch();
    if ( batch_quit_at_end ) {
        exit(0);
    }
}

void
StartUserInterface(int *argc, char **argv) {
    // All options should have disappeared from argv now
    if ( *argc > 1 ) {
        if ( *argv[1] == '-' ) {
            Error(nullptr, "Unrecognized option '%s'", argv[1]);
        } else {
            ReadFile(argv[1]);
        }
    }

    // Create the window in which to render (canvas window)
    if ( hres <= 0 ) {
        hres = 1920;
    }
    if ( vres <= 0 ) {
        vres = 1080;
    }
    CreateOffscreenCanvasWindow(hres, vres);

    if ( batch_processing ) {
        StartBatchProcessing();
    }
}

void
ParseInterfaceOptions(int *argc, char **argv) {
    ParseOptions(interfaceOptions, argc, argv);
    ParseBatchOptions(argc, argv);
}
