/*
 * =====================================================================================
 *
 *       Filename:  genjs.h
 *
 *    Description:  declarations of proxy generator for JavaScript
 *
 *        Version:  1.0
 *        Created:  02/28/2024 02:49:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2024 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once
#include <sys/stat.h>
#include "rpc.h"
using namespace rpcf;
#include "astnode.h"
#include "gencpp.h"
#include "sha1.h"

gint32 GenJsProj(
    const std::string& strOutPath,
    const std::string& strAppName,
    ObjPtr pRoot );

struct CJsFileSet : public IFileSet
{
    std::string  m_strStructsJs;
    std::string  m_strIndex;
    std::string  m_strObjDesc;
    std::string  m_strDriver;
    std::string  m_strMakefile;
    std::string  m_strReadme;

    typedef IFileSet super;
    CJsFileSet( const std::string& strOutPath,
        const std::string& strAppName );

    gint32 OpenFiles() override;
    gint32 AddSvcImpl(
        const stdstr& strSvcName ) override;
    ~CJsFileSet();
};

class CJsWriter : public CWriterBase
{
    public:
    ObjPtr m_pNode;
    typedef CWriterBase super;
    CJsWriter(
        const std::string& strPath,
        const std::string& strAppName,
        ObjPtr pStmts ) :
        super( strPath, strAppName )
    {
        std::unique_ptr< CJsFileSet > ptr(
            new CJsFileSet( strPath, strAppName ) );
        m_pFiles = std::move( ptr );
        m_pNode = pStmts;
    }

    CJsWriter(
        const std::string& strPath,
        const std::string& strAppName ) :
        super( strPath, strAppName )
    {}

    virtual gint32 SelectStructsFile()
    {
        CJsFileSet* pFiles = static_cast< CJsFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strStructsJs;
        return SelectFile( 0 );
    }

    virtual gint32 SelectIndexFile()
    {
        CJsFileSet* pFiles = static_cast< CJsFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strIndex;
        return SelectFile( 1 );
    }

    virtual gint32 SelectDescFile()
    {
        CJsFileSet* pFiles = static_cast< CJsFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strObjDesc;
        return SelectFile( 2 );
    }

    virtual gint32 SelectDrvFile()
    {
        CJsFileSet* pFiles = static_cast< CJsFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strDriver;
        return SelectFile( 3 );
    }

    virtual gint32 SelectMakefile()
    {
        CJsFileSet* pFiles = static_cast< CJsFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMakefile;
        return SelectFile( 4 );
    }

    virtual gint32 SelectReadme()
    {
        CJsFileSet* pFiles = static_cast< CJsFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strReadme;
        return SelectFile( 5 );
    }


    gint32 SelectImplFile(
        const std::string& strFile )
    {
        auto itr = m_pFiles->m_mapSvcImp.find( strFile );
        if( itr == m_pFiles->m_mapSvcImp.end() )
            return -ENOENT;
        gint32 idx = itr->second;
        m_strCurFile = strFile;
        return SelectFile( idx );
    }
};

class CDeclareJsStruct
{
    protected:
    CJsWriter* m_pWriter = nullptr;
    CStructDecl* m_pNode = nullptr;

    public:
    CDeclareJsStruct( CJsWriter* pWriter,
        ObjPtr& pNode );
    virtual gint32 Output();
    void OutputStructBase();
};

class CExportIndexJs
{
    CStatements* m_pNode = nullptr;
    CJsWriter* m_pWriter = nullptr;

    public:
    CExportIndexJs(
        CJsWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};

class CJsExportMakefile :
    public CExportBase
{
    public:
    typedef CExportBase super;
    CJsExportMakefile(
        CJsWriter* pWriter, ObjPtr& pNode );
};

class CImplJsMthdProxyBase :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplJsMthdProxyBase(
        CJsWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputSync( bool bSync = true );
    gint32 OutputAsyncCbWrapper();
    gint32 OutputEvent();
};

class CImplJsIfProxyBase
{
    CInterfaceDecl* m_pNode = nullptr;
    CJsWriter* m_pWriter = nullptr;

    public:
    CImplJsIfProxyBase(
        CJsWriter* pWriter, ObjPtr& pNode );

    gint32 Output();
};

class CImplJsSvcProxyBase
{
    CServiceDecl* m_pNode = nullptr;
    CJsWriter* m_pWriter = nullptr;

    public:
    CImplJsSvcProxyBase(
        CJsWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};

class CImplJsMthdProxy :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplJsMthdProxy(
        CJsWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputAsyncCallback();
    gint32 OutputEvent();
};

class CImplJsIfProxy
{
    CInterfaceDecl* m_pNode = nullptr;
    CJsWriter* m_pWriter = nullptr;

    public:
    CImplJsIfProxy(
        CJsWriter* pWriter, ObjPtr& pNode );

    gint32 Output();
};

class CImplJsSvcProxy
{
    CServiceDecl* m_pNode = nullptr;
    CJsWriter* m_pWriter = nullptr;

    public:
    CImplJsSvcProxy(
        CJsWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputSvcProxyClass();
};

class CExportJsReadme :
    public CExportReadme
{
    public:
    typedef CExportReadme super;
    CExportJsReadme( CWriterBase* pWriter,
        ObjPtr& pNode )
        : super( pWriter, pNode )
    {}
    gint32 Output();
    gint32 Output_en();
    gint32 Output_cn();
};

