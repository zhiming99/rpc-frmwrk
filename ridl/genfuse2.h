/*
 * =====================================================================================
 *
 *       Filename:  genfuse2.h
 *
 *    Description:  declarations of classes for code generator of fuse over ROS
 *
 *        Version:  1.0
 *        Created:  10/01/2022 09:57:22 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2021 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once

class CDeclInterfProxyFuse2 :
    public CDeclInterfProxyFuse
{
    public:
    typedef CDeclInterfProxyFuse super;
    CDeclInterfProxyFuse2(
        CCppWriter* pWriter, ObjPtr& pNode );
    gint32 Output() override;
};

class CDeclInterfSvrFuse2 :
    public CDeclInterfSvrFuse
{
    public:
    typedef CDeclInterfSvrFuse super;
    CDeclInterfSvrFuse2(
        CCppWriter* pWriter, ObjPtr& pNode ) :
        super( pWriter, pNode )
    {} 
    gint32 Output() override;
    gint32 OutputAsync( CMethodDecl* pmd ) override;
};

class CDeclServiceImplFuse2 :
    public CDeclServiceImplFuse
{
    public:
    typedef CDeclServiceImplFuse super;
    CDeclServiceImplFuse2( CCppWriter* pWriter,
        ObjPtr& pNode, bool bProxy ) :
        super( pWriter, pNode, bProxy )
    {} 
    gint32 Output() override;
};

class CImplIfMethodProxyFuse2 :
    public CImplIfMethodProxyFuse
{
    public:
    typedef CImplIfMethodProxyFuse super;
    CImplIfMethodProxyFuse2(
        CCppWriter* pWriter, ObjPtr& pNode ) :
        super( pWriter, pNode )
    {} 
    gint32 OutputEvent() override;
};

class CImplIfMethodSvrFuse2 :
    public CImplIfMethodSvrFuse
{
    public:
    typedef CImplIfMethodSvrFuse super;
    CImplIfMethodSvrFuse2(
        CCppWriter* pWriter, ObjPtr& pNode ) :
        super( pWriter, pNode )
    {} 
    gint32 OutputAsyncSerial() override;
    gint32 OutputAsyncCancelWrapper() override;
};

class CImplServiceImplFuse2 :
    public CImplServiceImplFuse
{
    public:
    typedef CImplServiceImplFuse super;
    CImplServiceImplFuse2( CCppWriter* pWriter,
        ObjPtr& pNode, bool bProxy ) :
        super( pWriter, pNode, bProxy )
    {} 
    gint32 Output() override;
    gint32 OutputROSSkel();
};

class CImplIufProxyFuse2:
    public CImplIufProxyFuse
{
    public:
    typedef CImplIufProxyFuse super;
    CImplIufProxyFuse2(
        CCppWriter* pWriter, ObjPtr& pNode ) :
        super( pWriter, pNode )
    {}
    gint32 Output() override; 
    gint32 OutputOnReqComplete();
};

class CImplIufSvrFuse2 :
    public CImplIufSvrFuse
{
    public:
    typedef CImplIufSvrFuse super;
    CImplIufSvrFuse2(
        CCppWriter* pWriter, ObjPtr& pNode ) :
        super( pWriter, pNode )
    {} 
    gint32 OutputDispatch() override;
};

class CImplMainFuncFuse2 :
    public CImplMainFuncFuse
{
    public:
    typedef CImplMainFuncFuse super;
    CImplMainFuncFuse2(
        CCppWriter* pWriter,
        ObjPtr& pNode,
        bool bProxy ) :
        super( pWriter, pNode, bProxy )
    {} 
    gint32 Output() override;
};

class CDeclServiceFuse2 :
    public CDeclService2
{
    public:
    typedef CDeclService2 super;
    CDeclServiceFuse2(
        CCppWriter* pWriter, ObjPtr& pNode ) :
        super( pWriter, pNode )
    {} 

    gint32 OutputROS( bool bProxy ) override;
    gint32 OutputROSSkel( bool bProxy ) override;
};

class CImplClassFactoryFuse2 :
    public CImplClassFactory
{
    public:
    typedef CImplClassFactory super;
    CImplClassFactoryFuse2(
        CCppWriter* pWriter, ObjPtr& pNode, bool bProxy )
        : super( pWriter, pNode, bProxy )
    {}
    gint32 Output() override;
};

class CImplServiceSkelFuse2 :
    public CMethodWriter
{
    CServiceDecl* m_pNode = nullptr;

    public:
    typedef CMethodWriter super;
    CImplServiceSkelFuse2( CCppWriter* pWriter,
        ObjPtr& pNode );
    gint32 Output();
};

