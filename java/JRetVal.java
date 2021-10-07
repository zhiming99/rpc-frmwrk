package org.rpcf.rpcbase;
import java.util.ArrayList;

public class JRetVal {
    protected  int iRetVal = 0;
    protected ArrayList< Object > lstResp;

    boolean ERROR( )
    { return iRetVal < 0; }

    boolean SUCCEEDED()
    { return iRetVal >= 0; }

    boolean isPending()
    { return iRetVal == 65537; }

    void setErrorJRet( int iRet )
    { iRetVal = iRet; }

    void addElemToJRet( Object pObj )
    { lstResp.add( pObj ); }

    Object[] getParams()
    { return lstResp.toArray(); }
}
