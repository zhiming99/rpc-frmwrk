package org.rpcf.rpcbase;
public class JavaReqContext 
{
    protected Object m_oHost = null;
    protected ObjPtr m_pCallback = null;
    protected boolean m_bSet = false;
    protected Object m_oUserData = null;

    public JavaReqContext( Object oHost,
        ObjPtr pCallback )
    {
        m_pCallback = pCallback;
        m_oHost = oHost;
    }

    public void setUserData( Object oData )
    { m_oUserData = oData; }

    public Object getUserData()
    { return m_oUserData; }

    public ObjPtr getCallback()
    { return m_pCallback; };

    public void setCallback(
        ObjPtr pCallback )
    { m_pCallback = pCallback; };

    public void setResponse(
        int iRet, Object ...args )
    { return; }

    public boolean hasResponse()
    { return m_bSet; }

    public int onServiceComplete(
        int iRet, Object ...args )
    { return 0; }
        
    public Object[] getResponse()
    { return new Object[ 0 ]; }
}
