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

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include "antlr4-runtime.h"
#include "stlexer.h"
#include "stparser.h"
#include "parse_context.h"

std::string CStIncludeManager::ResolveIncludePath(
    const std::string& strPath ) const
{
    namespace fs = std::filesystem;
    
    if( strPath.empty() )
        return std::string( "" );

    fs::path pathTarget( strPath );

    // Verify absolute paths instantly
    if( pathTarget.is_absolute() )
    {
        if( fs::exists( pathTarget ) )
            return fs::canonical( 
                pathTarget ).string();
        return std::string( "" );
    }

    // 1. Resolve relative to active file dir
    if( !m_vecIncludeStack.empty() )
    {
        gint32 iActiveIdx = 
            m_vecIncludeStack.back();
            
        if( iActiveIdx >= 0 && 
            iActiveIdx < static_cast<gint32>(
                m_vecFileRegistry.size()) )
        {
            fs::path pathCurrentFile( 
                m_vecFileRegistry[iActiveIdx]
                    .m_strAbsolutePath );
                    
            fs::path pathParentDir = 
                pathCurrentFile.parent_path();
                
            fs::path pathCombined = 
                pathParentDir / pathTarget;

            if( fs::exists( pathCombined ) )
            {
                return fs::canonical( 
                    pathCombined ).string();
            }
        }
    }

    // 2. Iterate registered -I flags
    for( const auto& strSearchDir : m_vecSearchPaths )
    {
        if( strSearchDir.empty() )
            continue;

        fs::path pathBase( strSearchDir );
        fs::path pathCombined = 
            pathBase / pathTarget;

        if( fs::exists( pathCombined ) )
            return fs::canonical( 
                pathCombined ).string();
    }

    // 3. Check current execution working dir
    if( fs::exists( pathTarget ) )
    {
        return fs::canonical( 
            pathTarget ).string();
    }

    return std::string( "" );
}

bool CStIncludeManager::IsCyclicInclude(
    const std::string& strAbsolutePath,
    int32_t& outExistingIdx)
{
    namespace fs = std::filesystem;
    if (!fs::exists(strAbsolutePath))
        return false;

    std::string strCanonical =
        fs::canonical(strAbsolutePath).string();

    // Find if it already exists in the registry
    int32_t iFoundIdx = -1;
    for (const auto& fileInfo : m_vecFileRegistry)
    {
        if (fileInfo.m_strAbsolutePath == strCanonical)
        {
            iFoundIdx = fileInfo.m_iFileIdx;
            break;
        }
    }

    if (iFoundIdx != -1)
    {
        outExistingIdx = iFoundIdx;
        // If the registered index is currently
        // sitting inside the active stack, it's a
        // loop!
        auto it = std::find(
            m_vecIncludeStack.begin(),
            m_vecIncludeStack.end(),
            iFoundIdx);
        return (it != m_vecIncludeStack.end());
    }

    outExistingIdx = -1;
    return false;
}

int32_t CStIncludeManager::RegisterNewFile(
    const std::string& strAbsolutePath)
{
    namespace fs = std::filesystem;
    CStFileInfo info;
    info.m_strAbsolutePath =
        fs::canonical(strAbsolutePath).string();

    info.m_iFileIdx =
        static_cast<int32_t>(m_vecFileRegistry.size());

    m_vecFileRegistry.push_back(info);
    return info.m_iFileIdx;
}

bool CStTokenPipeline::InjectIncludeStream(
    const std::string& strFilePath,
    antlr4::Token* pOriginalIncludeToken,
    size_t nInsertIndex )
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
    stlexer lexer(&input);
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

        endMarkerTok.m_iFileIdx =
            m_pIncludeMgr->GetParentFileIdx();

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

// --- ANTLR4 Core Stream Interface ---

antlr4::Token*
CStPragmaFilteringTokenStream::LT( ssize_t k)
{
    if( m_pPipeline == nullptr || 
        k == 0 ) 
    {
        return nullptr;
    }

    ssize_t sstTargetIdx = 0;
    if( k > 0 )
    {
        sstTargetIdx = static_cast<ssize_t>
            ( m_stCurrentIndex) + k - 1;
    }
    else
    {
        sstTargetIdx = static_cast<ssize_t>
            ( m_stCurrentIndex) + k;
    }

    if( sstTargetIdx < 0 || 
        sstTargetIdx >= static_cast<ssize_t>
        ( m_pPipeline->m_vecMasterTokens.size() ) )
    {
        return m_pPipeline->m_pEofToken.get(); 
    }

    auto& vecMaster = m_pPipeline->m_vecMasterTokens;
    return vecMaster[sstTargetIdx].m_pOriginalToken;
}

size_t CStPragmaFilteringTokenStream::LA(
    ssize_t k)
{
    antlr4::Token* pTok = LT(k);
    if( pTok != nullptr )
    {
        return pTok->getType();
    }
    return antlr4::Token::INVALID_TYPE;
}

void CStPragmaFilteringTokenStream::consume()
{
    if( m_pPipeline != nullptr && 
        m_stCurrentIndex < m_pPipeline->
            m_vecMasterTokens.size() )
    {
        m_stCurrentIndex++;
    }
}

antlr4::Token* CStPragmaFilteringTokenStream::get(
    size_t index) const
{
    if( m_pPipeline == nullptr || 
        index >= m_pPipeline->
            m_vecMasterTokens.size() )
    {
        return m_pPipeline->
            m_pEofToken.get();
    }
    return m_pPipeline->
        m_vecMasterTokens[index]
            .m_pOriginalToken;
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
    m_stCurrentIndex = index;
}

antlr4::TokenSource* 
CStPragmaFilteringTokenStream::getTokenSource() const
{
    if( m_pPipeline != nullptr )
        return m_pPipeline->m_pRootTokenSource;
    return nullptr;
}

// --- Range Text Extractions ---

std::string CStPragmaFilteringTokenStream::getText()
{
    if( m_pPipeline == nullptr || 
        m_pPipeline-> m_vecMasterTokens.empty() ) 
        return "";

    auto& vecMaster = 
        m_pPipeline->m_vecMasterTokens;

    return getText(
        vecMaster.front().m_pOriginalToken, 
        vecMaster.back().m_pOriginalToken);
}

std::string CStPragmaFilteringTokenStream::getText(
    antlr4::RuleContext* ctx)
{
    if( ctx == nullptr )
        return "";

    auto* pParserCtx = dynamic_cast<
        antlr4::ParserRuleContext*>( ctx );
        
    if( pParserCtx == nullptr )
        return "";

    return getText( 
        pParserCtx->start, pParserCtx->stop );
}

std::string CStPragmaFilteringTokenStream::getText(
    const antlr4::misc::Interval& interval)
{
    if( m_pPipeline == nullptr || 
        m_pPipeline->
            m_vecMasterTokens.empty() ) 
    {
        return "";
    }
    size_t stStart = 
        static_cast<size_t>(interval.a);
    size_t stStop  = 
        static_cast<size_t>(interval.b);
    
    auto& vecMaster = 
        m_pPipeline->m_vecMasterTokens;
    if( stStart >= vecMaster.size() ) 
        return "";

    if( stStop >= vecMaster.size() ) 
    {
        stStop = vecMaster.size() - 1;
    }
    
    return getText(
        vecMaster[stStart].m_pOriginalToken, 
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

    // Safe direct pointer distance check 
    // to bypass file token index shifts
    auto& vecMaster = 
        m_pPipeline->m_vecMasterTokens;
        
    auto itStart = std::find_if(
        vecMaster.begin(), vecMaster.end(),
        [start](const CStToken& wrapped) {
            return wrapped.m_pOriginalToken 
                   == start;
        });
        
    auto itStop = std::find_if(
        vecMaster.begin(), vecMaster.end(),
        [stop](const CStToken& wrapped) {
            return wrapped.m_pOriginalToken == stop;
        });

    if( itStart == vecMaster.end() || 
        itStop == vecMaster.end() || 
        itStart > itStop )
        return "";

    std::string strResult = "";
    for( auto it = itStart; it <= itStop; ++it )
    {
        if( it->m_eVirtualType == 
            ECStVirtualTokenType:: EndIncludeMarker )
        {
            strResult += it->m_strMarkerPath;
        }
        else if( it->m_pOriginalToken != nullptr )
        {
            strResult +=
                it->m_pOriginalToken->getText();
        }
    }
    return strResult;
}

// Establishes a speculative lookahead 
// checkpoint and returns a unique marker ID
ssize_t CStPragmaFilteringTokenStream::mark()
{
    // ANTLR4 expects a unique index tracking 
    // marker. Returning the current absolute 
    // stream index acts as a perfect marker ID.
    return static_cast<ssize_t>(
        m_stCurrentIndex );
}

// Releases a previously allocated checkpoint
void CStPragmaFilteringTokenStream::
    release( ssize_t marker )
{
    // Because our vector buffer is stable 
    // and managed inside m_vecMasterTokens, 
    // no physical allocation occurs during 
    // mark(). This can be a safe no-op block.
    (void)marker; 
}

// Returns the active source stream identifier 
// text for diagnostic compilers log formatters
std::string CStPragmaFilteringTokenStream::
    getSourceName() const
{
    std::string strDefault(
        "CStVirtualEngineStream" );

    if( m_pPipeline == nullptr )
        return strDefault;

    auto& ts = 
        m_pPipeline->m_pRootTokenSource;

    if( ts != nullptr )
        return ts->getSourceName();
    
    return strDefault;
}

