/*
 * =====================================================================================
 *
 *       Filename:  JavaSerialBase.java
 *
 *    Description:  serialization utilities for Java
 *
 *        Version:  1.0
 *        Created:  10/24/2021 17:37:00 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.nio.ByteBuffer;
import java.util.Map.Entry;
import java.nio.charset.StandardCharsets;

abstract public class JavaSerialBase
{
    public interface ISerializable
    {
        public abstract int serialize(
            ByteBuffer buf );
        public abstract int deserialize(
            ByteBuffer buf );
        public abstract void setInst(
            Object oInst );
    }

    public interface ISerialElem
    {
        public abstract int serialize(
            ByteBuffer buf, Object val );
    }
    public interface IDeserialElem
    {
        public abstract Object deserialize(
            ByteBuffer buf );
    }

    int serialBoolArr( ByteBuffer buf, boolean[] val )
    {
        int iCount = val.length;
        int iBytes = iCount;
        buf.putInt( iBytes );
        if( iCount == 0 )
            return 0;
        for( boolean i : val )
        {
            if( i )
                buf.put( ( byte )1 );
            else
                buf.put( ( byte )0 );
        }
        return 0;
    }
    int serialInt8Arr( ByteBuffer buf, byte[] val )
    {
        serialBuf( buf, val );
        return 0;
    }

    int serialInt16Arr( ByteBuffer buf, short[] val )
    {
        int iCount = val.length;
        int iBytes = iCount * 2;
        buf.putInt( iBytes );
        buf.putInt( iCount );
        if( iCount == 0 )
            return 0;
        for( short i : val )
             buf.putShort( i );
        return 0;
    }

    int serialInt32Arr( ByteBuffer buf, int[] val )
    {
        int iCount = val.length;
        int iBytes = iCount * 4;
        buf.putInt( iBytes );
        buf.putInt( iCount );
        if( iCount == 0 )
            return 0;
        for( int i : val )
             buf.putInt( i );
        return 0;
    }

    int serialInt64Arr( ByteBuffer buf, long[] val )
    {
        int iCount = val.length;
        int iBytes = iCount * 8;
        buf.putInt( iBytes );
        buf.putInt( iCount );
        if( iCount == 0 )
            return 0;
        for( long i : val )
             buf.putLong( i );
        return 0;
    }

    int serialFloatArr( ByteBuffer buf, float[] val )
    {
        int iCount = val.length;
        int iBytes = iCount * 4;
        buf.putInt( iBytes );
        buf.putInt( iCount );
        if( iCount == 0 )
            return 0;
        for( float i : val )
             buf.putFloat( i );
        return 0;
    }

    int serialDoubleArr( ByteBuffer buf, double[] val )
    {
        int iCount = val.length;
        int iBytes = iCount * 8;
        buf.putInt( iBytes );
        buf.putInt( iCount );
        if( iCount == 0 )
            return 0;
        for( double i : val )
             buf.putDouble( i );
        return 0;
    }

    int serialHStreamArr( ByteBuffer buf, long[] val )
    {
        int iCount = val.length;
        int iBytes = iCount * 8;
        buf.putInt( iBytes );
        buf.putInt( iCount );
        if( iCount == 0 )
            return 0;
        int ret = 0;
        for( long i : val )
        {
             ret = serialHStream( buf, i );
             if( ret < 0 )
                 break;
        }
        return ret;
    }

    static boolean isPrimType( Character ch )
    {
        switch( ch )
        {
        case 'Q':
        case 'q':
        case 'h':
        case 'D':
        case 'd':
        case 'W':
        case 'w':
        case 'f':
        case 'F':
        case 'b':
        case 'B':
            return true;
        }
        return false;
    }

    int serialPrimArray(
        ByteBuffer buf, Object val, String strSig )
    {
        if( strSig.length() == 0 )
            return -rpcbaseConstants.EINVAL;

        int ret = 0;
        switch( strSig.charAt( 0 ) )
        {
        case 'Q':
        case 'q':
        case 'h':
            {
                ret = serialInt64Arr(
                    buf, ( long[] )val );
                break;
            }
        case 'D':
        case 'd':
            {
                ret = serialInt32Arr(
                    buf, ( int[] )val );
                break;
            }
        case 'W':
        case 'w':
            {
                ret = serialInt16Arr(
                    buf, ( short[] )val );
                break;
            }
        case 'f':
            {
                ret = serialFloatArr(
                    buf, ( float[] )val );
                break;
            }
        case 'F':
            {
                ret = serialDoubleArr(
                    buf, ( double[] )val );
                break;
            }
        case 'b':
        case 'B':
            {
                ret = serialInt8Arr(
                    buf, ( byte[] )val );
                break;
            }
        default :
           ret = -rpcbaseConstants.EINVAL;
           break;
        }
        return ret; 
    }

    void serialBool( ByteBuffer buf, boolean val )
    {
        if( val )
            buf.put( ( byte )1 );
        else
            buf.put( ( byte )0 );
    }
    void serialInt8( ByteBuffer buf, byte val )
    { buf.put( val ); }

    void serialInt16( ByteBuffer buf, short val )
    { buf.putShort( val ); }

    void serialInt32( ByteBuffer buf, int val )
    { buf.putInt( val ); }

    void serialInt64( ByteBuffer buf, long val )
    { buf.putLong( val ); }

    void serialFloat( ByteBuffer buf, float val )
    { buf.putFloat( val ); }

    void serialDouble( ByteBuffer buf, double val )
    { buf.putDouble( val ); }

    abstract int serialHStream(
        ByteBuffer buf, long val );

    int serialBoolMap( ByteBuffer buf, Boolean val )
    {
        serialBool( buf, val.booleanValue() );
        return 0;
    }
    int serialInt8Map(
        ByteBuffer buf, Byte val )
    {
        serialInt8( buf, val.byteValue() );
        return 0;
    }

    int serialInt16Map(
        ByteBuffer buf, Short val )
    {
        serialInt16( buf, val.shortValue() );
        return 0;
    }

    int serialInt32Map(
        ByteBuffer buf, Integer val )
    {
        serialInt32( buf, val.intValue() );
        return 0;
    }

    int serialInt64Map(
        ByteBuffer buf, Long val )
    {
        serialInt64( buf, val.longValue() );
        return 0;
    }

    int serialFloatMap(
        ByteBuffer buf, Float val )
    {
        serialFloat( buf, val.floatValue() );
        return 0;
    }

    int serialDoubleMap(
        ByteBuffer buf, Double val )
    {
        serialDouble( buf, val.doubleValue() );
        return 0;
    }

    int serialHStreamMap(
        ByteBuffer buf, Long val )
    {
        return serialHStream( buf, val.longValue() );
    }

    int serialString( ByteBuffer buf, String val )
    {
        if( val == null )
        {
            buf.putInt( 0 );
            return 0;
        }
        byte[] b = val.getBytes(
            StandardCharsets.UTF_8 );

        int size = b.length; 
        buf.putInt( size );
        if( size == 0 )
            return 0;
        buf.put( b );
        return 0;
    }

    int serialBuf( ByteBuffer buf, byte[] val )
    {
        if( val == null )
        {
            buf.putInt( 0 );
            return 0;
        }
        int size = val.length; 
        buf.putInt( size );
        if( size == 0 )
            return 0;
        buf.put( val );
        return 0;
    }

    int serialObjPtr( ByteBuffer buf, ObjPtr val )
    {
        if( val == null )
        {
            // put an empty header
            buf.putInt(
                rpcbaseConstants.Clsid_Invalid );
            buf.putInt( 0 );
            buf.put( ( byte )1 );
            buf.put( new byte[] {0,0,0} );
            return 0;
        }
        JRetVal jret =
            ( JRetVal )val.SerialToByteArray();

        if( jret.ERROR() )
            return jret.getError();

        byte[] objBuf = ( byte[] )jret.getAt( 0 );

        if( objBuf == null )
            return -rpcbaseConstants.EFAULT;

        buf.put( objBuf );
        return 0;
    }

    abstract ObjPtr getInst();

    int serialStruct( ByteBuffer buf, ISerializable val )
    {
        int ret = 0;
        ObjPtr oInst = getInst();
        if( oInst == null )
            return -rpcbaseConstants.EFAULT;
        val.setInst( oInst );
        ret = val.serialize( buf );
        return ret;
    }

    int serialArray( ByteBuffer buf, Object val, String sig )
    {
        int sigLen = sig.length();

        if( sig.charAt( 0 ) != '(' )
            return -rpcbaseConstants.EINVAL;
        if( sig.charAt( sigLen -1 ) != ')' )
            return -rpcbaseConstants.EINVAL;
        if( sigLen <= 2 )
            return -rpcbaseConstants.EINVAL;

        int ret = 0;
        do{
            String sigElem =
                sig.substring( 1, sigLen - 1 );

            if( sigElem.length() == 1 )
            {
                if( isPrimType(
                    sigElem.charAt( 0 ) ) )
                {
                    ret = serialPrimArray(
                        buf, val, sigElem );
                    break;
                }
            }

            int sizeOff = buf.position();
            Object[] objArr = ( Object[] )val;
            int count = objArr.length;
            buf.putInt( 0 );
            buf.putInt( count );
            if( count == 0 )
                break;
            for( Object elem : objArr )
            {
                ret = serialElem(
                    buf, elem, sigElem );
                if( ret < 0 )
                    break;
            }
            if( ret < 0 )
                break;
            int curPos = buf.position();
            int arrSize = curPos - sizeOff - 8;
            if( arrSize <= 0 )
                ret = -rpcbaseConstants.ERANGE;

            buf.position( sizeOff );
            buf.putInt( arrSize );
            buf.position( curPos );

        }while( false );
        return ret;
    }

    int serialMap( ByteBuffer buf, Map<?,?> val, String sig )
    {
        int ret = 0;
        int sigLen = sig.length();
        Character ch = sig.charAt( 0 );
        if( sig.charAt( 1 ) != '[' )
            return -rpcbaseConstants.EINVAL;
        else if( sig.charAt( sigLen -1 ) != ']' )
            return -rpcbaseConstants.EINVAL;
        if( sigLen <= 2 )
            return -rpcbaseConstants.EINVAL;
        String sigElem =
            sig.substring( 1, sigLen - 1 );

        do{
            int sizeOff = buf.position();
            int count = val.size();
            buf.putInt( 0 );
            buf.putInt( count );
            if( count == 0 )
                break;

            for( Map.Entry<?,?> elem : val.entrySet() )
            {
                Object key = elem.getKey();
                Object value = elem.getValue();
                ret = serialElem(
                    buf, key, sigElem.substring( 0, 1 ) );
                if( ret < 0 )
                    break;
                ret = serialElem(
                    buf, value, sigElem.substring( 1 ) );
                if( ret < 0 )
                    break;
            }
            if( ret < 0 )
                break;
            int curPos = buf.position();
            int mapSize = curPos - sizeOff - 8;
            if( mapSize <= 0 )
            {
                ret = -rpcbaseConstants.ERANGE;
                break;
            }

            buf.position( sizeOff );
            buf.putInt( mapSize );
            buf.position( curPos );

        }while( false );

        return ret;
    }

    int serialElem( ByteBuffer buf, Object val, String sig )
    {
        int ret = 0;
        int sigLen = sig.length();
        Character ch = sig.charAt( 0 );
        if( ch == '(' )
        {
            return serialArray( buf, val, sig );
        }
        else if( ch == '[' )
        {
            return serialMap(
                buf, (Map<?,?>)val, sig );
        }
        if( ! m_SerialFuncs.containsKey( ch ) )
            return -rpcbaseConstants.EINVAL;

        ISerialElem o = m_SerialFuncs.get( ch );
        o.serialize( buf, val );
        return 0;
    }

    long deserialInt64( ByteBuffer buf )
    { return buf.getLong(); }

    int deserialInt32( ByteBuffer buf )
    { return buf.getInt(); }

    short deserialInt16( ByteBuffer buf )
    { return buf.getShort(); }

    boolean deserialBool( ByteBuffer buf )
    {
        byte b = buf.get();
        if( b == 1 )
            return true;
        else if( b == 0 )
            return false;
        throw new IllegalArgumentException(
            "deserialBool with bad value" );
    }

    byte deserialInt8( ByteBuffer buf )
    { return buf.get(); }

    float deserialFloat( ByteBuffer buf )
    { return buf.getFloat(); }

    double deserialDouble( ByteBuffer buf )
    { return buf.getDouble(); }

    abstract long deserialHStream( ByteBuffer buf );

    long[] deserialInt64Arr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > 500 * 1024 * 1024 || iBytes < 0 )
            throw new IndexOutOfBoundsException();

        if( count != ( iBytes >> 3 ) )
            throw new IndexOutOfBoundsException();
        long[] vals = new long[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getLong();
        return vals;
    }

    int[] deserialInt32Arr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > 500 * 1024 * 1024 || iBytes < 0 )
            throw new IndexOutOfBoundsException();
        if( count != ( iBytes >> 2 ) )
            throw new IndexOutOfBoundsException();
        int[] vals = new int[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getInt();
        return vals;
    }

    short[] deserialInt16Arr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > 500 * 1024 * 1024 || iBytes < 0 )
            throw new IndexOutOfBoundsException();
        if( count != ( iBytes >> 1 ) )
            throw new IndexOutOfBoundsException();
        short[] vals = new short[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getShort();
        return vals;
    }

    boolean[] deserialBoolArr( ByteBuffer buf )
    {
        byte[] bytes = deserialBuf( buf );
        boolean[] bools = new boolean[ bytes.length ];
        for( int i = 0; i < bytes.length; i++ )
        {
            if( bytes[ i ] == 1 )
                bools[ i ] = true;
            else
                bools[ i ] = false;
        }
        return bools;
    }

    byte[] deserialInt8Arr( ByteBuffer buf )
    {
        return deserialBuf( buf );
    }

    float[] deserialFloatArr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > 500 * 1024 * 1024 || iBytes < 0 )
            throw new IndexOutOfBoundsException();
        if( count != ( iBytes >> 2 ) )
            throw new IndexOutOfBoundsException();
        float[] vals = new float[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getFloat();
        return vals;
    }

    double[] deserialDoubleArr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > 500 * 1024 * 1024 || iBytes < 0 )
            throw new IndexOutOfBoundsException();
        if( count != ( iBytes >> 2 ) )
            throw new IndexOutOfBoundsException();
        double[] vals = new double[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getDouble();
        return vals;
    }

    long[] deserialHStreamArr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > 500 * 1024 * 1024 || iBytes < 0 )
            throw new IndexOutOfBoundsException();

        if( count != ( iBytes >> 3 ) )
            throw new IndexOutOfBoundsException();
        long[] vals = new long[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = deserialHStream( buf );
        return vals;
    }

    Object deserialPrimArray(
        ByteBuffer buf, String strSig )
    {
        if( strSig.length() == 0 )
            return -rpcbaseConstants.EINVAL;

        int ret = 0;
        switch( strSig.charAt( 0 ) )
        {
        case 'Q':
        case 'q':
        case 'h':
            {
                long[] val =
                    deserialInt64Arr( buf );
                break;
            }
        case 'D':
        case 'd':
            {
                int[] val =
                    deserialInt32Arr( buf );
                break;
            }
        case 'W':
        case 'w':
            {
                short[] val =
                    deserialInt16Arr( buf );
                break;
            }
        case 'f':
            {
                float[] val =
                    deserialFloatArr( buf );
                break;
            }
        case 'F':
            {
                double[] val =
                    deserialDoubleArr( buf );
                break;
            }
        case 'b':
        case 'B':
            {
                byte[] val = deserialBuf( buf );
                break;
            }
        default :
           ret = -rpcbaseConstants.EINVAL;
           break;
        }
        return ret; 
    }

    Long deserialInt64Map( ByteBuffer buf )
    { return new Long( buf.getLong() ); }

    Integer deserialInt32Map( ByteBuffer buf )
    { return new Integer( buf.getInt() ); }

    Short deserialInt16Map( ByteBuffer buf )
    { return buf.getShort(); }

    Boolean deserialBoolMap( ByteBuffer buf )
    {
        byte b = buf.get();
        if( b == 1 )
            return new Boolean( true );
        else if( b == 0 )
            return new Boolean( false );

        throw new IllegalArgumentException(
            "deserialBooMap with bad value" );
    }

    Byte deserialInt8Map( ByteBuffer buf )
    { return new Byte( buf.get() ); }

    Float deserialFloatMap( ByteBuffer buf )
    { return new Float( buf.getFloat() ); }

    Double deserialDoubleMap( ByteBuffer buf )
    { return new Double( buf.getDouble() ); }

    Long deserialHStreamMap( ByteBuffer buf )
    { return new Long( deserialHStream( buf ) ); }

    String deserialString( ByteBuffer buf )
    {
        int iSize = deserialInt32( buf );
        if( iSize == 0 )
            return new String( "" );
        if( iSize > 1024 * 1024 || iSize < 0 )
            throw new IndexOutOfBoundsException(
                "string size out of bounds");

        byte[] b = new byte[ iSize ];
        buf.get( b );
        return new String( b );
    }

    byte[] deserialBuf( ByteBuffer buf )
    {
        int iSize = deserialInt32( buf );
        if( iSize == 0 )
            return new byte[0];
        if( iSize > 500 * 1024 * 1024 || iSize < 0 )
            throw new IndexOutOfBoundsException(
                "byte array size out of bounds" );

        byte[] b = new byte[ iSize ];
        buf.get( b );
        return b;
    }
        
    ObjPtr deserialObjPtr( ByteBuffer buf )
    {
        ObjPtr pNewObj = new ObjPtr();

        int curPos = buf.position();
        int clsid = buf.getInt();
        int iSize = buf.getInt();
        int iVersion = buf.getInt();
        if( iSize > 1024 * 1024  || iSize < 0)
            throw new IndexOutOfBoundsException();

        if( iSize == 0 )
            return pNewObj;

        byte[] b = new byte[
            buf.position() - curPos + iSize ];
        buf.position( curPos );
        buf.get( b );
        JRetVal jret = ( JRetVal )
            pNewObj.DeserialObjPtr( b, 0 );
        if( jret.ERROR() )
            throw new RuntimeException(
                "Error deserialObj" +
                jret.getError() );

        return pNewObj;
    }

    ISerializable deserialStruct( ByteBuffer buf )
    {
        int curPos = buf.position();
        int id = buf.getInt();
        buf.position( curPos );
        ISerializable oStruct =
            StructFactoryBase.create( id );
        if( oStruct == null )
            throw new NullPointerException();

        oStruct.setInst( getInst() );
        oStruct.deserialize( buf );
        return oStruct;
    }

    Object deserialArray( ByteBuffer buf, String sig )
    {
        int sigLen = sig.length();
        if( sig.charAt( 0 ) != '(' ||
            sig.charAt( sigLen -1 ) != ')' )
            throw new IllegalArgumentException();
        if( sigLen <= 2 )
            throw new IllegalArgumentException();

        String sigElem =
            sig.substring( 1, sigLen - 1 );

        if( sigElem.length() == 1 )
        {
            if( !isPrimType(
                sigElem.charAt( 0 ) ) )
            {
                return deserialPrimArray(
                    buf, sigElem );
            }
        }

        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > 500 * 1024 * 1024 || iBytes < 0 )
            throw new IndexOutOfBoundsException();

        if( count > 1024 * 1024 || count < 0 )
            throw new IndexOutOfBoundsException();

        Object[] objArr = new Object[ count ];

        int beginPos = buf.position();
        if( count == 0 )
            return objArr;

        for( int i = 0; i < count; i++ )
        {
            Object oElem =
                deserialElem( buf, sigElem );

            if( oElem == null )
                throw new NullPointerException();

            objArr[ i ] = oElem;
        }
        if( buf.position() - beginPos != iBytes )
            throw new IndexOutOfBoundsException();

        return objArr;
    }

    Map<Object, Object> deserialMap(
        ByteBuffer buf, String sig )
    {
        int sigLen = sig.length();
        if( sig.charAt( 0 ) != '[' ||
            sig.charAt( sigLen -1 ) != ']' )
            throw new IllegalArgumentException();
        if( sigLen <= 2 )
            throw new IllegalArgumentException();

        String sigElem =
            sig.substring( 1, sigLen - 1 );

        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > 500 * 1024 * 1024 || iBytes < 0 )
            throw new IndexOutOfBoundsException();

        if( count > 1024 * 1024 || count < 0 )
            throw new IndexOutOfBoundsException();

        Map<Object, Object >mapRet =
            new HashMap< Object, Object >();
        int beginPos = buf.position();
        if( count == 0 )
            return mapRet;

        for( int i = 0; i < count; i++ )
        {
            Object key = deserialElem(
                buf, sigElem.substring( 0, 1 ) );
            if( key == null )
                throw new NullPointerException();
            Object val = deserialElem(
                buf, sigElem.substring( 1 ) );
            if( val == null )
                throw new NullPointerException();
            mapRet.put( key, val );
        }

        if( iBytes != buf.position() - beginPos )
            throw new IndexOutOfBoundsException();
        return mapRet;
    }

    Object deserialElem(
        ByteBuffer buf, String sig )
    {
        Character ch = sig.charAt( 0 );
        if( ch == '(' )
        {
            Object oArr =
                deserialArray( buf, sig );
            return oArr;
        }
        if( ch == '[' )
        {
            Map<Object, Object> oMap =
                deserialMap( buf, sig );
            return ( Object )oMap;
        }

        if( !m_DeserialFuncs.containsKey( ch ) )
            throw new IllegalArgumentException();

        if( m_DeserialFuncs.containsKey( ch ) )
            throw new RuntimeException(
                "Error deserialElem" );

        IDeserialElem desFun =
            m_DeserialFuncs.get( ch );

        return desFun.deserialize( buf );
    }
    public Map< Character, ISerialElem >
        m_SerialFuncs = createSerialMap();

    private Map<Character, ISerialElem> createSerialMap()
    {
        Map< Character, ISerialElem > m =
            new HashMap< Character, ISerialElem >();

        ISerialElem ent = new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialInt64,    # TOK_UINT64,
            { return serialInt64Map( buf, (Long)elem ); } };
        m.put( 'Q', ent );
        m.put( 'q', ent );

        ent = new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialInt32,    # TOK_UINT32,
            { return serialInt32Map( buf, (Integer)elem ); } };
        m.put( 'D', ent );
        m.put( 'd', ent );
        ent = new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialInt16,    # TOK_UINT16
            { return serialInt16Map( buf, (Short)elem ); } };
        m.put( 'W', ent );
        m.put( 'w', ent );
        m.put( 'f', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialFloat,    # TOK_FLOAT
            { return serialFloatMap( buf, (Float)elem ); } } );
        m.put( 'F', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialDouble,    # TOK_DOUBLE
            { return serialDoubleMap( buf, (Double)elem ); } } );
        m.put( 'b', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialBool,    # TOK_BOOL
            { return serialBoolMap( buf, (Boolean)elem ); } } );
        m.put( 'B' , new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialInt8,    # TOK_BYTE
            { return serialInt8Map( buf, (Byte)elem ); } } );
        m.put( 'h', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialHStream,    # TOK_HSTREAM
            { return serialHStreamMap( buf, (Long)elem ); } } );
        m.put( 's', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialString,    # TOK_STRING
            { return serialString( buf, (String)elem ); } } );
        m.put( 'a', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialBuf,    # TOK_BYTEARR
            { return serialBuf( buf, (byte[])elem ); } } );
        m.put( 'o', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialObjPtr,    # TOK_OBJPTR
            { return serialObjPtr( buf, (ObjPtr)elem ); } } );
        m.put( 'O', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialStruct,    # TOK_STRUCT
            { return serialStruct( buf, (ISerializable)elem ); } } );
        return m;
    }

    public Map< Character, IDeserialElem >
        m_DeserialFuncs = createDeserialMap();

    private Map<Character, IDeserialElem> createDeserialMap()
    {
        Map< Character, IDeserialElem > m =
            new HashMap< Character, IDeserialElem >();
        IDeserialElem ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // SerialInt64,    # TOK_UINT64,
            { return ( Object )deserialInt64Map( buf ); } }; 
        m.put( 'Q', ent );
        m.put( 'q', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialInt32,    # TOK_UINT32,
            { return ( Object )deserialInt32Map( buf ); } }; 
        m.put( 'D', ent );
        m.put( 'd', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialInt16,    # TOK_UINT16,
            { return ( Object )deserialInt32Map( buf ); } }; 
        m.put( 'W', ent );
        m.put( 'w', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialBool,    # TOK_BOOL,
            { return ( Object )deserialBoolMap( buf ); } }; 
        m.put( 'b', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialInt8,    # TOK_BYTE,
            { return ( Object )deserialBoolMap( buf ); } }; 
        m.put( 'B', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialFloat,    # TOK_FLOAT,
            { return ( Object )deserialFloatMap( buf ); } }; 
        m.put( 'f', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialDouble,    # TOK_DOUBLE,
            { return ( Object )deserialDoubleMap( buf ); } }; 
        m.put( 'F', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialHStream,    # TOK_HSTREAM,
            { return ( Object )deserialHStreamMap( buf ); } }; 
        m.put( 'h', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialString,    # TOK_STRING, 
            { return ( Object )deserialString( buf ); } }; 
        m.put( 's', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialBuf,    # TOK_BYTEARR,
            { return ( Object )deserialBuf( buf ); } }; 
        m.put( 'a', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialObjPtr,    # TOK_OBJPTR,
            { return ( Object )deserialObjPtr( buf ); } }; 
        m.put( 'o', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialStruct  # TOK_STRUCT,
            { return ( Object )deserialStruct( buf ); } }; 
        m.put( 'O', ent );

        return m;
    }
}

