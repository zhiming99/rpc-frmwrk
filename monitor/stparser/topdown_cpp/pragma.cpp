/*
 * =====================================================================================
 *
 *       Filename:  pragma.cpp
 *
 *    Description:  implementation of pragma related methods 
 *
 *        Version:  1.0
 *        Created:  09/27/2026 05:49:10 PM
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
#include <regex>
#include <filesystem>
#include "antlr4-runtime.h"
#include "stlexer.h"
#include "stparser.h"
#include "parse_context.h"
#include "pragma.h"
#include "pragma_expr/pragma_exprParser.h"
#include "pragma_expr/pragma_exprLexer.h"

std::any CStPragmaEvaluator::visitPragma_condition(
    pragma_exprParser::Pragma_conditionContext* ctx)
{
    if (ctx->expr() != nullptr)
        return visit(ctx->expr());
    return false;
}

std::any CStPragmaEvaluator::visitExpr(
    pragma_exprParser::ExprContext* ctx)
{
    // Handle lower precedence operations
    if( ctx->and_expr() != nullptr &&
        ctx->expr() == nullptr )
        return visit(ctx->and_expr());

    bool bLeft = std::any_cast<bool>(
        visit(ctx->expr()));

    bool bRight = std::any_cast<bool>(
        visit(ctx->and_expr()));

    if (ctx->TOK_OR() != nullptr)
        return bLeft || bRight;
    if (ctx->TOK_XOR() != nullptr)
        return bLeft != bRight;

    return false;
}

std::any CStPragmaEvaluator::visitAnd_expr(
    pragma_exprParser::And_exprContext* ctx)
{
    if( ctx->primary_expr() != nullptr &&
        ctx->and_expr() == nullptr )
        return visit(ctx->primary_expr());

    bool bLeft = std::any_cast<bool>(
        visit(ctx->and_expr()));
    bool bRight = std::any_cast<bool>(
        visit(ctx->primary_expr()));

    return bLeft && bRight;
}

std::any CStPragmaEvaluator::visitPrimary_expr(
    pragma_exprParser::Primary_exprContext* ctx)
{
    if (m_pContext == nullptr)
        return false;

    // Unary NOT operation
    if (ctx->TOK_NOT() != nullptr)
    {
        bool bVal = std::any_cast<bool>(
            visit(ctx->primary_expr()));
        return !bVal;
    }

    // Parentheses grouping
    if (ctx->expr() != nullptr)
        return visit(ctx->expr());

    // HASVALUE(target, val) -> Checks Symbol Tables,
    // not macro flags
    if (ctx->TOK_HASVALUE() != nullptr)
    {
        // First identifier is the target variable
        // name, second is the expected value
        std::string strTarget =
            ctx->TOK_IDENTIFIER(0)->getText();

        std::string strVal    =
            ctx->TOK_IDENTIFIER(1)->getText();

        // TODO: Query your compiler's live symbol
        // table environment here
        // return m_pSymbolTable->CheckVariableValue(
        //    strTarget, strVal);
        return false;
    }

    // HASATTRIBUTE(target, attr) -> Queries custom
    // metadata/reflection tags
    if (ctx->TOK_HASATTRIBUTE() != nullptr)
    {
        // First identifier is the target variable/FB,
        // second is the attribute string
        std::string strTarget =
            ctx->TOK_IDENTIFIER(0)->getText();

        std::string strAttr   =
            ctx->TOK_IDENTIFIER(1)->getText();

        // TODO: Query variable metadata
        // attributes or reflection properties
        // return m_pSymbolTable->HasVariableAttribute(
        //     strTarget, strAttr);
        return false;
    }

    // HASCONSTANT(target) -> Checks if identifier is a
    // known constant/literal macro
    if (ctx->TOK_HASCONSTANT() != nullptr)
    {
        std::string strTarget =
            ctx->TOK_IDENTIFIER(0)->getText();

        // Checks if it is a command line macro or a
        // parsed ST constant
        if (m_pContext->GetMacros().count(strTarget) > 0)
            return true;

        // return
        // m_pSymbolTable->IsCompileTimeConstant(strTarget);
        return false;
    }

    // Standalone identifier mapping or DEFINED logic
    if (ctx->TOK_IDENTIFIER(0) != nullptr)
    {
        std::string strId =
            ctx->TOK_IDENTIFIER(0)->getText();

        auto it = m_pContext->GetMacros().find(strId);
        if (it != m_pContext->GetMacros().end())
            return it->second != "0" && !it->second.empty();

        // Fallback to checking if variable/type is
        // defined in active scope return
        // m_pSymbolTable->ContainsSymbol(strId);
        return false;
    }

    return false;
}

std::any CStPragmaEvaluator::visitInclude_directive(
    pragma_exprParser::Include_directiveContext* ctx)
{
    if( m_pContext == nullptr ||
        ctx->TOK_LSTRING() == nullptr)
        return false;

    // Extract the raw string path literal:
    // 'filename.st'
    std::string strRawPath =
        ctx->TOK_LSTRING()->getText();

    // Safety bounds check: strip leading and trailing
    // single quotes
    if( strRawPath.size() < 2 )
       return false;

    std::string strCleanPath =
        strRawPath.substr(1, strRawPath.size() - 2);

    auto pMainStream = m_pContext->m_pMainStream;

    if( pMainStream == nullptr )
    {
        std::cerr << "StParser Error: "
            << "main stream is empty"
            << std::endl;
        return false;
    }
    auto pPipeLine = pMainStream->GetPipeLine(); 
    // Resolve the target location through your
    // active include path structures (Assuming you
    // pass or provide the current compiling file path
    // in m_pContext)
    auto pIncManager =
        pPipeLine->GetIncludeManager();
    std::string strTargetFile =
        pIncManager->ResolveIncludePath(
            strCleanPath );

    if (strTargetFile.empty())
    {
        std::cerr << "StParser Error: Unable "
            << "to locate include path matching " 
            << strCleanPath << std::endl;
        return false;
    }

    // Callback logic to signal the core token
    // multiplexer stream to append the asset
    auto pToken = pMainStream->LT( 1 );
    pPipeLine->InjectIncludeStream(
        strTargetFile, pToken,
        pMainStream->index() );
    return true;
}

std::any CStPragmaEvaluator::visitIdentifier_type_pair(
    pragma_exprParser::Identifier_type_pairContext* ctx )
{
    return visitChildren( ctx );
}

std::any CStPragmaEvaluator::visitQuery_type(
    pragma_exprParser::Query_typeContext* ctx )
{
    return visitChildren( ctx );
}


CPragmaRecoveryStrategy::CPragmaRecoveryStrategy(
    guint32 dwPragmaType,
    CStParseContext* pCtx ) :
    super(), m_pContext( pCtx )
{ m_dwPragmaTokenType = dwPragmaType; }

void CPragmaRecoveryStrategy::recover(
    antlr4::Parser* recognizer,
    std::exception_ptr e )
{
    antlr4::TokenStream* pTokens =
        recognizer->getTokenStream();

    antlr4::Token* pOffendingToken =
        recognizer->getCurrentToken();

    if( pOffendingToken == nullptr )
    {
        super::recover( recognizer, e );
        return;
    }

    guint32 dwType = pOffendingToken->getType();
    if( dwType == m_dwPragmaTokenType )
    {
        std::string strText =
            pOffendingToken->getText();

        std::regex reIf(
            "^\\{[ \t]*if[ \t]+",
            std::regex_constants::icase );

        std::regex reInclude(
            "^\\{[ \t]*include[ \t]+",
            std::regex_constants::icase );

        std::regex reEndIncl(
            "^\\{[ \t]*endincl[ \t]*}?",
            std::regex_constants::icase );


        // Handle conditional pruning blocks
        if( std::regex_search( strText, reIf ) )
        {
            size_t nStartIndex = pTokens->index();

            size_t nNextIndex = 
                ProcessAndPruneWholeBlock(
                    pTokens, nStartIndex );

            pTokens->seek( nNextIndex );
            return;
        }

        // Handle End of Include marker token
        // boundary transitions
        if( std::regex_search( strText, reEndIncl ) )
        {
            size_t nStartIndex = pTokens->index();

            auto pStream =
                m_pContext->m_pMainStream;
            auto pPipeline =
                pStream->GetPipeLine();
            auto pIncMgr =
                pPipeline->GetIncludeManager();

            if( pIncMgr != nullptr )
            {
                gint32 iParentIdx =
                    pIncMgr->GetParentFileIdx();

                auto& vecMaster =
                    pPipeline->m_vecMasterTokens;
                if( vecMaster.size() > nStartIndex )
                {
                    auto& oToken =
                        vecMaster[ nStartIndex ];
                    if( oToken.m_iFileIdx ==
                        iParentIdx )
                    {
                        pIncMgr->PopFile();
                    }
                }
            }

            // Advance past the marker token
            // seamlessly so it does not trigger
            // parser errors
            pTokens->consume();
            return;
        }

        // Handle standalone inline active
        // inclusion paths if skipped or evaluated
        // on the fly
        if( std::regex_search( strText, reInclude ) )
        {
            // Optional: Handle inline processing
            // steps if needed or skip active
            // markers
            std::string strClause =
                strText.substr(1, strText.size() - 2);
            EvaluateInclude( strClause );
            auto pToken = dynamic_cast
                < antlr4::CommonToken* >( pOffendingToken );

            if( pToken )
                pToken->setChannel(
                    antlr4::Token::HIDDEN_CHANNEL );
            // pTokens->consume();
            return;
        }
    }

    super::recover( recognizer, e );
}

size_t CPragmaRecoveryStrategy::ProcessAndPruneWholeBlock(
    antlr4::TokenStream* pTokens,
    size_t nIfIndex )
{
    size_t nCursor = nIfIndex;
    gint32 iDepth = 1;

    struct SBranchClause
    {
        size_t nPragmaIdx;
        size_t nContentStartIdx;
        size_t nContentEndIdx;
        std::string strConditionText;
        gboolean bIsElse = FALSE;
    };
    std::vector< SBranchClause > vecClauses;

    while( nCursor < pTokens->size() && iDepth > 0 )
    {
        antlr4::Token* pT = pTokens->get( nCursor );
        if( pT->getType() == m_dwPragmaTokenType )
        {
            std::string strTxt = pT->getText();
            if( strTxt.rfind( "{IF ", 0 ) == 0 )
            {
                if( nCursor == nIfIndex )
                {
                    SBranchClause clause;
                    clause.nPragmaIdx = nCursor;
                    clause.nContentStartIdx = nCursor + 1;
                    clause.nContentEndIdx = 0;
                    clause.strConditionText = strTxt;
                    clause.bIsElse = FALSE;
                    vecClauses.push_back( clause );
                }
                else
                {
                    iDepth++;
                }
            }
            else if( strTxt == "{END_IF}" )
            {
                iDepth--;
                if( iDepth == 0 )
                {
                    vecClauses.back().nContentEndIdx = nCursor - 1;
                }
            }
            else if( iDepth == 1 )
            {
                vecClauses.back().nContentEndIdx = nCursor - 1;
                gboolean bElse = ( strTxt == "{ELSE}" ) ? TRUE : FALSE;

                SBranchClause clause;
                clause.nPragmaIdx = nCursor;
                clause.nContentStartIdx = nCursor + 1;
                clause.nContentEndIdx = 0;
                clause.strConditionText = strTxt;
                clause.bIsElse = bElse;
                vecClauses.push_back( clause );
            }
        }
        nCursor++;
    }

    gint32 iWinningBranchIdx = -1;
    for( size_t i = 0; i < vecClauses.size(); ++i )
    {
        if( vecClauses[i].bIsElse )
        {
            iWinningBranchIdx =
                static_cast< gint32 >( i );
            break;
        }
        if( EvaluateCondition(
            vecClauses[i].strConditionText ) )
        {
            iWinningBranchIdx =
                static_cast< gint32 >( i );
            break;
        }
    }

    constexpr size_t hidden =
        antlr4::Token::HIDDEN_CHANNEL;

    for( size_t i = 0; i < vecClauses.size(); ++i )
    {
        antlr4::Token* pPragTok =
            pTokens->get( vecClauses[i].nPragmaIdx );

        auto pToken = dynamic_cast
            < antlr4::CommonToken* >( pPragTok );

        if( pToken )
            pToken->setChannel( hidden );

        if( static_cast< gint32 >( i ) !=
            iWinningBranchIdx )
        {
            size_t nStart = vecClauses[i].nContentStartIdx;
            size_t nEnd = vecClauses[i].nContentEndIdx;
            for( size_t nIdx = nStart; nIdx <= nEnd; ++nIdx )
            {
                auto pTok = dynamic_cast<antlr4::CommonToken*>
                    ( pTokens->get( nIdx ) );
                if( pTok )
                    pTok->setChannel( hidden );
            }
        }
    }

    auto pEndIfTok =
        dynamic_cast<antlr4::CommonToken*>
        ( pTokens->get( nCursor - 1 ) );
    if( pEndIfTok )
        pEndIfTok->setChannel( hidden );

    if( iWinningBranchIdx == -1 )
        return -1;
    auto& winning = vecClauses[ iWinningBranchIdx ];
    return winning.nContentStartIdx;
}

bool CPragmaRecoveryStrategy::EvaluateCondition(
    const std::string& strCondText )
{
    antlr4::ANTLRInputStream input(strCondText);
    pragma_exprLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);

    // Use the extended parser with predicates
    CStPragmaParser parser(&tokens);

    parser.SetParseContext( m_pContext );

    // Invoke the root rule of your micro-grammar
    // to build the parse tree.  This triggers the
    // LL(*) state machine over the pragma
    // expression tokens.
    auto pTree = parser.pragma_condition();
    if (pTree == nullptr)
        return false;

    CStPragmaEvaluator evaluator( m_pContext );

    // Traverse the tree dynamically to compute
    // the final boolean outcome.  The visitor
    // will recursively process logical operators
    // (AND, OR, NOT) and resolve functions like
    // DEFINED() or HASVALUE() against your symbol
    // maps.
    std::any anyResult = evaluator.visit(pTree);

    // Extract and return the underlying boolean
    // value safely.
    try
    {
        bool bOutcome =
            std::any_cast<bool>(anyResult);
        return bOutcome;
    }
    catch (const std::bad_any_cast& e)
    {
        std::cerr << "Error: Pragma "
            << "expression did not evaluate to a "
            << "boolean outcome. Default to false\n";
        return false;
    }
    return false;
}

bool CPragmaRecoveryStrategy::EvaluateInclude(
    const std::string& strIncText )
{
    antlr4::ANTLRInputStream input(strIncText);
    pragma_exprLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);

    // Use the extended parser with predicates
    CStPragmaParser parser(&tokens);

    parser.SetParseContext( m_pContext );

    // Invoke the root rule of your micro-grammar
    // to build the parse tree.  This triggers the
    // LL(*) state machine over the pragma
    // expression tokens.
    auto pTree = parser.include_directive();
    if (pTree == nullptr)
        return false;

    CStPragmaEvaluator evaluator( m_pContext );
    std::any anyResult = evaluator.visit(pTree);

    // Extract and return the underlying boolean
    // value safely.
    try
    {
        bool bOutcome =
            std::any_cast<bool>(anyResult);
        return bOutcome;
    }
    catch (const std::bad_any_cast& e)
    {
        std::cerr << "Error: 'include' "
            << "directive did not compile. "
            << "Default to false\n";
        return false;
    }
    return false;
}

