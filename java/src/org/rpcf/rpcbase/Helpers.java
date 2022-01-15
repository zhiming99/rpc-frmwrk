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

    public static boolean isInterface( Object obj ) 
    {
        return obj !=
            null && obj.getClass().isInterface();
    }

    private static HashMap< String, Integer >
        InitMap( HashMap< String, Integer > oMap )
    {
        oMap.put( "byte", RC.typeByte );
        oMap.put( "boolean", RC.typeByte ); 
        oMap.put( "short", RC.typeUInt16 ); 
        oMap.put( "int", RC.typeUInt32 ); 
        oMap.put( "long", RC.typeUInt64 ); 
        oMap.put( "float", RC.typeFloat ); 
        oMap.put( "double", RC.typeDouble ); 
        oMap.put( "org.rpcf.rpcbase.ObjPtr", RC.typeObj );
        oMap.put( "java.lang.Integer", RC.typeUInt32Obj );
        oMap.put( "java.lang.Long", RC.typeUInt64Obj );
        oMap.put( "java.lang.Short", RC.typeUInt16Obj );
        oMap.put( "java.lang.Byte", RC.typeByteObj );
        oMap.put( "java.lang.Float", RC.typeFloatObj );
        oMap.put( "java.lang.Double", RC.typeDoubleObj );
        oMap.put( "java.lang.Boolean", RC.typeByteObj );
        oMap.put( "java.lang.String", RC.typeString ); 
        oMap.put( "org.rpcf.rpcbase.BufPtr", RC.typeBufPtr );
        oMap.put( "[B", RC.typeByteArr ); 
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
            return RC.typeNone;

        if( iTypeId == RC.typeByte )
        {
            if( isArray( obj ) )
                return RC.typeByteArr; 
        }

        return iTypeId;
    }

    public static vectorVars
        convertObjectToVars( Object[] arrObjs )
    {
        int ret = 0;
        vectorVars vecVars = new vectorVars();
        for( Object i : arrObjs )
        {
            int iType = Helpers.getTypeId( i );
            Variant oVar = new Variant();
            switch( iType )
            {
            case RC.typeString:
                {
                   ret = oVar.SetString(
                       ( String )i );
                   break;
                }
            case RC.typeObj:
                {
                    ret = oVar.SetObjPtr( ( ObjPtr )i );
                    break;
                }
            case RC.typeByteArr:
                {
                    ret = oVar.SetByteArray(
                        ( byte[] )i );
                    break;
                }
            case RC.typeByteObj:
                {
                    ret = oVar.SetByte(
                        ( ( Byte )i ).byteValue() );
                    break;
                }
            case RC.typeUInt16Obj:
                {
                    ret = oVar.SetShort(
                        ( ( Short )i ).shortValue() );
                    break;
                }
            case RC.typeUInt32Obj:
                {
                    ret = oVar.SetInt(
                        ( ( Integer )i ).intValue() );
                    break;
                }
            case RC.typeUInt64Obj:
                {
                    ret = oVar.SetLong(
                        ( ( Long )i ).longValue() );
                    break;
                }
            case RC.typeFloatObj:
                {
                    ret = oVar.SetFloat(
                        ( ( Float )i ).floatValue() );
                    break;
                }
            case RC.typeDoubleObj:
                {
                    ret = oVar.SetDouble(
                        ( ( Double )i ).doubleValue() );
                    break;
                }
            case RC.typeBufPtr:
                {
                    ret = oVar.SetBufPtr( (BufPtr) i );
                    break;
                }
            case RC.typeDMsg:
            default:
                {
                    ret = -RC.ENOTSUP;
                    break;
                }
            }
            if( ret < 0 )
                break;

            vecVars.add( oVar );
            oVar = null;
        }
        if( ret < 0 )
            return null;

        return vecVars;
    }
}
