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
import java.util.NoSuchElementException;

abstract public class JavaSerialBase
{
    public static int m_iMaxSize =
        RC.MAX_BUF_SIZE;
    public interface ISerializable
    {
        public abstract int serialize(
            BufPtr buf, Integer offset );
        public abstract int deserialize(
            ByteBuffer buf );
        public abstract void setInst(
            Object oInst );
    }

    public interface ISerialElem
    {
        public abstract int serialize(
            BufPtr buf, Integer offset, Object val );
    }
    public interface IDeserialElem
    {
        public abstract Object deserialize(
            ByteBuffer buf );
    }

    public int serialBoolArr( BufPtr buf, Integer offset, boolean[] val )
    {
        int iCount = val.length;
        int iBytes = iCount;
        serialInt32( buf, offset, iBytes );
        if( iCount == 0 )
            return 0;

        int ret = 0;
        for( boolean i : val )
        {
            if( i )
                serialInt8(
                    buf, offset, ( byte )1 );
            else
                serialInt8(
                    buf, offset, ( byte )0 );
        }
        return ret;
    }
    public int serialInt8Arr(
        BufPtr buf, Integer offset, byte[] val )
    {
        offset = buf.SerialByteArray( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialInt16Arr(
        BufPtr buf, Integer offset, short[] val )
    {
        offset = buf.SerialShortArr( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialInt32Arr(
        BufPtr buf, Integer offset, int[] val )
    {
        offset = buf.SerialIntArr( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialInt64Arr(
        BufPtr buf, Integer offset, long[] val )
    {
        offset = buf.SerialLongArr( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialFloatArr(
        BufPtr buf, Integer offset, float[] val )
    {
        offset = buf.SerialFloatArr( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialDoubleArr(
        BufPtr buf, Integer offset, double[] val )
    {
        offset = buf.SerialDoubleArr( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialHStreamArr( 
        BufPtr buf, Integer offset, long[] val )
    {
        int iCount = val.length;
        int iBytes = iCount * 8;
        offset = buf.SerialInt( offset, iBytes );
        if( RC.ERROR( offset ) )
            return offset;

        if( iCount == 0 )
            return 0;

        int ret = 0;
        for( long i : val )
        {
            ret = serialHStream( buf, offset, i );
            if( ret < 0 )
                break;
        }
        return ret;
    }

    public static boolean isPrimType( Character ch )
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

    public int serialPrimArray(
        BufPtr buf, Integer offset, Object val, String strSig )
    {
        if( strSig.length() == 0 )
            return -RC.EINVAL;

        int ret = 0;
        switch( strSig.charAt( 0 ) )
        {
        case 'Q':
        case 'q':
        case 'h':
            {
                ret = serialInt64Arr(
                    buf, offset, ( long[] )val );
                break;
            }
        case 'D':
        case 'd':
            {
                ret = serialInt32Arr(
                    buf, offset, ( int[] )val );
                break;
            }
        case 'W':
        case 'w':
            {
                ret = serialInt16Arr(
                    buf, offset, ( short[] )val );
                break;
            }
        case 'f':
            {
                ret = serialFloatArr(
                    buf, offset, ( float[] )val );
                break;
            }
        case 'F':
            {
                ret = serialDoubleArr(
                    buf, offset, ( double[] )val );
                break;
            }
        case 'b':
        case 'B':
            {
                ret = serialInt8Arr(
                    buf, offset, ( byte[] )val );
                break;
            }
        default :
           ret = -RC.EINVAL;
           break;
        }
        return ret; 
    }

    public int serialBool(
        BufPtr buf, Integer offset, boolean val )
    {
        if( val )
        {
            offset = buf.SerialByte(
                offset, ( byte )1 );
        }
        else
        {
            offset = buf.SerialByte(
                offset, ( byte )0 );
        }
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }
    public int serialInt8( BufPtr buf, Integer offset, byte val )
    {
        offset = buf.SerialByte( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialInt16( BufPtr buf, Integer offset, short val )
    {
        offset = buf.SerialShort( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialInt32( BufPtr buf, Integer offset, int val )
    {
        offset = buf.SerialInt( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialInt64( BufPtr buf, Integer offset, long val )
    {
        offset = buf.SerialLong( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialFloat( BufPtr buf, Integer offset, float val )
    {
        offset = buf.SerialFloat( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialDouble( BufPtr buf, Integer offset, double val )
    {
        offset = buf.SerialDouble( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public abstract int serialHStream(
        BufPtr buf, Integer offset, long val );

    public int serialString(
        BufPtr buf, Integer offset, String val )
    {
        offset = buf.SerialString( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialBuf(
        BufPtr buf, Integer offset, byte[] val )
    {
        offset = buf.SerialByteArray( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public int serialObjPtr(
        BufPtr buf, Integer offset, ObjPtr val )
    {
        offset = buf.SerialObjPtr( offset, val );
        if( RC.ERROR( offset ) )
            return offset;
        return 0;
    }

    public abstract ObjPtr getInst();

    public int serialStruct( BufPtr buf, Integer offset, ISerializable val )
    {
        int ret = 0;
        ObjPtr oInst = getInst();
        if( oInst == null )
            return -RC.EFAULT;
        val.setInst( oInst );
        ret = val.serialize( buf, offset );
        return ret;
    }

    public int serialArray( BufPtr buf,
        Integer offset, Object val, String sig )
    {
        int sigLen = sig.length();

        if( sig.charAt( 0 ) != '(' )
            return -RC.EINVAL;
        if( sig.charAt( sigLen -1 ) != ')' )
            return -RC.EINVAL;
        if( sigLen <= 2 )
            return -RC.EINVAL;

        int ret = 0;
        do{
            String sigElem =
                sig.substring( 1, sigLen - 1 );

            if( sigElem.length() == 1 )
            {
                Character ch =
                    sigElem.charAt( 0 );

                if( isPrimType( ch ) )
                {
                    ret = serialPrimArray( buf,
                        offset, val, sigElem );
                    break;
                }
            }

            int sizeOff = offset;
            Object[] objArr = ( Object[] )val;
            int count = objArr.length;

            ret = serialInt32( buf, offset, 0 );
            if( ret < 0 )
                break;

            ret = serialInt32( buf, offset, count );
            if( ret < 0 )
                break;

            if( count == 0 )
                break;

            for( Object elem : objArr )
            {
                ret = serialElem(
                    buf, offset, elem, sigElem );
                if( ret < 0 )
                    break;
            }
            if( ret < 0 )
                break;

            int arrSize = offset - sizeOff - 8;
            if( arrSize <= 0 )
                ret = -RC.ERANGE;

            Integer oSizeOff = sizeOff;
            ret = serialInt32(
                buf, oSizeOff, arrSize );

        }while( false );

        return ret;
    }

    public int serialMap( BufPtr buf, Integer offset, Map<?,?> val, String sig )
    {
        int sigLen = sig.length();
        Character ch = sig.charAt( 0 );
        if( sig.charAt( 1 ) != '[' )
            return -RC.EINVAL;
        else if( sig.charAt( sigLen -1 ) != ']' )
            return -RC.EINVAL;
        if( sigLen <= 2 )
            return -RC.EINVAL;
        String sigElem =
            sig.substring( 1, sigLen - 1 );

        int ret = 0;
        do{
            int sizeOff = offset;
            int count = val.size();
            ret = serialInt32( buf, offset, 0 );
            if( ret < 0 )
                break;

            serialInt32( buf, offset, count );
            if( ret < 0 )
                break;

            if( count == 0 )
                break;

            for( Map.Entry<?,?> elem : val.entrySet() )
            {
                Object key = elem.getKey();
                Object value = elem.getValue();
                ret = serialElem(
                    buf, offset, key, sigElem.substring( 0, 1 ) );
                if( ret < 0 )
                    break;
                ret = serialElem(
                    buf, offset, value, sigElem.substring( 1 ) );
                if( ret < 0 )
                    break;
            }
            if( ret < 0 )
                break;
            int curPos = offset;
            int mapSize = curPos - sizeOff - 8;
            if( mapSize <= 0 )
            {
                ret = -RC.ERANGE;
                break;
            }
            
            Integer oSizeOff = sizeOff;
            ret = serialInt32(
                buf, oSizeOff, mapSize );

        }while( false );

        return ret;
    }

    public int serialElem( BufPtr buf,
        Integer offset, Object val, String sig )
    {
        int ret = 0;
        int sigLen = sig.length();
        Character ch = sig.charAt( 0 );
        if( ch == '(' )
        {
            return serialArray(
                buf, offset, val, sig );
        }
        else if( ch == '[' )
        {
            return serialMap(
                buf, offset, (Map<?,?>)val, sig );
        }
        if( ! m_SerialFuncs.containsKey( ch ) )
            return -RC.ENOENT;

        ISerialElem o = m_SerialFuncs.get( ch );
        o.serialize( buf, offset, val );
        return 0;
    }

    public long deserialInt64( ByteBuffer buf )
    { return buf.getLong(); }

    public int deserialInt32( ByteBuffer buf )
    { return buf.getInt(); }

    public short deserialInt16( ByteBuffer buf )
    { return buf.getShort(); }

    public boolean deserialBool( ByteBuffer buf )
    {
        byte b = buf.get();
        if( b == 1 )
            return true;
        else if( b == 0 )
            return false;
        throw new IllegalArgumentException(
            "deserialBool with bad value" );
    }

    public byte deserialInt8( ByteBuffer buf )
    { return buf.get(); }

    public float deserialFloat( ByteBuffer buf )
    { return buf.getFloat(); }

    public double deserialDouble( ByteBuffer buf )
    { return buf.getDouble(); }

    public abstract long deserialHStream( ByteBuffer buf );

    public long[] deserialInt64Arr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = iBytes >> 3;
        if( iBytes > m_iMaxSize || iBytes < 0 )
            throw new IndexOutOfBoundsException();
        long[] vals = new long[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getLong();
        return vals;
    }

    public int[] deserialInt32Arr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = iBytes >> 2;
        if( iBytes > m_iMaxSize || iBytes < 0 )
            throw new IndexOutOfBoundsException();
        int[] vals = new int[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getInt();
        return vals;
    }

    public short[] deserialInt16Arr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = iBytes >> 1;
        if( iBytes > m_iMaxSize || iBytes < 0 )
            throw new IndexOutOfBoundsException();
        short[] vals = new short[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getShort();
        return vals;
    }

    public boolean[] deserialBoolArr( ByteBuffer buf )
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

    public byte[] deserialInt8Arr( ByteBuffer buf )
    {
        return deserialBuf( buf );
    }

    public float[] deserialFloatArr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > m_iMaxSize || iBytes < 0 )
            throw new IndexOutOfBoundsException();
        if( count != ( iBytes >> 2 ) )
            throw new IndexOutOfBoundsException();
        float[] vals = new float[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getFloat();
        return vals;
    }

    public double[] deserialDoubleArr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > m_iMaxSize || iBytes < 0 )
            throw new IndexOutOfBoundsException();
        if( count != ( iBytes >> 2 ) )
            throw new IndexOutOfBoundsException();
        double[] vals = new double[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = buf.getDouble();
        return vals;
    }

    public long[] deserialHStreamArr( ByteBuffer buf )
    {
        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        if( iBytes > m_iMaxSize || iBytes < 0 )
            throw new IndexOutOfBoundsException();

        long[] vals = new long[ count ];
        for( int i = 0; i < count; i++ )
            vals[ i ] = deserialHStream( buf );
        return vals;
    }

    public Object deserialPrimArray(
        ByteBuffer buf, String strSig )
    {
        if( strSig.length() == 0 )
            return -RC.EINVAL;

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
           ret = -RC.EINVAL;
           break;
        }
        return ret; 
    }

    public String deserialString( ByteBuffer buf )
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

    public byte[] deserialBuf( ByteBuffer buf )
    {
        int iSize = deserialInt32( buf );
        if( iSize == 0 )
            return new byte[0];
        if( iSize > m_iMaxSize || iSize < 0 )
            throw new IndexOutOfBoundsException(
                "byte array size out of bounds" );

        byte[] b = new byte[ iSize ];
        buf.get( b );
        return b;
    }
        
    public ObjPtr deserialObjPtr( ByteBuffer buf )
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

    public ISerializable deserialStruct( ByteBuffer buf )
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

    public Object deserialArray( ByteBuffer buf, String sig )
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
        if( iBytes > m_iMaxSize || iBytes < 0 )
            throw new IndexOutOfBoundsException();

        if( count < 0 || count > iBytes )
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

    public Map<Object, Object> deserialMap(
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
        if( iBytes > m_iMaxSize || iBytes < 0 )
            throw new IndexOutOfBoundsException();

        if( count < 0 || count > iBytes )
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

    public Object deserialElem(
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
            throw new NoSuchElementException();

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
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialInt64,    # TOK_UINT64,
            { return serialInt64( buf, offset, (Long)elem ); } };
        m.put( 'Q', ent );
        m.put( 'q', ent );

        ent = new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialInt32,    # TOK_UINT32,
            { return serialInt32( buf, offset, (Integer)elem ); } };
        m.put( 'D', ent );
        m.put( 'd', ent );
        ent = new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialInt16,    # TOK_UINT16
            { return serialInt16( buf, offset, (Short)elem ); } };
        m.put( 'W', ent );
        m.put( 'w', ent );
        m.put( 'f', new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialFloat,    # TOK_FLOAT
            { return serialFloat( buf, offset, (Float)elem ); } } );
        m.put( 'F', new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialDouble,    # TOK_DOUBLE
            { return serialDouble( buf, offset, (Double)elem ); } } );
        m.put( 'b', new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialBool,    # TOK_BOOL
            { return serialBool( buf, offset, (Boolean)elem ); } } );
        m.put( 'B' , new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialInt8,    # TOK_BYTE
            { return serialInt8( buf, offset, (Byte)elem ); } } );
        m.put( 'h', new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialHStream,    # TOK_HSTREAM
            { return serialHStream( buf, offset, (Long)elem ); } } );
        m.put( 's', new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialString,    # TOK_STRING
            { return serialString( buf, offset, (String)elem ); } } );
        m.put( 'a', new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialBuf,    # TOK_BYTEARR
            { return serialBuf( buf, offset, (byte[])elem ); } } );
        m.put( 'o', new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialObjPtr,    # TOK_OBJPTR
            { return serialObjPtr( buf, offset, (ObjPtr)elem ); } } );
        m.put( 'O', new ISerialElem() {
            public int serialize( BufPtr buf, Integer offset, Object elem ) // SerialStruct,    # TOK_STRUCT
            { return serialStruct( buf, offset, (ISerializable)elem ); } } );
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
            { return ( Long )deserialInt64( buf ); } }; 
        m.put( 'Q', ent );
        m.put( 'q', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialInt32,    # TOK_UINT32,
            { return ( Integer )deserialInt32( buf ); } }; 
        m.put( 'D', ent );
        m.put( 'd', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialInt16,    # TOK_UINT16,
            { return ( Short )deserialInt16( buf ); } }; 
        m.put( 'W', ent );
        m.put( 'w', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialBool,    # TOK_BOOL,
            { return ( Boolean )deserialBool( buf ); } }; 
        m.put( 'b', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialInt8,    # TOK_BYTE,
            { return ( Byte )deserialInt8( buf ); } }; 
        m.put( 'B', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialFloat,    # TOK_FLOAT,
            { return ( Float )deserialFloat( buf ); } }; 
        m.put( 'f', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialDouble,    # TOK_DOUBLE,
            { return ( Double )deserialDouble( buf ); } }; 
        m.put( 'F', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialHStream,    # TOK_HSTREAM,
            { return ( Long )deserialHStream( buf ); } }; 
        m.put( 'h', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialString,    # TOK_STRING, 
            { return deserialString( buf ); } }; 
        m.put( 's', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialBuf,    # TOK_BYTEARR,
            { return deserialBuf( buf ); } }; 
        m.put( 'a', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialObjPtr,    # TOK_OBJPTR,
            { return deserialObjPtr( buf ); } }; 
        m.put( 'o', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialStruct  # TOK_STRUCT,
            { return deserialStruct( buf ); } }; 
        m.put( 'O', ent );

        return m;
    }
}

