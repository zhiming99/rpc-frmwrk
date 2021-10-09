package org.rpcf.rpcbase;
import java.lang.String;
import java.util.HashMap;

public class JavaRpcContext 
{
    protected Object m_pIoMgr;
    protected String m_strModName;

    Object CreateIoMgr( String strModName )
    {
        Object pIoMgr = null;
        do{
            int iRet = rpcbase.CoInitialize(0);
            
            CParamList oCfg=rpcbase.CParamList();
            ret = oCfg.PushStr( strModName );
            if( ret < 0 )
                break;
            CfgPtr pCfg =
                rpcbase.CastToCfg( oCfg.GetCfg() );

            pIoMgr=rpcbase.CreateObject(
                rpcbase.Clsid_CIoManager, pCfg );
            if( pIoMgr == null )
            {
                ret = -1;
                break;
            }

        }while( 0 );

        if( ERROR( ret ) ) 
            return null;

        return pIoMgr;
    }

    int StartIoMgr( Object pIoMgr ) 
    {
        IService pSvc =
            rpcbase.CastToSvc( pIoMgr );

        if( pSvc == null )
            return -1;

        ret = pSvc.Start();
        return ret;
    }

    int StopIoMgr( Object pIoMgr );
    {
        IService pSvc =
            rpcbase.CastToSvc( pIoMgr );

        if( pSvc == null )
            return -1;

        ret = pSvc.Stop();
        return ret;
    }

    int CleanUp()
    {
        return rpcbase.CoUninitialize();
    }

    int DestroyRpcCtx() :
    {
        System.out.println( "rpc context is about to quit..." );
        return CleanUp();
    }

    JavaRpcContext( String strModName = "JavaRpcProxy" )
    {
        m_strModName = strModName;
    }

    int Start()
    {
        int ret = 0;
        System.out.println( "entering..." );
        m_pIoMgr = CreateIoMgr( m_strModName );
        if( m_pIoMgr != null )
        {
            ret = StartIoMgr( m_pIoMgr );
            if( ret < 0 )
                StopIoMgr( m_pIoMgr );
            else
                ret = LoadJavaFactory( m_pIoMgr );
        }
        else
        {
            ret = -1;
        }
        return ret;
    }

    int Stop()
    {
        System.out.println( "exiting..." );
        if( m_pIoMgr != null )
        {
            StopIoMgr( m_pIoMgr );
            m_pIoMgr = null;
        }
        return DestroyRpcCtx();
    }
}

public class JavaRpcService
{
    protected int m_iError = 0;
    protected  m_iError = 0;
    int GetError()
    { return m_iError }

    int SetError( iErr ) :
    { m_iError = iErr; }

    def Start( self ) :
        m_oInst.SetJavaHost( self )
        ret = self.oInst.Start()
        isServer = self.IsServer()

        if ret < 0 :
            if isServer :
                print( "Failed start server..." )
            else :
                print( "Failed start proxy..." )
            return ret
        else :
            if isServer :
                print( "Server started..." )
            else :
                print( "Proxy started..." )

        oCheck = self.oInst.GetPyHost()
        return 0

    def Stop( self ) :
        self.oInst.RemovePyHost()
        return self.oInst.Stop()
    void TimerCallback(
        Object callback, Object context )
    {
        sig =signature( callback );
        iCount = len( sig.parameters );
        if iCount != 2 :
            print( "TimerCallback: params number",
                "does not match,", iCount );
            return;
        callback( self, context );
    }

    /*create a timer object, which is due in
    * timeoutSec seconds. And the `callback' will be
    * called with the parameter context.

    * it returns a two element list, the first
    * is the error code, and the second is a
    * timerObj if the error code is
    * STATUS_SUCCESS.
    */
    JRetVal AddTimer( int timeoutSec,
        Object callback,  Object context )
        return self.oInst.AddTimer(
            np.uint32( timeoutSec ),
            callback, context )

    /*Remove a previously scheduled timer.
    */
    def DisableTimer( self, timerObj ) :
        return self.oInst.DisableTimer(
            timerObj )

    /* attach a complete notification to the task
     * to complete. When the task is completed or
     * canceled, this notification will be triggered.
     * the notification has the following layout
     * callback( self, ret, *listArgs ), as is the
     * same as the response callback. The `ret' is
     * the error code of the task.
     */
    def InstallCompNotify(
        self, task, callback, *listArgs ):
        listResp = [ 0, [ *listArgs ] ]
        self.oInst.InstallCompNotify(
            task, callback, listResp )

    def HandleAsyncResp( self,
        callback, seriProto, listResp, context ) :
        listArgs = []
        sig =signature( callback )
        iArgNum = len( sig.parameters ) - 2
        for i in range( iArgNum ):
            listArgs.append( None )

        if listResp is None :
            print( "HandleAsyncResp: bad response" )
            listArgs.insert( 0, -cpp.EBADMSG )
            self.InvokeCallback( callback, listArgs )
            return

        if iArgNum < 0 :
            print( "HandleAsyncResp: bad callback")
            listArgs.insert( 0, -cpp.EBADMSG )
            self.InvokeCallback( callback, listArgs )
            return

        ret = listResp[ 0 ]
        if seriProto == cpp.seriNone :
            if len( listResp ) <= 1 :
                listArgs.insert( 0, ret )
                self.InvokeCallback( callback, listArgs )
            elif isinstance( listResp[ 1 ], list ) :
                listArgs = listResp[ 1 ]
                listArgs.insert( 0, ret )
                self.InvokeCallback( callback, listArgs )
            else :
                listArgs.insert( 0, ret )
                self.InvokeCallback( callback, listArgs )

        elif seriProto == cpp.seriPython :
            if len( listResp ) <= 1 :
                listArgs.insert( 0, ret )
                self.InvokeCallback( callback, listArgs )
            elif isinstance( listResp[ 1 ], bytearray ) :
                try:
                    trueResp = pickle.loads( listResp[ 1 ] )
                    if isinstance( trueResp, list ) :
                        trueResp.insert( 0, ret )
                        self.InvokeCallback( callback, trueResp )
                except PickleError:
                    listArgs.insert( 0, -cpp.EBADMSG )
                    self.InvokeCallback( callback, listArgs )
            else :
                listArgs.insert( 0, -cpp.EBADMSG )
                self.InvokeCallback( ret, listArgs )

        elif seriProto == cpp.seriRidl :
            listArgs.insert( 0, context )
            listArgs.insert( 1, ret )
            if len( listResp ) <= 1 :
                self.InvokeCallback( callback, listArgs )
            elif isinstance( listResp[ 1 ], list ) :
                if len( listResp[ 1 ] ) > 0 and iArgNum == 1 :
                    listArgs[ 2 ] = listResp[ 1 ][ 0 ]
                self.InvokeCallback( callback, listArgs )
            else :
                self.InvokeCallback( callback, listArgs )
        return

    def GetObjType( self, pObj ) :
        return GetObjType( pObj )

    def GetNumpyValue( typeid, val ) :
        return GetNpValue( typeid, val )

    /* establish a stream channel and
     * return a handle to the channel on success
     */
    def StartStream( self ) -> int :
        oDesc = cpp.CParamList()
        tupRet = self.oInst.StartStream(
            oDesc.GetCfgAsObj() )
        ret = tupRet[ 0 ]
        if ret < 0 :
            print( "Error start stream", ret )
            return 0
        return tupRet[ 1 ]
        

    def CloseStream( self, hChannel ) :
        return self.oInst.CloseStream( hChannel )

    def WriteStream( self, hChannel, pBuf ) :
        return self.oInst.WriteStream( hChannel, pBuf )

    def WriteStreamAsync( self,
        hChannel, pBuf, callback )->int :
        return self.oInst.WriteStreamAsync(
            hChannel, pBuf, callback )

    /* ReadStream to read `size' bytes from the
     * stream `hChannel'.  If `size' is zero, the
     * number of bytes read depends on the first
     * message received. If you are not sure what
     * will come from the stream, you are recommended
     * to use zero size.

     * The return value is a list of 3 elements,
     * element 0 is the error code. If it is
     * negative, no data is returned. If it is
     * STATUS_SUCCESS, element 1 is a bytearray
     * object read from the stream. element 2 is a
     * hidden object to keep element 1 valid, just
     * leave it alone.
    */
    def ReadStream( self, hChannel, size = 0
        )-> Tuple[ int, bytearray, cpp.BufPtr ] :
        return self.oInst.ReadStream( hChannel, size )

    /*
     * ReadStreamAsync to read `size' bytes from the
     * stream `hChannel', and give control to
     * callback after the data is read from the
     * stream. If `size' is zero, the number of bytes
     * read depends on the first message in the
     * pending message queue. If you are not sure
     * what will come from the stream, you are
     * recommended to use zero size.

     * The return value is a list of 3 elements,
     * element 0 is the error code. If it is
     * STATUS_PENDING, there is no data in the
     * pending message queue, and wait till the
     * callback is called in the future time.

     * If the return value is STATUS_SUCCESS, element
     * 1 holds a bytearray object of the bytes read
     * from the stream. And element 2 is a hidden
     * object, that you don't want to access.
     * Actually it controls the life time of element
     * 1, so make sure not to access elemen 1 if
     * element2 is no longer valid.
     */

    def ReadStreamAsync( self,
        hChannel, callback, size = 0)->Tuple[
            int, Optional[ bytearray ], Optional[ cpp.ObjPtr] ]:

        return self.oInst.ReadStreamAsync(
            hChannel, callback, size )

    /*
     * ReadStreamNoWait to read the first block of
     * bufferrd data from the stream. If there are
     * not data bufferred, error -EAGIN will be
     * returned, and will not blocked to wait.
     */
    def ReadStreamNoWait( self, hChannel
        )-> Tuple[ int, bytearray, cpp.BufPtr ] :
        return self.oInst.ReadStreamNoWait( hChannel )

    /* event called when the stream `hChannel' is
     * ready
     */
    int OnStmReady( gint64 hChannel )
    { return 0; }

    /* event called when the stream `hChannel' is
     * about to close
     */
    int OnStmClosing( gint64 hChannel ) 
    { return 0; }

    /* Convert the arguments in the `pObj' c++
     * object to a list of python object.
     *
     * The return value is a two element list. The
     * first element is the error code and the second
     * element is a list of python object. The second
     * element should be None if error occurs during
     * the conversion.
     */

    def ArgObjToList( self,
        seriProto:int,
        pObj:cpp.ObjPtr )->Tuple[ int, list ] :

        pCfg = cpp.CastToCfg( pObj )
        if pCfg is None :
            return [ -errno.EFAULT, ]
        oParams = cpp.CParamList( pCfg )
        ret = oParams.GetSize()
        if ret[ 0 ] < 0 :
            resp[ 0 ] = ret[ 0 ]
        iSize = ret[ 1 ]

        if iSize < 0 :
            return [ -errno.EINVAL, ]

        argList = []
        if seriProto == cpp.seriPython :
            ret = pCfg.GetProperty( 0 )
            if ret[ 0 ] < 0 :
                return [ 0, argList ]
            val = ret[ 1 ]
            if val.IsEmpty() :
                return [ -errno.EFAULT, ]

            pyBuf = bytearray( val.size() )
            iRet = val.CopyToPython( pyBuf )
            if iRet < 0 : 
                return [ iRet, ]

            try :
                argList = pickle.loads( pyBuf )
            except PickleError :
                return [ -errno.EBADMSG, ]

            if not isinstance( argList, list ):
                return [ -errno.EBADMSG, ]

            return [ 0, argList ]

        elif seriProto == cpp.seriRidl :
            ret = pCfg.GetProperty( 0 )
            if ret[ 0 ] < 0 :
                return [ 0, argList ]

            val = ret[ 1 ]
            if val.IsEmpty() :
                return [ 0, argList ]

            pyBuf = bytearray( val.size() )
            iRet = val.CopyToPython( pyBuf )
            if iRet < 0 : 
                return [ iRet, ]

            argList.append( pyBuf )
            return [ 0, argList ]

        elif seriProto != cpp.seriNone :
            return [ -errno.EBADMSG, ]
            
        for i in range( iSize ) :
            ret = oParams.GetPropertyType( i )
            if ret[ 0 ] < 0 :
                break
            iType = ret[ 1 ]
            if iType == cpp.typeUInt32 :
                ret = oParams.GetIntProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]
            elif iType == cpp.typeDouble :
                ret = oParams.GetDoubleProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeByte :
                ret = oParams.GetByteProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeUInt16 :
                ret = oParams.GetShortProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeUInt64 :
                ret = oParams.GetQwordProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeFloat :
                ret = oParams.GetFloatProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeString :
                ret = oParams.GetStrProp( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]

            elif iType == cpp.typeObj :
                ret = oParams.GetObjPtr( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]
                if val.IsEmpty() :
                    ret = [ -errno.EFAULT, ]
                    break
            elif iType == cpp.typeByteArr :
                ret = pCfg.GetProperty( i )
                if ret[ 0 ] < 0 :
                    break
                val = ret[ 1 ]
                if val.IsEmpty() :
                    ret = [ -errno.EFAULT, ]
                    break
                pyBuf = bytearray( val.size() )
                iRet = val.CopyToPython( pyBuf )
                if iRet < 0 : 
                    ret = [ iRet, ]
                    break

                val = pyBuf
            argList.append( val )

        if ret[ 0 ] < 0 :
            return ret

        return [ 0, argList ]

    def GetMethod( self, strIfName : str, strMethod : str ) -> object :
        while True :
            found = False
            typeFound = None
            bases = type( self ).__bases__ 
            for iftype in bases :
                if not hasattr( iftype, "_ifName_" ) :
                    continue
                if iftype._ifName_ != strIfName :
                    continue
                found = True
                typeFound = iftype
                break
            if not found :
                break

            found = False
            oMembers = inspect.getmembers(
                typeFound, inspect.isfunction)

            for oMethod in oMembers :
                if strMethod != oMethod[ 0 ]: 
                    continue
                targetMethod = oMethod[ 1 ]
                found = True
                break

            break

        if found :
            return targetMethod

        return None


    //for event handler 
    def InvokeMethod( self, callback,
        ifName, methodName,
        seriProto:int,
        cppargs:cpp.ObjPtr ) ->Tuple[ int, list ]:
        resp = [ 0 ]
        while True :

            ret = self.ArgObjToList( seriProto, cppargs )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret
                break

            argList = ret[ 1 ]
            found = False
            typeFound = None
            bases = type( self ).__bases__ 
            for iftype in bases :
                if not hasattr( iftype, "_ifName_" ) :
                    continue
                if iftype._ifName_ != ifName :
                    continue
                found = True
                typeFound = iftype
                break
            if not found :
                resp[ 0 ] = -errno.ENOTSUP
                break

            isServer = self.oInst.IsServer()
            nameComps = methodName.split( '_' )
            if not isServer :
                if nameComps[ 0 ] != "UserEvent" :
                    resp[ 0 ] = -errno.EINVAL
                    break
            else :
                if nameComps[ 0 ] != "UserMethod" :
                    resp[ 0 ] = -errno.EINVAL
                    break

            found = False
            oMembers = inspect.getmembers(
                typeFound, inspect.isfunction)

            if( seriProto != cpp.seriRidl ) :
                for oMethod in oMembers :
                    if nameComps[ 1 ] != oMethod[ 0 ]: 
                        continue
                    targetMethod = oMethod[ 1 ]
                    found = True
                    break
            else :
                wrapMethod = nameComps[ 1 ] + "Wrapper"
                for oMethod in oMembers :
                    if wrapMethod != oMethod[ 0 ]: 
                        continue
                    targetMethod = oMethod[ 1 ]
                    found = True
                    break

            if not found :
                resp[ 0 ] = -errno.EINVAL
                break

            if targetMethod is None :
                resp[ 0 ] = -errno.EFAULT
                break

            try:
                resp = targetMethod( self, callback, *argList )
            except:
                resp[ 0 ] = -errno.EFAULT
                
            if seriProto == cpp.seriNone :
                break

            if seriProto == cpp.seriRidl :
                break

            if seriProto != cpp.seriPython :
                resp[ 0 ] = -errno.EINVAL
                break

            ret = resp[ 0 ]
            if ret < 0 :
                break

            if len( resp ) <= 1 : 
                break

            if not isinstance( resp[ 1 ], list ) :
                resp[ 0 ] = -errno.EBADMSG
                break

            listResp = resp[ 1 ]
            pBuf = pickle.dumps( listResp )
            resp[ 1 ] = [ pBuf, ]
            break //while True

        return resp

    /* Invoke a callback function depending
     * it is bound function or not
     */
    def InvokeCallback( self, callback, listArgs):
        ret = None
        if inspect.isfunction( callback ) :
            ret = callback( self, *listArgs )
        elif inspect.ismethod( callback ) :
            ret = callback( *listArgs )
        return ret

    /* run the callback on a new context instead
     * of the current context
     * 
     * DeferCall, like a functor object bind to the
     * callback function with the arguments. And it
     * can be scheduled run on a new stack context.
     * The purpose is to reduce the depth of the call
     * stack and provide a locking free or light
     * locking context as the current one cannot.
     */
    def DeferCall( self, callback, *args ) :
        return self.oInst.PyDeferCall(
            callback, [ *args ] )

    def DeferCallback( self, callback, listArgs ) :
        self.InvokeCallback( callback, listArgs )
}
