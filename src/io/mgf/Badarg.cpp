#include "java/lang/Character.h"
#include "io/context/WordsContext.h"
#include "io/mgf/Badarg.h"

/**
Check argument list against format string
*/
int
checkForBadArguments(int ac, char **av, const char *fl) {
    // Check argument list
    if ( fl == nullptr ) {
	    // No arguments?
        fl = "";
    }
    for ( int formatIndex = 0; fl[formatIndex] != '\0'; formatIndex++ ) {
        const int argumentIndex = formatIndex + 1;
        if ( argumentIndex > ac || av[formatIndex] == nullptr ) {
            return -1;
        }
        switch ( fl[formatIndex] ) {
            case 's': // String
                if ( av[formatIndex][0] == '\0' || java::Character::isSpace(av[formatIndex][0]) ) {
                    return argumentIndex;
                }
                break;
            case 'i': // Integer
                if ( !WordsContext::isIntDelimited(av[formatIndex], " \t\r\n") ) {
                    return argumentIndex;
                }
                break;
            case 'f': // Float
                if ( !WordsContext::isFloatDelimited(av[formatIndex], " \t\r\n") ) {
                    return argumentIndex;
                }
                break;
            default: // Bad call!
                return -1;
        }
    }
    return 0; // All's well
}
