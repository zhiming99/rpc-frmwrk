/*
 * =====================================================================================
 *
 *       Filename:  stbase.h
 *
 *    Description:  declarations of utilities functions for lexer.
 *
 *        Version:  1.0
 *        Created:  05/26/2026 07:07:26 PM
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
#include <cstring>
#include <strings.h>
#include <ctime>
#include <cctype>
#include <cstdlib>
#include <string>
#include <stdint.h>
#include <sstream>
#include <iomanip>
#include "rpc.h"

timespec st_time_to_timespec(const char* text);
uint64_t ParseStTimeToUnix(const char* input);
std::string TranslateSTString(const std::string& input);
ObjPtr ParsePeriAddr( const char* yytext, CSTParserContext* pCtx );
ObjPtr ParseRpcfAddr( const char* yytext, CSTParserContext* pCtx );
