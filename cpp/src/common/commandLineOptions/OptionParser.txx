template<typename TOptionBase>
bool OptionParser<TOptionBase>::parse(int *argc, char **argv, TOptionBase *registry, int registryCount) {
    if ( registry == nullptr || argc == nullptr || argv == nullptr ) {
        return false;
    }

    for ( int i = 0; i < *argc; i++ ) {
        if ( argv[i] == nullptr ) {
            continue;
        }
        for ( int j = 0; j < registryCount; j++ ) {
            if ( !registry[j].isConfigured() ) {
                continue;
            }
            if ( !matchOption(argv[i], registry[j].getName(), registry[j].getAbbreviationLength()) ) {
                continue;
            }
            int consumesValue = registry[j].consumesValue();
            if ( consumesValue != 0 ) {
                if ( i + consumesValue >= *argc ) {
                    return false;
                }
                bool missingValue = false;
                for ( int k = 1; k <= consumesValue; k++ ) {
                    if ( argv[i + k] == nullptr ) {
                        missingValue = true;
                        break;
                    }
                }
                if ( missingValue ) {
                    return false;
                }
                if ( !registry[j].apply(consumesValue, argv + i + 1) ) {
                    return false;
                }
                argv[i] = nullptr;
                for ( int k = 1; k <= consumesValue; k++ ) {
                    argv[i + k] = nullptr;
                }
                i += consumesValue;
            } else {
                if ( !registry[j].apply(0, nullptr) ) {
                    return false;
                }
                argv[i] = nullptr;
            }
            break;
        }
    }

    int writeIndex = 0;
    for ( int readIndex = 0; readIndex < *argc; readIndex++ ) {
        if ( argv[readIndex] != nullptr ) {
            argv[writeIndex++] = argv[readIndex];
        }
    }
    for ( int i = writeIndex; i < *argc; i++ ) {
        argv[i] = nullptr;
    }
    *argc = writeIndex;
    return true;
}
