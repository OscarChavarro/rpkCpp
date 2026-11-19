package vsdk.toolkit.common;

/**
Event emitted by an `Entity` to its subscribers (see
`Entity.addEntityListener`) when the entity changes or is discarded.
*/
public class EntityEvent
{
    /**
    Kinds of events an entity can emit.
    */
    public enum Type
    {
        /** Some attribute of the entity changed, see `Entity.update()` */
        UPDATED,
        /** The entity was discarded, see `Entity.dispose()` */
        DELETED
    }

    private final Entity source;
    private final Type type;

    /**
    @param source entity that emits the event
    @param type kind of event
    */
    public EntityEvent(Entity source, Type type)
    {
        this.source = source;
        this.type = type;
    }

    /**
    @return the entity that emitted the event
    */
    public Entity getSource()
    {
        return source;
    }

    /**
    @return the kind of event
    */
    public Type getType()
    {
        return type;
    }

}
