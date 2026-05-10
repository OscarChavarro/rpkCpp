/**
Classes and routines for chains of bsdf scattering modes
and operations with the chains on paths

A flag chain corresponds to a scattering mode
A chain list is a set of scattering modes
*/

package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;

public class FlagChain {
    public byte[] chain;
    public int length;
    public boolean subtract;

    public void init(int inLength, boolean inSubtract) {
        length = inLength;
        subtract = inSubtract;

        if ( inLength > 0 ) {
            chain = new byte[inLength];
            for ( int i = 0; i < inLength; i++ ) {
                chain[i] = 0;
            }
        } else {
            chain = null;
        }
    }

    public void init(int inLength) {
        init(inLength, false);
    }

    public FlagChain(int paramLength, boolean paramSubtract) {
        chain = null;
        init(paramLength, paramSubtract);
    }

    public FlagChain(int paramLength) {
        this(paramLength, false);
    }

    public FlagChain() {
        this(0, false);
    }

    // Copy constructor
    public FlagChain(FlagChain c) {
        this(c != null ? c.length : 0, c != null && c.subtract);

        if ( c != null && c.chain != null && chain != null ) {
            for ( int i = 0; i < length; i++ ) {
                chain[i] = c.chain[i];
            }
        }
    }

    public static boolean compare(FlagChain c1, FlagChain c2) {
        // Determine if equal

        int nrDifferent = 0;

        if ( c1 == null || c2 == null ) {
            return false;
        }

        if ( (c1.length != c2.length) || (c1.subtract != c2.subtract) ) {
            return false;
        }

        for ( int i = 0; (i < c1.length) && (nrDifferent == 0); i++ ) {
            if ( c1.chain[i] != c2.chain[i] ) {
                nrDifferent++;
            }
        }

        // combine into new chain

        if ( nrDifferent == 0 ) {
            // flag chains identical
            return true;
        }

        // Not combinable

        return false;
    }

    public static FlagChain combine(FlagChain c1, FlagChain c2) {
        // Determine if combinable
        int nrDifferent = 0;
        int diffIndex = 0;

        if ( c1 == null || c2 == null ) {
            return null;
        }

        if ( (c1.length != c2.length) || (c1.subtract != c2.subtract) ) {
            return null;
        }

        for ( int i = 0; (i < c1.length) && (nrDifferent <= 1); i++ ) {
            if ( c1.chain[i] != c2.chain[i] ) {
                nrDifferent++;
                diffIndex = i;
            }
        }

        // Combine into new chain
        if ( nrDifferent == 0 ) {
            // Flag chains identical - maybe dangerous if someone wants to
            // count one contribution twice...
            return new FlagChain(c1);
        }

        if ( nrDifferent == 1 ) {
            // Combinable
            FlagChain newFlagChain = new FlagChain(c1);
            newFlagChain.chain[diffIndex] = (byte)(c1.chain[diffIndex] | c2.chain[diffIndex]);
            return newFlagChain;
        }

        // Not combinable
        return null;
    }

    /**
    Compute : calculate the product of bsdf components defined
    by the chain. Eye and light node ARE INCLUDED
    */
    public ColorRgb compute(BiPath path) {
        ColorRgb result = new ColorRgb();
        ColorRgb tmpCol;
        result.setMonochrome(1.0f);
        int eyeSize = path.m_eyeSize;
        int lightSize = path.m_lightSize;

        SimpleRaytracingPathNode node;

        if ( lightSize + eyeSize != length ) {
            Error.error("FlagChain::Compute", "Wrong path length");
            return result;
        }

        // Flag chain start at the light node and end at the eye node
        node = path.m_lightPath;

        for ( int i = 0; i < lightSize; i++ ) {
            tmpCol = node.m_bsdfComp.Sum(chain[i]);
            result.selfScalarProduct(tmpCol);
            node = node.next();
        }

        node = path.m_eyePath;

        for ( int i = 0; i < eyeSize; i++ ) {
            tmpCol = node.m_bsdfComp.Sum(chain[length - 1 - i]);
            result.selfScalarProduct(tmpCol);
            node = node.next();
        }

        if ( subtract ) {
            result.scale(-1.0f);
        }

        return result;
    }
}
