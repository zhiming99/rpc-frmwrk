# GENERATED BY RIDLC, MAKE SURE TO BACKUP BEFORE RUNNING RIDLC AGAIN
# Copyright (C) 2024  zhiming <woodhead99@gmail.com>
# This program can be distributed under the terms of the GNU GPLv3.
# ridlc --client --services AppManager --sync_mode IDataProducer=sync -psO . ./appmon.ridl 
from typing import Tuple
from rpcf.rpcbase import *
from rpcf.proxy import *
from seribase import CSerialBase
from seribase import Variant
from appmonstructs import *
import errno

interval = 10
from AppManagerclibase import *
class CIAppStorecli( IIAppStore_CliImpl ):

    '''
    Event handler
    Add code here to process the event
    '''
    def OnPointChanged( self,
        strPtPath : str,
        value : Variant
        ) :
        print( strPtPath, value.val )
        if strPtPath == 'timer1/interval1':
            global interval
            interval = value.val
            print( 'interval changed to', interval )
        return
        
    '''
    Event handler
    Add code here to process the event
    '''
    def OnPointsChanged( self,
        arrKVs : list
        ) :
        return
        
    
class CIDataProducercli( IIDataProducer_CliImpl ):

    pass
    
class CAppManagerProxy(
    CIAppStorecli,
    CIDataProducercli,
    PyRpcProxy ) :
    def __init__( self, pIoMgr, strDesc, strObjName ) :
        PyRpcProxy.__init__( self,
            pIoMgr, strDesc, strObjName )

    def OnPostStop(self):
        print( 'connection is down' )
        return 0