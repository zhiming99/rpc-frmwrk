/*
 * =====================================================================================
 *
 *       Filename:  ifclient.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/15/2018 01:15:23 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ming Zhi( woodhead99@gmail.com ), 
 *   Organization:  
 *
 * =====================================================================================
 */


#include <rpc.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cppunit/TestFixture.h>
#include "ifsvr.h"
#include "ifhelper.h"

using namespace std;

/**
* @name InitUserFuncs
*    to init the service handler map for this interface
*    object. It is mandatory to call
*    super::InitUserFuncs(), otherwise some of the
*    interfaces, such as the admin interface with 
*    pause/resume/cancel cannot work.
* @{ */
/** 
 * 
 * NOTE: when you are using BEGIN_DECL_PROXY_SYNC
 * or BEGIN_DECL_IF_PROXY_SYNC, you can ignore the
 * following text. 
 *
 * You can use two sets of macro's to to init the
 * function map, one include ADD_USER_PROXY_METHOD
 * and ADD_PROXY_METHOD, and the other includes
 * ADD_USER_PROXY_METHOD_EX and
 * ADD_PROXY_METHOD_EX. Usually it is recommended to
 * use the EX set of macro's
 * 
 * The differences between the two are as following:
 *
 * 1. EX macro requires the proxy function to call
 * the macro FORWARD_CALL, and return the error
 * code.
 *
 * while the Non-EX macro requires to call in two
 * steps, SyncCall to send the request and
 * FillArgs to assign the response to the output
 * parameters, which requires you have a better
 * understanding of the system.
 * 
 * 2. EX macro strictly requires the proxy
 * function to have both input and output
 * parameters in the formal parameter list, that
 * is, input parameters come first, and then the
 * output parameters follow.  both are in the same
 * order as the one on the server side.
 *
 * While the Non-EX macro's has no requirement on
 * the formal parameter list of the proxy method.
 *
 * 3. the macro's argument lists are different.
 * The EX macro requires the method and the number
 * of input parameters in the arg list. While the
 * Non-EX macro requires the types of all the
 * input parameters in the arg list of the macro.
 * 
 * for example of Non-EX, please refer to other
 * tests, such as prtest or katest under the
 * `test' directory.
 *
 * @} */

