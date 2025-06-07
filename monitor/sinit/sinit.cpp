/*
 * =====================================================================================
 *
 *       Filename:  sinit.cpp
 *
 *    Description:  this is a tool to store and manage the user credentials for
 *                  simple authentication mechanism.
 *
 *        Version:  1.0
 *        Created:  05/06/2025 03:17:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#include <iostream>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include <signal.h>
#include <algorithm>
#include <errno.h>
#include <termios.h>

#include "rpc.h"
#include "proxy.h"
using namespace rpcf;
#include "blkalloc.h"
#include "encdec.h"

#define SIMPAUTH_DIR "simpleauth"
std::atomic< bool > g_bExit = { false };

void SignalHandler( int signum )
{
    if( signum == SIGINT )
    {
        g_bExit = true;
        OutputMsg( 0,
            "signalhandler triggered." );
    }
    else if( signum == SIGUSR1 )
        OutputMsg( 0,
            "remote connection is down" );
}

void DisableEchoing( termios& oOldSettings )
{
    termios oNewSettings;
    tcgetattr(STDIN_FILENO, &oOldSettings);
    oNewSettings = oOldSettings;
    oNewSettings.c_lflag &= ~(ICANON | ECHO);
    tcsetattr( STDIN_FILENO,
        TCSANOW, &oNewSettings);
}

void EnableEchoing(
    const termios& oOldSettings )
{
    tcsetattr(STDIN_FILENO,
        TCSANOW, &oOldSettings );
}

bool g_bGmSSL = false;
const std::string g_strPubKey = "clientkey.pem";
const std::string g_strCliReg;

const std::string g_strCredDir =
    "/" SIMPAUTH_DIR "/credentials";

const std::string g_strDefCred =
    "/" SIMPAUTH_DIR "/credentials/.default";

RegFsPtr g_pfs;
ObjPtr g_pIoMgr;

RegFsPtr GetFs()
{ return g_pfs; }

std::string GetClientRegPath()
{
    stdstr strCliReg = GetHomeDir();
    strCliReg += "/.rpcf/clientreg.dat";
    return strCliReg;
}

gint32 InitContext()
{
    gint32 ret = CoInitialize( 0 );
    if( ERROR( ret ) )
        return ret;
    do{
        
        CParamList oParams;
        oParams.Push( "regfsmnt" );

        oParams[ propSearchLocal ] = false;
        oParams[ propConfigPath ] =
            "invalidpath/driver.json";

        // adjust the thread number if necessary
        oParams[ propMaxIrpThrd ] = 0;
        oParams[ propMaxTaskThrd ] = 2;

        ret = g_pIoMgr.NewObj(
            clsid( CIoManager ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        IService* pSvc = g_pIoMgr;
        ret = pSvc->Start();

    }while( 0 );

    if( ERROR( ret ) )
    {
        g_pIoMgr.Clear();
        CoUninitialize();
    }
    return ret;
}

gint32 DestroyContext()
{
    IService* pSvc = g_pIoMgr;
    if( pSvc != nullptr )
    {
        pSvc->Stop();
        g_pIoMgr.Clear();
    }

    CoUninitialize();
    DebugPrintEx( logErr, 0,
        "#Leaked objects is %d",
        CObjBase::GetActCount() );
    return STATUS_SUCCESS;
}

gint32 StartRegistry()
{
    gint32 ret = 0;
    bool bStop = false;
    do{ 
        CParamList oParams;
        oParams.SetStrProp( propConfigPath,
            GetClientRegPath() );
        ret = g_pfs.NewObj(
            clsid( CRegistryFs ),
            oParams.GetCfg() );
        if( ERROR( ret ) )
            break;

        ret = g_pfs->Start();
        if( ERROR( ret ) )
        {
            std::cout << "Error start the registry "
                << "and you may want to format the registry "
                << "with command 'regfsmnt -i "
                << GetClientRegPath() << "' first";
            break;
        }
    }while( 0 );
    return ret;
}

gint32 StopRegistry()
{
    if( g_pfs.IsEmpty() )
        return 0;
    gint32 ret = g_pfs->Stop(); 
    g_pfs.Clear();
    return ret;
}

bool IsDirExists(const std::string& strDir) {
    RegFsPtr pReg = GetFs();
    struct stat st;
    gint32 ret = pReg->GetAttr( strDir, st );
    if( ERROR( ret ) )
        return false;
    return S_ISDIR(st.st_mode);
}

gint32 MakeDirIfNotExist(
    const std::string& strDir )
{
    gint32 ret = 0;
    RegFsPtr pReg = GetFs();
    if( !IsDirExists( strDir ) )
        ret = pReg->MakeDir(
            strDir.c_str(), 0700 );
    return ret;
}

gint32 GetDefaultCredential( stdstr& strDefault )
{
    RegFsPtr pfs = GetFs();
    RFHANDLE hFile = INVALID_HANDLE;
    gint32 ret = 0;
    do{
        ret = pfs->OpenFile(
            g_strDefCred, O_RDONLY, hFile );
        if( ERROR( ret ) )
            break;
        struct stat st;
        ret = pfs->GetAttr( hFile, st );
        if( ERROR( ret ) )
            break;
        guint32 dwSize = st.st_size;
        BufPtr pBuf( true );
        pBuf->Resize( dwSize );
        ret = pfs->ReadFile(
            hFile, pBuf->ptr(), dwSize, 0 );
        if( ERROR( ret ) )
            break;
        strDefault.append(
            pBuf->ptr(), pBuf->size() );
    }while( 0 );
    if( hFile != INVALID_HANDLE )
        pfs->CloseFile( hFile );
    return ret;
}

gint32 OpenFileForWrite(
    const stdstr strFile,
    guint32 dwFlags,
    RFHANDLE& hFile )
{
    gint32 ret = 0;
    do{
        RegFsPtr pfs = GetFs();
        ret = pfs->OpenFile( strFile,
            O_TRUNC | O_WRONLY, hFile );
        if( ret == -ENOENT )
        {
            ret = pfs->CreateFile( strFile,
                0600, O_WRONLY, hFile );
        }
    }while( 0 );
    return ret;
}

gint32 SetDefaultCredential(
    const std::string& strUserName)
{

    gint32 ret = 0;
    RegFsPtr pfs = GetFs();
    RFHANDLE hFile = INVALID_HANDLE;
    do{
        ret = OpenFileForWrite( g_strDefCred,
            O_TRUNC | O_WRONLY, hFile );
        if( ERROR( ret ) )
            break;
        guint32 dwSize = strUserName.size();
        ret = pfs->WriteFile( hFile,
            strUserName.c_str(), dwSize, 0 );
        pfs->CloseFile( hFile );
    }while( 0 );
    return ret;
}

void RemoveDefaultCredential() {
    remove(g_strDefCred.c_str());
}

gint32 ListCredentialFiles(
     std::vector<std::string>& vecUserNames,
     std::set< std::string >* psetGmSSL = nullptr )
{
    gint32 ret = 0;
    RegFsPtr pfs = GetFs();
    RFHANDLE hDir = INVALID_HANDLE;
    do{
        ret = pfs->OpenDir(
            g_strCredDir, O_RDONLY, hDir );
        if( ERROR( ret ) )
            break;
        std::vector< KEYPTR_SLOT > vecDirEnt;
        ret = pfs->ReadDir( hDir, vecDirEnt );
        if( ERROR( ret ) )
            break;
        for( auto oEntry : vecDirEnt )
        {
            std::string strName = oEntry.szKey;
            if (strName.length() < 5 )
                continue;
            size_t iExtPos = strName.length() - 5;
            if( strName.substr(iExtPos) == ".cred")
            {
                vecUserNames.push_back(
                    strName.substr( 0, iExtPos ) );
                continue;
            }
            iExtPos--; 
            if( strName.substr(iExtPos) == ".gmssl" &&
                psetGmSSL != nullptr )
            {
                ( *psetGmSSL ).insert(
                    strName.substr( 0, iExtPos ) );
            }
        }
    }while( 0 );
    if( hDir != INVALID_HANDLE )
        pfs->CloseFile(hDir);
    if( SUCCEEDED( ret ) && vecUserNames.size() )
    {
        std::sort( vecUserNames.begin(),
            vecUserNames.end() );
    }
    return ret;
}

gint32 StoreCredential(
    const std::string& strUserName,
    const std::string& strPassword)
{
    gint32 ret = 0;
    BufPtr pBuf( true );
    do{
        stdstr strHash = GenPasswordSaltHash(
            strPassword, strUserName, g_bGmSSL );
        if( strHash.empty() )
        {
            std::cerr 
                << "Error generating hash.\n";
            break;
        }
        BufPtr pData( true );
        ret = pData->Append(
            strHash.c_str(), strHash.size() );
        if( ERROR( ret ) )
            break;

        ret = EncryptWithPubKey(
            pData, pBuf, g_bGmSSL );
        if( ERROR( ret ) )
        {
            std::cerr 
                << "Error encryption failed.\n";
            break;
        }

        ret = MakeDirIfNotExist(  g_strCredDir );
        if( ERROR( ret ) )
            break;

        RegFsPtr pfs = GetFs();
        RFHANDLE hFile = INVALID_HANDLE;
        stdstr strCred =  g_strCredDir +
            "/" + strUserName;

        ret = OpenFileForWrite( strCred + ".cred",
            O_TRUNC | O_WRONLY, hFile );
        if( ERROR( ret ) )
            break;
        guint32 dwSize = pBuf->size();
        ret = pfs->WriteFile( hFile,
            pBuf->ptr(), dwSize, 0 );
        pfs->CloseFile( hFile );
        if( ERROR( ret ) )
        {
            pfs->RemoveFile( strCred + ".cred" );
            std::cout << "Error failed to store "
                << "credential for user: "
                << strUserName << "\n";
            break;
        }
        std::cout << "Credential stored for user: "
            << strUserName << "\n";
        if( g_bGmSSL )
        {
            hFile = INVALID_HANDLE;
            // create a tag
            ret = OpenFileForWrite(
                strCred + ".gmssl",
                O_TRUNC | O_WRONLY, hFile );
            if( SUCCEEDED( ret ) )
                pfs->CloseFile( hFile );
        }
        else
        {
            pfs->RemoveFile( strCred + ".gmssl" );
        }
        // If this is the only credential, set as default
        std::vector<std::string> vecUsers;
        ret = ListCredentialFiles( vecUsers );
        if( ERROR( ret ) )
            break;

        if( vecUsers.size() == 1 )
            SetDefaultCredential(strUserName);
    }while( 0 );
    return ret;
}

void RemoveCredential(
    const std::string& strUserName )
{
    std::string strFilePath =  g_strCredDir +
        "/" + strUserName;

    RegFsPtr pfs = GetFs();
    std::string strDefault;
    GetDefaultCredential( strDefault );

    if( pfs->RemoveFile( strFilePath + ".cred" ) == 0 )
    {
        std::cout << "Credential removed for user: "
            << strUserName << "\n";

        // remove the gmssl tag if exists
        pfs->RemoveFile( strFilePath + ".gmssl" );
        // If removed credential is default, set new
        // default
        if( strUserName == strDefault )
        {
            std::vector<std::string> vecUsers;
            ListCredentialFiles( vecUsers );
            if( !vecUsers.empty() )
            {
                SetDefaultCredential( vecUsers[0] );
            }
            else
            {
                RemoveDefaultCredential();
            }
        }
    }
    else
    {
        std::cout << "No credential found for user: "
            << strUserName << "\n";
    }
}

void ListCredentials()
{

    gint32 ret = MakeDirIfNotExist( g_strCredDir );
    if( ERROR( ret ) )
        return;

    std::string strDefault;
    ret = GetDefaultCredential( strDefault );
    if( ERROR( ret ) )
    {
        std::cout << "Stored users: None\n";
        return;
    }

    std::set< std::string > setGtag;
    std::vector<std::string> vecUsers;
    ret = ListCredentialFiles(
        vecUsers, &setGtag );
    if( ERROR( ret ) )
    {
        std::cout << "Error failed to list credentials\n";
        return;
    }

    std::cout << "Stored users:\n";
    for (size_t i = 0; i < vecUsers.size(); ++i)
    {
        std::string star =
            (vecUsers[i] == strDefault) ? "*" : " ";
        if( setGtag.empty() )
        {
            std::cout << " " << star
                << "  " << vecUsers[i] << "\n";
            continue;
        }
        auto itr = setGtag.find( vecUsers[ i ] );
        if( itr == setGtag.end() )
            std::cout << " " << star
                << "  " << vecUsers[i] << "\n";
        else
            std::cout << " " << star
                << "g " << vecUsers[i] << "\n";
    }
}

void ChangeDefUser( const stdstr& strNewDef )
{
    std::vector<std::string> vecUsers;
    ListCredentialFiles( vecUsers );
    bool bFound = false;
    for( auto strUser : vecUsers )
    {
        int i = strcmp( strNewDef.c_str(),
            strUser.c_str() );
        if( i == 0 )
        {
            bFound = true;
            break;
        }
        if( i < 0 )
            break;
    }
    if( bFound )
    {
        SetDefaultCredential( strNewDef );
    }
    else
    {
        std::cerr << "Error the specified user "
            "was not found";
    }
}

void Usage(const char* szProg)
{
    std::cout << "Usage: " << szProg << " [-g] -s <userName> [-p <password>]\n"
              << "       " << szProg << " -r <userName>\n"
              << "       " << szProg << " -d <userName>\n"
              << "       " << szProg << " -l\n"
              << "Options:\n"
              << "  -d <userName>   Set <userName> as the default credential\n"
              << "  -s <userName>   Store credential for userName\n"
              << "  -p <password>   Password (if omitted, will prompt)\n"
              << "  -r <userName>   Remove credential of userName\n"
              << "  -l              List the users of each stored credentials."
                  "[*] is the default user, [g] indicates the credential is"
                  "GmSSL credential. \n"
              << "  -g              Use GmSSL to encrypt the key hash, should be used with '-s' option\n";
}

gint32 CheckRegistry()
{
    RegFsPtr pReg = GetFs();
    gint32 ret = 0;
    do{
        if( pReg.IsEmpty() )
        {
            ret = -EFAULT;
            break;
        }

        struct stat stbuf;
        ret = pReg->GetAttr(
            "/" SIMPAUTH_DIR, stbuf );
        if( ret == -ENOENT )
        {
            ret = pReg->MakeDir(
                "/" SIMPAUTH_DIR, 0700 );
            if( ERROR( ret ) )
                break;
            ret = pReg->GetAttr(
                "/" SIMPAUTH_DIR, stbuf );
            if( ERROR( ret ) )
                break;
        }
        else
        {
            break;
        }

        if( S_ISDIR(stbuf.st_mode) == 0 )
        {
            ret = -ENOTDIR;
            break;
        }
        if( ( stbuf.st_mode & 0500 ) != 0500 )
        {
            ret = -EACCES;
            break;
        }

    }while( 0 );
    return ret;
}

int main( int nArgc, char* pszArgv[] )
{
    int nOpt;
    bool bDoStore = false;
    bool bDoRemove = false;
    bool bDoList = false;
    bool bChgDef = false;
    gint32 ret = 0;
    std::string strUserName, strPassword, strNewDef;

    while( (nOpt = getopt(nArgc, pszArgv, "d:s:p:r:glh" ) ) != -1 )
    {
        switch (nOpt) {
            case 'd':
                bChgDef = true;
                strNewDef = optarg;
                break;
            case 's':
                bDoStore = true;
                strUserName = optarg;
                break;
            case 'p':
                strPassword = optarg;
                break;
            case 'r':
                bDoRemove = true;
                strUserName = optarg;
                break;
            case 'l':
                bDoList = true;
                break;
            case 'g':
                g_bGmSSL = true;
                break;
            case 'h':
            default:
                Usage(pszArgv[0]);
                return 1;
        }
    }

    auto iOldSig = 
        signal( SIGINT, SignalHandler );
    termios oOldSettings;
    DisableEchoing( oOldSettings );

    try{
        CProcessLock oLock( GetClientRegPath() );
        do{
            ret = InitContext();
            if( ERROR( ret ) )
                break;

            ret = StartRegistry();
            if( ERROR( ret ) )
                break;

            ret = CheckRegistry();
            if( ERROR( ret ) )
                break;

            if( bDoStore )
            {
                if( strUserName.empty() )
                {
                    std::cerr << "UserName required for store.\n";
                    Usage(pszArgv[0]);
                    ret = -EINVAL;
                    break;
                }
                gint32 iRetries = 0;
                while( strPassword.empty() && iRetries < 3 )
                {
                    stdstr strPassword1;
                    std::cout << "Enter password: ";
                    char ch = 0;
                    while( ( ch = getc( stdin ) ) != '\n' )
                    {
                        if( ch == EOF || g_bExit )
                        {
                            DebugPrint( ( int )g_bExit,
                                "set retry to 3" );
                            iRetries = 3;
                            break;
                        }

                        strPassword.append( 1, ch );
                    }
                    if( iRetries >= 3 || g_bExit )
                        break;

                    std::cout << "\nReenter password: ";
                    while( ( ch = getc( stdin ) ) != '\n' )
                    {
                        if( ch == EOF || g_bExit )
                        {
                            DebugPrint( ( int )g_bExit,
                                "set retry to 3" );
                            iRetries = 3;
                            break;
                        }
                        strPassword1.append( 1, ch );
                    }
                    if( iRetries >= 3 || g_bExit )
                        break;

                    std::cout << "\n";
                    if( strPassword1 != strPassword )
                    {
                        strPassword.clear();
                        std::cout << "Error the passwords do "
                            "not match, retry.\n";
                        iRetries++;
                    }
                }
                if( g_bExit )
                {
                    std::cout << "Canceled on user input\n";
                    break;
                }

                if( iRetries >= 3 || strPassword.empty() )
                {
                    std::cout << "Error failed to set password\n";
                    break;
                }
                StoreCredential(
                    strUserName, strPassword );
            }
            else if( bDoRemove )
            {
                if( strUserName.empty() )
                {
                    std::cerr <<
                        "UserName required for remove.\n";
                    Usage(pszArgv[0]);
                    ret = -EINVAL;
                    break;
                }
                RemoveCredential(strUserName);
            }
            else if( bDoList )
            {
                ListCredentials();
            }
            else if( bChgDef )
            {
                if( strNewDef.empty() )
                {
                    std::cerr <<
                        "New default UserName required "
                        "for change.\n";
                    Usage(pszArgv[0]);
                    ret = -EINVAL;
                    break;
                }
                ChangeDefUser( strNewDef );
            }
            else
            {
                Usage(pszArgv[0]);
                ret = -EINVAL;
                break;
            }
        }while( 0 );
        StopRegistry();
        DestroyContext();
    }
    catch( const std::runtime_error& e )
    {
        std::cerr << "Error accquiring the process lock, "
            << e.what() <<"\n";
        ret = -EFAULT;
    }
    catch( ... )
    {
        std::cerr << "Unknown exception occurred.\n";
        ret = -EFAULT;
    }
    EnableEchoing( oOldSettings );
    signal( SIGINT, iOldSig );
    return ret;
}

