/*
 * =====================================================================================
 *
 *       Filename:  st_parse_listener.cpp
 *
 *    Description:  implementation of the listener hooks
 *
 *        Version:  1.0
 *        Created:  09/17/2026 11:27:56 AM
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
#include "stparser.h"
#include "stparserBaseListener.h"
#include "antlr4-runtime.h"
#include "clsids.h"
#include "parse_context.h"
#include "st_parse_listener.h"

void CStParseListener::exitEveryRule(
    antlr4::ParserRuleContext *ctx )
{
}
