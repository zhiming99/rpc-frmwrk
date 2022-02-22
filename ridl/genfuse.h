/*
 * =====================================================================================
 *
 *       Filename:  genfuse.h
 *
 *    Description:  declaration of fuse integration related classes 
 *
 *        Version:  1.0
 *        Created:  02/11/2022 10:44:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2022 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#include <sys/stat.h>
#include "rpc.h"
#include "astnode.h"
#include "gencpp.h"
#include "sha1.h"
#include "proxy.h"

extern std::string g_strAppName;
extern guint32 g_dwFlags;

class CEmitSerialCodeFuse :
    public CArgListUtils
{
    public:
    typedef CArgListUtils super;
    CWriterBase* m_pWriter = nullptr;
    CAstListNode* m_pArgs;

    CEmitSerialCodeFuse(
        CWriterBase* pWriter, ObjPtr& pNode );

    gint32 OutputSerial(
        const std::string& strObjc,
        const std::string strBuf );

    gint32 OutputDeserial(
        const std::string& strObjc,
        const std::string strBuf );
};

class CDeclInterfProxyFuse
    : public CMethodWriter
{
    CInterfaceDecl* m_pNode = nullptr;

    public:
    typedef CMethodWriter super;
    CDeclInterfProxyFuse( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
    gint32 OutputEvent( CMethodDecl* pmd );
    gint32 OutputAsync( CMethodDecl* pmd );
};

class CDeclInterfSvrFuse
    : public CMethodWriter
{
    CInterfaceDecl* m_pNode = nullptr;

    public:

    typedef CMethodWriter super;
    CDeclInterfSvrFuse( CCppWriter* pWriter,
        ObjPtr& pNode );

    gint32 Output();
    gint32 OutputEvent( CMethodDecl* pmd );
    gint32 OutputAsync( CMethodDecl* pmd );
};

class CDeclServiceImplFuse :
    public CDeclServiceImpl
{
    public:
    typedef CDeclServiceImpl super;
    CDeclServiceImplFuse( CCppWriter* pWriter,
        ObjPtr& pNode, bool bServer ) :
        super( pWriter, pNode, bServer )
    {}

    gint32 Output() override;
};

struct CMethodWriterFuse
    : public CMethodWriter
{
    typedef CMethodWriter super;
    CMethodWriterFuse( CWriterBase* pWriter ):
        super( pWriter )
    {}

    virtual gint32 GenSerialArgs(
        ObjPtr& pArgList,
        const std::string& strBuf,
        bool bDeclare, bool bAssign,
        bool bNoRet = false );

    virtual gint32 GenDeserialArgs(
        ObjPtr& pArgList,
        const std::string& strBuf,
        bool bDeclare, bool bAssign,
        bool bNoRet = false );
};

class CImplIfMethodProxyFuse
    : public CMethodWriterFuse
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriterFuse super;

    CImplIfMethodProxyFuse(
        CCppWriter* pWriter, ObjPtr& pNode );

    gint32 OutputAsyncCbWrapper();
    gint32 OutputEvent();
    gint32 OutputAsync();
    gint32 Output();
};

class CImplIfMethodSvrFuse
    : public CMethodWriterFuse
{
    CMethodDecl* m_pNode = nullptr;
    CInterfaceDecl* m_pIf = nullptr;

    public:
    typedef CMethodWriterFuse super;
    CImplIfMethodSvrFuse(
        CCppWriter* pWriter, ObjPtr& pNode );

    gint32 OutputEvent();
    gint32 OutputAsyncCallback();
    gint32 OutputAsyncCancelWrapper();
    gint32 OutputAsyncSerial();
    gint32 OutputAsync();
    gint32 Output();
};

class CImplServiceImplFuse :
    public CDeclServiceImplFuse
{
    public:
    typedef CDeclServiceImplFuse super;
    CImplServiceImplFuse( CCppWriter* pWriter,
        ObjPtr& pNode, bool bServer )
        : super( pWriter, pNode, bServer )
    {}

    gint32 Output() override;
};

class CImplIufProxyFuse
    : public CImplIufProxy
{
    public:
    typedef CImplIufProxy super;
    CImplIufProxyFuse( CCppWriter* pWriter,
        ObjPtr& pNode ) :
        super( pWriter, pNode )
    {}

    gint32 OutputDispatch();
    inline gint32 Output()
    {
        gint32 ret = super::Output();
        if( ERROR( ret ) )
            return ret;
        return OutputDispatch();
    }
};

class CImplIufSvrFuse :
    public CImplIufSvr
{
    public:
    typedef CImplIufSvr super;
    CImplIufSvrFuse( CCppWriter* pWriter,
        ObjPtr& pNode ) :
        super( pWriter, pNode )
    {}
    gint32 OutputDispatch();
    inline gint32 Output()
    {
        gint32 ret = super::Output();
        if( ERROR( ret ) )
            return ret;
        return OutputDispatch();
    }
};
