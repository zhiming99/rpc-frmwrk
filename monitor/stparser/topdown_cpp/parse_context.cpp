/*
 * =====================================================================================
 *
 *       Filename:  parse_context.cpp
 *
 *    Description:  implementations of classes for parser context
 *
 *        Version:  1.0
 *        Created:  10/06/2026 10:11:37 PM
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

#include "parse_context.h"

bool CStTokenPipeline::InjectIncludeStream(
    const std::string& strFilePath,
    antlr4::Token* pOriginalIncludeToken )
{
    gint32 iFileIdx = -1;

    // Cyclic verification
    if( m_pIncludeMgr->IsCyclicInclude(
        strFilePath, iFileIdx ) )
    {
        std::cerr << "CSt Compiler Error: Cyclic "
            << "include detected on file: " 
            << strFilePath << "\n";
        return false;
    }

    size_t nInsertIndex = 0;
    if( pOriginalIncludeToken != nullptr )
    {
        // Search our adapter vector to find
        // exactly where the include pragma token
        // sits right now
        auto it = std::find_if(
            m_vecMasterTokens.begin(),
            m_vecMasterTokens.end(),
            [pOriginalIncludeToken](const CStToken& wrappedTok) {
                return wrappedTok.m_pOriginalToken ==
                    pOriginalIncludeToken;
            });

        if( it != m_vecMasterTokens.end() )
        {
            // Calculate the exact distance offset
            // to locate our master splice
            // position
            nInsertIndex = std::distance(
                m_vecMasterTokens.begin(), it);
        }
        else
        {
            // Fallback: If not found, append
            // safely at the current master tail
            nInsertIndex = m_vecMasterTokens.size();
        }
    }
    else
    {
        // If the token is null, we are processing
        // the primary root file initialization
        // pass
        nInsertIndex = m_vecMasterTokens.size();
    }

    if (iFileIdx == -1)
        iFileIdx = m_pIncludeMgr->RegisterNewFile(
            strFilePath );

    m_pIncludeMgr->PushFile(iFileIdx);

    // Standard ANTLR file stream payload
    // extraction pass
    std::ifstream fileStream(strFilePath);
    if (!fileStream.is_open())
    {
        std::cerr << "CSt Compiler Error: Unable "
            << "to open file stream: "
            << strFilePath << "\n";
        m_pIncludeMgr->PopFile();
        return false;
    }

    antlr4::ANTLRInputStream input(fileStream);
    CStLexer lexer(&input);
    antlr4::CommonTokenStream tokenStream(&lexer);
    tokenStream.fill();

    std::vector<antlr4::Token*> vecRawTokens =
        tokenStream.getTokens();

    std::vector<CStToken> vecFileTokens;
    vecFileTokens.reserve( vecRawTokens.size() + 1 );

    size_t stFileLastLine = 1;

    for( auto* pTok : vecRawTokens )
    {
        if (pTok->getType() == antlr4::Token::EOF)
            continue;

        CStToken cstTok;

        cstTok.m_pOriginalToken = pTok;

        cstTok.m_iFileIdx = iFileIdx;

        cstTok.m_stOrigLine = pTok->getLine();

        cstTok.m_stCharPos =
            pTok->getCharPositionInLine();

        cstTok.m_stVirtualLine =
            m_stGlobalLineCounter + (pTok->getLine() - 1);

        stFileLastLine = pTok->getLine();

        vecFileTokens.push_back(cstTok);
    }

    m_stGlobalLineCounter += stFileLastLine;

    // Append your cloned endmarker pragma token
    // directly behind this file's payload
    if (pOriginalIncludeToken != nullptr)
    {
        CStToken endMarkerTok;
        endMarkerTok.m_pOriginalToken =
            pOriginalIncludeToken;

        endMarkerTok.m_iFileIdx = iFileIdx;
        endMarkerTok.m_stOrigLine =
            pOriginalIncludeToken->getLine();

        endMarkerTok.m_stCharPos =
            pOriginalIncludeToken->getCharPositionInLine();

        endMarkerTok.m_stVirtualLine =
            m_stGlobalLineCounter;

        endMarkerTok.m_eVirtualType =
            ECStVirtualTokenType::EndIncludeMarker;

        endMarkerTok.m_strMarkerPath =
            "{endincl '" + strFilePath + "'}";

        vecFileTokens.push_back(endMarkerTok);
    }

    // Safely insert all wrapped tokens inline
    // directly ahead of the include token's spot
    m_vecMasterTokens.insert(
        m_vecMasterTokens.begin() + nInsertIndex,
        vecFileTokens.begin(),
        vecFileTokens.end()
    );

    return true;
}


antlr4::Token* CStPragmaFilteringTokenStream::LT( ssize_t k)
{
    if( m_pPipeline == nullptr || k == 0 )
        return nullptr;

    ssize_t sstTargetIdx = 0;
    if( k > 0 )
    {
        sstTargetIdx = static_cast<ssize_t>
            ( m_stCurrentIndex ) + k - 1;
    }
    else
    {
        sstTargetIdx = static_cast<ssize_t>
            ( m_stCurrentIndex ) + k;
    }

    // Safety fallback checks against master
    // vector boundary limits
    if( sstTargetIdx < 0 ||
        sstTargetIdx >= static_cast<ssize_t>(
            m_pPipeline->m_vecMasterTokens.size() ) )
    {
        // Fall back to returning standard global
        // pipeline EOF tokens context
        return m_pPipeline->m_pEofToken.get();
    }

    auto& a = m_pPipeline->m_vecMasterTokens;
    return a[sstTargetIdx].m_pOriginalToken;
}

size_t CStPragmaFilteringTokenStream::LA( ssize_t k )
{
    antlr4::Token* pTok = LT(k);
    if (pTok != nullptr)
    {
        return pTok->getType();
    }
    return antlr4::Token::INVALID_TYPE;
}

void CStPragmaFilteringTokenStream::consume()
{
    if( m_pPipeline != nullptr &&
        m_stCurrentIndex <
            m_pPipeline->m_vecMasterTokens.size() )
    {
        m_stCurrentIndex++;
    }
}

antlr4::Token*
CStPragmaFilteringTokenStream::get( size_t index )
{
    if( m_pPipeline == nullptr ||
        index >= m_pPipeline->m_vecMasterTokens.size() )
    {
        return m_pPipeline->m_pEofToken.get();
    }

    auto& a = m_pPipeline->m_vecMasterTokens;
    return a[index].m_pOriginalToken;
}

size_t CStPragmaFilteringTokenStream::size()
{
    return m_pPipeline != nullptr ?
        m_pPipeline->m_vecMasterTokens.size() : 0;
}

size_t CStPragmaFilteringTokenStream::index()
{
    return m_stCurrentIndex;
}

void CStPragmaFilteringTokenStream::seek(
    size_t index)
{
    // Instantly snaps index cursors to historical
    // parser checkpoints / rewinds
    m_stCurrentIndex = index;
}

antlr4::TokenSource*
CStPragmaFilteringTokenStream::getTokenSource()
{
    if( m_pPipeline != nullptr )
    {
        return m_pPipeline->m_pRootTokenSource;
    }
    return nullptr;
}

std::string
CStPragmaFilteringTokenStream::getText()
{
    if( m_pPipeline == nullptr ||
        m_pPipeline->m_vecMasterTokens.empty() )
        return "";
    auto& a = m_pPipeline->m_vecMasterTokens;
    return getText( a.front().m_pOriginalToken,
                   a.back().m_pOriginalToken);
}

std::string CStPragmaFilteringTokenStream::getText(
    antlr4::RuleContext* ctx)
{
    if( ctx == nullptr )
        return "";
    return getText(ctx->start, ctx->stop);
}

std::string CStPragmaFilteringTokenStream::getText(
    const antlr4::misc::Interval& interval)
{
    if( m_pPipeline == nullptr ||
        m_pPipeline->m_vecMasterTokens.empty())
       return "";
    auto& vecMaster = m_pPipeline->m_vecMasterTokens;
    size_t stStart = static_cast<size_t>(interval.a);
    size_t stStop  = static_cast<size_t>(interval.b);

    if( stStart >= vecMaster.size() )
        return "";
    if( stStop >= vecMaster.size() )
        stStop = vecMaster.size() - 1;

    return getText(vecMaster[stStart].m_pOriginalToken,
        vecMaster[stStop].m_pOriginalToken);
}

std::string CStPragmaFilteringTokenStream::getText(
    antlr4::Token* start,
    antlr4::Token* stop)
{
    if( start == nullptr ||
        stop == nullptr ||
        m_pPipeline == nullptr )
        return "";

    size_t stStartIdx = start->getTokenIndex();
    size_t stStopIdx  = stop->getTokenIndex();

    // High-Precision Bridge: If capturing our
    // custom text-swapped virtual {endincl}
    // pragma, intercept and return our rewritten
    // string payload instantly instead of
    // original token buffer text.
    auto& vecMaster = m_pPipeline->m_vecMasterTokens;
    if (stStartIdx == stStopIdx &&
        stStartIdx < vecMaster.size())
    {
        const CStToken& oCStTok = vecMaster[stStartIdx];
        if( oCStTok.m_eVirtualType ==
            ECStVirtualTokenType::EndIncludeMarker )
        {
            // Returns "{endincl 'path/to/file.st'}"
            return oCStTok.m_strMarkerPath;
        }
    }

    std::string strResult = "";
    for( size_t i = stStartIdx;
        i <= stStopIdx && i < vecMaster.size(); ++i )
    {
        const CStToken& oCStTok = vecMaster[i];
        if( oCStTok.m_eVirtualType ==
            ECStVirtualTokenType::EndIncludeMarker )
        {
            strResult += oCStTok.m_strMarkerPath;
        }
        else if (oCStTok.m_pOriginalToken != nullptr)
        {
            strResult +=
                oCStTok.m_pOriginalToken->getText();
        }
    }
    return strResult;
}
