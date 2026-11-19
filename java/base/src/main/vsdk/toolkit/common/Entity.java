package vsdk.toolkit.common;

// Java basic classes
import java.io.Serial;
import java.lang.reflect.Method;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

/**
This class is a base superclass for all classes in the VSDK model (as of
Vitral applications are based upon Model-View-Controller or MVC design
pattern). Note that this class supports two functionalities:
  - As implementing the Serializable interface, this permits to serialize
    all of the
*/

public class Entity implements ModelElement, Serializable
{
    @Serial
    private static final long serialVersionUID = 20150218L;

    /// Constants used for operations of type getSizeInBytes
    public static final int BYTE_SIZE_IN_BYTES = 1;
    public static final int INT_SIZE_IN_BYTES = 4;
    public static final int LONG_SIZE_IN_BYTES = 8;
    public static final int FLOAT_SIZE_IN_BYTES = 4;
    public static final int DOUBLE_SIZE_IN_BYTES = 8;
    public static final int VECTOR3D_SIZE_IN_BYTES = 24;
    public static final int COLORRGB_SIZE_IN_BYTES = 24;
    public static final int POINTER_SIZE_IN_BYTES = 8;

    /**
    Control specifications used by reflection-based generic editors to build
    GUI dialogs for this entity. Kept null until first used, so fine-grained
    entities (vertices, half-edges, ...) do not pay for an empty list.
    */
    private List<String> controlSpecifications = null;

    /**
    Subscribers notified by `update()` and `dispose()`. Transient because
    subscribers are runtime objects (editors, views) that are not part of the
    model; kept null until the first subscription.
    */
    private transient List<EntityListener> entityListeners = null;

    /**
    This is a value used for the standard java serialization mechanism to
    keep track of software versions.  To avoid warning at compilation time
    and to ease to keep compatibility tracking of software structure changes
    in retrieving old saved data, it is suggested that all Entity's in VSDK
    define this value.  The proposed number to asign is the concatenation of
    8 digits YYYYMMDD, for year, month and day respectively.
    */
    //@Serial private static final long serialVersionUID = 20060502L;

    /**
    Each Entity object in the VSDK model should be responsible for calculating
    the size in bytes that occupies in RAM, including its own attributes and
    the aggregated objects (note that the associated objects only must count
    the size of the references).  If the class doen't overload this method,
    a 0 size is assumed.

    This is important for applications implementing memory chaching schema.
    @return the number of bytes current object ocupies in RAM when loaded
    */
    public int getSizeInBytes()
    {
        return 0;
    }

    /**
    Returns the control specifications used by reflection-based generic
    editors, creating an empty list on first access.
    @return the mutable list of control specifications for this entity
    */
    public List<String> getControlSpecifications()
    {
        if ( controlSpecifications == null ) {
            controlSpecifications = new ArrayList<String>();
        }
        return controlSpecifications;
    }

    /**
    Replaces the control specifications used by reflection-based generic
    editors.
    @param controlSpecifications new list of control specifications; may be
    null to release the current list
    */
    public void setControlSpecifications(List<String> controlSpecifications)
    {
        this.controlSpecifications = controlSpecifications;
    }

    /**
    Subscribes a listener to the events of this entity. Adding the same
    listener twice has no effect.
    @param listener object to notify; null is ignored
    */
    public void addEntityListener(EntityListener listener)
    {
        if ( listener == null ) {
            return;
        }
        if ( entityListeners == null ) {
            entityListeners = new ArrayList<EntityListener>();
        }
        if ( !entityListeners.contains(listener) ) {
            entityListeners.add(listener);
        }
    }

    /**
    Unsubscribes a listener from the events of this entity.
    @param listener object to stop notifying
    */
    public void removeEntityListener(EntityListener listener)
    {
        if ( entityListeners != null ) {
            entityListeners.remove(listener);
        }
    }

    /**
    Notifies the subscribers that this entity changed. Code that modifies an
    entity (editors, tools) calls this method after the change, so the
    objects that depend on the entity can refresh.
    */
    public void update()
    {
        fireEntityEvent(EntityEvent.Type.UPDATED);
    }

    /**
    Notifies the subscribers that this entity was discarded (for example,
    deleted from a scene) and then drops all of them. This is the Java
    counterpart of emitting the event from a C++ destructor: the entity data is
    kept, so an undo operation can still insert it again.
    */
    public void dispose()
    {
        fireEntityEvent(EntityEvent.Type.DELETED);
        entityListeners = null;
    }

    private void fireEntityEvent(EntityEvent.Type type)
    {
        if ( entityListeners == null || entityListeners.isEmpty() ) {
            return;
        }
        EntityEvent event = new EntityEvent(this, type);
        // Copy: listeners may unsubscribe while being notified
        List<EntityListener> listeners;
        listeners = new ArrayList<EntityListener>(entityListeners);
        for ( EntityListener listener : listeners ) {
            listener.notifyEntityEvent(event);
        }
    }

    /**
    Calculate a set of variable specs in "type:name" format, from method set and
    detecting pair of get / set methods.
    @return
    */
    public ArrayList<String> getEncapsulatedVariables()
    {
        int i;
        int j;
        Class<?> c;
        Method methods[];
        Method m;
        Class<?> t[];
        Class<?> r;

        c = this.getClass();
        methods = c.getMethods();

        // Stage 1: get a list of getters with no argument and a single return
        ArrayList<Method> getters = new ArrayList<Method>();
        for ( i = 0; i < methods.length; i++ ) {
            m = methods[i];
            if ( m.getName().startsWith("get") ) {
                t = m.getParameterTypes();
                r = m.getReturnType();

                if ( t.length == 0 && typeIsSupported(r) ) {
                    getters.add(m);
                }
            }
        }

        // Stage 2: get a list of setters with a single argument and with no
        // return
        ArrayList<Method> setters = new ArrayList<Method>();
        for ( i = 0; i < methods.length; i++ ) {
            m = methods[i];
            if ( m.getName().startsWith("set") ) {
                t = m.getParameterTypes();
                r = m.getReturnType();
                if ( t.length == 1 && r.getName().equals("void")) {
                    setters.add(m);
                }

            }
        }

        // Stage 3: report get/set pairs with the same name and parameter type
        ArrayList<String> variableSpecs;
        variableSpecs = new ArrayList<String>();
        for ( i = 0; i < getters.size(); i++ ) {
            String gname;
            m = getters.get(i);
            gname = m.getName().substring(3, m.getName().length());
            r = m.getReturnType();
            for ( j = 0; j < setters.size(); j++ ) {
                String sname = setters.get(j).getName().substring(
                    3, setters.get(j).getName().length());
                t = setters.get(j).getParameterTypes();
                if ( t.length == 1 && gname.equals(sname) &&
                     t[0].getName().equals(r.getName()) ) {
                    String s = gname.toLowerCase();
                    String variableName = s.substring(0, 1) +
                        gname.substring(1);
                    variableSpecs.add(r.getName() + ":" + variableName);
                    break;
                }
            }
        }

        return variableSpecs;
    }

    private boolean typeIsSupported(Class<?> r)
    {
        String n = r.getName();

        if ( n.equals("long") ) {
            return true;
        }
        else if ( n.equals("int") ) {
            return true;
        }
        else if ( n.equals("float") ) {
            return true;
        }
        else if ( n.equals("double") ) {
            return true;
        }
        else if ( n.equals("byte") ) {
            return true;
        }
        else if ( n.equals("boolean") ) {
            return true;
        }
        else if ( n.equals("char") ) {
            return true;
        }
        else if ( n.equals("short") ) {
            return true;
        }
        else if ( n.equals("java.lang.String") ) {
            return true;
        }
        else if ( n.equals("vsdk.toolkit.common.linealAlgebra.Vector2Dd") ) {
            return true;
        }
        else if ( n.equals("vsdk.toolkit.common.linealAlgebra.Vector3Dd") ) {
            return true;
        }
        else if ( n.equals("vsdk.toolkit.common.linealAlgebra.Vector4Dd") ) {
            return true;
        }
        else if ( n.equals("vsdk.toolkit.common.linealAlgebra.ColorRgb") ||
                  n.equals("vsdk.toolkit.common.color.ColorRgb") ) {
            return true;
        }
        else if ( n.equals("vsdk.toolkit.common.linealAlgebra.Matrix4x4d") ) {
            return true;
        }
        return false;
    }

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
