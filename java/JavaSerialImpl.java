package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.nio.ByteBuffer;

public class JavaSerialImpl extends JavaSerialBase
{
    protected InstType m_oInst = null;
    public JavaSerialImpl( Object pIf )
    { m_oInst = ( InstType )pIf; }

    ObjPtr getInst()
    {
        return m_oInst.CastToObjPtr();
    }
    int serialHStream( ByteBuffer buf, long val )
    {
        if( m_oInst == null )
            return -rpcbaseConstants.EFAULT;

        JRetVal jret =
            ( JRetVal )m_oInst.GetIdHash( val );
        if( jret.ERROR() ) 
            return jret.getError();

        serialInt64(
            buf, ( Long )jret.getAt( 0 ) );

        return 0;
    }

    Long deserialHStream( ByteBuffer buf )
    {
        if( m_oInst == null )
            throw new NullPointerException(
                "c++ object is empty");

        Long qwHash = deserialInt64( buf );
        if( qwHash == rpcbaseConstants.INVALID_HANDLE )
            throw new NullPointerException(
                "channel hash is empty");

        long ret = m_oInst.GetChanByIdHash(
            qwHash.longValue() );

        if( ret == rpcbaseConstants.INVALID_HANDLE )
            throw new NullPointerException(
                "channel handle is invalid");

        return new Long( ret );
    }
}
