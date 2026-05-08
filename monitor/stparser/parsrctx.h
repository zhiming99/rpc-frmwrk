/*
 * =====================================================================================
 *
 *       Filename:  parsrctx.h
 *
 *    Description:  declarations of classes for parser context
 *
 *        Version:  1.0
 *        Created:  05/08/2026 06:28:51 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:
 *
 *      Copyright:  2026 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  Licensed under GPL-3.0. You may not use this file except in
 *                  compliance with the License. You may find a copy of the
 *                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */

#pragma once

#include <vector>
#include <string>
#include <memory>

#define CLEAR_RSYMBS \
for( int i = 0; i < ( yylen ); i++ ) \
    { yyvsp[ -i ].Clear(); }

#define DEFAULT_ACTION \
    yyval = yyvsp[ 1 - yylen ]; CLEAR_RSYMBS;

#include "rpc.h"

#define YYPUSH_ACCEPT       0 // Success / Input Accepted
#define YYPUSH_ABORT        1 // Need more tokens
#define YYPUSH_NOMEM        2 // Syntax Error

#define CONFLICT_STATE 297

#define GetParserState( ps ) ( ps->yystate )

typedef enum
{
    invalidProgress = -1;
    scanning = 0,
    building,
    leaving,
} enumCPProgress;

typedef enum
{
    firstBlock,
    elsifBlock,
    elseBlock,
    endifBlock,
} enumBlkType;

struct FILECTX2
{
    FILE*       m_fp = nullptr;
    rpcf::Variant     m_oVal;
    std::string m_strPath;
    std::string m_strLastVal;
    YYLTYPE2    m_oLocation;

    FILECTX2();
    FILECTX2( const std::string& strPath );
    FILECTX2( const FILECTX2& fc );
    ~FILECTX2();
};

struct YYLTYPE2 :
    public YYLTYPE
{
    void initialize(
        const char* szFileName ) 
    {
        first_line = 1;
        first_column = 1;
        last_line = 1;
        last_column = 1;
    }
};

struct CondPragmaState
{
    enumCPProgress m_iProgress = scanning;
    enumBlkType m_iBlkType = firstBlock;
    gint32 m_iBlkIdx = 0;
    std::vector< YYLTYPE2 > m_vecBlockPos;

    // const expr value of m_iBlkIdx if m_iProgress is scanning
    bool m_bExprValue = false; 

    // conditional depth in the block m_iBlkIdx
    gint32 m_iCondDepth = 0;
};

struct CSTParserContext
{
    std::vector< CondPragmaState > m_vecCondStack;
    // the last pragma representing current non-main ps is parsing.
    gint32 m_iLastPragma = nt_invalid;
    gint32 IsCondStackEmpty() const
    { return m_vecCondStack.empty(); }

    std::vector< stdstr > m_vecInclPaths;
    std::vector< std::unique_ptr< FILECTX2 > > m_vecInclStack;

    gint32 GetCondStackSize() const
    { return m_vecCondStack.size(); }

    inline const CondPragmaState& GetTopState() const
    {
        static const CondPragmaState oInvalidState =
            { invalidProgress, 0, 0 };
        if( m_vecCondStack.empty() )
            return oInvalidState;
        return m_vecCondStack.back();
    }

    inline CondPragmaState& GetTopState()
    {
        static CondPragmaState oInvalidState =
            { invalidProgress, 0, 0 };
        if( m_vecCondStack.empty() )
            return oInvalidState;
        return m_vecCondStack.back();
    }

    inline void PushState(
        const CondPragmaState& cps )
    { m_vecCondStack.push_back( cps ); }

    inline void PopState()
    { m_vecCondStack.pop_back(); }

    inline gint32 GetLastPragma() const
    { return m_iLastPragma; }

    inline void SetLastPragma( gint32 iPragma )
    { m_iLastPragma = iPragma; }

    FILE* TryOpenFile(
        const std::string& strFile );
}

extern std::shared_ptr< CSTParserContext > g_pParserCtx;

inline CSTParserContext* GetParserCtx()
{ return g_pParserCtx.get(); }

