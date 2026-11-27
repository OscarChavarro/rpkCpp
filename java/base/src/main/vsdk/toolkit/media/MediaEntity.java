package vsdk.toolkit.media;
import java.io.Serial;

import vsdk.toolkit.common.Entity;

/**
A `MediaEntity` in VitralSDK is a software element with data
structures associated with multimedia information, and some minor basic
algorithms for supporting them.

The MediaEntity abstract class provides an interface for multimedia related
classes NOT related with 3D geometry. This serves two purposes:
  - To help in design level organization of multimedia related classes (this
    eases the study of the class hierarchy)
  - To provide a place to locate possible future operations, common to all
    multimedia objects. Note that currently none of such operations have been
    detected.
*/
public abstract class MediaEntity extends Entity {
    @SuppressWarnings("FieldNameHidesFieldInSuperclass")
    @Serial private static final long serialVersionUID = 20150218L;
    
    /**
    Just to do not the inheritance chain.
    @return
    @throws CloneNotSupportedException 
    */
    @Override
    public Object clone() throws CloneNotSupportedException
    {
        super.clone();
        return null;
    }

}
