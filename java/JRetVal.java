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
        vectorBufPtr vecBufs =
            Helpers.convertObjectToBuf(
                m_listResp.toArray() );

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
