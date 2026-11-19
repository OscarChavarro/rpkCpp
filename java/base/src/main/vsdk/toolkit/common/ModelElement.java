package vsdk.toolkit.common;

/**
A `ModelElement` in Vitral is a software element with algorithms and data
structures (i.e. a class) with the specific functionality of representing
application internal state or "Model".

All ModelElement's are implemented in a way that uses only a basic set
of basic Java classes (some of the most fundamental classes on java.lang, 
java.io and java.util packages). All ModelElement's are very portable
across several platforms (as such Java desktop, Java for Android, Java
servlet and Java MicroEdition) since are elements which does not depend
on any specific API or extensions as such OpenGL, JOGL, Swing, Awt or
database access.

Note that model elements from Vitral SDKs are also called "Entities" and
are Serializable.

The ModelElement abstract class provides an interface to model classes. 
This serves two purposes:
  - To help in design level organization of renderers (this eases the
    study of the class hierarchy)
  - To provide a place to locate possible future operations, common to
    all renderers classes and renderers' private utility/supporting
    classes (but none of these as been detected yet)
Note that this is a very high-level organizing class inside the Vitral SDK,
and it is supposed to be here to support operations common to any renderer,
independent of the rendering technology/interface. Possibly none operation
of such high level should be included here, but to this class subclasses
(the organizers of each specific rendering technology classes).
*/

public interface ModelElement {
}
