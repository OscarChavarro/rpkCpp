template<typename TOptionBase>
bool OptionParser<TOptionBase>::parse(int *argc, char **argv, TOptionBase *registry, int registryCount) {
    OptionGroupT<TOptionBase> singleGroup("default", registry, registryCount);
    return parse(argc, argv, &singleGroup, 1);
}

template<typename TOptionBase>
bool OptionParser<TOptionBase>::parse(int *argc, char **argv, OptionGroupT<TOptionBase> *groups, int groupCount) {
    if ( groups == NULL || groupCount <= 0 || argc == NULL || argv == NULL ) {
        return false;
    }

    for ( int i = 0; i < *argc; i++ ) {
        if ( argv[i] == NULL ) {
            continue;
        }
        bool matched = false;
        for ( int g = 0; g < groupCount && !matched; g++ ) {
            if ( groups[g].options == NULL || groups[g].count <= 0 ) {
                continue;
            }
            for ( int j = 0; j < groups[g].count; j++ ) {
                if ( !groups[g].options[j].isConfigured() ) {
                    continue;
                }
                if ( !matchOption(argv[i], groups[g].options[j].getName(), groups[g].options[j].getAbbreviationLength()) ) {
                    continue;
                }
                int consumesValue = groups[g].options[j].consumesValue();
                if ( consumesValue != 0 ) {
                    if ( i + consumesValue >= *argc ) {
                        return false;
                    }
                    bool missingValue = false;
                    for ( int k = 1; k <= consumesValue; k++ ) {
                        if ( argv[i + k] == NULL ) {
                            missingValue = true;
                            break;
                        }
                    }
                    if ( missingValue ) {
                        return false;
                    }
                    if ( !groups[g].options[j].apply(consumesValue, argv + i + 1) ) {
                        return false;
                    }
                    argv[i] = NULL;
                    for ( int clearIndex = 1; clearIndex <= consumesValue; clearIndex++ ) {
                        argv[i + clearIndex] = NULL;
                    }
                    i += consumesValue;
                } else {
                    if ( !groups[g].options[j].apply(0, NULL) ) {
                        return false;
                    }
                    argv[i] = NULL;
                }
                matched = true;
                break;
            }
        }
    }

    int writeIndex = 0;
    for ( int readIndex = 0; readIndex < *argc; readIndex++ ) {
        if ( argv[readIndex] != NULL ) {
            argv[writeIndex++] = argv[readIndex];
        }
    }
    for ( int clearIndex = writeIndex; clearIndex < *argc; clearIndex++ ) {
        argv[clearIndex] = NULL;
    }
    *argc = writeIndex;
    return true;
}
