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
#pragma once
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

struct CPyFileSet : public IFileSet
{
    std::string  m_strStructsPy;
    std::string  m_strInitPy;
    std::string  m_strObjDesc;
    std::string  m_strDriver;
    std::string  m_strMakefile;
    std::string  m_strMainCli;
    std::string  m_strMainSvr;
    std::string  m_strReadme;

    typedef IFileSet super;
    CPyFileSet( const std::string& strOutPath,
        const std::string& strAppName );

    gint32 OpenFiles() override;
    gint32 AddSvcImpl(
        const stdstr& strSvcName ) override;
    ~CPyFileSet();
};

class CPyWriter : public CWriterBase
{
    public:
    ObjPtr m_pNode;
    typedef CWriterBase super;
    CPyWriter(
        const std::string& strPath,
        const std::string& strAppName,
        ObjPtr pStmts ) :
        super( strPath, strAppName )
    {
        std::unique_ptr< CPyFileSet > ptr(
            new CPyFileSet( strPath, strAppName ) );
        m_pFiles = std::move( ptr );
        m_pNode = pStmts;
    }

    CPyWriter(
        const std::string& strPath,
        const std::string& strAppName ) :
        super( strPath, strAppName )
    {}

    virtual gint32 SelectStructsFile()
    {
        CPyFileSet* pFiles = static_cast< CPyFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strStructsPy;
        return SelectImplFile( m_strCurFile );
    }

    virtual gint32 SelectInitFile()
    {
        CPyFileSet* pFiles = static_cast< CPyFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strInitPy;
        return SelectImplFile( m_strCurFile );
    }

    virtual gint32 SelectDescFile()
    {
        CPyFileSet* pFiles = static_cast< CPyFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strObjDesc;
        return SelectImplFile( m_strCurFile );
    }

    virtual gint32 SelectDrvFile()
    {
        CPyFileSet* pFiles = static_cast< CPyFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strDriver;
        return SelectImplFile( m_strCurFile );
    }

    virtual gint32 SelectMakefile()
    {
        CPyFileSet* pFiles = static_cast< CPyFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMakefile;
        return SelectImplFile( m_strCurFile );
    }

    virtual gint32 SelectMainCli()
    {
        CPyFileSet* pFiles = static_cast< CPyFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainCli;
        return SelectImplFile( m_strCurFile );
    }

    virtual gint32 SelectMainSvr()
    {
        CPyFileSet* pFiles = static_cast< CPyFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strMainSvr;
        return SelectImplFile( m_strCurFile );
    }

    virtual gint32 SelectReadme()
    {
        CPyFileSet* pFiles = static_cast< CPyFileSet* >
            ( m_pFiles.get() );
        m_strCurFile = pFiles->m_strReadme;
        return SelectImplFile( m_strCurFile );
    }
};

class CDeclarePyStruct
{
    protected:
    CPyWriter* m_pWriter = nullptr;
    CStructDecl* m_pNode = nullptr;

    public:
    CDeclarePyStruct( CPyWriter* pWriter,
        ObjPtr& pNode );
    virtual gint32 Output();
    void OutputStructBase();
};

class CExportInitPy
{
    CStatements* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CExportInitPy(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};

class CPyExportMakefile :
    public CExportBase
{
    public:
    typedef CExportBase super;
    CPyExportMakefile(
        CPyWriter* pWriter, ObjPtr& pNode );
};

class CImplPyMthdProxyBase :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplPyMthdProxyBase(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputSync( bool bSync = true );
    gint32 OutputAsyncCbWrapper();
    gint32 OutputEvent();

    void EmitOptions();
};

class CImplPyIfProxyBase
{
    CInterfaceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPyIfProxyBase(
        CPyWriter* pWriter, ObjPtr& pNode );

    gint32 Output();
};

class CImplPySvcProxyBase
{
    CServiceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPySvcProxyBase(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};

class CImplPyMthdSvrBase :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplPyMthdSvrBase(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputSync( bool bSync = true );
    gint32 OutputAsyncWrapper();
    gint32 OutputAsyncCompHandler();
    gint32 OutputAsyncCHWrapper();
    gint32 OutputEvent();
};

class CImplPyIfSvrBase
{
    CInterfaceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPyIfSvrBase(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};

class CImplPySvcSvrBase
{
    CServiceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPySvcSvrBase(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};

class CImplPyMthdSvr :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplPyMthdSvr(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();

    gint32 OutputSync( bool bSync = true );

    gint32 OutputAsyncCancelHandler();
};

class CImplPyIfSvr
{
    CInterfaceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPyIfSvr(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
};

class CImplPySvcSvr
{
    CServiceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPySvcSvr(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputSvcSvrClass();
};

class CImplPyMthdProxy :
    public CMethodWriter
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriter super;
    CImplPyMthdProxy(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputAsyncCallback();
    gint32 OutputEvent();
};

class CImplPyIfProxy
{
    CInterfaceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPyIfProxy(
        CPyWriter* pWriter, ObjPtr& pNode );

    gint32 Output();
};

class CImplPySvcProxy
{
    CServiceDecl* m_pNode = nullptr;
    CPyWriter* m_pWriter = nullptr;

    public:
    CImplPySvcProxy(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputSvcProxyClass();
};

class CImplPyMainFunc :
    public CArgListUtils
{
    CPyWriter* m_pWriter = nullptr;
    CStatements* m_pNode = nullptr;
    public:
    typedef CMethodWriter super;

    CImplPyMainFunc(
        CPyWriter* pWriter, ObjPtr& pNode );
    gint32 Output();
    gint32 OutputCli(
        std::vector< ObjPtr >& vecSvcs );
    gint32 OutputSvr(
        std::vector< ObjPtr >& vecSvcs );
    gint32 EmitGetOpt( bool bProxy );
    gint32 EmitUsage( bool bProxy );
    gint32 EmitMainloopServer( 
        std::vector< ObjPtr >& vecSvcs );
    gint32 EmitProxySampleCode( 
        std::vector< ObjPtr >& vecSvcs );
    gint32 EmitDefineUserMain( 
        std::vector< ObjPtr >& vecSvcs,
        bool bProxy );
    gint32 EmitKProxyLoop();
};

class CExportPyReadme :
    public CExportReadme
{
    public:
    typedef CExportReadme super;
    CExportPyReadme( CWriterBase* pWriter,
        ObjPtr& pNode )
        : super( pWriter, pNode )
    {}
    gint32 Output();
    gint32 Output_en();
    gint32 Output_cn();
};

#define EMIT_DISCLAIMER_PY EMIT_DISCLAIMER_INNER( "#" )
