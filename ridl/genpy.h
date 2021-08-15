/*
 * =====================================================================================
 *
 *       Filename:  genpy.h
 *
 *    Description:  declarations of proxy/server generator for Python
 *
 *        Version:  1.0
 *        Created:  08/14/2021 03:03:00 PM
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
#include <sys/stat.h>
#include "rpc.h"
using namespace rpcf;
#include "astnode.h"
#include "gencpp.h"
#include "sha1.h"

gint32 GenPyProj(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot );

struct CPyFileSet
{
    std::string  m_strPath;
    std::string  m_strStructsPy;
    std::string  m_strInitPy;
    std::string  m_strObjDesc;
    std::string  m_strDriver;
    std::string  m_strMakefile;
    std::string  m_strMainCli;
    std::string  m_strMainSvr;

    std::map< std::string, gint32 > m_mapSvcImp;
    std::vector< STMPTR > m_vecFiles;

    CPyFileSet( const std::string& strOutPath,
        const std::string& strAppName );

    gint32 OpenFiles();
    gint32 AddSvcImpl( const std::string& strSvcName );
    ~CPyFileSet();

    const stdstr& GetOutPath() const
    { return m_strPath; }
};

struct CPyWriterBase
{
    guint32 m_dwIndWid = 4;
    guint32 m_dwIndent = 0;

    std::unique_ptr< CPyFileSet > m_pFiles;

    std::ofstream* m_curFp = nullptr;
    std::string m_strCurFile;

    CPyWriterBase(
        const std::string& strPath,
        const std::string& strAppName,
        guint32 dwIndent = 0 )
    {
        std::unique_ptr< CPyFileSet > ptr(
            new CPyFileSet( strPath, strAppName ) );
        m_pFiles = std::move( ptr );
        m_dwIndent = 0;
    }

    void WriteLine1( const std::string& strText )
    {
        NewLine();
        AppendText( strText );
    }

    void WriteLine0( const std::string& strText )
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
        if( m_dwIndent > 0 )
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

    void Reset()
    {
        m_dwIndent = 0;
    }

    gint32 AddSvcImpl(
        const std::string& strSvcName )
    {
        return m_pFiles->AddSvcImpl( strSvcName );
    }

    const stdstr& GetCurFile() const
    { return m_strCurFile; }

    const stdstr& GetOutPath() const
    { return m_pFiles->GetOutPath(); }
};

class CPyWriter : public CPyWriterBase
{
    public:
    ObjPtr m_pNode;
    typedef CPyWriterBase super;
    CPyWriter(
        const std::string& strPath,
        const std::string& strAppName,
        ObjPtr pStmts ) :
        super( strPath, strAppName )
    { m_pNode = pStmts; }

    inline gint32 SelectStructsFile()
    {
        m_strCurFile = m_pFiles->m_strStructsPy;
        return SelectFile( 0 );
    }

    inline gint32 SelectInitFile()
    {
        m_strCurFile = m_pFiles->m_strInitPy;
        return SelectFile( 1 );
    }

    inline gint32 SelectDescFile()
    {
        m_strCurFile = m_pFiles->m_strObjDesc;
        return SelectFile( 2 );
    }

    inline gint32 SelectDrvFile()
    {
        m_strCurFile = m_pFiles->m_strDriver;
        return SelectFile( 3 );
    }

    inline gint32 SelectMakefile()
    {
        m_strCurFile = m_pFiles->m_strMakefile;
        return SelectFile( 4 );
    }

    inline gint32 SelectMainCli()
    {
        m_strCurFile = m_pFiles->m_strMainCli;
        return SelectFile( 5 );
    }

    inline gint32 SelectMainSvr()
    {
        m_strCurFile = m_pFiles->m_strMainSvr;
        return SelectFile( 6 );
    }

    inline gint32 SelectImplFile(
        const std::string& strFile )
    {
        decltype( m_pFiles->m_mapSvcImp )::iterator itr =
        m_pFiles->m_mapSvcImp.find( strFile );
        if( itr == m_pFiles->m_mapSvcImp.end() )
            return -ENOENT;
        gint32 idx = itr->second;
        m_strCurFile = strFile;
        return SelectFile( idx );
    }
};

class CDeclarePyStruct
{
    CPyWriter* m_pWriter = nullptr;
    CStructDecl* m_pNode = nullptr;

    public:
    CDeclarePyStruct( CPyWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
    void OutputStructBase();
};

class CExportInitPy
{
    CPyWriter* m_pWriter = nullptr;
    CStatements* m_pNode = nullptr;

    public:
    CExportInitPy(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};
