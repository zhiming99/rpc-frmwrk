/*
 * =====================================================================================
 *
 *       Filename:  JavaSerialImpl.java
 *
 *    Description:  serialization utilities for Java
 *
 *        Version:  1.0
 *        Created:  10/24/2021 17:37:00 PM
 *       Revision:  none
 *       Compiler:  javac
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
package XXXXX;
import java.nio.ByteBuffer;
import org.rpcf.rpcbase.*;

public class JavaSerialImpl extends JavaSerialBase
{
    protected InstType m_oInst;
    public JavaSerialImpl( InstType pIf )
    { m_oInst = pIf; }

    public Object getInst()
    { return m_oInst; }

    public int serialHStream(
        BufPtr buf, int[] offset, long val )
    {
        if( m_oInst == null )
            return -RC.EFAULT;

        JRetVal jret =
            ( JRetVal )m_oInst.GetIdHash( val );
        if( jret.ERROR() ) 
            return jret.getError();

        serialInt64( buf, offset,
            ( Long )jret.getAt( 0 ) );

        return 0;
    }

    public long deserialHStream( ByteBuffer buf )
    {
        if( m_oInst == null )
            throw new NullPointerException(
                "c++ instance is empty");

        long qwHash = deserialInt64( buf );
        if( qwHash == RC.INVALID_HANDLE )
            throw new NullPointerException(
                "channel hash is empty");

        long ret = m_oInst.GetChanByIdHash(
            qwHash );

        if( ret == RC.INVALID_HANDLE )
            throw new NullPointerException(
                "channel handle is invalid");

        return ret;
    }
}
