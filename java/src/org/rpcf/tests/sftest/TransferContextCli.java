package org.rpcf.tests.sftest;

import org.rpcf.rpcbase.IRpcService;

import java.util.concurrent.Semaphore;

public class TransferContextCli extends TransferContext{
    TransferContextCli(IRpcService oInst)
    {super(oInst);}

    Semaphore m_sem = new Semaphore(0);

    @Override
    synchronized void onTransferDone(long hChannel, int iRet) {
        TransFileContext o = m_mapChanToCtx.get(hChannel);
        if(o == null)
            return;
        if(!o.isDone()) {
            o.setError(iRet);
            o.cleanUp();
            notifyComplete();
        }
    }

    void waitForComplete(){
        while( true)
        try {
            m_sem.acquire();
            break;
        }catch(InterruptedException e)
        {
            continue;
        }
    }
    void notifyComplete()
    {m_sem.release();}
}
