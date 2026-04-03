package vsdk.toolkit.common.dataStructures;

public class CircularListBase {
    private CircularListLink last;

    public CircularListBase() {
        last = null;
    }

    public void addLink(CircularListLink data) {
        if (last != null) {
            data.nextLink = last.nextLink;
        }
        else {
            last = data;
        }
        last.nextLink = data;
    }

    public void appendLink(CircularListLink data) {
        if (last != null) {
            data.nextLink = last.nextLink;
            last = last.nextLink = data;
        }
        else {
            last = data;
            data.nextLink = data;
        }
    }

    public CircularListLink remove() {
        if (last == null) {
            return null;
        }

        CircularListLink first = last.nextLink;
        if (first == last) {
            last = null;
        }
        else {
            last.nextLink = first.nextLink;
        }

        return first;
    }

    public void clear() {
        last = null;
    }

    public CircularListLink lastLink() {
        return last;
    }
}
