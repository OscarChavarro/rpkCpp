package vsdk.toolkit.common.dataStructures;

public class CircularListBaseIterator {
    private CircularListLink currentElement;
    private CircularListBase currentList;

    public CircularListBaseIterator(CircularListBase list) {
        init(list);
    }

    public void init(CircularListBase list) {
        currentList = list;
        currentElement = currentList.lastLink();
    }

    public CircularListLink next() {
        CircularListLink response;

        if (currentElement == null) {
            response = null;
        }
        else {
            currentElement = currentElement.nextLink;
            response = currentElement;
        }

        if (currentElement == currentList.lastLink()) {
            currentElement = null;
        }

        return response;
    }
}
