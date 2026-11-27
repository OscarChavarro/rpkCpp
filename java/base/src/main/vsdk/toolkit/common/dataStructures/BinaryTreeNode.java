package vsdk.toolkit.common.dataStructures;
import java.io.Serial;

import vsdk.toolkit.common.FundamentalEntity;

/**
@param <T>
*/
public class BinaryTreeNode<T> extends FundamentalEntity
{
    @SuppressWarnings("FieldNameHidesFieldInSuperclass")
    @Serial private static final long serialVersionUID = 20150601L;

    private T data;
    private BinaryTreeNode<T> sibling = null;
    private BinaryTreeNode<T> child = null;

    /**
    @param data 
    */
    public BinaryTreeNode(T data)
    {
        this.data = data;
    }
    
    /**
    @return the sibling
    */
    public BinaryTreeNode<T> getSibling() 
    {
        return sibling;
    }

    /**
    @param sibling the sibling to set
    */
    public void setSibling(BinaryTreeNode<T> sibling) 
    {
        this.sibling = sibling;
    }

    /**
    @return the child
    */
    public BinaryTreeNode<T> getChild() 
    {
        return child;
    }

    /**
    @param child the child to set
    */
    public void setChild(BinaryTreeNode<T> child) 
    {
        this.child = child;
    }

    /**
    @return the data
    */
    public T getData() 
    {
        return data;
    }

    /**
    @param data the data to set
    */
    public void setData(T data) 
    {
        this.data = data;
    }    
}
