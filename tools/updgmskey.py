import json
import os
import sys
import errno

def UpdInitCfg_GmSSL( cfgPath : str, strKeyPath : str )->int:
    try:
        fp = open( cfgPath, "r" )
        initCfg = json.load( fp )
        fp.close()

        if 'Security' in initCfg :
            oSecurity = initCfg['Security']
        else :
            return -errno.ENOENT

        if 'SSLCred' in oSecurity :
            oSSLCred = oSecurity['SSLCred']
        else :
            return -errno.ENOENT

        oSSLCred[ "KeyFile"] = strKeyPath + "/signkey.pem"
        oSSLCred[ "CertFile"] = strKeyPath + "/certs.pem"
        oSSLCred[ 'CACertFile' ] = strKeyPath + "/cacert.pem"
        oSSLCred[ 'SecretFile' ] = ""

        # output the console instead of overwrite the file
        strCfg = json.dumps( initCfg, indent=4)
        print( strCfg )
        return 0
    except Exception as err:
        print( err )
        return -errno.EFAULT


def UpdDrv_GmSSL( cfgPath : str, strKeyPath : str )->int :
    try:
        fp = open( cfgPath, "r" )
        drvVal = json.load( fp )
        fp.close()

        if 'Ports' in drvVal :
            ports = drvVal['Ports']
        else :
            return -errno.ENOENT

        for port in ports :
            if port[ 'PortClass'] != "RpcGmSSLFido" :
                continue
            sslFiles = port[ 'Parameters']
            if sslFiles is None :
                continue
            sslFiles[ "KeyFile"] = strKeyPath + "/clientkey.pem"
            sslFiles[ "CertFile"] = strKeyPath + "/clientcert.pem"
            sslFiles[ 'CACertFile' ] = strKeyPath + "/rootcacert.pem"
            sslFiles[ 'SecretFile' ] = ""
            break

        strCfg = json.dumps( drvVal, indent=4)
        print( strCfg )
        return 0
    except Exception as err:
        print( err )
        return -errno.EFAULT

def main()->int:
    cfgPath = sys.argv[1]
    strKeyPath = sys.argv[2]
    strBase = os.path.basename( cfgPath )

    if not os.path.isabs( strKeyPath ) :
        strKeyPath = os.path.abspath( strKeyPath )

    if strBase == 'driver.json':
        ret = UpdDrv_GmSSL( cfgPath, strKeyPath )
    else :
        ret = UpdInitCfg_GmSSL( cfgPath, strKeyPath )
    quit( -ret )

if __name__ == "__main__":
    main()
