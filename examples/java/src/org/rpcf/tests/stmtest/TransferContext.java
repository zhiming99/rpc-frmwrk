package org.rpcf.tests.stmtest;

import java.util.concurrent.atomic.AtomicInteger;

public class TransferContext {
    public AtomicInteger m_iCounter = new AtomicInteger(0);
    public int m_iError = 0;

    public int incCounter()
    {return m_iCounter.incrementAndGet();}

    public int getCounter()
    {return m_iCounter.get();}

    public void setError( int iError )
    {m_iError = iError; }

    public int getError()
    {return m_iError;}
}
