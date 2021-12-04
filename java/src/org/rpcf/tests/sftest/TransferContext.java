package org.rpcf.tests.sftest;

import java.io.File;

public class TransferContext {
    public int m_iError = 0;
    public int m_iBytesLeft = 0;
    public int m_iBytesSent = 0;
    public File m_oFile;
    public Character m_cDirection = 'u';
    public String m_strPath;
    public long m_lOffset = 0;
    public long m_lSize = 0;

    public void setError(int iError)
    {m_iError=iError;}
    public int getError()
    {return m_iError;}
}
