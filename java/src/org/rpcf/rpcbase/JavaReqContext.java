package org.rpcf.rpcbase;
public class JavaReqContext 
{
    protected Object m_oHost;
    protected ObjPtr m_pCallback;
    protected boolean m_bSet = false;

    public JavaReqContext( Object oHost,
        ObjPtr pCallback )
    {
        m_pCallback = pCallback;
        m_oHost = oHost;
    }

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
    { return null; }
}
