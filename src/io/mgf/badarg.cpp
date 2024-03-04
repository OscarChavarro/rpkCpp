#include <cctype>

#include "io/mgf/words.h"
#include "io/mgf/badarg.h"

/**
Check argument list against format string.
*/
int
checkForBadArguments(int ac, char **av, char *fl) {
    // Check argument list
    if ( fl == nullptr ) {
	    // No arguments?
        fl = (char *)"";
    }
    for ( int i = 1; *fl; i++, av++, fl++ ) {
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
                if ( !isIntDWords(*av, (char *) " \t\r\n")) {
                    return i;
                }
                break;
            case 'f': // float
                if ( !isFloatDWords(*av, (char *) " \t\r\n")) {
                    return i;
                }
                break;
            default: // bad call!
                return -1;
        }
    }
    return 0; // all's well
}
