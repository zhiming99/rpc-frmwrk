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

    std::map< std::string, gint32 > m_mapSvcImp;

    std::vector< STMPTR > m_vecFiles;

    CFileSet( const std::string& strOutPath,
        const std::string& strAppName );
    gint32 OpenFiles();
    gint32 AddSvcImpl( const std::string& strSvcName );
    ~CFileSet();
};


#define COUT ( *m_curFp )
struct CWriterBase
{
    guint32 m_dwIndWid = 4;
    guint32 m_dwIndent = 0;

    std::unique_ptr< CFileSet > m_pFiles;

    std::ofstream* m_curFp = nullptr;
    std::string m_strCurFile;

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

    inline std::string NewLineStr(
        gint32 iCount = 1 ) const
    { 
        std::string strChars( iCount, '\n' );
        strChars.append( m_dwIndent, ' ' );
        return strChars;
    }

    inline void NewLine( gint32 iCount = 1 )
    { COUT << NewLineStr( iCount ); }

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
        AppendText( "}" );
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

    gint32 AddSvcImpl(
        const std::string& strSvcName )
    {
        return m_pFiles->AddSvcImpl( strSvcName );
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

    inline gint32 SelectImplFile(
        const std::string& strFile )
    {
        decltype( m_pFiles->m_mapSvcImp )::iterator itr =
        m_pFiles->m_mapSvcImp.find( strFile );
        if( itr == m_pFiles->m_mapSvcImp.end() )
            return -ENOENT;
        gint32 idx = itr->second;
        return SelectFile( idx );
    }
};

#define INDENT_UP   m_pWriter->IndentUp()
#define INDENT_DOWN m_pWriter->IndentDown()


#define Wa( text )  m_pWriter->WriteLine0( text )
#define Wb( text )  m_pWriter->WriteLine1( text )

#define AP( text ) \
    m_pWriter->AppendText( text)

#define NEW_LINE m_pWriter->NewLine()
#define NEW_LINES( i ) m_pWriter->NewLine( i )

#define INDENT_UPL   { m_pWriter->IndentUp(); NEW_LINE; }
#define INDENT_DOWNL { m_pWriter->IndentDown(); NEW_LINE; }

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
    gint32 Output();
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

struct CArgListUtils
{
    gint32 GetArgCount( ObjPtr& pArgs ) const;

    gint32 ToStringInArgs(
        ObjPtr& pArgs,
        std::vector< std::string >& vecArgs ) const;

    gint32 ToStringOutArgs(
        ObjPtr& pArgs,
        std::vector< std::string >& vecArgs ) const;

    gint32 GenLocals( ObjPtr& pArgList,
        std::vector< std::string >& vecLocals ) const;

    gint32 GetHstream( ObjPtr& pArgList,
        std::vector< ObjPtr >& vecHstms ) const;

    gint32 GetArgsForCall( ObjPtr& pArgList,
        std::vector< std::string >& vecArgs ) const;

    gint32 GetArgTypes( ObjPtr& pArgList,
        std::vector< std::string >& vecTypes ) const;

    gint32 GetArgTypes( ObjPtr& pArgList,
        std::set< ObjPtr >& vecTypes ) const;
};

struct CMethodWriter 
    : public CArgListUtils
{
    CCppWriter* m_pWriter = nullptr;

    typedef CArgListUtils super;
    CMethodWriter( CCppWriter* pWriter );

    gint32 GenActParams(
        ObjPtr& pArgList );

    gint32 GenActParams(
        ObjPtr& pArgList,
        ObjPtr& pArgList2 );

    gint32 GenSerialArgs(
        ObjPtr& pArgList,
        const std::string& strBuf,
        bool bDeclare, bool bAssign );

    gint32 GenDeserialArgs(
        ObjPtr& pArgList,
        const std::string& strBuf,
        bool bDeclare, bool bAssign );

    gint32 DeclLocals( ObjPtr& pArgList );

    gint32 GenFormArgs(
        ObjPtr& pArg, bool bIn );

    gint32 GenFormInArgs( ObjPtr& pArg )
    { return GenFormArgs( pArg, true ); }

    gint32 GenFormOutArgs( ObjPtr& pArg )
    { return GenFormArgs( pArg, false ); }
};

class CDeclInterfProxy 
    : public CMethodWriter
{
    CInterfaceDecl* m_pNode = nullptr;

    public:
    typedef CMethodWriter super;

    CDeclInterfProxy( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
    gint32 OutputEvent( CMethodDecl* pmd );
    gint32 OutputSync( CMethodDecl* pmd );
    gint32 OutputAsync( CMethodDecl* pmd );
};


class CDeclInterfSvr
    : public CMethodWriter
{
    CInterfaceDecl* m_pNode = nullptr;

    public:

    typedef CMethodWriter super;
    CDeclInterfSvr( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
    gint32 OutputEvent( CMethodDecl* pmd );
    gint32 OutputSync( CMethodDecl* pmd );
    gint32 OutputAsync( CMethodDecl* pmd );
};

class CSetStructRefs :
    public CArgListUtils
{
    CServiceDecl* m_pNode = nullptr;
    public:

    CSetStructRefs( ObjPtr& pNode )
    { m_pNode = pNode; }

    gint32 SetStructRefs();

    gint32 ExtractStructArr( ObjPtr& pArray,
        std::set< ObjPtr >& setStructs );

    gint32 ExtractStructMap( ObjPtr& pMap,
        std::set< ObjPtr >& setStructs );
};

class CDeclService
{
    CCppWriter* m_pWriter = nullptr;
    CServiceDecl* m_pNode = nullptr;

    public:
    CDeclService( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
};

class CDeclServiceImpl :
    public CMethodWriter
{
    CServiceDecl* m_pNode = nullptr;

    public:
    typedef CMethodWriter super;
    CDeclServiceImpl( CCppWriter* pWriter,
        ObjPtr& pNode );

    using ABSTE = std::pair< std::string, ObjPtr >;

    gint32 FindAbstMethod(
        std::vector< ABSTE >& vecMethods,
        bool bProxy );

    gint32 DeclAbstMethod(
        ABSTE oMethod, bool bProxy );

    gint32 Output();
};

class CImplSerialStruct
{
    CCppWriter* m_pWriter = nullptr;
    CStructDecl* m_pNode = nullptr;

    public:
    CImplSerialStruct( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 OutputSerial();
    gint32 OutputDeserial();
    gint32 Output();
};

class CImplIufProxy
    : public CArgListUtils
{
    CCppWriter* m_pWriter = nullptr;
    CInterfaceDecl* m_pNode = nullptr;

    public:
    CImplIufProxy( CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CImplIufSvr :
    public CArgListUtils
{
    CCppWriter* m_pWriter = nullptr;
    CInterfaceDecl* m_pNode = nullptr;

    public:
    CImplIufSvr( CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

class CEmitSerialCode
    : public CArgListUtils
{
    public:
    CCppWriter* m_pWriter = nullptr;
    CAstListNode* m_pArgs;

    CEmitSerialCode( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 OutputSerial(
        const std::string& strObj,
        const std::string strBuf = "pBuf" );

    gint32 OutputDeserial(
        const std::string& strObj,
        const std::string strBuf = "pBuf" );
};

class CImplIfMethodProxy
    : public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;

    CImplIfMethodProxy(
        CCppWriter* pWriter, ObjPtr& pNode );

    gint32 OutputAsyncCbWrapper();
    gint32 OutputEvent();
    gint32 OutputSync();
    gint32 OutputAsync();
    gint32 Output();
};

class CImplIfMethodSvr
    : public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplIfMethodSvr(
        CCppWriter* pWriter, ObjPtr& pNode );

    gint32 OutputEvent();
    gint32 OutputSync();
    gint32 OutputAsyncCallback();
    gint32 OutputAsync();
    gint32 Output();
};

class ImplDllLoadFactory
{
};

class CImplClassFactory
{
    public:
    CCppWriter* m_pWriter = nullptr;
    CStatements* m_pNode = nullptr;
    CImplClassFactory(
        CCppWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};

class CImplMsgFactory
{
};

class CImplMainFunc
{
};

class CExportObjDesc
{
};

class CExportDrivers
{
};

class CExportMakefile
{
};


