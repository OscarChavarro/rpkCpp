package vsdk.toolkit.io.mgf;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.io.context.EntityTypeContext;
import vsdk.toolkit.io.context.FilePositionContext;
import vsdk.toolkit.io.context.ParseErrorContext;
import vsdk.toolkit.io.context.ParseRuntimeContext;
import vsdk.toolkit.io.context.TokenValidationContext;
import vsdk.toolkit.io.context.TransformArrayContext;
import vsdk.toolkit.io.context.TransformContext;
import vsdk.toolkit.io.context.TransformScopeContext;
import vsdk.toolkit.io.context.TransformSequenceContext;
import vsdk.toolkit.io.context.TransformStackContext;

/**
Routines for 4x4 homogeneous, rigid-body transformations
*/
public class MgfTransformationSupport {
    /**
    Compute unique ID from matrix
    */
    private static long computeUniqueId(Matrix4x4d xfm) {
        byte[] shiftTab = new byte[] {
            15, 5, 11, 5, 6, 3, 9, 15,
            13, 2, 13, 5, 2, 12, 14, 11,
            11, 12, 12, 3, 2, 11, 8, 12,
            1, 12, 5, 4, 15, 9, 14, 5,
            13, 14, 2, 10, 10, 14, 12, 3,
            5, 5, 14, 6, 12, 11, 13, 9,
            12, 8, 1, 6, 5, 12, 7, 13,
            15, 8, 9, 2, 6, 11, 9, 11
        };
        long xid = 0;

        // Compute unique transform id
        ByteBuffer buffer = ByteBuffer.allocate(16 * 8).order(ByteOrder.LITTLE_ENDIAN);
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                buffer.putDouble(xfm.m[r][c]);
            }
        }
        byte[] raw = buffer.array();
        for (int i = 0; i + 1 < raw.length; i += 2) {
            int value = (raw[i] & 0xff) | ((raw[i + 1] & 0xff) << 8);
            xid ^= ((long)value) << shiftTab[(i / 2) & 63];
        }
        return xid;
    }

    private static double d2r(double a) {
        return (Math.PI / 180.0) * a;
    }

    /**
    Check argument list against format string
    */
    private static int checkForBadArguments(int ac, String[] av, String fl) {
        if (fl == null) {
            // No arguments?
            fl = "";
        }
        for (int formatIndex = 0; formatIndex < fl.length(); formatIndex++) {
            final int argumentIndex = formatIndex + 1;
            if (argumentIndex > ac || av[formatIndex] == null) {
                return -1;
            }
            switch (fl.charAt(formatIndex)) {
                case 's': // String
                    if (av[formatIndex].isEmpty() || Character.isWhitespace(av[formatIndex].charAt(0))) {
                        return argumentIndex;
                    }
                    break;
                case 'i': // Integer
                    if (!TokenValidationContext.isIntDelimited(av[formatIndex], " \t\r\n")) {
                        return argumentIndex;
                    }
                    break;
                case 'f': // Float
                    if (!TokenValidationContext.isFloatDelimited(av[formatIndex], " \t\r\n")) {
                        return argumentIndex;
                    }
                    break;
                default: // Bad call!
                    return -1;
            }
        }
        return 0; // All's well
    }

    private static boolean checkArgument(int a, String l, int ac, String[] av, int i) {
        if (av[i].length() != a) {
            return false;
        }
        String[] tail = new String[Math.max(0, ac - i - 1)];
        if (tail.length > 0) {
            System.arraycopy(av, i + 1, tail, 0, tail.length);
        }
        if (checkForBadArguments(ac - i - 1, tail, l) != 0) {
            return false;
        }
        return true;
    }

    /**
    Put out name for this instance
    */
    private static int transformName(TransformSequenceContext ap, ParseRuntimeContext context) {
        String[] oav = new String[] {
            context.entityNames[EntityTypeContext.OBJECT],
            null
        };
        if (ap == null) {
            return MgfEntityControl.mgfHandle(EntityTypeContext.OBJECT, 1, oav, context);
        }
        StringBuilder oName = new StringBuilder(10 * TransformSequenceContext.TRANSFORM_MAXIMUM_DIMENSIONS);
        oName.append('a');
        for (int i = 0; i < ap.numberOfDimensions; i++) {
            oName.append(ap.transformArguments[i].arg);
            oName.append('.');
        }
        oav[1] = oName.toString();
        return MgfEntityControl.mgfHandle(EntityTypeContext.OBJECT, 2, oav, context);
    }

    /**
    Allocate new transform structure
    */
    private static TransformStackContext newTransform(int ac, String[] av, ParseRuntimeContext context) {
        TransformScopeContext stack = context.transformStack;
        int nDim = 0;
        final int previousArgumentCount = stack.argumentCountFor(context.transformContext);

        // Compute space required by arguments
        for (int i = 0; i < ac; i++) {
            if ("-a".equals(av[i])) {
                nDim++;
                i++;
            }
        }
        if (nDim > TransformSequenceContext.TRANSFORM_MAXIMUM_DIMENSIONS) {
            return null;
        }

        TransformStackContext spec = new TransformStackContext();
        spec.ownedArgumentCount = (short)ac;
        if (ac > 0) {
            spec.ownedArgumentCopies = new String[ac];
        }

        if (nDim != 0) {
            spec.transformationArray = new TransformSequenceContext();
            FilePositionContext fp = spec.transformationArray.startingPosition;
            MgfEntityControl.mgfGetFilePosition(fp, context);
            spec.transformationArray.numberOfDimensions = 0; // Incremented below
        } else {
            spec.transformationArray = null;
        }
        spec.xac = (short)(ac + previousArgumentCount);

        // Allocate the argument list with the new arguments first and inherited
        // arguments appended after them.
        String[] newArgumentList = null;
        if (spec.xac > 0) {
            newArgumentList = new String[spec.xac + 1];

            final int previousStartIndex = stack.argumentCount - previousArgumentCount;
            for (int i = 0; i < previousArgumentCount; i++) {
                newArgumentList[ac + i] = stack.argumentList[previousStartIndex + i];
            }
            newArgumentList[spec.xac] = null;
        }
        stack.argumentList = newArgumentList;
        stack.argumentCount = spec.xac;

        // Use memory allocated above
        for (int i = 0; i < ac; i++) {
            if ("-a".equals(av[i])) {
                stack.argumentList[i] = stack.iterateArgument;
                spec.ownedArgumentCopies[i] = null;
                i++;
                TransformArrayContext transformArgument =
                    spec.transformationArray.transformArguments[spec.transformationArray.numberOfDimensions];
                transformArgument.arg = "0";
                transformArgument.argumentIndex = (short)i;
                spec.ownedArgumentCopies[i] = null;
                stack.argumentList[i] = transformArgument.arg;
                transformArgument.i = 0;
                transformArgument.n = (short)Integer.parseInt(av[i]);
                spec.transformationArray.numberOfDimensions++;
            } else {
                String argumentCopy = av[i];
                stack.argumentList[i] = argumentCopy;
                spec.ownedArgumentCopies[i] = argumentCopy;
            }
        }
        return spec;
    }

    /**
    Transform a point by the current matrix
    */
    public static void mgfTransformPoint(Vector3Dd v1, Vector3Dd v2, ParseRuntimeContext context) {
        if (context.transformContext == null) {
            v1.copy(v2);
            return;
        }
        context.transformContext.xf.transformMatrix.multiplyWithTranslation(v1, v2);
    }

    /**
    Transform a vector using current matrix
    */
    public static void mgfTransformVector(Vector3Dd v1, Vector3Dd v2, ParseRuntimeContext context) {
        if (context.transformContext == null) {
            v1.copy(v2);
            return;
        }
        context.transformContext.xf.transformMatrix.multiply(v1, v2);
    }

    private static void finish(int count, TransformContext ret, Matrix4x4d transformMatrix, double scaTransform) {
        while (count-- > 0) {
            Matrix4x4d.multiplyMatrix4(ret.transformMatrix, ret.transformMatrix, transformMatrix);
            ret.scaleFactor *= scaTransform;
        }
    }

    /**
    Get transform specification
    */
    private static int xf(TransformContext ret, int ac, String[] av) {
        ret.transformMatrix.identity();
        ret.scaleFactor = 1.0;

        int counter = 1;
        Matrix4x4d transformMatrix = new Matrix4x4d();
        transformMatrix.identity();
        double scaTransform = 1.0;

        int i;
        double tmp;
        for (i = 0; i < ac && av[i] != null && av[i].length() > 0 && av[i].charAt(0) == '-'; i++) {
            Matrix4x4d m4 = new Matrix4x4d();

            if (av[i].length() < 2) {
                finish(counter, ret, transformMatrix, scaTransform);
                return i;
            }

            switch (av[i].charAt(1)) {

                case 't':
                    // Translate
                    if (!checkArgument(2, "fff", ac, av, i)) {
                        finish(counter, ret, transformMatrix, scaTransform);
                        return i;
                    }
                    m4.m[3][0] = Double.parseDouble(av[++i]);
                    m4.m[3][1] = Double.parseDouble(av[++i]);
                    m4.m[3][2] = Double.parseDouble(av[++i]);
                    break;

                case 'r':
                    // Rotate
                    {
                        char suffix = av[i].length() > 2 ? av[i].charAt(2) : '\0';
                        switch (suffix) {
                            case 'x':
                                if (!checkArgument(3, "f", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                tmp = d2r(Double.parseDouble(av[++i]));
                                m4.m[1][1] = m4.m[2][2] = Math.cos(tmp);
                                m4.m[2][1] = -(m4.m[1][2] = Math.sin(tmp));
                                break;
                            case 'y':
                                if (!checkArgument(3, "f", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                tmp = d2r(Double.parseDouble(av[++i]));
                                m4.m[0][0] = m4.m[2][2] = Math.cos(tmp);
                                m4.m[0][2] = -(m4.m[2][0] = Math.sin(tmp));
                                break;
                            case 'z':
                                if (!checkArgument(3, "f", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                tmp = d2r(Double.parseDouble(av[++i]));
                                m4.m[0][0] = m4.m[1][1] = Math.cos(tmp);
                                m4.m[1][0] = -(m4.m[0][1] = Math.sin(tmp));
                                break;
                            default: {
                                if (!checkArgument(2, "ffff", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                float x = Float.parseFloat(av[++i]);
                                float y = Float.parseFloat(av[++i]);
                                float z = Float.parseFloat(av[++i]);
                                float a = (float)d2r(Double.parseDouble(av[++i]));
                                float s = (float)Math.sqrt(x * x + y * y + z * z);
                                x /= s;
                                y /= s;
                                z /= s;
                                float c = (float)Math.cos(a);
                                s = (float)Math.sin(a);
                                float t = 1 - c;
                                m4.m[0][0] = t * x * x + c;
                                m4.m[1][1] = t * y * y + c;
                                m4.m[2][2] = t * z * z + c;
                                float A = t * x * y;
                                float B = s * z;
                                m4.m[0][1] = A + B;
                                m4.m[1][0] = A - B;
                                A = t * x * z;
                                B = s * y;
                                m4.m[0][2] = A - B;
                                m4.m[2][0] = A + B;
                                A = t * y * z;
                                B = s * x;
                                m4.m[1][2] = A + B;
                                m4.m[2][1] = A - B;
                            }
                        }
                    }
                    break;

                case 's':
                    // Scale
                    {
                        char suffix = av[i].length() > 2 ? av[i].charAt(2) : '\0';
                        switch (suffix) {
                            case 'x':
                                if (!checkArgument(3, "f", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                tmp = Double.parseDouble(av[i + 1]);
                                if (tmp == 0.0) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                m4.m[0][0] = tmp;
                                break;
                            case 'y':
                                if (!checkArgument(3, "f", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                tmp = Double.parseDouble(av[i + 1]);
                                if (tmp == 0.0) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                m4.m[1][1] = tmp;
                                break;
                            case 'z':
                                if (!checkArgument(3, "f", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                tmp = Double.parseDouble(av[i + 1]);
                                if (tmp == 0.0) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                m4.m[2][2] = tmp;
                                break;
                            default:
                                if (!checkArgument(2, "f", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                tmp = Double.parseDouble(av[i + 1]);
                                if (tmp == 0.0) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                scaTransform *=
                                m4.m[0][0] =
                                m4.m[1][1] =
                                m4.m[2][2] = tmp;
                                break;
                        }
                        i++;
                    }
                    break;

                case 'm':
                    // Mirror
                    {
                        char suffix = av[i].length() > 2 ? av[i].charAt(2) : '\0';
                        switch (suffix) {
                            case 'x':
                                if (!checkArgument(3, "", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                scaTransform *=
                                m4.m[0][0] = -1.0;
                                break;
                            case 'y':
                                if (!checkArgument(3, "", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                scaTransform *=
                                m4.m[1][1] = -1.0;
                                break;
                            case 'z':
                                if (!checkArgument(3, "", ac, av, i)) {
                                    finish(counter, ret, transformMatrix, scaTransform);
                                    return i;
                                }
                                scaTransform *=
                                m4.m[2][2] = -1.0;
                                break;
                            default:
                                finish(counter, ret, transformMatrix, scaTransform);
                                return i;
                        }
                    }
                    break;

                case 'i':
                    // Iterate
                    if (!checkArgument(2, "i", ac, av, i)) {
                        finish(counter, ret, transformMatrix, scaTransform);
                        return i;
                    }
                    while (counter-- > 0) {
                        Matrix4x4d.multiplyMatrix4(ret.transformMatrix, ret.transformMatrix, transformMatrix);
                        ret.scaleFactor *= scaTransform;
                    }
                    counter = Integer.parseInt(av[++i]);
                    transformMatrix.identity();
                    scaTransform = 1.0;
                    continue;

                default:
                    finish(counter, ret, transformMatrix, scaTransform);
                    return i;

            }
            Matrix4x4d.multiplyMatrix4(transformMatrix, transformMatrix, m4);
        }

        finish(counter, ret, transformMatrix, scaTransform);
        return i;
    }

    private static boolean compactTransformArguments(ParseRuntimeContext context, TransformStackContext stackContext) {
        return context.transformStack.compactTo(stackContext);
    }

    /**
    Handle xf entity
    */
    public static int handleTransformationEntity(int ac, String[] av, ParseRuntimeContext context) {
        TransformScopeContext stack = context.transformStack;
        TransformStackContext spec;
        int n;

        if (ac == 1) {
            // Something with existing transform
            spec = context.transformContext;
            if (spec == null) {
                return ParseErrorContext.MGF_ERROR_UNMATCHED_CONTEXT_CLOSE;
            }
            n = -1;
            if (spec.transformationArray != null) {
                // check for iteration
                TransformSequenceContext ap = spec.transformationArray;

                transformName(null, context);
                n = ap.numberOfDimensions;
                while (n-- > 0) {
                    if (++ap.transformArguments[n].i < ap.transformArguments[n].n) {
                        break;
                    }
                    ap.transformArguments[n].arg = "0";
                    if (ap.transformArguments[n].argumentIndex >= 0
                        && ap.transformArguments[n].argumentIndex < stack.argumentCount) {
                        stack.argumentList[ap.transformArguments[n].argumentIndex] = ap.transformArguments[n].arg;
                    }
                    ap.transformArguments[n].i = 0;
                }
                if (n >= 0) {
                    int rv = MgfEntityControl.mgfGoToFilePosition(ap.startingPosition, context);
                    if (rv != ParseErrorContext.MGF_OK) {
                        return rv;
                    }
                    ap.transformArguments[n].arg = Integer.toString(ap.transformArguments[n].i);
                    if (ap.transformArguments[n].argumentIndex >= 0
                        && ap.transformArguments[n].argumentIndex < stack.argumentCount) {
                        stack.argumentList[ap.transformArguments[n].argumentIndex] = ap.transformArguments[n].arg;
                    }
                    transformName(ap, context);
                }
            }
            if (n < 0) {
                // Pop transform
                if (!compactTransformArguments(context, spec.prev)) {
                    return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
                }
                context.transformContext = spec.prev;
                context.transformStack.freeTransformContext(spec);
                return ParseErrorContext.MGF_OK;
            }
        } else {
            // Allocate transform
            String[] slice = new String[ac - 1];
            System.arraycopy(av, 1, slice, 0, ac - 1);
            spec = newTransform(ac - 1, slice, context);
            if (spec == null) {
                return ParseErrorContext.MGF_ERROR_OUT_OF_MEMORY;
            }
            if (spec.transformationArray != null) {
                transformName(spec.transformationArray, context);
            }
            spec.prev = context.transformContext; // Push onto stack
            context.transformContext = spec;
        }

        // Translate new specification
        n = stack.argumentCountFor(spec);
        n -= stack.argumentCountFor(spec.prev); // Incremental comp. is more eff.
        String[] specAv = stack.argumentVectorFor(spec);
        if (xf(spec.xf, n, specAv) != n) {
            return ParseErrorContext.MGF_ERROR_ARGUMENT_TYPE;
        }

        // Check for vertex reversal
        spec.rev = (short)(spec.xf.scaleFactor < 0.0 ? 1 : 0);
        if (spec.rev != 0) {
            spec.xf.scaleFactor = -spec.xf.scaleFactor;
        }

        // Compute total transformation
        if (spec.prev != null) {
            Matrix4x4d.multiplyMatrix4(spec.xf.transformMatrix, spec.xf.transformMatrix, spec.prev.xf.transformMatrix);
            spec.xf.scaleFactor *= spec.prev.xf.scaleFactor;
            spec.rev = (short)((spec.rev ^ spec.prev.rev) != 0 ? 1 : 0);
        }
        spec.xid = computeUniqueId(spec.xf.transformMatrix); // Compute unique ID
        return ParseErrorContext.MGF_OK;
    }

    public static void mgfTransformFreeMemory(ParseRuntimeContext context) {
        if (context != null) {
            context.transformStack.clearArguments();
        }
    }
}
