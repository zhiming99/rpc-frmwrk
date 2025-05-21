/*
 * =====================================================================================
 *
 *       Filename:  encdec.h
 *
 *    Description:  declarations of functions related to encryption and
 *    decryption with OpenSSL and GmSSL.
 *
 *        Version:  1.0
 *        Created:  05/21/2025 05:52:35 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com )
 *   Organization:  
 *
 *      Copyright:  2025 Ming Zhi( woodhead99@gmail.com )
 *
 *        License:  This program is free software; you can redistribute it
 *                  and/or modify it under the terms of the GNU General Public
 *                  License version 3.0 as published by the Free Software
 *                  Foundation at 'http://www.gnu.org/licenses/gpl-3.0.html'
 *
 * =====================================================================================
 */
#pragma once
#include "rpc.h"
#include "defines.h"
namespace rpcf
{

std::string GetPubKeyPath( bool bGmSSL );
std::string GetPrivKeyPath( bool bGmSSL );

gint32 EncryptWithPubKey(
    BufPtr& pBlock,
    BufPtr& pEncrypted,
    bool bGmSSL = false );

gint32 DecryptWithPrivKey(
    const BufPtr& pEncrypted,
    BufPtr& pBlock,
    bool bGmSSL = false );

gint32 EncryptAesGcmBlock(
    const BufPtr& pBlock,
    const BufPtr& pKey,
    BufPtr& pEncrypted,
    bool bGmSSL = false );

gint32 DecryptAesGcmBlock(
    const BufPtr& pKey,
    const BufPtr& pToken,
    BufPtr& outPlaintext,
    bool bGmSSL = false );

gint32 GenSha2Hash(
    const stdstr& strData,
    stdstr& strHash,
    bool bGmSSL = false );

std::string GenPasswordSaltHash(
    const std::string& strPassword,
    const std::string& strSalt,
    bool bGmSSL = false );
}
