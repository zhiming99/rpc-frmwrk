from rpcfrmwrk import *

import errno
import inspect
import types

class PyRpcContext :
    def CreateIoMgr( self, strModName ) :
        cpp.CoInitialize(0)
        p1=cpp.CParamList()
        ret = p1.SetStrProp( 0, strModName );
        if ret < 0 :
            return None
        p2 = cpp.CastToCfg( p1.GetCfgAsObj() );
        p3=cpp.CreateObject(
            cpp.Clsid_CIoManager, p2 )
        return p3

    def StartIoMgr( self, pIoMgr ) :
        p = cpp.CastToSvc( pIoMgr );
        if p is None :
            return -errno.EFAULT
        ret = p.Start();
        return ret

    def StopIoMgr( self, pIoMgr ) :
        p = cpp.CastToSvc( pIoMgr );
        if p is None :
            return -errno.EFAULT
        ret = p.Stop();
        return ret

    def CleanUp( self ) :
        cpp.CoUninitialize();

    def DestroyRpcCtx( self ) :
        print( "test.py: about to quit..." )
        self.CleanUp();

    def __init__( self ) :
        pass

    def __enter__( self ) :
        print( "entering..." );
        self.pIoMgr = self.CreateIoMgr( "PyRpcProxy" );
        if self.pIoMgr is not None :
            ret = self.StartIoMgr( self.pIoMgr );
            if ret > 0 :
                self.StopIoMgr( self.pIoMgr );
            else :
                LoadPyFactory( self.pIoMgr );

    def __exit__( self, type, val, traceback ) :
        print( "exiting..." );
        if self.pIoMgr is not None :
            self.StopIoMgr( self.pIoMgr );
        self.DestroyRpcCtx();

class PyRpcProxy :
    def GetError( self ) :
        if self.iError is None :
            return 0
        return self.iError

    def SetError( self, iErr ) :
        self.iError = iErr;

    def Common_Proxy( self, ifName, methodName, argsList ) :
        pass

    def __init__( self, pIoMgr, strDesc, strSvrObj ) :
        self.pIoMgr = pIoMgr;
        self.iError = 0
        oParams = cpp.CParamList();
        oObj = CreateProxy( pIoMgr,
            strDesc, strSvrObj,
            oParams.GetCfgAsObj() );
        if oObj is None or oObj.IsEmpty() :
            print( "failed to create proxy..." )
            self.SetError( -errno.EFAULT );
            return

        oInst = CastToProxy( oObj );
        if oInst is None :
            print( "failed to create proxy 2..." )
        self.oInst = oInst;
        self.oObj = oObj;
        return

    def Start( self ) :
        self.oInst.SetPyHost( self );
        ret = self.oInst.Start()
        if ret < 0 :
            print( "Failed start proxy..." )
        else :
            print( "Proxy started..." )

        oCheck = self.oInst.GetPyHost()
        print( oCheck, self )

    def Stop( self ) :
        self.oInst.RemovePyHost();
        self.oInst.Stop()

    def __enter__( self ) :
        self.Start()

    def __exit__( self, type, val, traceback ) :
        self.Stop()

    def sendRequest( *args ) :
        strIfName = args[ 1 ]
        strMethod = args[ 2 ]
        self = args[ 0 ]
        resp = [ 0, None ];

        listArgs = list( args[3:] )
        self.MakeCall( strIfName, strMethod,
            listArgs, resp )

        return resp;

    def MakeCall( self, strIfName, strMethod, args, resp ) :
        ret = self.oInst.PyProxyCallSync(
            strIfName, strMethod, args, resp )
        if ret < 0 :
            resp[ 0 ] = ret
        return resp

    def MakeCallAsync( self, callback, strIfName, strMethod, args, resp ) :
        ret = self.oInst.PyProxyCall(
            callback, strIfName, strMethod, args, resp )
        if ret < 0 :
            resp[ 0 ] = ret

        if ret == 65537 :
            resp[ 0 ] = ret

        return resp;

    def ArgObjToList( self, pObj ) :
        pCfg = cpp.CastToCfg( pObj )
        if pCfg is None :
            return [ -errno.EFAULT, ]
        oParams = cpp.CParamList( pCfg )
        ret = oParams.GetSize()
        if ret[ 0 ] < 0 :
            resp[ 0 ] = ret[ 0 ];
        iSize = ret[ 1 ]
        if iSize <= 0 :
            return [ -errno.EINVAL, ]
        argList = []
        for i in range( iSize ) :
            ret = oParams.GetPropertyType( i )
            if ret[ 0 ] < 0 :
                break;
            iType = ret[ 1 ];
            if iType == cpp.typeUInt32 :
                ret = oParams.GetIntProp( i )
                if ret[ 0 ] < 0 :
                    break;
                val = ret[ 1 ]
            elif iType == cpp.typeDouble :
                ret = oParams.GetDoubleProp( i )
                if ret[ 0 ] < 0 :
                    break;
                val = ret[ 1 ]

            elif iType == cpp.typeByte :
                ret = oParams.GetByteProp( i )
                if ret[ 0 ] < 0 :
                    break;
                val = ret[ 1 ]

            elif iType == cpp.typeUInt16 :
                ret = oParams.GetShortProp( i )
                if ret[ 0 ] < 0 :
                    break;
                val = ret[ 1 ]

            elif iType == cpp.typeUInt64 :
                ret = oParams.GetQwordProp( i )
                if ret[ 0 ] < 0 :
                    break;
                val = ret[ 1 ]

            elif iType == cpp.typeFloat :
                ret = oParams.GetFloatProp( i )
                if ret[ 0 ] < 0 :
                    break;
                val = ret[ 1 ]

            elif iType == cpp.typeString :
                ret = oParams.GetStrProp( i )
                if ret[ 0 ] < 0 :
                    break;
                val = ret[ 1 ]

            elif iType == cpp.typeObj :
                ret = oParams.GetObjPtr( i )
                if ret[ 0 ] < 0 :
                    break;
                val = ret[ 1 ]
                if val.IsEmpty() :
                    ret = [ -errno.EFAULT, ]
                    break
            elif iType == cpp.typeByteArr :
                ret = pCfg.GetProperty( i );
                if ret[ 0 ] < 0 :
                    break;
                val = ret[ 1 ]
                if val.IsEmpty() :
                    ret = [ -errno.EFAULT, ]
                    break
                pyBuf = bytearray( val.size() );
                ret = val.CopyToPython( pyBuf );
                if ret < 0 : 
                    ret = ( ret, )
                    break;

                val = pyBuf;

            argList.append( val )

        if ret[ 0 ] < 0 :
            return ret;

        ret = [ 0, argList ]
        return ret

    #for event handler 
    def InvokeMethod(
        self, callback, ifName, methodName, cppargs ) :
        resp = [ 0 ];
        while True :

            ret = self.ArgObjToList( cppargs )
            if ret[ 0 ] < 0 :
                resp[ 0 ] = ret;
                break;

            argList = ret[ 1 ]
            found = False
            for iftype in type(self).__bases__ :
                if not hasattr( iftype, "ifName" ) :
                    prevType = iftype
                    continue;
                if iftype.ifName != ifName :
                    prevType = iftype
                    continue;
                found = True
                typeFound = iftype;
                break;
                    
            if not found :
                resp[ 0 ] = -errno.EINVAL;
                break;

            nameComps = methodName.split( '_' )
            if nameComps[ 0 ] != "UserEvent" :
                resp[ 0 ] = -errno.EINVAL;
                break;

            found = False
            oMembers = inspect.getmembers( typeFound,
                predicate=inspect.isfunction);
            for oMethod in oMembers :
                if nameComps[ 1 ] != oMethod[ 0 ]: 
                    continue;
                targetMethod = oMethod[ 1 ];
                found = True;               
                break;

            if not found :
                resp[ 0 ] = -errno.EINVAL;
                break;

            if targetMethod is None :
                resp[ 0 ] = -error.EFAULT;
                break;

            resp = targetMethod( self, callback, *argList )

            break; #while True

        return resp
        
