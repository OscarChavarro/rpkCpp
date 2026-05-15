package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.dataStructures.CircularList;
import vsdk.toolkit.common.dataStructures.CircularListIterator;

// A linked list of flag chains.
// Chains in the list are of fixed length !
public final class FlagChainList extends CircularList<FlagChain> {
    public int length;
    public int count;

    public FlagChainList() {
        count = 0;
        length = 0;
    }

    public void add(FlagChainList list) {
        // Add all chains in 'list'
        CircularListIterator<FlagChain> iter = new CircularListIterator<>(list);
        FlagChain tmpChain;

        while ( (tmpChain = iter.nextOnSequence()) != null ) {
            add(tmpChain);
        }
    }

    @Override
    public void add(FlagChain chain) {
        if ( count > 0 ) {
            if ( chain.length != length ) {
                Logger.error("CChainList::add", "Wrong length flag chain inserted!");
                return;
            }
        } else {
            // first element
            length = chain.length;
        }

        count++;
        append(new FlagChain(chain));
    }

    public void addDisjoint(FlagChain chain) {
        if ( count > 0 ) {
            if ( chain.length != length ) {
                Logger.error("CChainList::add", "Wrong length flag chain inserted!");
                return;
            }
        } else {
            // first element
            length = chain.length;
        }

        CircularListIterator<FlagChain> iter = new CircularListIterator<>(this);
        FlagChain tmpChain;
        boolean found = false;

        while ( (tmpChain = iter.nextOnSequence()) != null && !found ) {
            found = FlagChain.compare(tmpChain, chain);
        }

        if ( !found ) {
            count++;
            append(new FlagChain(chain));
        }
    }

    public ColorRgb compute(BiPath path) {
        ColorRgb result = new ColorRgb();
        ColorRgb tmpCol;

        result.clear();

        CircularListIterator<FlagChain> iter = new CircularListIterator<>(this);
        FlagChain chain;

        while ( (chain = iter.nextOnSequence()) != null ) {
            tmpCol = chain.compute(path);
            result.add(tmpCol, result);
        }

        return result;
    }

    /**
    simplify the chain list returning the equivalent
    simplified chain list. Equal entries MAY be reduced to a
    single entry! (So in fact no equal entries is advisable)
    */
    public FlagChainList simplify() {
        // Try a simple simplification scheme, just comparing pair wise chains
        FlagChainList newList = new FlagChainList();
        FlagChain c1;
        FlagChain c2;
        FlagChain cCombined;
        CircularListIterator<FlagChain> iter = new CircularListIterator<>(this);

        c1 = iter.nextOnSequence();

        if ( c1 != null ) {
            while ( (c2 = iter.nextOnSequence()) != null ) {
                cCombined = FlagChain.combine(c1, c2);
                if ( cCombined != null ) {
                    c1 = cCombined; // Combined
                } else {
                    newList.add(c1);
                    c1 = c2;
                }
            }

            // Add final chain still in c1
            newList.add(c1);
        }

        return newList;
    }
}
