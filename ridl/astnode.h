/*
 * =====================================================================================
 *
 *       Filename:  astnode.h
 *
 *    Description:  declarations of the tree nodes for abstract syntax tree
 *
 *        Version:  1.0
 *        Created:  02/10/2021 05:01:32 PM
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

#include <string>
#include <map>
#include "ridlc.h"
#include "rpc.h"
#include "idlclsid.h" 
#include <regex>

#define SET_PARENT_OF( pObj ) \
if( !pObj.IsEmpty() ) \
{ \
    CAstNodeBase* pnode = pObj; \
    pnode->SetParent( this ); \
}

#define PROP_ABSTMETHOD_PROXY       0
#define PROP_ABSTMETHOD_SERVER      1

#define PROP_MESSAGE_ID             3

#define PROP_METHODFLAG_PROXY       4
#define PROP_METHODFLAG_SERVER      5

#define MF_IN_ARG         1
#define MF_OUT_ARG        2

#define MF_IN_AS_IN       4
#define MF_OUT_AS_IN      8

#define MF_OUT_AS_OUT     0
#define MF_IN_AS_OUT      0

extern std::map< gint32, char > g_mapTypeSig;
std::string GetTypeSig( ObjPtr& pObj );

// declarations
struct CDeclMap
{
    std::map< std::string, ObjPtr > m_mapDecls;
    inline gint32 GetDeclNode(
        const std::string& strName, ObjPtr& pObj )
    {
        std::map< std::string, ObjPtr >::iterator
            itr = m_mapDecls.find( strName );
        if( itr == m_mapDecls.end() )
            return -ENOENT;
        pObj = itr->second;
        return STATUS_SUCCESS;
    }

    inline gint32 AddDeclNode(
        const std::string& strName,
        ObjPtr& pNode )
    {
        std::map< std::string, ObjPtr >::iterator
            itr = m_mapDecls.find( strName );
        if( itr != m_mapDecls.end() )
            return -EEXIST;
        m_mapDecls[ strName ] = pNode;
        return STATUS_SUCCESS;
    }

    inline bool IsDeclared(
        const std::string& strName )
    {
        ObjPtr pObj;
        gint32 ret = GetDeclNode(
            strName, pObj );
        if( SUCCEEDED( ret ) )
            return true;
        return false;
    }

    inline void Clear()
    { m_mapDecls.clear(); }
};

extern CDeclMap g_mapDecls;

struct CAliasMap
{
    std::map< std::string, ObjPtr > m_mapAlias;
    inline gint32 GetAliasType(
        const std::string& strName,
        ObjPtr& pType )
    {
        std::map< std::string, ObjPtr >::iterator
            itr = m_mapAlias.find( strName );
        if( itr == m_mapAlias.end() )
            return -ENOENT;
        pType = itr->second;
        return STATUS_SUCCESS;
    }

    inline gint32 AddAliasType(
        const std::string& strName,
        ObjPtr& pType )
    {
        if( pType.IsEmpty() )
            return -EINVAL;

        std::map< std::string, ObjPtr >::iterator
            itr = m_mapAlias.find( strName );
        if( itr != m_mapAlias.end() )
            return -EEXIST;
        m_mapAlias[ strName ] = pType;
        return STATUS_SUCCESS;
    }
};

extern CAliasMap g_mapAliases;

#define NODE_FLAG_STREAM    1
#define NODE_FLAG_SERIAL    2

#define NODE_FLAG_ASYNCP    1
#define NODE_FLAG_ASYNCS    2
#define NODE_FLAG_ASYNC     3

struct CAstNodeBase :
    public CObjBase
{
    typedef CObjBase super;
    CAstNodeBase():super() {} 
    guint32 m_dwFlags = 0;
    CAstNodeBase* m_pParent;

    inline void SetParent(
        CAstNodeBase* pParent )
    {
        if( pParent != nullptr )
            m_pParent = pParent;
    }

    CAstNodeBase* GetParent() const
    { return m_pParent; }

    virtual std::string ToStringCpp() const
    { return std::string( "" ); }

    virtual std::string ToStringPy() const
    { return std::string( "" ); }

    virtual std::string ToStringJava() const
    { return std::string( "" ); }

    bool IsStream() const 
    {
        if( ( m_dwFlags & NODE_FLAG_STREAM ) > 0 )
            return true;
        return false;
    }

    void EnableStream( bool bEnable = true )
    {
        if( bEnable )
            m_dwFlags |= NODE_FLAG_STREAM;
        else
            m_dwFlags &= ~NODE_FLAG_STREAM;
    }

    bool IsSerialize() const
    {
        if( ( m_dwFlags & NODE_FLAG_SERIAL ) > 0 )
            return true;
        return false;
    }

    void EnableSerialize( bool bEnable = true )
    {
        if( bEnable )
            m_dwFlags |= NODE_FLAG_SERIAL;
        else
            m_dwFlags &= ~NODE_FLAG_SERIAL;
    }

    virtual std::string GetSignature() const
    { return std::string( "" ); }
};

typedef CAutoPtr< clsid( Invalid ), CAstNodeBase > NodePtr;

struct CNamedNode :
    public CAstNodeBase
{
    typedef CAstNodeBase super;
    std::string m_strName;

    inline void SetName(
        const std::string& strName )
    { m_strName = strName; }

    inline const std::string& GetName() const
    { return m_strName; }
};

struct CConstDecl:
    public CNamedNode
{
    typedef CNamedNode super;
    CConstDecl() : CNamedNode()
    { SetClassId( clsid( CConstDecl ) ); }
};

struct CAstListNode : CAstNodeBase
{
    std::deque< ObjPtr > m_queChilds;
    typedef CAstNodeBase super;
    CAstListNode() : CAstNodeBase()
    {}

    inline void AddChild( ObjPtr& pChild )
    {
        if( !pChild.IsEmpty() )
        {
            m_queChilds.push_back( pChild );
            SET_PARENT_OF( pChild );
        }
    }

    inline ObjPtr GetChild( guint32 i ) const
    {
        if( i >= m_queChilds.size() )
            return ObjPtr();
        return m_queChilds[ i ];
    }

    guint32 GetCount() const
    { return m_queChilds.size(); }

    virtual gint32 InsertChild( ObjPtr& pChild )
    {
        if( !pChild.IsEmpty() )
        {
            m_queChilds.push_front( pChild );
            SET_PARENT_OF( pChild );
        }
        return 0;
    }
};

struct CAttrExp : public CAstNodeBase
{
    typedef CAstNodeBase super;

    CAttrExp() : CAstNodeBase()
    { SetClassId( clsid( CAttrExp ) ); }
    guint32 m_dwAttrName;
    BufPtr m_pVal;

    inline void SetName( guint32 dwAttrName )
    { m_dwAttrName = dwAttrName; }

    inline guint32 GetName() const
    { return m_dwAttrName; }

    inline void SetVal( BufPtr& pVal )
    { m_pVal = pVal; }

    inline BufPtr& GetVal()
    { return m_pVal; }
};

struct CAttrExps : public CAstListNode
{
    typedef CAstListNode super;
    CAttrExps() : CAstListNode()
    { SetClassId( clsid( CAttrExps ) ); }

    guint32 GetAsyncFlags() const
    {
        guint32 dwFlags = 0;
        for( auto elem : m_queChilds )
        {
            CAttrExp* pExp = elem;
            if( pExp == nullptr )
                continue;
            guint32 dwToken = pExp->GetName();
            if( dwToken == TOK_ASYNC )
                dwFlags = NODE_FLAG_ASYNC;
            else if( dwToken == TOK_ASYNCP )
                dwFlags = NODE_FLAG_ASYNCP;
            else if( dwToken == TOK_ASYNCS )
                dwFlags = NODE_FLAG_ASYNCS;
            if( dwFlags > 0 )
                break;
        }
        return dwFlags;
    }

    gint32 GetAttrByToken(
        guint32 token, BufPtr& pVal ) const
    {
        gint32 ret = -ENOENT;
        for( auto elem : m_queChilds )
        {
            CAttrExp* pExp = elem;
            if( pExp == nullptr )
                continue;
            guint32 dwToken = pExp->GetName();
            if( dwToken == token )
            {
                ret = 0;
                BufPtr& pValue = pExp->GetVal();
                if( pValue.IsEmpty() ||
                    pValue->empty() )
                    break;
                pVal = pValue;
                break;
            }
        }
        return ret;
    }

    gint32 CheckTimeoutSec() const
    {
        BufPtr pBuf;
        gint32 ret = GetAttrByToken(
            TOK_TIMEOUT, pBuf );
        if( ret == -ENOENT )
            return STATUS_SUCCESS;

        if( ERROR( ret ) )
            return ret;

        if( pBuf.IsEmpty() || pBuf->empty() )
        {
            return -ENOENT;
        }
        if( pBuf->GetExDataType() !=
            typeUInt32 )
        {
            return  -EINVAL;
        }
        return ret;
    }

    guint32 GetTimeoutSec() const
    {
        BufPtr pBuf;
        guint32 dwTimeout = 0;
        do{
            gint32 ret = GetAttrByToken(
                TOK_TIMEOUT, pBuf );

            if( ERROR( ret ) )
                break;

            if( pBuf.IsEmpty() || pBuf->empty() )
                break;

            dwTimeout = *pBuf;

        }while( 0 );

        return dwTimeout;
    }

    bool IsEvent() const
    {
        BufPtr pBuf;
        gint32 ret = GetAttrByToken(
            TOK_EVENT, pBuf );
        if( ERROR( ret ) )
            return false;
        return true;
    }
};

struct CPrimeType : public CAstNodeBase
{
    guint32 m_dwAttrName;
    typedef CAstNodeBase super;
    CPrimeType() : super()
    { SetClassId( clsid( CPrimeType ) ); }

    inline void SetName( guint32 dwAttrName )
    { m_dwAttrName = dwAttrName; }

    inline guint32 GetName() const
    { return m_dwAttrName; }

    std::string GetSignature() const
    {
        if( g_mapTypeSig.find( m_dwAttrName ) ==
            g_mapTypeSig.end() )
            return std::string( "" );
        std::string strRet;
        strRet += g_mapTypeSig[ m_dwAttrName ];
        return strRet;
    }

    std::string ToStringCpp() const
    {
        std::string strName;
        switch( m_dwAttrName )
        {
        case TOK_STRING:
            strName = "std::string";
            break;
        case TOK_UINT64:
            strName = "guint64";
            break;
        case TOK_INT64:
            strName = "gint64";
            break;
        case TOK_UINT32:
            strName = "guint32";
            break;
        case TOK_INT32:
            strName = "gint32";
            break;
        case TOK_UINT16:
            strName = "guint16";
            break;
        case TOK_INT16:
            strName = "gint16";
            break;
        case TOK_FLOAT:
            strName = "float";
            break;
        case TOK_DOUBLE:
            strName = "double";
            break;
        case TOK_BYTE:
            strName = "guint8";
            break;
        case TOK_BOOL:
            strName = "bool";
            break;
        case TOK_BYTEARR:
            strName = "BufPtr";
            break;
        case TOK_OBJPTR:
            strName = "ObjPtr";
            break;
        case TOK_HSTREAM:
            strName = "HSTREAM";
            break;
        default:
            break;
        }
        return strName;
    }

    std::string ToStringPy() const
    {
        std::string strName;
        switch( m_dwAttrName )
        {
        case TOK_STRING:
            strName = "str";
            break;
        case TOK_UINT64:
            strName = "int";
            break;
        case TOK_INT64:
            strName = "int";
            break;
        case TOK_UINT32:
            strName = "int";
            break;
        case TOK_INT32:
            strName = "int";
            break;
        case TOK_UINT16:
            strName = "int";
            break;
        case TOK_INT16:
            strName = "nt";
            break;
        case TOK_FLOAT:
            strName = "float";
            break;
        case TOK_DOUBLE:
            strName = "float";
            break;
        case TOK_BYTE:
            strName = "int";
            break;
        case TOK_BOOL:
            strName = "bool";
            break;
        case TOK_BYTEARR:
            strName = "cpp.BufPtr";
            break;
        case TOK_OBJPTR:
            strName = "cpp.ObjPtr";
            break;
        case TOK_HSTREAM:
            strName = "HSTREAM";
            break;
        default:
            break;
        }
        return strName;
    }
};

struct CArrayType : public CPrimeType
{
    typedef CPrimeType super;

    CArrayType() : super()
    { SetClassId( clsid( CArrayType ) ); }

    ObjPtr m_pElemType;

    void SetElemType( ObjPtr& pElem )
    {
        m_pElemType = pElem;
    }

    ObjPtr& GetElemType()
    { return m_pElemType; }

    std::string GetSignature() const
    {
        std::string strSig = "(";
        ObjPtr pObj = m_pElemType;
        strSig += GetTypeSig( pObj );
        strSig += ")";
        return strSig;
    }

    std::string ToStringCpp() const
    {
        std::string strName;
        strName = "std::vector<";
        CAstNodeBase* pNode = m_pElemType;
        std::string strElem = pNode->ToStringCpp();
        strName += strElem;
        strName += ">";
        return strName;
    }
};

struct CMapType : public CArrayType
{
    typedef CArrayType super;
    ObjPtr m_pKeyType;

    CMapType() : super()
    { SetClassId( clsid( CMapType ) ); }

    void SetKeyType( ObjPtr& pKey )
    {
        m_pKeyType = pKey;
        SET_PARENT_OF( pKey );
    }

    ObjPtr& GetKeyType()
    { return m_pKeyType; }

    std::string GetSignature() const
    {
        std::string strSig = "[";
        ObjPtr pObj = m_pKeyType;
        strSig += GetTypeSig( pObj );
        pObj = m_pElemType;
        strSig += GetTypeSig( pObj );
        strSig += "]";
        return strSig;
    }

    std::string ToStringCpp() const
    {
        std::string strName;
        strName = "std::map<";

        CAstNodeBase* pKey = m_pKeyType;
        strName += pKey->ToStringCpp() + ",";
        CAstNodeBase* pElem = m_pElemType;
        strName += pElem->ToStringCpp();
        strName += ">";
        return strName;
    }
};

struct CStructRef : public CPrimeType
{
    typedef CPrimeType super;

    CStructRef() : super()
    { SetClassId( clsid( CStructRef ) ); }

    std::string m_strName;

    inline void SetName(
        const std::string& strName )
    { m_strName = strName; }

    inline const std::string& GetName() const
    { return m_strName; }

    gint32 GetStructType( ObjPtr& pType ) const
    {
        // this method is to get the struct_decl
        // if pType is already known to be a
        // struct_decl
        ObjPtr pTemp;
        gint32 ret = g_mapDecls.GetDeclNode(
            m_strName, pTemp );
        if( SUCCEEDED( ret ) )
        {
            pType = pTemp;
            return STATUS_SUCCESS;
        }

        ret = g_mapAliases.GetAliasType(
            m_strName, pTemp );

        if( SUCCEEDED( ret ) )
            pType = pTemp;

        return ret;
    }

    std::string GetSignature() const
    {
        ObjPtr pObj;
        pObj = ObjPtr( ( CObjBase* )this, true );
        return GetTypeSig( pObj );
    }
    std::string ToStringCpp() const
    {
        ObjPtr pTemp;
        if( g_mapDecls.IsDeclared( m_strName ) )
            return m_strName;

        ObjPtr pType;
        gint32 ret = g_mapAliases.GetAliasType(
            m_strName, pType );
        if( SUCCEEDED( ret ) )
            return m_strName;

        return std::string( "");
    }
};


struct CFieldDecl : public CNamedNode
{
    typedef CNamedNode super;
    ObjPtr m_pType;
    BufPtr m_pVal;

    CFieldDecl() : super()
    { SetClassId( clsid( CFieldDecl ) ); }

    void SetType( ObjPtr& pType )
    {
        m_pType = pType;
        SET_PARENT_OF( pType );
    }

    ObjPtr& GetType()
    { return m_pType; }

    inline void SetVal( BufPtr& pVal )
    { m_pVal = pVal; }

    inline BufPtr& GetVal()
    { return m_pVal; }

    inline std::string ToStringCpp() const
    { return ToStringCppRef( false ); }

    std::string ToStringCppRef( bool bRef ) const
    {
        std::string strRet;
        do{
            std::string strType, strName;
            std::string& strVal = strRet;
            CAstNodeBase* pType = m_pType;
            strVal = pType->ToStringCpp();

            if( bRef )
                strVal += "& ";
            else
                strVal += " ";

            strVal += GetName();
            if( m_pVal.IsEmpty() ||
                m_pVal->empty() )
                break;
            strVal += " = ";

            if( pType->GetClsid() !=
                clsid( CPrimeType ) )
                break;

            CPrimeType* ppt = m_pType;
            switch( ppt->GetName() )
            {
            case TOK_STRING:
                strVal += '"';
                strVal +=
                    ( const char*)m_pVal->ptr();
                strVal += '"';
                break;
            case TOK_UINT64:
                strVal += std::to_string(
                    *( guint64* )m_pVal->ptr() );
                break;
            case TOK_INT64:
                strVal += std::to_string(
                    *( gint64* )m_pVal->ptr() );
                break;
            case TOK_UINT32:
                strVal += std::to_string(
                    *( guint32* )m_pVal->ptr() );
                break;
            case TOK_INT32:
                strVal += std::to_string(
                    *( gint32* )m_pVal->ptr() );
                break;
            case TOK_UINT16:
                strVal += std::to_string(
                    *( guint32* )m_pVal->ptr() );
                break;
            case TOK_INT16:
                strVal += std::to_string(
                    *( gint32* )m_pVal->ptr() );
                break;
            case TOK_FLOAT:
                strVal += std::to_string(
                    *( float* )m_pVal->ptr() );
                break;
            case TOK_DOUBLE:
                strVal += std::to_string(
                    *( double* )m_pVal->ptr() );
                break;
            case TOK_BYTE:
                strVal += std::to_string(
                    *m_pVal->ptr() );
                break;
            case TOK_BOOL:
                {
                    bool bVal =
                        *( bool*)m_pVal->ptr();
                    if( bVal )
                        strVal += "true";
                    else
                        strVal += "false";
                    break;
                }
            default:
                break;
            }

        }while( 0 );

        return strRet;
    }
};

struct CFieldList : public CAstListNode
{
    typedef CAstListNode super;
    CFieldList() : super()
    { SetClassId( clsid( CFieldList ) ); }
};

struct CStructDecl : public CNamedNode
{
    typedef CNamedNode super;
    ObjPtr m_oFieldList;
    CCfgOpener m_oContext;
    guint32 m_dwRefCount = 0;

    CStructDecl() : super()
    { SetClassId( clsid( CStructDecl ) ); }

    void SetFieldList( ObjPtr& oFields )
    {
        m_oFieldList = oFields;
        SET_PARENT_OF( oFields );
    }

    ObjPtr& GetFieldList()
    { return m_oFieldList; }

    gint32 SetProperty(
        gint32 iProp, const BufPtr& val )
    {
        return m_oContext.SetProperty(
            iProp, val );
    }

    gint32 GetProperty(
        gint32 iProp, BufPtr& val ) const
    {
        return m_oContext.GetProperty(
            iProp, val );
    }

    guint32 AddRef()
    { return ++m_dwRefCount; }

    guint32 RefCount() const
    { return m_dwRefCount; }
};

struct CFormalArg : public CFieldDecl
{
    typedef CFieldDecl super;
    CFormalArg() : super()
    { SetClassId( clsid( CFormalArg ) ); }
};

struct CArgList : public CAstListNode
{
    typedef CAstListNode super;
    CArgList() : super()
    { SetClassId( clsid( CArgList ) ); }

    virtual gint32 InsertChild( ObjPtr& pChild )
    {
        do{
            CFormalArg* pnewfa = pChild;
            std::string strName = pnewfa->GetName();
            for( auto& elem : m_queChilds )
            {
                CFormalArg* pfa = elem;
                if( pfa == nullptr )
                    continue;
                if( pfa->GetName() == strName )
                    return -EEXIST;
            }

        }while( 0 );

        return super::InsertChild( pChild );
    }

    std::string ToStringCpp()
    {
        return ToStringCppRef( false );
    }

    // output args as lvalue references
    std::string ToStringCppRef( bool bRef )
    {
        std::string strRet;
        for( auto& elem : m_queChilds )
        {
            CFormalArg* pfa = elem;
            if( pfa == nullptr )
                continue;
            strRet += pfa->
                ToStringCppRef( bRef ) + ", ";
        }
        if( strRet.empty() )
            return strRet;

        strRet.erase( strRet.size() - 2, 2 );
        std::regex e ("HSTREAM");

        strRet = std::regex_replace(
            strRet, e, "HANDLE" );

        return strRet;
    }
};

struct CMethodDecl : public CNamedNode
{
    ObjPtr m_pInArgs;
    ObjPtr m_pOutArgs;
    ObjPtr m_pAttrList;

    bool m_bEvent = false;
    bool m_bAsyncp = false;
    bool m_bAsyncs = false;

    guint32 m_dwTimeoutSec = 0;
    guint32 m_dwKeepAliveSec = 0;

    CCfgOpener m_oContext;

    typedef CNamedNode super;

    CMethodDecl() : super()
    { SetClassId( clsid( CMethodDecl ) ); }

    void SetInArgs( ObjPtr& pInArgs )
    {
        m_pInArgs = pInArgs;
        SET_PARENT_OF( pInArgs );
    }
    ObjPtr& GetInArgs()
    { return m_pInArgs; }

    void SetOutArgs( ObjPtr& pOutArgs )
    {
        m_pOutArgs = pOutArgs;
        SET_PARENT_OF( pOutArgs );
    }
    ObjPtr& GetOutArgs()
    { return m_pOutArgs; }

    void SetAttrList( ObjPtr& pAttrList )
    {
        m_pAttrList = pAttrList;
        SET_PARENT_OF( pAttrList );
    }
    ObjPtr& GetAttrList()
    { return m_pAttrList; }

    gint32 CheckTimeoutSec() const
    {
        if( m_pAttrList.IsEmpty() )
            return 0;

        CAttrExps* pList = m_pAttrList;
        if( pList == nullptr )
            return 0;

        return pList->CheckTimeoutSec();
    }
    guint32 GetTimeoutSec() const
    {
        if( m_pAttrList.IsEmpty() )
            return 0;

        CAttrExps* pList = m_pAttrList;
        if( pList == nullptr )
            return 0;

        return pList->GetTimeoutSec();
    }

    guint32 GetAsyncFlags() const
    {
        if( m_pAttrList.IsEmpty() )
            return 0;

        CAttrExps* pList = m_pAttrList;
        if( pList == nullptr )
            return 0;

        return pList->GetAsyncFlags();
    }

    bool IsAsyncp() const
    {
        guint32 dwFlags = GetAsyncFlags();
        if( dwFlags & NODE_FLAG_ASYNCP )
            return true;
        return false;
    }

    bool IsAsyncs() const
    {
        guint32 dwFlags = GetAsyncFlags();
        if( dwFlags & NODE_FLAG_ASYNCS )
            return true;
        return false;
    }

    bool IsEvent() const
    {
        if( m_pAttrList.IsEmpty() )
            return false;

        CAttrExps* pList = m_pAttrList;
        if( pList == nullptr )
            return false;

        return pList->IsEvent();
    }

    bool IsNoReply() const
    {
        if( m_pAttrList.IsEmpty() )
            return false;

        CAttrExps* pList = m_pAttrList;
        if( pList == nullptr )
            return false;

        BufPtr pBuf;
        gint32 ret = pList->GetAttrByToken(
            TOK_NOREPLY, pBuf );
        if( ERROR( ret ) )
            return false;
        return true;
    }

    void SetAbstDecl(
        std::string strDecl, bool bProxy = true )
    {
        if( bProxy )
            m_oContext[ 0 ] = strDecl;
        else
            m_oContext[ 1 ] = strDecl;
    }

    std::string GetAbstDecl( bool bProxy = true ) const
    {
        std::string strDecl;
        if( bProxy )
        {
            m_oContext.GetStrProp(
                0, strDecl );
        }
        else
        {
            m_oContext.GetStrProp(
                1, strDecl );
        }
        return strDecl;
    }

    gint32 GetAbstFlag( guint32& dwFlags,
        bool bProxy = true ) const
    {
        if( bProxy )
            return m_oContext.GetIntProp(
                PROP_METHODFLAG_PROXY, dwFlags );

        return m_oContext.GetIntProp(
            PROP_METHODFLAG_SERVER, dwFlags );
    }

    gint32 SetAbstFlag( guint32 dwFlags,
        bool bProxy = true )
    {
        if( bProxy )
            return m_oContext.SetIntProp(
                PROP_METHODFLAG_PROXY, dwFlags );

        return m_oContext.SetIntProp(
            PROP_METHODFLAG_SERVER, dwFlags );
    }

    gint32 SetProperty(
        gint32 iProp, const BufPtr& val )
    {
        return m_oContext.SetProperty(
            iProp, val );
    }

    gint32 GetProperty(
        gint32 iProp, BufPtr& val ) const
    {
        return m_oContext.GetProperty(
            iProp, val );
    }
};

struct CMethodDecls : public CAstListNode
{
    typedef CAstListNode super;
    CMethodDecls() : super()
    { SetClassId( clsid( CMethodDecls ) ); }

    bool ExistMethod(
        const std::string& strName )
    {
        for( auto& elem : m_queChilds )
        {
            CMethodDecl* pNode = elem;
            if( pNode == nullptr )
                continue;
            if( pNode->GetName() == strName )
                return true;
        }
        return false;
    }
};

struct CInterfaceDecl : public CNamedNode
{
    typedef CNamedNode super;
    ObjPtr m_pMdl;
    guint32 m_dwRefCount = 0;

    CInterfaceDecl() : super()
    { SetClassId( clsid( CInterfaceDecl ) ); }

    void SetMethodList( ObjPtr& pMethods )
    {
        m_pMdl = pMethods;
        SET_PARENT_OF( pMethods );
    }

    ObjPtr& GetMethodList()
    { return m_pMdl; }

    guint32 AddRef()
    { return ++m_dwRefCount; }

    guint32 RefCount() const
    { return m_dwRefCount; }
};

struct CInterfRef : public CNamedNode
{
    typedef CNamedNode super;
    guint32 dwInheritNo = 0;

    CInterfRef() : super()
    { SetClassId( clsid( CInterfRef ) ); }

    gint32 GetIfDecl( ObjPtr& pIfDecl )
    {
        std::string strIfName = GetName();
        return g_mapDecls.GetDeclNode(
            strIfName, pIfDecl );
    }
};

struct CInterfRefs : public CAstListNode
{
    typedef CAstListNode super;
    CInterfRefs() : super()
    { SetClassId( clsid( CInterfRefs ) ); }

    guint32 GetStreamIfCount() const
    {
        if( IsStream() == false )
            return 0;
        guint32 dwCount = 0;
        for( auto& elem : m_queChilds )
        {
            CAstNodeBase* pNode = elem;
            if( pNode->IsStream() )
                dwCount++;
        }
        return dwCount;
    }

    gint32 GetStreamIfs(
        std::vector< ObjPtr >& vecIfs )
    {
        if( IsStream() == false )
            return -ENOENT;
        for( auto& elem : m_queChilds )
        {
            CAstNodeBase* pNode = elem;
            if( pNode->IsStream() )
                vecIfs.push_back( pNode );
        }
        return STATUS_SUCCESS;
    }

    gint32 GetIfRefs(
        std::vector< ObjPtr >& vecIfs )
    {
        for( auto& elem : m_queChilds )
            vecIfs.push_back( elem );

        return STATUS_SUCCESS;
    }

    bool ExistIf(
        const std::string& strName )
    {
        for( auto& elem : m_queChilds )
        {
            CInterfRef* pNode = elem;
            if( pNode == nullptr )
                continue;
            if( pNode->GetName() == strName )
                return true;
        }
        return false;
    }
};

struct CServiceDecl : public CInterfRef
{
    ObjPtr m_pInterfList;
    ObjPtr m_pAttrList;

    typedef CInterfRef super;
    CServiceDecl() : super()
    { SetClassId( clsid( CServiceDecl ) ); }

    void SetInterfList( ObjPtr& pInterfList )
    {
        m_pInterfList = pInterfList;
        SET_PARENT_OF( pInterfList );
    }
    ObjPtr& GetInterfList()
    { return m_pInterfList; }

    void SetAttrList( ObjPtr& pAttrList )
    {
        m_pAttrList = pAttrList;
        SET_PARENT_OF( pAttrList );
    }
    ObjPtr& GetAttrList()
    { return m_pAttrList; }

    gint32 GetStreamIfs(
        std::vector< ObjPtr >& vecIfs )
    {
        if( !IsStream() )
            return -ENOENT;

        CInterfRefs* pifrs = m_pInterfList;
        if( pifrs == nullptr )
            return -ENOENT;
        return pifrs->GetStreamIfs( vecIfs );
    }

    gint32 GetIfRefs(
        std::vector< ObjPtr >& vecIfs )
    {
        CInterfRefs* pifrs = m_pInterfList;
        if( pifrs == nullptr )
            return -ENOENT;
        return pifrs->GetIfRefs( vecIfs );
    }

    bool CheckBoolAttr( guint32 token )
    {
        bool bRet = false;
        do{
            CAttrExps* pal = m_pAttrList;
            if( pal == nullptr )
                break;

            BufPtr pBuf;
            gint32 ret = pal->
                GetAttrByToken( TOK_SSL, pBuf );
            if( ERROR( ret ) )
                break;

            bRet = true;

        }while( 0 );

        return bRet;
    }

    gint32 GetValueAttr(
        guint32 token, BufPtr& pBuf )
    {
        gint32 ret = 0;
        do{
            CAttrExps* pal = m_pAttrList;
            if( pal == nullptr )
            {
                ret = -ENOENT;
                break;
            }

            ret = pal->GetAttrByToken(
                token, pBuf );

            if( ERROR( ret ) )
                break;

        }while( 0 );

        return ret;
    }

    bool IsSSL()
    { return CheckBoolAttr( TOK_SSL ); }

    bool IsWebSocket()
    { return CheckBoolAttr( TOK_WEBSOCK ); }

    gint32 GetAuthVal( BufPtr& pAuth )
    {
        gint32 ret = 0;
        do{
            ret = GetValueAttr(
                TOK_AUTH, pAuth );
            if( ERROR( ret ) )
                break;

            if( pAuth.IsEmpty() || pAuth->empty() )
            {
                ret = -ENOENT;
                break;
            }

            if( pAuth->GetExDataType() !=
                typeString )
            {
                ret = -EINVAL;
                break;
            }

        }while( 0 );

        return ret;
    }

    bool IsCompress()
    { return CheckBoolAttr( TOK_COMPRES ); }

    gint32 GetRouterPath( std::string& strRtPath )
    {
        BufPtr pBuf;
        gint32 ret = 0;
        do{
            ret = GetValueAttr(
                TOK_RTPATH, pBuf );
            if( ERROR( ret ) )
                break;

            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -ENOENT;
                break;
            }

            if( pBuf->GetExDataType() !=
                typeString )
            {
                ret = -EINVAL;
                break;
            }

            strRtPath = *pBuf;

        }while( 0 );

        return ret;
    }

    gint32 GetIpAddr( std::string& strIpAddr )
    {
        BufPtr pBuf;
        gint32 ret = 0;
        do{
            ret = GetValueAttr(
                TOK_IPADDR, pBuf );
            if( ERROR( ret ) )
                break;

            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -ENOENT;
                break;
            }

            if( pBuf->GetExDataType() !=
                typeString )
            {
                ret = -EINVAL;
                break;
            }

            strIpAddr = *pBuf;

        }while( 0 );

        return ret;
    }

    gint32 GetTimeoutSec( guint32& dwTimeout )
    {
        BufPtr pBuf;
        gint32 ret = 0;
        do{
            ret = GetValueAttr(
                TOK_TIMEOUT, pBuf );
            if( ERROR( ret ) )
                break;

            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -ENOENT;
                break;
            }

            if( pBuf->GetExDataType() !=
                typeUInt32 )
            {
                ret = -EINVAL;
                break;
            }

            dwTimeout = *pBuf;

        }while( 0 );

        return ret;
    }

    gint32 GetPortNum( guint32& dwPort )
    {
        BufPtr pBuf;
        gint32 ret = 0;
        do{
            ret = GetValueAttr(
                TOK_PORTNUM, pBuf );
            if( ERROR( ret ) )
                break;

            if( pBuf.IsEmpty() || pBuf->empty() )
            {
                ret = -ENOENT;
                break;
            }

            if( pBuf->GetExDataType() !=
                typeUInt32 )
            {
                ret = -EINVAL;
                break;
            }

            dwPort = *pBuf;

        }while( 0 );

        return ret;
    }
};

struct CAppName : public CNamedNode
{
    typedef CNamedNode super;
    CAppName() : super()
    { SetClassId( clsid( CAppName ) ); }
};

struct CAliasList : public CAstNodeBase
{
    std::deque< std::string > m_queChilds;
    typedef CAstNodeBase super;
    CAliasList() : super()
    { SetClassId( clsid( CAliasList ) ); }

    inline void AddChild( const std::string& strAlias )
    {
        if( !strAlias.empty() )
            m_queChilds.push_back( strAlias );
    }

    inline const std::string GetChild( guint32 i ) const
    {
        if( i >= m_queChilds.size() )
            return "";
        return m_queChilds[ i ];
    }

    guint32 GetCount() const
    { return m_queChilds.size(); }

    void InsertChild( const std::string& strAlias )
    {
        if( !strAlias.empty() )
            m_queChilds.push_front( strAlias );
    }

    std::string ToStringCpp() const
    {
        std::string strVal;
        for( auto& elem : m_queChilds )
            strVal += elem + ", ";
        if( strVal.empty() ) 
            return strVal;
        strVal.erase( strVal.size() - 2, 2 );
        return strVal;
    }
};

struct CTypedefDecl : public CAstNodeBase
{
    typedef CAstNodeBase super;
    ObjPtr m_pType;
    ObjPtr m_pAliasList;
    CTypedefDecl() : super()
    { SetClassId( clsid( CTypedefDecl ) ); }

    void SetType( ObjPtr& pType )
    {
        m_pType = pType;
        SET_PARENT_OF( pType );
    }
    ObjPtr& GetType()
    { return m_pType; }

    void SetAliasList( ObjPtr& pAliasList )
    {
        m_pAliasList = pAliasList;
        SET_PARENT_OF( pAliasList );
    }
    ObjPtr& GetAliasList()
    { return m_pAliasList; }

    std::string ToStringCpp() const
    {
        CAstNodeBase* pType = m_pType;
        std::string strType =
            pType->ToStringCpp();

        CAliasList* pAliases = m_pAliasList;
        std::string strVal =
            "typedef " + strType +
            " " + pAliases->ToStringCpp() +
            ";";
        return strVal;
    }
};

struct CStatements : public CAstListNode
{
    typedef CAstListNode super;
    CStatements() : CAstListNode()
    { SetClassId( clsid( CStatements ) ); }

    std::string m_strName;    

    inline void SetName(
        const std::string& strName )
    { m_strName = strName; }

    inline const std::string& GetName() const
    { return m_strName; }

    gint32 CheckAppName()
    {
        for( auto elem : m_queChilds )
        {
            if( elem->GetClsid() !=
                clsid( CAppName ) )
                continue;
            CAppName* pAppName = elem;
            if( pAppName == nullptr )
                continue;

            SetName( pAppName->GetName() );
            break;
        }
        if( GetName().empty() )
            return -ENOENT;

        return STATUS_SUCCESS;
    }

    gint32 GetStmtsByType( guint32 dwClsid,
        std::vector< ObjPtr >& vecStmts )
    {
        for( auto elem : m_queChilds )
        {
            if( elem->GetClsid() != dwClsid )
                continue;
            vecStmts.push_back( elem );
        }

        if( vecStmts.empty() )
            return -ENOENT;

        return STATUS_SUCCESS;
    }

    gint32 GetSvcDecls(
        std::vector< ObjPtr >& vecSvcs )
    {
        return GetStmtsByType(
            clsid( CServiceDecl ),
            vecSvcs );
    }

    gint32 GetIfDecls(
        std::vector< ObjPtr >& vecIfs )
    {
        return GetStmtsByType(
            clsid( CInterfaceDecl ),
            vecIfs );
    }

    gint32 GetStructDecls(
        std::vector< ObjPtr >& vecStructs )
    {
        return GetStmtsByType(
            clsid( CStructDecl ),
            vecStructs );
    }

    bool IsStreamNeeded()
    {
        std::vector< ObjPtr > vecSvcs;
        gint32 ret = GetSvcDecls( vecSvcs );
        if( ERROR( ret ) )
            return false;

        for( auto& elem : vecSvcs )
        {
            CServiceDecl* psd = elem;
            if( psd == nullptr )
                continue;
            if( psd->IsStream() )
                return true;
        }
        return false;
    }
};

