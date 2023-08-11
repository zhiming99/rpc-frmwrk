#!/usr/bin/env python3
#*
#* =====================================================================================
#*
#*       Filename:  krbparse.py
#*
#*    Description:  implementation of a limited parser of krb5.conf
#*
#*        Version:  1.0
#*        Created:  08/10/2023 09:11:00 PM
#*       Revision:  none
#*       Compiler:  gcc
#*
#*         Author:  Ming Zhi( woodhead99@gmail.com )
#*   Organization:
#*
#*      Copyright:  2023 Ming Zhi( woodhead99@gmail.com )
#*
#*        License:  Licensed under GPL-3.0. You may not use this file except in
#*                  compliance with the License. You may find a copy of the
#*                  License at 'http://www.gnu.org/licenses/gpl-3.0.html'
#*
#* =====================================================================================
#*
import os
from shutil import move
from copy import deepcopy
from typing import Dict
from typing import Tuple
import errno
import re
from updwscfg import *

class NodeType:
    invalid = 0
    wholeline=1
    section = 2
    assign=3
    key=4
    value=5
    block=6
    root=7
    comment=8

sections = {'[logging]', '[libdefaults]', '[realms]', '[domain_realm]', '[appdefaults]', '[capaths]' }

class AstNode:
    def __init__(self) :
        self.lineNum = 0
        self.strVal = ''
        self.id = NodeType.root
        self.children = []
        self.parent = None

    def SetParent( self, node ) :
        self.parent = node

    def GetParent( self, node ) :
        return self.parent

    def AddChild( self, node : object ):
        if node is None:
            raise Exception( "Error empty node to add")
        self.children.append( node )
        node.SetParent( self )

    def RemoveChildren( self ) :
        if len( self.children ) == 0:
            self.parent = None
            return
        for i in self.children:
            i.RemoveChildren()
        self.children.clear()

    def Dump( self, indent : int = 0 ) :
        if self.id == NodeType.root:
            for i in self.children:
                i.Dump( 0 )
        elif self.id == NodeType.section:
            print( self.strVal )
            for i in self.children:
                i.Dump( 0 )
            print( "" )
        elif self.id == NodeType.assign:
            strText = self.children[0].strVal + \
                " = " + self.children[1].strVal
            print( ' ' * indent + strText )
            if self.children[1].id == NodeType.block:
                self.children[1].Dump( indent + 4 )
        elif self.id == NodeType.wholeline or\
            self.id == NodeType.comment:
            print( self.strVal )
        elif self.id == NodeType.block:
            for i in self.children:
                i.Dump( indent )
            print( ' ' * indent + "}" )

    def Dumps( self, indent : int ) -> str:
        strRet = ''
        if self.id == NodeType.root:
            for i in self.children:
                strRet += i.Dumps( 0 )
        elif self.id == NodeType.section:
            strRet += self.strVal + "\n"
            for i in self.children:
                strRet += i.Dumps( 0 )
            strRet += "\n"
        elif self.id == NodeType.assign:
            strText = self.children[0].strVal + \
                " = " + self.children[1].strVal
            strRet += ' ' * indent + strText + "\n"
            if self.children[1].id == NodeType.block:
                strRet += self.children[1].Dumps( indent + 4 )
        elif self.id == NodeType.wholeline or\
            self.id == NodeType.comment:
            strRet += self.strVal + "\n"
        elif self.id == NodeType.block:
            for i in self.children:
                strRet += i.Dumps( indent )
            strRet += ' ' * indent + "}\n"
        return strRet

    def Clone( self )->object:
        try:
            copyNode = AstNode()
            copyNode.strVal = self.strVal
            copyNode.id = self.id
            copyNode.lineNum = self.lineNum
            for i in self.children:
                copyNode.AddChild( i.Clone() )
            return copyNode
        except Exception as err:
            print( err )
            return None

def GetTokens( line : str, bkv = False ) :
    global statStack
    if not bkv:
        results = re.split(" +|\t+|\n+", line)
    else:
        results1 = line.strip().split( '=', maxsplit=2 )
        results = []
        for i in results1:
            results.append(i.strip())
    return list(filter(None, results))

def ParseBlock( fp, coord, parent: AstNode ) -> int:
    ret = 0
    try:
        for line in fp:
            tokens = GetTokens(line, True)
            if len( tokens ) == 2 :
                assign = AstNode()
                assign.id = NodeType.assign
                assign.strVal = '='
                assign.lineNum = coord[0]
                parent.AddChild( assign )

                key = AstNode()
                key.strVal = tokens[0]
                key.id = NodeType.key
                key.lineNum = coord[0]

                assign.AddChild( key )
                if tokens[ 1 ] != '{':
                    value = AstNode()
                    value.id = NodeType.value
                    value.strVal = tokens[1]
                    value.lineNum = coord[0]
                    assign.AddChild( value )
                    coord[0]+=1
                    continue
                else:
                    block = AstNode()
                    block.strVal = '{'
                    block.id = NodeType.block
                    block.lineNum = coord[0]
                    assign.AddChild( block )
                    coord[0]+=1
                    ret = ParseBlock( fp, coord, block )
                    if ret < 0:
                        return ret

            elif len(tokens) == 0:
                coord[0]+=1
            else:
                coord[0]+=1
                if tokens[0] == '}' :
                    return ret
                ret = -errno.EINVAL
                raise Exception( "Error unexpected token %s", tokens[0])
       
    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return ret

def ParseSection( fp, coord, parent: AstNode ) -> Tuple[int, object]:
    ret = 0
    global sections
    try:
        for line in fp:

            tokens = GetTokens(line, True)
            if len( tokens ) == 0:
                coord[0]+=1
                continue
            elif len( tokens ) == 1 and \
                tokens[0] in sections:
                section = AstNode()
                section.strVal = tokens[0]
                section.id = NodeType.section
                section.lineNum = coord[0]
                coord[0] += 1
                return (0, section)
            elif len( tokens ) == 1 and \
                not tokens[0] in sections:
                raise Exception( "Error unexpected token %s(%d)" % (tokens[0],coord[0]) )

            assign = AstNode()
            assign.strVal = '='
            assign.id = NodeType.assign
            assign.lineNum = coord[0]
            parent.AddChild( assign )

            key = AstNode()
            key.strVal = tokens[0]
            key.id = NodeType.key
            key.lineNum = coord[0]

            assign.AddChild( key )
            if tokens[ 1 ] != '{':
                value = AstNode()
                value.strVal = tokens[1]
                value.id = NodeType.key
                value.lineNum = coord[0]
                assign.AddChild( value )
            else:
                block = AstNode()
                block.strVal = '{'
                block.id = NodeType.block
                block.lineNum = coord[0]
                assign.AddChild( block )
            coord[0]+=1

            if tokens[1] != '{': 
                continue

            ret = ParseBlock( fp, coord, block )
            if ret < 0:
                raise Exception( "Error parsing block at %d" % coord[0] )

    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return ( ret, None )


def ParseKrb5Conf( strPath : str ) -> Tuple[ int, object ] :
    ret = 0
    astRoot = AstNode()
    parent = astRoot
    global sections
    try:
        with open(strPath, 'r') as fp:
            coord = [ 0, 0 ]
            node = None
            for line in fp:
                if line[:8] == 'include ' or \
                    line[:11] == 'includedir' or \
                    line[:7] == 'module ':
                    incnode = AstNode()
                    incnode.strVal = line
                    incnode.id = NodeType.wholeline
                    incnode.lineNum = coord[0]
                    coord[0]+=1
                    parent.AddChild( incnode )
                    continue
                if line[0:1] == '#' :
                    incnode = AstNode()
                    incnode.strVal = line
                    incnode.id = NodeType.comment
                    incnode.lineNum = coord[0]
                    coord[0]+=1
                    parent.AddChild( incnode )
                    continue

                listTokens = GetTokens( line )
                while len( listTokens ) > 0:
                    token = listTokens[0]
                    if token not in sections:
                        raise Exception( "unexpected token '%s'" % token )
                    node = AstNode()
                    node.strVal = token
                    node.id = NodeType.section
                    node.lineNum = coord[0]
                    listTokens.pop(0)
                    coord[1] += 1
                    if len( listTokens ) > 0:
                        raise Exception( "unexpected token '%s'" % token )
                    coord[0]+=1
                    coord[1] = 0
                    parent.AddChild( node )
                    break
                while True:
                    resp = ParseSection( fp, coord, node )
                    if resp[0] < 0:
                        break
                    if resp[1] is None:
                        break
                    node = resp[1]
                    parent.AddChild(node)
                ret = resp[0]
                if ret < 0:
                    break

            if ret < 0:
                astRoot.RemoveChildren()
                astRoot = None
            return (ret, astRoot)

    except Exception as err:
        print( err )
        if ret == 0:
            ret = -errno.EFAULT
    return (ret, None)

