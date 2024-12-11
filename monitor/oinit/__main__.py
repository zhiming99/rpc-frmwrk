from .oinit import *

if __name__ == '__main__' :
    ret = MainEntryCli()
    if bRemove:
        quit( 0 )
    if ret == 0 :
        print( "Login Completed" )
    elif ret < 0:
        print( "Error occurs during login({error})".format( error=-ret ) )
    quit( -ret )
