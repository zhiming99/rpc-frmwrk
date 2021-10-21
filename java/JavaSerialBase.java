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

    void serialBool( ByteBuffer buf, boolean val )
    {
        if( val )
            buf.put( ( byte )1 );
        else
            buf.put( ( byte )0 );
    }
    int serialBool( ByteBuffer buf, Boolean val )
    {
        if( val )
            buf.put( ( byte )1 );
        else
            buf.put( ( byte )0 );
        return 0;
    }

    void serialInt8( ByteBuffer buf, byte val )
    { buf.put( val ); }

    int serialInt8( ByteBuffer buf, Byte val )
    {
        buf.put( val.byteValue() );
        return 0;
    }

    void serialInt16( ByteBuffer buf, short val )
    { buf.putShort( val ); }

    int serialInt16( ByteBuffer buf, Short val )
    {
        buf.putShort( val.shortValue() );
        return 0;
    }

    void serialInt32( ByteBuffer buf, int val )
    { buf.putInt( val ); }

    int serialInt32( ByteBuffer buf, Integer val )
    {
        buf.putInt( val.intValue() );
        return 0;
    }

    void serialInt64( ByteBuffer buf, long val )
    { buf.putLong( val ); }

    int serialInt64( ByteBuffer buf, Long val )
    {
        buf.putLong( val.longValue() );
        return 0;
    }

    void serialFloat( ByteBuffer buf, float val )
    { buf.putFloat( val ); }

    int serialFloat( ByteBuffer buf, Float val )
    {
        buf.putFloat( val.floatValue() );
        return 0;
    }

    void serialDouble( ByteBuffer buf, double val )
    { buf.putDouble( val ); }

    int serialDouble( ByteBuffer buf, Double val )
    {
        buf.putDouble( val.doubleValue() );
        return 0;
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

    abstract int serialHStream(
        ByteBuffer buf, long val );

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

    int serialArray( ByteBuffer buf, List<?> val, String sig )
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

            int sizeOff = buf.position();
            int count = val.size();
            buf.putInt( 0 );
            buf.putInt( count );
            if( count == 0 )
                break;
            for( Object elem : val )
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
            return serialArray(
                buf, (List<?>)val, sig );
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

    Long deserialInt64( ByteBuffer buf )
    { return new Long( buf.getLong() ); }

    Integer deserialInt32( ByteBuffer buf )
    { return new Integer( buf.getInt() ); }

    Short deserialInt16( ByteBuffer buf )
    { return new Short( buf.getShort() ); }

    Boolean deserialBool( ByteBuffer buf )
    {
        byte b = buf.get();
        if( b == 1 )
            return new Boolean( true );
        return new Boolean( false );
    }

    Byte deserialInt8( ByteBuffer buf )
    { return new Byte( buf.get() ); }

    Float deserialFloat( ByteBuffer buf )
    { return new Float( buf.getFloat() ); }

    Double deserialDouble( ByteBuffer buf )
    { return new Double( buf.getDouble() ); }

    abstract Long deserialHStream( ByteBuffer buf );

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

    List<Object> deserialArray( ByteBuffer buf, String sig )
    {
        int sigLen = sig.length();
        if( sig.charAt( 0 ) != '(' ||
            sig.charAt( sigLen -1 ) != ')' )
            throw new IllegalArgumentException();
        if( sigLen <= 2 )
            throw new IllegalArgumentException();

        String sigElem =
            sig.substring( 1, sigLen - 1 );

        int iBytes = deserialInt32( buf );
        int count = deserialInt32( buf );
        List<Object> listRet =
            new ArrayList< Object >();

        int beginPos = buf.position();
        if( count == 0 )
            return listRet;

        for( int i = 0; i < count; i++ )
        {
            Object oElem =
                deserialElem( buf, sigElem );

            if( oElem == null )
                throw new NullPointerException();

            listRet.add( oElem );
        }
        if( buf.position() - beginPos != iBytes )
            throw new IndexOutOfBoundsException();

        return listRet;
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
            List<Object> oArr =
                deserialArray( buf, sig );
            return ( Object )oArr;
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
            { return serialInt64( buf, (Long)elem ); } };
        m.put( 'Q', ent );
        m.put( 'q', ent );

        ent = new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialInt32,    # TOK_UINT32,
            { return serialInt32( buf, (Integer)elem ); } };
        m.put( 'D', ent );
        m.put( 'd', ent );
        ent = new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialInt16,    # TOK_UINT16
            { return serialInt16( buf, (Short)elem ); } };
        m.put( 'W', ent );
        m.put( 'w', ent );
        m.put( 'f', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialFloat,    # TOK_FLOAT
            { return serialFloat( buf, (Float)elem ); } } );
        m.put( 'F', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialDouble,    # TOK_DOUBLE
            { return serialDouble( buf, (Double)elem ); } } );
        m.put( 'b', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialBool,    # TOK_BOOL
            { return serialBool( buf, (Boolean)elem ); } } );
        m.put( 'B' , new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialInt8,    # TOK_BYTE
            { return serialInt8( buf, (Byte)elem ); } } );
        m.put( 'h', new ISerialElem() {
            public int serialize( ByteBuffer buf, Object elem ) // SerialHStream,    # TOK_HSTREAM
            { return serialHStream( buf, (Long)elem ); } } );
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
            { return ( Object )deserialInt64( buf ); } }; 
        m.put( 'Q', ent );
        m.put( 'q', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialInt32,    # TOK_UINT32,
            { return ( Object )deserialInt32( buf ); } }; 
        m.put( 'D', ent );
        m.put( 'd', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialInt16,    # TOK_UINT16,
            { return ( Object )deserialInt32( buf ); } }; 
        m.put( 'W', ent );
        m.put( 'w', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialBool,    # TOK_BOOL,
            { return ( Object )deserialBool( buf ); } }; 
        m.put( 'b', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialInt8,    # TOK_BYTE,
            { return ( Object )deserialBool( buf ); } }; 
        m.put( 'B', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialFloat,    # TOK_FLOAT,
            { return ( Object )deserialFloat( buf ); } }; 
        m.put( 'f', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialDouble,    # TOK_DOUBLE,
            { return ( Object )deserialDouble( buf ); } }; 
        m.put( 'F', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialHStream,    # TOK_HSTREAM,
            { return ( Object )deserialHStream( buf ); } }; 
        m.put( 'h', ent );

        ent = new IDeserialElem() {
            public Object deserialize( ByteBuffer buf ) // DeserialString,    # TOK_STRING, 
            { return ( Object )deserialHStream( buf ); } }; 
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

