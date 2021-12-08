package org.rpcf.tests.sftest;

import org.rpcf.rpcbase.RC;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;

public class TransFileContext {
    public int m_iError = 0;
    public int m_iBytesLeft = 0;
    public int m_iBytesSent = 0;
    public RandomAccessFile m_oFile = null;
    public FileChannel m_oChannel = null;
    public Character m_cDirection = 'u';
    public boolean m_bServer = false;
    public String m_strPath;
    public long m_lOffset = 0;
    public long m_lSize = 0;
    boolean m_bRead = false;

    TransFileContext(String strPath)
    {m_strPath = strPath;}

    public void setError(int iError)
    {m_iError=iError;}
    public int getError()
    {return m_iError;}

    public boolean isDone()
    {return m_oFile == null;}

    public static String[] splitPath(String strPath)
    {
        return strPath.split("(?<=/)(?=[^/]+$)");
    }

    public static String getFileName(String strPath)
    {
        String[] comps = splitPath(strPath);
        if(comps.length > 1)
            return comps[1];
        if(comps.length == 1 && !comps[0].equals("/"))
            return comps[0];
        return "";
    }

    public static String getDirName(String strPath)
    {
        String[] comps = splitPath(strPath);
        if(comps.length > 1)
            return comps[0];
        if(comps.length == 1 && comps[0].equals("/"))
            return comps[0];
        return ".";
    }

    public int openFile() {
        if (m_strPath == null || m_strPath.equals(""))
            return -RC.ERROR_STATE;
        int ret = 0;
        try {
            do {
                if ((m_cDirection == 'u' && !m_bServer) ||
                        (m_cDirection == 'd' && m_bServer)) {
                    m_bRead = true;
                    m_oFile = new RandomAccessFile(m_strPath, "r");
                    m_oChannel = m_oFile.getChannel();
                    long iSize = m_oChannel.size();
                    if (m_lOffset > iSize) {
                        ret = -RC.ERANGE;
                        break;
                    }
                    if(m_lOffset > 0)
                        m_oFile.seek(m_lOffset);
                    if (m_lSize > 0) {
                        if (m_lSize > iSize - m_lOffset)
                            m_lSize = iSize - m_lOffset;
                    } else {
                        m_lSize = iSize - m_lOffset;
                    }
                    if (m_lSize > 512 * 1024 * 1024) {
                        ret = -RC.ERANGE;
                        break;
                    }
                }
                else{
                    if(m_lSize==0)
                    {
                        // bytes to write
                        ret = -RC.ERANGE;
                        break;
                    }

                    m_oFile = new RandomAccessFile(m_strPath, "rw");
                    m_oChannel = m_oFile.getChannel();
                    long iSize = m_oChannel.size();
                    if (m_lOffset > iSize) {
                        ret = -RC.ERANGE;
                        break;
                    }
                    m_oFile.seek(m_lOffset);
                    m_oChannel.truncate(m_lOffset);
                }
                m_iBytesLeft = (int)m_lSize;


            } while (false);
        } catch (IOException e) {
            ret = -RC.ERROR_FAIL;
        }
        return ret;
    }
    public void cleanUp()
    {
        try {
            if (m_oChannel != null) {
                m_oChannel.close();
                m_oChannel = null;
            }
            if (m_oFile != null) {
                m_oFile.close();
                m_oFile = null;
            }
        }catch(IOException e){
        }
    }
}
