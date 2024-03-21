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
            case 's': // String
                if ( **av == '\0' || isspace(**av)) {
                    return i;
                }
                break;
            case 'i': // Integer
                if ( !isIntDWords(*av, (char *) " \t\r\n")) {
                    return i;
                }
                break;
            case 'f': // Float
                if ( !isFloatDWords(*av, (char *) " \t\r\n")) {
                    return i;
                }
                break;
            default: // Bad call!
                return -1;
        }
    }
    return 0; // All's well
}
