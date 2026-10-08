/*
 * =====================================================================================
 *
 *       Filename:  pragma.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/27/2026 05:46:31 PM
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
#include "pragma_expr/pragma_exprVisitor.h"
#include "parse_context.h"

#include "DefaultErrorStrategy.h"
#include "Parser.h"
#include "TokenStream.h"


struct CStParseContext;
struct CStPragmaEvaluator :
    public pragma_exprVisitor
{
    typedef pragma_exprVisitor super;

    CStParseContext* m_pContext = nullptr;

    CStPragmaEvaluator(
        CStParseContext* pCtx) : super()
    {
        m_pContext = pCtx;
    }

    virtual std::any visitPragma_condition(
        pragma_exprParser::Pragma_conditionContext* ctx) override;

    virtual std::any visitExpr(
        pragma_exprParser::ExprContext* ctx) override;

    virtual std::any visitAnd_expr(
        pragma_exprParser::And_exprContext* ctx) override;

    virtual std::any visitPrimary_expr(
        pragma_exprParser::Primary_exprContext* ctx) override;

    virtual std::any visitInclude_directive(
        pragma_exprParser::Include_directiveContext* ctx) override;

    virtual std::any visitIdentifier_type_pair(
        pragma_exprParser:: Identifier_type_pairContext* ctx ) override;

    virtual std::any visitQuery_type(
        pragma_exprParser:: Query_typeContext* ctx ) override;
};

struct CPragmaRecoveryStrategy :
    public antlr4::DefaultErrorStrategy
{
    typedef antlr4::DefaultErrorStrategy super;

    guint32 m_dwPragmaTokenType = 0;
    CStParseContext* m_pContext = nullptr;

    CPragmaRecoveryStrategy(
        guint32 dwPragmaType,
        CStParseContext* pCtx );

    virtual void recover(
        antlr4::Parser* recognizer,
        std::exception_ptr e ) override;

    size_t ProcessAndPruneWholeBlock(
        antlr4::TokenStream* pTokens,
        size_t nIfIndex );

    bool EvaluateCondition(
        const std::string& strCondText );
    bool EvaluateInclude(
        const std::string& strIncText );

    virtual void sync( antlr4::Parser* ) override
    {}
};

class CStPragmaParser : public pragma_exprParser
{
public:
    CStParseContext* m_pContext = nullptr;

    CStPragmaParser (antlr4::TokenStream* input) :
        pragma_exprParser(input), m_pContext(nullptr)
    {}

    void SetParseContext(CStParseContext* ctx)
    { m_pContext = ctx; }

    CStParseContext* GetParseContext() const
    { return m_pContext; }
};
