package XXXXX;
import java.lang.String;
import java.util.HashMap;
import java.util.Map;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.NoSuchElementException;
import org.rpcf.rpcbase.*;

class JVariant extends JavaSerialBase.ISerializable
{
    public int iType = rpcbase.typeNone;
    public Object val = null;

    public Object m_oInst = null;

    public void setInst( Object oInst )
    { m_oInst = oInst; }

    public Object getInst()
    { return m_oInst; }
    
    public int serialize( BufPtr buf, int[] offset )
    {
        JavaSerialBase osb = getSerialBase();
        int ret = osb.serialInt8( buf, offset, ( byte )iType );
        if( RC.ERROR( ret ) )
            return ret;
        switch( iType )
        {
        case RC.typeByte:
            ret = osb.serialInt8( buf, offset, ( byte )val );
            break;
        case RC.typeUInt16:
            ret = osb.serialInt16( buf, offset, ( short )val );
            break;
        case RC.typeUInt32:
            ret = osb.serialInt32( buf, offset, ( int )val );
            break;
        case RC.typeUInt64:
            ret = osb.serialInt64( buf, offset, ( long )val );
            break;
        case RC.typeFloat:
            ret = osb.serialFloat( buf, offset, ( float )val );
            break;
        case RC.typeDouble:
            ret = osb.serialDouble( buf, offset, ( double )val );
            break;
        case RC.typeString:
            ret = osb.serialString( buf, offset, ( String )val );
            break;
        case RC.typeByteArr:
            ret = osb.serialBuf( buf, offset, ( byte[] )val );
            break;
        case RC.typeObj:
            if( val instanceof JavaSerialBase.ISerializable )
                ret = osb.serialStruct( buf, offset,
                    ( JavaSerialBase.ISerializable )val );
            else
                ret = osb.serialObjPtr(
                    buf, offset, ( ObjPtr )val );
            break;
        case RC.typeNone:
            break;
        default:
            ret = -RC.ENOTSUP;
        }
        return ret;
    }

    public int deserialize( ByteBuffer buf )
    {
        int ret = 0;
        JavaSerialBase osb = getSerialBase();
        this.iType = osb.deserialInt8( buf );
        switch( this.iType )
        {
        case RC.typeByte:
            this.val = osb.deserialInt8( buf );
            break;
        case RC.typeUInt16:
            this.val = osb.deserialInt16( buf );
            break;
        case RC.typeUInt32:
            this.val = osb.deserialInt32( buf );
            break;
        case RC.typeUInt64:
            this.val = osb.deserialInt64( buf );
            break;
        case RC.typeFloat:
            this.val = osb.deserialFloat( buf );
            break;
        case RC.typeDouble:
            this.val = osb.deserialDouble( buf );
            break;
        case RC.typeString:
            this.val = osb.deserialString( buf );
            break;
        case RC.typeByteArr:
            this.val = osb.deserialBuf( buf );
            break;
        case RC.typeObj:
            {
                buf.mark();
                int first = buf.getInt();
                int second = buf.getInt();
                buf.reset();
                if( first == 0x73747275 )
                    this.val = osb.deserialStruct( buf );
                else
                    this.val = osb.deserialObjPtr( buf );
                break;
            }
        case RC.typeNone:
            { break; }
        default:
            ret = -RC.ENOTSUP;
        }
        return ret;
    }
}
