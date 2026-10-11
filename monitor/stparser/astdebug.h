/*
 * =====================================================================================
 *
 *       Filename:  astdebug.h
 *
 *    Description:  Utility functions for debugging and dumping AST trees
 *
 *        Version:  1.0
 *        Created:  01/15/2026
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:
 *   Organization:
 *
 *      Copyright:
 *
 * =====================================================================================
 */

#pragma once

#include <iostream>
#include <string>
#include "stlexer.h"      // pulls in rpc.h and the parser defines (YYLTYPE2)
#include "stclsids.h"
#include "astnodes.h"

namespace rpcf
{

/**
 * @brief Dump AST node tree to output stream for debugging
 *
 * @param pNode The root node to dump
 * @param os Output stream (default: std::cout)
 * @param iDepth Current indentation depth (default: 0)
 * @param strIndent String used for indentation (default: "  ")
 */
void DumpAstTree(
    const ObjPtr& pNode,
    std::ostream& os = std::cout,
    gint32 iDepth = 0,
    const std::string& strIndent = "  " );

/**
 * @brief Dump AST node tree to string for debugging
 *
 * @param pNode The root node to dump
 * @return std::string String representation of the AST tree
 */
std::string AstTreeToString( const ObjPtr& pNode );

/**
 * @brief Get human-readable node type name from CLSID
 *
 * @param eClsid The class ID
 * @return std::string Human-readable type name
 */
std::string GetNodeTypeName( EnumClsid eClsid );

/**
 * @brief Get node information as a formatted string
 *
 * @param pNode The node
 * @return std::string Formatted node information
 */
std::string GetNodeDebugInfo( const ObjPtr& pNode );

} // namespace rpcf