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
    public JavaSerialImpl( InstType pIf )
    { m_oInst = pIf; }

    public ObjPtr getInst()
    {
        return ( ObjPtr )m_oInst.CastToObjPtr();
    }
    public int serialHStream( BufPtr buf, Integer offset, long val )
    {
        if( m_oInst == null )
            return -RC.EFAULT;

        JRetVal jret =
            ( JRetVal )m_oInst.GetIdHash( val );
        if( jret.ERROR() ) 
            return jret.getError();

        serialInt64( buf, offset,
            ( Long )jret.getAt( 0 ) );

        return 0;
    }

    public long deserialHStream( ByteBuffer buf )
    {
        if( m_oInst == null )
            throw new NullPointerException(
                "c++ instance is empty");

        Long qwHash = deserialInt64( buf );
        if( qwHash == RC.INVALID_HANDLE )
            throw new NullPointerException(
                "channel hash is empty");

        long ret = m_oInst.GetChanByIdHash(
            qwHash.longValue() );

        if( ret == RC.INVALID_HANDLE )
            throw new NullPointerException(
                "channel handle is invalid");

        return ret;
    }
}
