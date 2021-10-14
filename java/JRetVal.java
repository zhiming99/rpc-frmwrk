package org.rpcf.rpcbase;
import java.util.ArrayList;
import java.util.List;

public class JRetVal {

    protected  int m_iRetVal = 0;

    protected List<Object> m_listResp =
        new ArrayList< Object >();

    boolean ERROR( )
    { return m_iRetVal < 0; }

    boolean SUCCEEDED()
    { return m_iRetVal >= 0; }

    boolean isPending()
    { return m_iRetVal == 65537; }

    void setError( int iRet )
    { m_iRetVal = iRet; }

    void addElem( Object pObj )
    { m_listResp.add( pObj ); }

    void clear()
    {
        m_listResp.clear();
        m_iRetVal = 0;
    }
    Object[] getParamArray()
    {
        if( m_listResp.size() == 0 )
            return new Object[ 0 ];
        return m_listResp.toArray();
    }
    vectorBufPtr getParams()
    {
        int ret = 0;
        vectorBufPtr vecBufs = new vectorBufPtr();
        for( Object i : m_listResp )
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

    int getParamCount()
    { return m_listResp.size(); }

    int getError()
    { return m_iRetVal; }

    Object getAt( int idx )
    {
        if( getParamCount() <= idx )
            return null;
        return m_listResp.get( idx );
    }
}
