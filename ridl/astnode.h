#include <string>
#include <map>
#include "ridl.tab.h++"
#include "rpc.h"

struct CAstNodeBase :
    public CObjBase
{
    CAstNodeBase() {} 
    gint32 m_iType;
    ObjPtr m_pSibling;
    CAstNodeBase* m_pParent;
    std::vector< ObjPtr > m_queChilds;

    inline void SetSibling( ObjPtr& pSib )
    { m_pSibling = pSib; }

    inline void AddChild( ObjPtr& pChild )
    {
        if( !pChild.IsEmpty() )
        {
            m_queChilds.push_back( pChild );
            CAstNodeBase* pNode = pChild;
            pNode->SetParent( this );
        }
    }

    inline ObjPtr GetChild( guint32 i )
    {
        if( i >= m_queChilds.size() )
            return ObjPtr();
        return m_queChilds[ i ];
    }

    inline void SetParent(
        CAstNodeBase* pParent )
    {
        if( pParent != nullptr )
            m_pParent = pParent;
    }
};

typedef CAutoPtr< clsid( Invalid ), CAstNodeBase > NodePtr;

struct CAttrExp : public CAstNodeBase
{
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
}

struct CAttrExps : public CAstNodeBase
{
    CAttrExps() : CAstNodeBase()
    { SetClassId( clsid( CAttrExps ) ); }

    gint32 GetCount()
    { return m_queChilds.size(); }

    void InsertChild( ObjPtr& pChild )
    {
        if( !pChild.IsEmpty() )
            m_queChilds.push_front( pChild );
    }
}
