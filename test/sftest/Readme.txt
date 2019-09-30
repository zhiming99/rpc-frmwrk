UploadFile/DownloadFile test. 
1. test if local/remote UploadFile works 
2. test if local/remote DownloadFile works
3. test if the multi-interface mechanism works. So far, the CSimpFileServer has
4 interfaces include CMyFileServer, IInterfaceServer, CStatCountersServer, IStream, and
CEchoServer.
To run the test:
0. setup the bridge/forwarder and the sfsvrtst properly.
1. for sfclitst, copy a file to /tmp/sfsvr-root/${whoami}/
2. update directory of the UploadFile's parameter in sftest.cpp's main function for proxy.
3. run the test.
4. I will make it more convenient in the future.
NOTE that, this is an advanced application of the rpc-framework. Unlike other tests, it requires a lot of internal knowlege.

