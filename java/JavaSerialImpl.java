package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;
import java.nio.ByteBuffer;
import static java.util.Map.entry;

public class JavaSerialImpl extends JavaSerialBase
{
    protected InstType m_oInst = null;
    public JavaSerialBase( Object pIf )
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
        if( jret.Error() ) 
            return jret.getError();

        serialInt64( buf, ( Long )jret.getAt( 0 ) )
        return 0 
    }

    Object[] deserialHStream( ByteBuffer buf, int offset )
    {
        if( m_oInst == null )
        {
            return new Object[0];
        }
        val = deserialInt64( buf, offset )
        ret = m_oInst.GetChanByIdHash( val[ 0 ] )
        if ret == ErrorCode.INVALID_HANDLE :
            return new Object[ 2 ]{ 0, 0};
        return new Object[ 2 ]{ret, offset + 8 };
    }
}
