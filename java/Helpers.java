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

    public static vectorBufPtr
        convertObjectToBuf( Object[] arrObjs )
    {
        int ret = 0;
        vectorBufPtr vecBufs = new vectorBufPtr();
        for( Object i : arrObjs )
        {
            int iType = Helpers.getTypeId( i );
            BufPtr pBuf = new BufPtr( true );
            switch( iType )
            {
            case rpcbaseConstants.typeString:
                {
                   ret = pBuf.SetString(
                       ( String )i );
                   break;
                }
            case rpcbaseConstants.typeObj:
                {
                    ret = pBuf.SetObjPtr(
                        ( ObjPtr )i );
                    break;
                }
            case rpcbaseConstants.typeByteArr:
                {
                    ret = pBuf.SetByteArray(
                        ( byte[] )i );
                    break;
                }
            case rpcbaseConstants.typeByteObj:
                {
                    ret = pBuf.SetByte(
                        ( ( Byte )i ).byteValue() );
                    break;
                }
            case rpcbaseConstants.typeUInt16Obj:
                {
                    ret = pBuf.SetShort(
                        ( ( Short )i ).shortValue() );
                    break;
                }
            case rpcbaseConstants.typeUInt32Obj:
                {
                    ret = pBuf.SetInt(
                        ( ( Integer )i ).intValue() );
                    break;
                }
            case rpcbaseConstants.typeUInt64Obj:
                {
                    ret = pBuf.SetLong(
                        ( ( Long )i ).longValue() );
                    break;
                }
            case rpcbaseConstants.typeFloatObj:
                {
                    ret = pBuf.SetFloat(
                        ( ( Float )i ).floatValue() );
                    break;
                }
            case rpcbaseConstants.typeDoubleObj:
                {
                    ret = pBuf.SetDouble(
                        ( ( Double )i ).doubleValue() );
                    break;
                }
            case rpcbaseConstants.typeDMsg:
            default:
                {
                    ret = -rpcbaseConstants.ENOTSUP;
                    break;
                }
            }
            if( ret < 0 )
                break;

            vecBufs.add( pBuf );
        }
        if( ret < 0 )
            return null;

        return vecBufs;
    }
}
