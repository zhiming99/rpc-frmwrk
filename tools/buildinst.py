#!/usr/bin/env python3
import getopt
import os
import shutil
import tarfile
import time
import sys
from updwscfg import *
from updk5cfg import GenKrb5InstFilesFromInitCfg
import errno

def get_instcfg_content()->str :
    content='''#!/bin/bash
if [ -f debian ]; then
    md5sum *.deb
    dpkg -i --force-depends ./*.deb
    apt-get -y --fix-broken install
elif [ -f fedora ]; then
    md5sum *.rpm
    if command -v dnf; then 
        dnf -y install ./*.rpm
    elif command -v yum; then
        yum -y install ./*.rpm
    fi
fi
paths=$(echo $PATH | tr ':' ' ' ) 
for i in $paths; do
    a=$i/rpcf/rpcfgnui.py
    if [ -f $a ]; then
        rpcfgnui=$a
        break
    fi
done

if [ "x$rpcfgnui" == "x" ]; then
    $rpcfgnui="/usr/local/bin/rpcf/rpcfgnui.py"
    if [ ! -f $rpcfgnui ]; then
        exit 1
    fi       
fi
if [ -f USESSL ]; then
    if [ "x$1" == "x" ]; then
        echo Warning: no key index provided, using 0 as default.
        echo key index is the index of the key file to be installed, and start from 0
        keyidx=0
    else
        keyidx=$1
    fi
    if grep -q 'UsingGmSSL":."true"' ./initcfg.json; then
        keydir=$HOME/.rpcf/gmssl
    else     
        keydir=$HOME/.rpcf/openssl
    fi       
    if [ ! -d $keydir ]; then
        mkdir -p $keydir || exit 1
    fi   
    updinitcfg=`dirname $rpcfgnui`/updinitcfg.py
    if [ -f clidx ]; then
        clikeyidx=`cat clidx`
        clikeyidx=`expr $clikeyidx + $keyidx`
    elif [ -f svridx ]; then
        svrkeyidx=`cat svridx`
        svrkeyidx=`expr $svrkeyidx + $keyidx`
    else
        echo Error bad installer
    fi
    if [ ! -f $updinitcfg ]; then exit 1; fi
    if [ "x$clikeyidx" != "x" ]; then
        keyfile=clientkeys-$clikeyidx.tar.gz
        option="-c"
    elif [ "x$svrkeyidx" != "x" ]; then
        keyfile=serverkeys-$svrkeyidx.tar.gz
        option=
    fi       
    if [ ! -f $keyfile ]; then
        echo Error cannot find key file $keyfile
        echo Installation aborted
        exit 1
    else
        echo Installing $keyfile
        tar -C $keydir -xf $keyfile || exit 1
        python3 $updinitcfg $option $keydir ./initcfg.json
        chmod 400 $keydir/*.pem
        cat /dev/null > $keyfile
    fi
fi

echo setup rpc-frmwrk configurations...
initcfg=$(pwd)/initcfg.json
python3 $rpcfgnui $initcfg
'''
    return content

def get_instscript_content() :
    content = '''#!/bin/bash
unzipdir=$(mktemp -d /tmp/rpcfinst_XXXXX)
GZFILE=`awk '/^__GZFILE__/ {print NR + 1; exit 0; }' $0`
tail -n+$GZFILE $0 | tar -zxv -C $unzipdir > /dev/null 2>&1
if (($?==0)); then
    echo unzip success;
else
    echo unzip failed;
    exit 1;
fi
pushd $unzipdir > /dev/null;
bash ./instcfg.sh $1;
if (($?==0)); then
    echo install complete;
else
    echo install failed;
fi
popd > /dev/null
rm -rf $unzipdir
exit 0
__GZFILE__
'''
    return content

# create installer for keys generated outside rpcfg.py
def CreateInstaller( initCfg : object,
    cfgPath : str, destPath : str,
    bServer : bool, bInstKeys : bool,
    debPath : str = None ) -> int:
    ret = 0
    keyPkg = None
    destPkg = None
    try:
        if bServer :
            destPkg = destPath + "/instsvr.tar"
            installer = destPath + "/instsvr"
        else:
            destPkg = destPath + "/instcli.tar"
            installer = destPath + "/instcli"

        if bInstKeys :
            bGmSSL = False
            sslFiles = initCfg[ 'Security' ][ 'SSLCred' ]
            if "UsingGmSSL" in sslFiles:
                if sslFiles[ "UsingGmSSL" ] == "true": 
                    bGmSSL = True
            files = [ sslFiles[ 'KeyFile' ],
                sslFiles[ 'CertFile' ] ]

            if 'CACertFile' in sslFiles and sslFiles[ 'CACertFile' ] != "":
                files.append( sslFiles[ 'CACertFile' ] )

            if 'SecretFile' in sslFiles and sslFiles[ 'SecretFile' ] != "":
                oSecret = sslFiles[ 'SecretFile' ]
                if oSecret == "" or oSecret == "1234":
                   pass 
                elif os.access( oSecret, os.R_OK ):
                    files.append( oSecret )

            if bServer :
                keyPkg = destPath + "/serverkeys-0.tar.gz"
            else:
                keyPkg = destPath + "/clientkeys-1.tar.gz"

            dirPath = os.path.dirname( files[ 0 ] )
            fileName = os.path.basename( files[ 0 ] )
            cmdLine = 'tar cf ' + keyPkg + " -C " + dirPath + " " + fileName
            ret = rpcf_system( cmdLine )
            if ret != 0:
                raise Exception( "Error create tar file " + files[ 0 ] )
            files.pop(0)
            for i in files:
                dirPath = os.path.dirname( i )
                fileName = os.path.basename( i )
                cmdLine = 'tar rf ' + keyPkg + " -C " + dirPath + " " +  fileName
                ret = rpcf_system( cmdLine )
                if ret != 0:
                    raise Exception( "Error appending to tar file " + i )

            cmdLine = "touch " + destPath + "/USESSL"
            rpcf_system( cmdLine )

        fp = open( destPath + '/instcfg.sh', 'w' )
        fp.write( get_instcfg_content() )
        fp.close()

        cmdLine = "cd " + destPath + ";" 
        suffix = ".sh"

        if bInstKeys:
            if bServer :
                cmdLine += "echo 0 > svridx;"
                cmdLine += "echo 1 > endidx;"
                cmdLine += "tar cf " + destPkg
                cmdLine += " svridx endidx "
                suffix = "-0-1.sh"
            else:
                cmdLine += "echo 1 > clidx;"
                cmdLine += "echo 2 > endidx;"
                cmdLine += "tar cf " + destPkg
                cmdLine += " clidx endidx "
                suffix = "-1-1.sh"
            cmdLine += os.path.basename( keyPkg ) + " USESSL"
        else:
            cmdLine += "tar cf " + destPkg
        cmdLine += " instcfg.sh;"
        cmdLine += "rm instcfg.sh || true;"
        if bInstKeys:
            if bServer:
                cmdLine += "rm svridx || true;"
            else:
                cmdLine += "rm clidx || true;"
        cmdLine += "rm -rf " + destPkg + ".gz"
        if bInstKeys:
           cmdLine += " USESSL "+ keyPkg

        ret = rpcf_system( cmdLine )
        if ret != 0:
            raise Exception( "Error creating install package" )

        
        keyPkg = None
        # add instcfg to destPkg
        cfgDir = os.path.dirname( cfgPath )
        cmdLine = 'tar rf ' + destPkg + " -C " + cfgDir + " initcfg.json;"

        if len( debPath ) > 0:
            cmdLine += AddInstallPackages(
                debPath, destPkg, bServer )

        bAuthFile = False
        # add krb5 files to the package
        filesToAdd = [ "/krb5.conf" ]
        if bServer:
            filesToAdd.append( "/krb5.keytab" )
        else:
            filesToAdd.append( "/krb5cli.keytab" )

        if len( [ f for f in filesToAdd if os.path.exists( destPath + f ) ] ) == len( filesToAdd ):
            ret = True
        else:
            ret = False

        if ret :
            bAuthFile = True
            cmdLine += 'tar rf ' + destPkg + " -C " + destPath + \
                " krb5.conf "
            if bServer:
                cmdLine += 'krb5.keytab;'
            else:
                cmdLine += 'krb5cli.keytab;'

        cmdLine += "gzip " + destPkg
        ret = rpcf_system( cmdLine )
        if ret != 0:
            raise Exception( "Error creating install package" )
        destPkg += ".gz"

        # create the installer
        curDate = time.strftime('-%Y-%m-%d')
        if bInstKeys:
            if bGmSSL:
                curDate = "-g" + curDate
            else:
                curDate = "-o" + curDate
        installer += curDate + suffix
        fp = open( installer, "w" )
        fp.write( get_instscript_content() )
        fp.close()

        cmdLine = "cd " + destPath + ";" 
        cmdLine += "cat " + destPkg + " >> " + installer + ";"
        cmdLine += "chmod 700 " + installer + ";"
        cmdLine += "rm -rf " + destPkg
        if bInstKeys:
            cmdLine += " USESSL endidx "
            if bServer :
                cmdLine += "svridx"
            else:
                cmdLine += "clidx"
        if bAuthFile :
            cmdline += " krb5.conf krb5.keytab krb5cli.keytab"
        rpcf_system( cmdLine )

    except Exception as err:
        print( "CreateInstaller error:" + str( err ) )
        try:
            if keyPkg is not None:
                os.unlink( keyPkg )
            if destPkg is not None:
                os.unlink( destPkg )
        except:
            pass
        if ret == 0:
            ret = -errno.EFAULT

    return ret

def BuildInstallersInternal( initCfg : object,
    cfgPath : str, debPath :str,
    bSSL : bool, bServer : bool )->int :
    ret = 0
    try:
        strKeyPath = os.path.expanduser( "~" ) + "/.rpcf"
        bGmSSL = False
        try:
            sslFiles = initCfg[ 'Security' ][ 'SSLCred' ]
            if 'UsingGmSSL' in sslFiles: 
                if sslFiles[ 'UsingGmSSL' ] == 'true':
                    bGmSSL = True
        except Exception as err:
            bSSL = False

        if bGmSSL:
            strKeyPath += "/gmssl"
        else:
            strKeyPath += "/openssl"

        curDir = os.path.dirname( cfgPath )
        svrPkg = curDir + "/instsvr.tar"
        cliPkg = curDir + "/instcli.tar"

        bSSL2 = False
        if 'Connections' in initCfg:
            oConn = initCfg[ 'Connections']
            for oElem in oConn :
                if 'EnableSSL' in oElem :
                    if oElem[ 'EnableSSL' ] == 'true' :
                        bSSL2 = True
                        break
        if bSSL2:
            try:
                strKeyFile = sslFiles[ 'KeyFile' ]
                strCertFile = sslFiles[ 'CertFile' ]
                if( len( strKeyFile.strip() ) == 0 or
                    len( strCertFile.strip() ) == 0 or
                    not os.path.exists( strCertFile ) or
                    not os.path.exists( strKeyFile ) or
                    not os.access( strCertFile, os.R_OK ) or
                    not os.access( strKeyFile, os.R_OK ) ):
                    print( "Warning key file or cert file not accessible, trying to use self-signed keys" )
                    if bServer:
                        sslFiles[ 'KeyFile' ] = strKeyPath + "/signkey.pem"
                        sslFiles[ 'CertFile' ] = strKeyPath + "/signcert.pem"
                    else:
                        sslFiles[ 'KeyFile' ] = strKeyPath + "/clientkey.pem"
                        sslFiles[ 'CertFile' ] = strKeyPath + "/clientcert.pem"
                    sslFiles[ 'CaCertFile' ] = strKeyPath + "/certs.pem"
            except:
                bSSL2 = False

        if( IsRpcfSelfGenKey( bGmSSL, sslFiles['CertFile'] ) ):
            ret = CopyInstPkg( strKeyPath, curDir, bServer )
        else:
            ret = -1
        if ret < 0:
            ret = CreateInstaller(
                initCfg, cfgPath, curDir,
                bServer, bSSL and bSSL2,
                debPath )
            if ret < 0:
                raise Exception( "Error CreateInstaller in " + curDir + " with " + cfgPath )

            if bSSL and bSSL2 :
                return ret

            #non ssl situation, create both
            ret = CreateInstaller(
                initCfg, cfgPath, curDir,
                not bServer, False,
                debPath )
            return ret

        if os.access( cliPkg, os.W_OK ):
            bCliPkg = True
        else:
            bCliPkg = False

        if os.access( svrPkg, os.W_OK ):
            bSvrPkg = True
        else:
            bSvrPkg = False

        if not bSSL or not bSSL2:
            #removing the keys if not necessary
            if bCliPkg:
                cmdline = "keyfiles=`tar tf " + cliPkg + " | grep '.*keys.*tar' | tr '\n' ' '`;"
                cmdline += "for i in $keyfiles; do tar --delete -f " + cliPkg + " $i;done"
                rpcf_system( cmdline )

            if bSvrPkg and bServer:
                cmdline = "keyfiles=`tar tf " + svrPkg + " | grep '.*keys.*tar' | tr '\n' ' '`;"
                cmdline += "for i in $keyfiles; do tar --delete -f " + svrPkg + " $i;done"
                rpcf_system( cmdline )

        # generate the master install package
        instScript=get_instscript_content()

        objs = []
        svrObj = InstPkg()
        if bSvrPkg:
            svrObj.pkgName = svrPkg
            svrObj.startIdx = "svridx"
        svrObj.instName = "instsvr"
        svrObj.isServer = True
        if bServer:
            objs.append( svrObj )

        cliObj = InstPkg()
        if bCliPkg:
            cliObj.pkgName = cliPkg
            cliObj.startIdx = "clidx"
        cliObj.instName = "instcli"
        cliObj.isServer = False
        objs.append( cliObj )

        for obj in objs :
            if obj.pkgName == "" :
                ret = CreateInstaller(
                    initCfg, cfgPath,
                    curDir, bServer,
                    False, debPath )
                if ret < 0:
                    raise Exception( "Error CreateInstaller in " + \
                        curDir + " with " + cfgPath )
                continue

            #if not obj.isServer and not bServer:
            #    continue

            fp = open( curDir + '/instcfg.sh', 'w' )
            fp.write( get_instcfg_content() )
            fp.close()

            cmdline = "tar rf " + obj.pkgName + " -C " + curDir + " initcfg.json instcfg.sh;"
            if len( debPath ) > 0:
                cmdline += AddInstallPackages(
                    debPath, obj.pkgName, obj.isServer )
            bHasKey = False
            if bSSL and bSSL2:
                cmdline += "touch " + curDir + "/USESSL;"
                cmdline += "tar rf " + obj.pkgName + " -C " + curDir + " USESSL;"
                bHasKey = True

            # add krb5 files to the package
            filesToAdd = [ "/krb5.conf" ]
            if bServer:
                filesToAdd.append( "/krb5.keytab" )
            else:
                filesToAdd.append( "/krb5cli.keytab" )

            if len( [ f for f in filesToAdd if os.path.exists( curDir + f ) ] ) == len( filesToAdd ):
                ret = True
            else:
                ret = False

            if ret:
                cmdline += 'tar rf ' + obj.pkgName + " -C " + curDir + \
                    " krb5.conf "
                if obj.isServer:
                    cmdline += 'krb5.keytab;'
                else:
                    cmdline += 'krb5cli.keytab;'

            cmdline += "rm -f " + obj.pkgName + ".gz || true;"
            cmdline += "gzip " + obj.pkgName + ";"
            ret = rpcf_system( cmdline )
            if ret != 0 :
                if obj.isServer :
                    strMsg = "error create server installer"
                else:
                    strMsg = "error create client installer"
                raise Exception( strMsg )

            obj.pkgName += ".gz"
            installer = curDir + "/" + obj.instName + ".sh"
            fp = open( installer, "w" )
            fp.write( instScript )
            fp.close()
            cmdline = "cat " + obj.pkgName + " >> " + installer
            rpcf_system( cmdline )
            cmdline = "chmod 700 " + installer + ";"
            rpcf_system( cmdline )

            # generate a more intuitive name
            curDate = time.strftime('%Y-%m-%d')
            if bHasKey :
                if bGmSSL:
                    indicator= '-g-'
                else:
                    indicator= '-o-'
                newName = curDir + '/' + \
                    obj.instName + indicator + curDate
                try:
                    tf = tarfile.open( obj.pkgName, "r:gz")
                    ti = tf.getmember(obj.startIdx)
                    startFp = tf.extractfile( ti )
                    startIdx = int( startFp.read(4) )
                    startFp.close()
                    ti = tf.getmember( 'endidx' )
                    endFp = tf.extractfile( ti)
                    endIdx = int( endFp.read(4) )
                    count = endIdx - startIdx
                    endFp.close()
                    if count < 0 :
                        raise Exception( "bad index file" )
                    newName += "-" + str( startIdx ) + "-" + str( count ) + ".sh"
                except Exception as err:     
                    newName += ".sh"
                finally:
                    tf.close()
            else:
                newName = curDir + '/' + \
                    obj.instName + '-' + curDate + ".sh"

            rpcf_system( "mv " + installer + " " + newName )

        cmdline = "cd " + curDir + " && ( rm -f ./USESSL > /dev/null 2>&1;" + \
            "rm -f *inst*.gz krb5.conf krb5.keytab krb5cli.keytab instcfg.sh > /dev/null 2>&1);"
        #cmdline += "rm " + curDir + "/initcfg.json || true;"
        rpcf_system( cmdline )

    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT

    return ret

def GenAuthInstFilesFromInitCfg(
    strInitCfg : str,
    bServer : bool ) -> int:
    ret = GenKrb5InstFilesFromInitCfg(
        strInitCfg, bServer )
    return ret

def BuildInstallers( strInitCfg : str,
    destPath : str, debPath :str ) :

    bSSL = False
    bServer = True
    initCfg = json.load( open( strInitCfg, 'r' ) )
    if "IsServer" in initCfg:
        if initCfg[ "IsServer" ] == "false":
            bServer = False

    if "Security" in initCfg and "SSLCred" in initCfg[ "Security" ]:
        bSSL = True

    ret = GenAuthInstFilesFromInitCfg(
        strInitCfg, bServer )
    if ret < 0:
        return ret

    bRemove = False
    destPath = os.path.realpath( destPath ) + "/initcfg.json"
    if destPath != os.path.realpath( strInitCfg ):
        shutil.copy2( strInitCfg, destPath )
        strInitCfg = destPath
        bRemove = True

    ret = BuildInstallersInternal(
        initCfg, strInitCfg, debPath,
        bSSL, bServer )
    if bRemove:
        try:
            os.unlink( strInitCfg )
        except:
            pass
    return ret

def usage():
    print( "Usage: buildinst.py [-h] <initcfg.json> <dest path> [<deb/rpm path>]", file=sys.stderr )
    print( "\t-h: to outupt this help.", file=sys.stderr )
    print( "\t\tOtherwise it generates both installers for both server and client.", file=sys.stderr )

if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hc" )
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(-errno.EINVAL)

    for o, a in opts:
        if o == "-h" :
            usage()
            sys.exit( 0 )
        else:
            assert False, "unhandled option"

    if len( args ) < 2 :
        usage()
        sys.exit( -errno.EINVAL )
    strInitCfg = os.path.realpath( args[0] )
    destPath = os.path.realpath( args[ 1 ] )
    debPath = ""
    if len( args ) >= 3:
        debPath = os.path.realpath( args[ 2 ] )

    ret = BuildInstallers(
        strInitCfg, destPath, debPath )
    sys.exit( ret )
