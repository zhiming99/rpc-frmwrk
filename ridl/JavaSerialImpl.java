package XXXXX;
import java.nio.ByteBuffer;
import org.rpcf.rpcbase.*;

public class JavaSerialImpl extends JavaSerialBase
{
    protected InstType m_oInst;
    public JavaSerialImpl( InstType pIf )
    { m_oInst = pIf; }

    public Object getInst()
    { return m_oInst; }

    public int serialHStream(
        BufPtr buf, int[] offset, long val )
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

        long qwHash = deserialInt64( buf );
        if( qwHash == RC.INVALID_HANDLE )
            throw new NullPointerException(
                "channel hash is empty");

        long ret = m_oInst.GetChanByIdHash(
            qwHash );

        if( ret == RC.INVALID_HANDLE )
            throw new NullPointerException(
                "channel handle is invalid");

        else if( RC.ERROR( ( int )ret ) )
            throw new NullPointerException(
                "deserialHstream failed with error " + ret);

        return ret;
    }
}
