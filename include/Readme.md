. This directory contains all the headers required by all the rpc-frmwrk related project. And `rpc.h` is the master header file in this directory. It is not necessarily the only header to include when `ipc` or `rpc` modules are involved.    

. The `curclsid.h` is a specifical file. It is not needed for compile. Instead, it is a clsid, propid, and iid allocation tracking file. Please increase the numbers respectively if you have added your own objects, interfaces or properties. The numbers will serve as the start point for future allocations. 
