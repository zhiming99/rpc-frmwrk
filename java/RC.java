package org.rpcf.rpcbase;
public class RC implements rpcbaseConstants{
    public final static boolean ERROR( int ret )
    { return ret < 0; }

    public final static boolean SUCCEEDED( int ret )
    { return ret == RC.STATUS_SUCCESS; }

    public final static boolean isPending( int ret )
    { return ret == STATUS_PENDING; }
}
