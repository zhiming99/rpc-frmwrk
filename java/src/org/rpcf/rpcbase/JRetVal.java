package org.rpcf.rpcbase;
import java.util.ArrayList;
import java.util.List;

public class JRetVal {

    protected  int m_iRetVal = 0;

    protected List<Object> m_listResp =
        new ArrayList< Object >();

    public boolean ERROR( )
    { return m_iRetVal < 0; }

    public boolean SUCCEEDED()
    { return m_iRetVal >= RC.STATUS_SUCCESS; }

    public boolean isPending()
    { return m_iRetVal == RC.STATUS_PENDING; }

    public void setError( int iRet )
    { m_iRetVal = iRet; }

    public void addElem( Object pObj )
    { m_listResp.add( pObj ); }

    public void AddElemInt( int val )
    { m_listResp.add( new Integer( val ) ); }

    public void AddElemByte( byte val )
    { m_listResp.add( new Byte( val ) ); }

    public void AddElemShort( short val )
    { m_listResp.add( new Short( val ) ); }

    public void AddElemLong( long val )
    { m_listResp.add( new Long( val ) ); }

    public void AddElemBool( boolean bval )
    { m_listResp.add( new Boolean( bval ) ); }

    public void AddElemFloat( float val )
    { m_listResp.add( new Float( val ) ); }

    public void AddElemDouble( double val )
    { m_listResp.add( new Double( val ) ); }

    public void clear()
    {
        m_listResp.clear();
        m_iRetVal = 0;
    }

    public Object[] getParamArray()
    {
        if( m_listResp.size() == 0 )
            return new Object[ 0 ];
        return m_listResp.toArray();
    }

    public vectorVars getParams()
    {
        vectorVars vecVars =
            Helpers.convertObjectToVars(
                m_listResp.toArray() );

        return vecVars;
    }

    public int getParamCount()
    { return m_listResp.size(); }

    public int getError()
    { return m_iRetVal; }

    public Object getAt( int idx )
    {
        if( idx < 0 || getParamCount() <= idx )
            throw new IndexOutOfBoundsException(
                "error JRetVal.getAt" );
        return m_listResp.get( idx );
    }

    public boolean getAtBool( int idx )
    { return ( Boolean )getAt( idx ); }

    public byte getAtByte( int idx )
    { return ( Byte )getAt( idx ); }

    public short getAtShort( int idx )
    { return ( Short )getAt( idx ); }

    public int getAtInt( int idx )
    { return ( Integer )getAt( idx ); }

    public long getAtLong( int idx )
    { return ( Long )getAt( idx ); }

    public float getAtFloat( int idx )
    { return ( Float )getAt( idx ); }

    public double getAtDouble( int idx )
    { return ( Double )getAt( idx ); }

    public long getTaskId()
    {
        if( !isPending() )
            throw new NullPointerException(
                "error no task id to get" );
        return getAtLong( 0 );
    }
}
