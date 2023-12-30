#include <cctype>
#include <cstdlib>

#include "MGF/words.h"

/**
Check argument list against format string.
*/
int
badarg(int ac, char **av, char *fl) {
    // check argument list
    int i;

    if ( fl == nullptr ) {
	// no arguments?
        fl = (char *)"";
    }
    for ( i = 1; *fl; i++, av++, fl++ ) {
        if ( i > ac || *av == nullptr ) {
            return -1;
        }
        switch ( *fl ) {
            case 's': // string
                if ( **av == '\0' || isspace(**av)) {
                    return i;
                }
                break;
            case 'i': // integer
                if ( !isintdWords(*av, (char *)" \t\r\n")) {
                    return i;
                }
                break;
            case 'f': // float
                if ( !isfltdWords(*av, (char *)" \t\r\n")) {
                    return i;
                }
                break;
            default: // bad call!
                return -1;
        }
    }
    return 0; // all's well
}
