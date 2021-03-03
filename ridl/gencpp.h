/*
 * =====================================================================================
 *
 *       Filename:  gencpp.h
 *
 *    Description:  Declarations of proxy/server generator for C++
 *
 *        Version:  1.0
 *        Created:  02/25/2021 01:34:32 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once
#include <memory>
#include <iostream>
#include <fstream>
#include "rpc.h"

guint32 GenClsid(
    const std::string& strName );

gint32 GenCppProj(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot );

typedef std::unique_ptr< std::ofstream > STMPTR;
struct CFileSet
{
    std::string  m_strPath;
    std::string  m_strAppHeader;
    std::string  m_strAppCpp;
    std::string  m_strObjDesc;
    std::string  m_strDriver;
    std::string  m_strMakefile;
    std::vector< std::pair<
        std::string, std::string > > m_vecIfImp;
    std::vector< STMPTR > m_vecFiles;

    CFileSet( const std::string& strOutPath,
        const std::string& strAppName );
    gint32 OpenFiles();
    ~CFileSet();
};


#define COUT ( *m_curFp )
struct CWriterBase
{
    guint32 m_dwIndWid = 4;
    guint32 m_dwIndent = 0;

    std::unique_ptr< CFileSet > m_pFiles;

    std::ofstream* m_curFp = nullptr;

    CWriterBase(
        const std::string& strPath,
        const std::string& strAppName,
        guint32 dwIndent = 0 )
    {
        std::unique_ptr< CFileSet > ptr(
            new CFileSet( strPath, strAppName ) );
        m_pFiles = std::move( ptr );
        m_dwIndent = 0;
    }

    void WriteLine1( std::string strText )
    {
        NewLine();
        AppendText( strText );
    }

    void WriteLine0( std::string strText )
    {
        AppendText( strText );
        NewLine();
    }

    void IndentUp()
    { m_dwIndent += m_dwIndWid; }

    inline void IndentDown()
    {
        if( m_dwIndent > m_dwIndWid )
            m_dwIndent -= m_dwIndWid;
        else
            m_dwIndent = 0;
    }

    inline void AppendText(
        const std::string& strText )
    { COUT << strText; };

    inline void NewLine( gint32 iCount = 1 )
    {
        std::string strChars( iCount, '\n' );
        COUT << strChars;
        strChars.clear();
        strChars.append( m_dwIndent, ' ' );
        COUT << strChars;
    }

    inline void BlockOpen()
    {
        AppendText( "{" );
        IndentUp();
        NewLine();
    }
    inline void BlockClose()
    {
        IndentDown();
        NewLine();
        AppendText( "};" );
    }

    guint32 SelectFile( guint32 idx )
    {
        if( idx > m_pFiles->m_vecFiles.size() )
            return -ERANGE;
        if( idx < 0 )
            return -ERANGE;
        m_curFp = m_pFiles->
            m_vecFiles[ idx ].get();
        return STATUS_SUCCESS;
    }

    void reset()
    {
        m_dwIndent = 0;
    }
};

class CCppWriter : public CWriterBase
{
    public:
    ObjPtr m_pNode;
    typedef CWriterBase super;
    CCppWriter(
        const std::string& strPath,
        const std::string& strAppName,
        ObjPtr pStmts ) :
        super( strPath, strAppName )
    { m_pNode = pStmts; }

    inline gint32 SelectHeaderFile()
    { return SelectFile( 0 ); }

    inline gint32 SelectCppFile()
    { return SelectFile( 1 ); }

    inline gint32 SelectDescFile()
    { return SelectFile( 2 ); }

    inline gint32 SelectDrvFile()
    { return SelectFile( 3 ); }

    inline gint32 SelectMakefile()
    { return SelectFile( 4 ); }

};

#define INDENT_UP   m_pWriter->IndentUp()
#define INDENT_DOWN m_pWriter->IndentDown()

#define Wa( text )  m_pWriter->WriteLine0( text )
#define Wb( text )  m_pWriter->WriteLine1( text )

#define AP( text ) \
    m_pWriter->AppendText( text)

#define NEW_LINE m_pWriter->NewLine()

#define BLOCK_OPEN  m_pWriter->BlockOpen()
#define BLOCK_CLOSE m_pWriter->BlockClose()

#define CCOUT ( ( std::ofstream& ) ( *m_pWriter->m_curFp ) )

class CHeaderPrologue
{
    CCppWriter* m_pWriter = nullptr;
    CStatements* m_pStmts = nullptr;

    public:
    CHeaderPrologue(
        CCppWriter* pWriter,
        ObjPtr& pNode )
    {
        m_pWriter = pWriter;
        m_pStmts = pNode;
    }
    gint32 Output()
    {
        Wa( "#pragma once" );
        Wa( "#include <string>" );
        Wa( "#include \"rpc.h\"" );
        Wa( "#include \"ifhelper.h\"" );
        Wa( "#include \"seribase.h\"" );
        if( m_pStmts->IsStreamNeeded() )
        {
            Wa( "#include \"streamex.h\"" );
        }
        return STATUS_SUCCESS;
    }
};

class CDeclareClassIds
{
    CCppWriter* m_pWriter = nullptr;
    CStatements* m_pStmts = nullptr;

    public:
    CDeclareClassIds(
        CCppWriter* pWriter,
        ObjPtr& pNode )
    {
        m_pWriter = pWriter;
        m_pStmts = pNode;
        if( m_pStmts == nullptr )
        {
            std::string strMsg =
                DebugMsg( -EFAULT,
                "error empty 'statements' node" );
            throw std::runtime_error( strMsg );
        }
    }

    gint32 Output()
    {
        bool bFirst = true;
        gint32 ret = STATUS_SUCCESS;
        Wa( "enum EnumMyClsid" );
        BLOCK_OPEN;
        std::vector< ObjPtr > vecSvcs;
        ret = m_pStmts->GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
        {
            printf(
            "error no service declared\n" );
            return ret;
        }
        std::vector< std::string > vecNames;
        for( auto& elem : vecSvcs )
        {
            CServiceDecl* pSvc = elem;
            vecNames.push_back(
                pSvc->GetName() );
        }
            
        if( vecNames.size() > 1 )
        {
            std::sort( vecNames.begin(),
                vecNames.end() );
        }

        std::string strAppName =
            m_pStmts->GetName();

        std::string strVal = strAppName;
        for( auto& elem : vecNames )
            strVal += elem;

        guint32 dwClsid = GenClsid( strVal );
        for( auto& elem : vecSvcs )
        {
            CServiceDecl* pSvc = elem;
            if( pSvc == nullptr )
                continue;

            std::string strSvcName =
                pSvc->GetName();
            
            if( bFirst )
            {
                CCOUT << "DECL_CLSID( "
                    << strSvcName
                    << " ) = "
                    << dwClsid
                    << ";";
                bFirst = false;
                continue;
            }
            CCOUT << "DECL_CLSID( "
                << strSvcName
                << " );";
        }
        std::vector< ObjPtr > vecIfs;
        m_pStmts->GetIfDecls( vecIfs );
        for( auto& elem : vecIfs )
        {
            CInterfaceDecl* pifd = elem;
            if( pifd == nullptr )
                continue;

            std::string strIfName =
                pifd->GetName();
            
            CCOUT << "DECL_IID( "
                << strIfName
                << " );";
        }
        BLOCK_CLOSE;

        return ret;
    }
};

class CDeclareTypedef
{
    CCppWriter* m_pWriter = nullptr;
    CTypedefDecl* m_pNode = nullptr;

    public:
    CDeclareTypedef(
        CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CDeclareStruct
{
    CCppWriter* m_pWriter = nullptr;
    CStructDecl* m_pNode = nullptr;

    public:
    CDeclareStruct( CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CDeclInterfProxy
{
    CCppWriter* m_pWriter = nullptr;
    CInterfaceDecl* m_pNode = nullptr;

    public:
    CDeclInterfProxy( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
    gint32 OutputEvent();
    gint32 OutputSync();
    gint32 OutputAsync();
};


class CDeclInterfSvr
{
    /*
    DeclInitUserFuncs
        AddSyncImpHandler
            AddSyncRidlHandler
            AddSyncNoneHandler
        AddAsyncImpHandler
            AddAsyncRidlHandler
            AddAsyncNoneHandler
    */
};

/*
class CDeclareServices
{
    gint32 AggregateIfs;
    ImplGetIidOfType;
};
class CImplSerialStruct
{
};

class CImplIfMethodPS
{
    ConvertHandleToHash
    ImplArgSerial
    SendReq
    ImplRespDeserial
    ConvertHashToHandle
};

class CImplIfMethodPA
{
    ConvertHandleToHash
    ImplArgSerial
    SendReq

    ImplCallback
        ImplRespDeserial
        ConvertHashToHandle
        resp
};

class CImplIfEventP
{
    ImplArgDeserial
    ConvertHashToHandle
    UserLogic
};

class CImplIfMethodSS
{
    ImplArgDeserial
    ConvertHashToHandle
    UserLogic
    ConvertHandleToHash
    ImplRespSerial
};

class CImplIfMethodSA
{
    ConvertHandleToHash
    ImplArgDeserial

    UserLogic

    ImplCallback
        ConvertHashToHandle
        ImplRespSerial
};

class CImplIfEventS
{
    ImplArgSerial
    ConvertHandleToHash
    BroadcastEvent
};
*/

class CExportObjDesc
{
};

class CExportDrivers
{
};

class CExportMakefile
{
};


