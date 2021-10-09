package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;

public class Helpers
{
    public static boolean isArray( Object obj ) 
    {
        return obj !=
            null && obj.getClass().isArray();
    }

    private static HashMap< String, Integer >
        InitMap( HashMap< String, Integer > oMap )
    {
        oMap.put( "byte", rpcbaseConstants.typeByte );
        oMap.put( "boolean", rpcbaseConstants.typeByte ); 
        oMap.put( "short", rpcbaseConstants.typeUInt16 ); 
        oMap.put( "int", rpcbaseConstants.typeUInt32 ); 
        oMap.put( "long", rpcbaseConstants.typeUInt64 ); 
        oMap.put( "float", rpcbaseConstants.typeFloat ); 
        oMap.put( "double", rpcbaseConstants.typeDouble ); 
        oMap.put( "org.rpcf.rpcbase.ObjPtr", rpcbaseConstants.typeObj );
        oMap.put( "java.lang.Integer", rpcbaseConstants.typeUInt32Obj );
        oMap.put( "java.lang.Long", rpcbaseConstants.typeUInt64Obj );
        oMap.put( "java.lang.Short", rpcbaseConstants.typeUInt16Obj );
        oMap.put( "java.lang.Byte", rpcbaseConstants.typeByteObj );
        oMap.put( "java.lang.Float", rpcbaseConstants.typeFloatObj );
        oMap.put( "java.lang.Double", rpcbaseConstants.typeDoubleObj );
        oMap.put( "java.lang.Boolean", rpcbaseConstants.typeByteObj );
        oMap.put( "java.lang.String", rpcbaseConstants.typeString ); 
        return oMap;
    }

    public static HashMap< String, Integer > mapType2Id =
        InitMap( new HashMap< String, Integer >() );

    public static int getTypeId( Object obj ) 
    {
        String strName =
            obj.getClass().getName();

        Integer iTypeId = mapType2Id.get( strName );
        if( iTypeId == null )
            return rpcbaseConstants.typeNone;

        if( iTypeId == rpcbaseConstants.typeByte )
        {
            if( isArray( obj ) )
                return rpcbaseConstants.typeByteArr; 
        }

        return iTypeId;
    }
}
