package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.material.BsdfComponent;

/**
An array of chain lists indexed by length
*/
public class ContribHandler {
    private static final byte BSDF_ALL_COMPONENTS = (byte)(
        BsdfComponent.BRDF_DIFFUSE_COMPONENT
            | BsdfComponent.BRDF_GLOSSY_COMPONENT
            | BsdfComponent.BRDF_SPECULAR_COMPONENT
            | BsdfComponent.BTDF_DIFFUSE_COMPONENT
            | BsdfComponent.BTDF_GLOSSY_COMPONENT
            | BsdfComponent.BTDF_SPECULAR_COMPONENT);

    public FlagChainList[] array;
    public int maxLength;

    public ContribHandler() {
        array = null;
        maxLength = 0;
    }

    public void init(int paramMaxLength) {
        this.maxLength = paramMaxLength;

        // For each length we need a chain list
        array = new FlagChainList[paramMaxLength + 1]; // 0 <= length <= maxlength !!
        for ( int i = 0; i <= paramMaxLength; i++ ) {
            array[i] = new FlagChainList();
        }
    }

    public ColorRgb compute(BiPath path) {
        ColorRgb result = new ColorRgb();
        int length;

        result.clear();

        length = path.m_eyeSize + path.m_lightSize;

        if ( length > maxLength ) {
            Error.error("CContribHandler::Compute", "Path too long !!");
            return result;
        }

        return array[length].compute(path);
    }

    public void addRegExp(String regExp) {
        if ( regExp == null || regExp.isEmpty() ) {
            return;
        }

        /**
        add a group of paths
        regExp indicates the regular expression covered by the sampling strategy
        The class of covered paths covered by the contrib handler is : (regSPaR)(regPath)
        regSPar is not needed here. The regExp must ensure disjoint paths!
        */
        if ( regExp.charAt(0) == '-' ) {
            doRegExp(regExp.substring(1), true);
        } else {
            doRegExp(regExp, false);
        }
    }

    protected void doRegExp(String regExp, boolean subtract) {
        doRegExpGeneral(regExp, subtract);
    }

    protected void doSyntaxError(String errString) {
        Error.error("Flag chain Syntax Error", errString);
        init(maxLength);
    }

    protected boolean getFlags(String regExp, int[] pos, byte[] flags) {
        char c;
        int p = pos[0];

        flags[0] = 0;

        if ( p >= regExp.length() || regExp.charAt(p++) != '(' ) {
            doSyntaxError("getFlags: '(' expected");
            return false;
        }

        while ( true ) {
            if ( p >= regExp.length() ) {
                doSyntaxError("getFlags: ')' expected");
                return false;
            }

            c = regExp.charAt(p++);
            if ( c == ')' ) {
                break;
            }

            switch ( c ) {
                case 'S':
                    if ( p < regExp.length() && regExp.charAt(p) == 'T' ) {
                        p++;
                        flags[0] |= (byte)BsdfComponent.BTDF_SPECULAR_COMPONENT;
                    } else if ( p < regExp.length() && regExp.charAt(p) == 'R' ) {
                        p++;
                        flags[0] |= (byte)BsdfComponent.BRDF_SPECULAR_COMPONENT;
                    } else {
                        flags[0] |= (byte)(BsdfComponent.BTDF_SPECULAR_COMPONENT | BsdfComponent.BRDF_SPECULAR_COMPONENT);
                    }
                    break;
                case 'G':
                    if ( p < regExp.length() && regExp.charAt(p) == 'T' ) {
                        p++;
                        flags[0] |= (byte)BsdfComponent.BTDF_GLOSSY_COMPONENT;
                    } else if ( p < regExp.length() && regExp.charAt(p) == 'R' ) {
                        p++;
                        flags[0] |= (byte)BsdfComponent.BRDF_GLOSSY_COMPONENT;
                    } else {
                        flags[0] |= (byte)(BsdfComponent.BTDF_GLOSSY_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT);
                    }
                    break;
                case 'D':
                    if ( p < regExp.length() && regExp.charAt(p) == 'T' ) {
                        p++;
                        flags[0] |= (byte)BsdfComponent.BTDF_DIFFUSE_COMPONENT;
                    } else if ( p < regExp.length() && regExp.charAt(p) == 'R' ) {
                        p++;
                        flags[0] |= (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT;
                    } else {
                        flags[0] |= (byte)(BsdfComponent.BTDF_DIFFUSE_COMPONENT | BsdfComponent.BRDF_DIFFUSE_COMPONENT);
                    }
                    break;
                case 'X':
                    if ( p < regExp.length() && regExp.charAt(p) == 'T' ) {
                        p++;
                        flags[0] |= (byte)(BsdfComponent.BTDF_DIFFUSE_COMPONENT | BsdfComponent.BTDF_GLOSSY_COMPONENT |
                            BsdfComponent.BTDF_SPECULAR_COMPONENT);
                    } else if ( p < regExp.length() && regExp.charAt(p) == 'R' ) {
                        p++;
                        flags[0] |= (byte)(BsdfComponent.BRDF_DIFFUSE_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT |
                            BsdfComponent.BRDF_SPECULAR_COMPONENT);
                    } else {
                        flags[0] |= BSDF_ALL_COMPONENTS;
                    }
                    break;
                case 'L':
                    if ( p >= regExp.length() || regExp.charAt(p) != 'X' ) {
                        doSyntaxError("getFlags: No 'X' after 'L'. Only LX supported");
                        return false;
                    }
                    p++;
                    flags[0] = BSDF_ALL_COMPONENTS;
                    break;
                case 'E':
                    if ( p >= regExp.length() || regExp.charAt(p) != 'X' ) {
                        doSyntaxError("getFlags: No 'X' after 'E'. Only EX supported");
                        return false;
                    }
                    p++;
                    flags[0] = BSDF_ALL_COMPONENTS;
                    break;
                case '|':
                    break;  // Do Nothing because we don't support other operators
                default:
                    doSyntaxError("getFlags: Unexpected character in token");
                    return false;
            }
        }

        pos[0] = p;
        return true;
    }

    protected boolean getToken(String regExp, int[] pos, char[] token, byte[] flags) {
        if ( pos[0] >= regExp.length() ) {
            return false;
        }

        switch ( regExp.charAt(pos[0]) ) {
            case '\0':
                return false;
            case '+':
                token[0] = '+';
                pos[0]++;
                break;
            case '*':
                token[0] = '*';
                pos[0]++;
                break;
            case '(':
                token[0] = 'F';
                return getFlags(regExp, pos, flags);
            default:
                doSyntaxError("Unknown token");
                return false;
        }

        return true;
    }

    protected void doRegExpGeneral(String regExp, boolean subtract) {
        FlagChain c = new FlagChain();

        // Build iteration arrays (not tree so no nested brackets!)
        final int MAX_REGEXP_ITEMS = 15;

        byte[] flagArray = new byte[MAX_REGEXP_ITEMS];
        char[] typeArray = new char[MAX_REGEXP_ITEMS];
        int[] countArray = new int[MAX_REGEXP_ITEMS];
        int[] pos = new int[] {0};
        int tokenCount = -1;
        int iteratorCount = 0;
        char[] token = new char[1];
        byte[] data = new byte[1];

        while ( getToken(regExp, pos, token, data) ) {
            if ( token[0] == 'F' ) {
                // A flag was read

                if ( tokenCount == MAX_REGEXP_ITEMS - 1 ) {
                    doSyntaxError("Too many tokens in regexp");
                    return;
                }

                tokenCount++;
                flagArray[tokenCount] = data[0];
                typeArray[tokenCount] = ' ';
                countArray[tokenCount] = 0;
            } else {
                // An iteration token was read
                if ( tokenCount == -1 ) {
                    doSyntaxError("Initial iteration token");
                    return;
                }

                if ( token[0] == '+' ) {
                    // Transform '+' in ' *'
                    if ( tokenCount == MAX_REGEXP_ITEMS - 1 ) {
                        doSyntaxError("Too many tokens in regexp");
                        return;
                    }

                    flagArray[tokenCount + 1] = flagArray[tokenCount];
                    tokenCount++;
                    token[0] = '*';
                }

                typeArray[tokenCount] = token[0];
                countArray[tokenCount] = 0;

                if ( (token[0] == '*') || (token[0] == '+') ) {
                    iteratorCount++;
                }
            }
        }

        if ( tokenCount == -1 ) {
            // No tokens read ?!
            doSyntaxError("No tokens in regexp");
            return;
        }

        tokenCount++;
        typeArray[tokenCount] = 0;

        // Iterate all possible lengths
        int beginLength = tokenCount - iteratorCount;
        int endLength = maxLength;
        int iteratorsFound;
        int remember;
        int maxIteration;
        int iterationsDone;
        int nextIterationsDone;
        int num;
        boolean done;

        if ( iteratorCount == 0 ) {
            // No iterators, we need just one chain length

            endLength = beginLength;
        }

        for ( int length = beginLength; length <= endLength; length++ ) {
            FlagChainList tmpList = new FlagChainList();
            c.init(length, subtract);

            maxIteration = length - tokenCount + iteratorCount;

            done = false;

            iterationsDone = 0;
            nextIterationsDone = 0;

            while ( !done ) {
                iteratorsFound = 0;
                remember = 0;
                pos[0] = 0; // Number of flags filled in

                for ( int i = 0; i < tokenCount; i++ ) {
                    if ( typeArray[i] == ' ' ) {
                        if ( c.chain != null ) {
                            c.chain[pos[0]] = flagArray[i];
                        }
                        pos[0]++;
                    } else {
                        // typeArray[i] == '*' !  Choose a number

                        iteratorsFound++;
                        if ( iteratorsFound == iteratorCount ) {
                            // Last iterator : fill in
                            num = maxIteration - iterationsDone;
                            if ( iteratorCount == 1 || remember != 0 ) {
                                done = true;  // Only one possible combination or all tried
                            }
                        } else {
                            num = countArray[i];
                            if ( iteratorsFound == 1 ) {
                                // First Iterator
                                countArray[i]++;
                                nextIterationsDone++;
                                if ( nextIterationsDone > maxIteration ) {
                                    // Too many
                                    nextIterationsDone -= countArray[i];
                                    countArray[i] = 0;
                                    remember = 1;
                                } else {
                                    remember = 0;
                                }
                            } else {
                                // In between iterator
                                if ( remember != 0 ) {
                                    // Overflow for next iteration
                                    countArray[i]++;
                                    nextIterationsDone++;
                                    if ( nextIterationsDone > maxIteration ) {
                                        // Too many
                                        nextIterationsDone -= countArray[i];
                                        countArray[i] = 0;
                                        remember = 1;
                                    } else {
                                        remember = 0;
                                    }
                                }
                            }
                        }

                        // Set num flags
                        for ( int j = 0; j < num; j++ ) {
                            if ( c.chain != null ) {
                                c.chain[pos[0]] = flagArray[i];
                            }
                            pos[0]++;
                        }
                    }
                }

                iterationsDone = nextIterationsDone;
                tmpList.addDisjoint(c);

                if ( iteratorCount == 0 ) {
                    done = true;
                }
            }

            array[length].add(tmpList.simplify()); // Add all chains
        }
    }
}
