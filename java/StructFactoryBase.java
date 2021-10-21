package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;
import java.util.Map;
import java.util.List;

abstract public class StructFactoryBase
{
    public interface IFactory 
    {
        public abstract JavaSerialBase.ISerializable create();
    }
    static Map< Integer, IFactory > m_mapStructs;
    public static JavaSerialBase.ISerializable create( int id )
    {
        if( !m_mapStructs.containsKey( id ) )
            return null;

        IFactory oFactory =
            m_mapStructs.get( id );
        if( oFactory == null )
            return null;
        return oFactory.create();
    }

    public static int addFactory( int id , IFactory clsObj )
    {
        if( m_mapStructs.containsKey( id ) )
            return -rpcbaseConstants.EEXIST;
        m_mapStructs.put( id, clsObj );
        return rpcbaseConstants.STATUS_SUCCESS;
    }
    public abstract void initMap();
}

