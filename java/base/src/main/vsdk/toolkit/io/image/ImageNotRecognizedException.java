package vsdk.toolkit.io.image;
import java.io.Serial;
import java.io.File;

import vsdk.toolkit.common.VSDKException;

/**
This class represents the exception in which an image can not be handled
by an image persistence operation.
*/
public class ImageNotRecognizedException extends VSDKException
{
    @Serial private static final long serialVersionUID = 20060502L;

    File image;

    /**
    Constructs an Image exception with a message and the image file that
    has been processed when the exception was generated.
    @param message The message to display in the stack trace
    @param image The image file that caused the exception
    */
    public ImageNotRecognizedException(String message, File image) 
    {
        super(message);
        this.image=image;
    }

    /**
    This method returns the image file that has been processed when this
    Exception was generated
    @return The image file that has been processed when the exception
    was generated
    */
    public File getImage()
    {
       return image;
    }
}
