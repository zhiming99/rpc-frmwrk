---
[`Sat Jan 13 2024 10:43:32 PM Beijing`]   
1. Merged the bugfix PR to the master branch.
2. Next i will build a new release `0.7.0` for this major bug-fix commit.
3. And then lets resume our development of Javascript support library.

[`Sun Dec 03 2023 01:59:06 PM Beijing`]   
1. Now merged Feature-Rate-Limiter branch to the master branch.
2. Next I will add a Javascript Support library, which will be the bed-rock for Javascript language support.

[`Wed Nov 29 2023 10:55:41 PM Beijing`]   
1. We have almost complete the BPS rate-limter. There are still some tests to go.
2. Next we will have add the QPS rate-limiter. It will take very short time to get it done.
 
[`Sat Oct 28 2023 05:41:32 PM Beijing`]   
1. Merged pull request #27 to the master branch
2. Bumped `rpc-frmwrk` version to 0.6.0. Congratulations!

[`Sun Oct 15 2023 09:53:43 PM Beijing`]   
1. Having merged pull request #22 to the master branch.
2. We still have a few issues (see issues) to fix before preceeding with new features development.

[`Fri Oct 06 2023 06:12:38 PM Beijing`]   
1. Having cleared all the blocking bugs. Now we can move on to fix the major performance issue.

[`Sat Sep 23 2023 09:01:53 PM Beijing`]   
1. Having fixed one memory leak, and two regression bugs. The progress is a bit slow. There are still two bugs to kill, besides fixing the performance issue as the PR requests

[`Sat Sep 16 2023 06:53:13 PM Beijing`]   
1. previously I was planning to release version 0.6.0, in the last week of Sep.
2. Now I have found 3 new memory leaks in the stress test. And I must postpone the release for one or two weeks.

[`Sat Sep 16 2023 04:41:49 AM Beijing`]   
1. The gpg key is expired, so the update will be delayed for a while.

[`Tue Sep 12 2023 08:41:51 PM Beijing`]    
1. The new pull request tries to fix a performance issue in built-in router server, which fails many incoming requests due to heavily usage of seqential task group. It can be solved easily. Also there are some memory leaks that have to be addressed along with the performance issue. Hopefully we can get all these bugs fixed next week.
2. After all this done, we will proceed with the 'rate limiter' feature.
 
[`Tue Aug 27 2023 08:59:30 PM Beijing`]   
1. Merged the branch `autocfg-websock-and-kerberos` to the master. We have officially finished this pull request. And this release has delivered the following features:
   * To automatically configure `nginx` and `apache` and install `SSL` keys for rpc-frmwrk to communicate via secured websocket connection
   * To automatically configure the `Kerberos` server, create service principals and user principals, deploy the keytab and rewrite the krb5.conf, and enable `kinit` proxy, to put the `Kerberos` ready for rpc-frmwrk.
   * To generate installers capable of setting up target server/client host with working Websocket or Kerberos authentication.
   * Added more command line options for built-in router applications, to cover more scenarios with `Kerberos` authentication enabled.
   
[`Tue Aug 22 2023 09:35:55 PM Beijing`]   
1. We have almost finished the autoconfig of web server and kerberos via rpcfg.py. There is only `enabling kinit proxy` left to complete. I will add it in the next few days. Also some documentation work remains to do for this autoconfig feature.

[`Tue Jul 25 2023 04:05:27 PM Beijing`]   
1. We have finished and merged the pull request bugfix-and-btinrt-techdebt.
2. Next we will move on to automate websocket and kerberos configuration process.
3. And then we may choose to develop feature from `rate limiter`, `connection recovery`, `oauth2` or `windows port`.

[`Mon Jun 12 2023 10:11:46 PM Beijing`]   
1. We have almose finished adding fastrpc and built-in router support to both Python and Java. The remaining work is to fix some bugs and make it stable.
2. The haunting `EchoMany response lost` issue is reopened. There is still an unknown bug in the rpcfs module.

[`Fri Jun 02 2023 03:28:02 PM Beijing`]    
1. Now Python has been enabled fastrpc and built-in router support. That is, Python can have a performance of around 500us with some configuration.
2. Next I will make Java to have the two features.
3. Besides, I am pretty sure that the `EchoMany response lost` issue has been spotted and fixed.

[`Thu May 18 2023 07:41:47 PM Beijing`]   
1. I have merged `add-non-socket-stream` to the master branch.
2. The new test shows the performance of `echo test` with built-in router has a breakthrough improvement of entering micro-second level. The average request is around 500us. **Congratulations**!

[`Tue May 16 2023 08:46:40 PM Beijing`]    
1. The pull-request is almost done, except some minor work to wrap up. I will merge it in the next few days. The performance gain is not cheerful. It seems requiring changes of the architechtual level to see the significant improvment in performance.

[`Fri May 05 2023 09:51:41 PM Beijing`]   
1. Code complete the non-socket-stream support. The extensive test can only be done when ridlc can generate the skelton with non-socket-stream enabled.
2. Next i will add the ridlc the new feature to generate integrated server/proxy with builtin rpcrouter rather than stand-alone rpcrouters, to see the maximum performance gain the non-socket-stream can reach.

[`Sat Apr 22 2023 10:14:15 AM Beijing`]    
1. We have modulized features `gmssl`, `openssl` `fuse3`, `auth`, `java`, `python`, and `testcases`. All the features are now pluggable at compile time, and can be disabled by adding a `--disable-xxx` when running script `configure` or `cfgsel`.
2. We have improved the deployment utility by generating the server/client side installer with `rpcfg.py`, which is preferable to the previous `initcfg.json` file. And the installer can install the private keys and certs as well as the system settings.
3. Next we will move on to add a new uxport to boast the streaming performance to the next level.

[`Sat Apr 15 2023 11:06:44 PM Beijing`]   
1. The PR remove-lowlevel-rw-fuse is merged and closed.
2. Next, I will continue to make `fuse` and `auth` modules compile-time pluggable.
3. After this, I will resume the speed boost task.

[`Thu Mar 16 2023 08:13:21 PM Beijing`]   
1. I have now finished GmSSL support development officially, congratulations.
2. Next I will remove the references to lowlevel API of the fuse, mainly read/write, because it goes beyond the FUSE expected usage of the API and will cost more extra effort to keep synchronized with the code changes from upstream.
3. I will also work on the next level speed boost for stream transfer. In a particular configuration, `rpc-frmwrk` will remove the unix sockets between the rpcrouter and the client/server, which can reduce greatly the overhead of moving bytes through the stream channel. Thanks to the earlier work on gencpp2 and genfuse2, we are now able to move on with less effort to achieve this goal.

[`Thu Mar 09 2023 09:20:04 PM Beijing`]   
1. Merged the add-gmssl-support to the master branch. This PR has also delivered many bug-fixes, and `rpc-frmwrk` should be more stable now. Congratulations!
2. The GmSSL's makefile is yet to provide because GmSSL itself is not provided as an off-the-shelf package and has to be built locally.
3. There are also some documentation work to do in the next few days for GmSSL, as well as testcases for GmSSL.

[`Thu Mar 09 2023 02:06:39 PM Beijing`]   
1. We have almost finished development of GmSSL support, and the GmSSL support can now run as smoothly as OpenSSL does.
2. Next we may continue to fix some bugs and close the PR.

[`Thu Mar 02 2023 08:58:47 PM Beijing`]   
1. Suspended investigation of `EchoMany response lost`. It happens outside `rpc-frmwrk`. However, we have found a way to prevent it from happening. It is not a fix, and we will work out a solution to the bug sometime in the future.
2. Moved on the development of GmSSL support. We can start debugging as soon as we have completed the changes made to `rpcfg.py` and `updatecfg.py`, the GUI config tool.

[`Sat Feb 18 2023 06:00:24 PM Beijing`]   
1. Still trying to fix `EchoMany response lost`. Now the bug seems to be outside the rpc-frmwrk, and it needs some time to verify.
2. Meanwhile, we have fixed the segment fault bug on python testcase `iftest` and `hellowld` quitting.
3. We now have more time to put on the development of gmssl support.

[`Sat Feb 04 2023 05:36:14 PM Beijing`]   
1. The mysterious `Errno 2` bug is finally fixed. It turns out to be a bug in the testscript. Besides, we have also fixed a memory leak, stream start failure and some other bugs. The progress is cheerful.
2. There could still be an `EchoMany reponse lost` issue to fix.
3. And let's continue development for gmssl support.

[`Thu Jan 19 2023 11:48:38 PM Beijing`]    
1. while writing gmssl port, the mysterious `Errno 2` bug shows up again. It is difficult to reproduce and therefore the bug-fix progress is slow.

[`Tue Jan 03 2023 11:01:11 PM Beijing`]    
1. Most of the code for gmssl support will be submitted to the gmssl fork in the next few weeks.
2. I will add some Chinese translations to the readme files in this repository.

[`Sun Dec 25 2022 12:58:35 PM Beijing`]   
1. Finally the bugs are fixed in making/removing service point directory.
2. Let's continue GMSSL support.

[`Wed Dec 21 2022 09:53:17 PM Beijing`]   
1. There are still some bugs to fix before the pull-request can be submitted. mainly bugs in making/removing service point directory in a busy environment.

[`Mon Dec 05 2022 21:39:10 PM Beijing`]    
1. The bug is fixed which made the bridge to have file descriptor leaks and unable to create new streams.
2. Found a defect in the `rpc-over-stream` design, that the server/proxy objects cannot be aggregrated to `hostsvr` and `hostcli` as a service point. It need some fundamental modifications in the `I/O subsystem`.

[`Sat Dec 03 2022 10:01:20 AM Beijing`]    
1. There is another bug emerging in the rpcrouter. I will fix it in the next two days.

[`Mon Nov 29 2022 12:40:20 PM Beijing`]   
1. fixed two old bugs in the stream creation process. 
2. Now let's continue adding `gmssl` to `rpc-frmwrk`.

[`Mon Nov 21 2022 22:25:30 PM Beijing`]    
1. merged genpy-for-rpcfs to the master branch. Next we will add support to the `gmssl`, the cipher suite for internet security defined by Chinese standard administration.

[`Sat Nov 19 2022 05:13:17 PM Beijing`]    
1. genpy-for-rpcfs is almost complete. The remaining work is to add the testcases to the testing process, and then we can conclude this pull-request.

[`Tue Nov 01 2022 20:52:27 PM Beijing`]    
1. I have to take a two weeks leave. So no regular update these weeks.

[`Tue Oct 18 2022 09:57:27 AM Beijing`]    
1. merged the pull request to shrink the fastrpc message size to the master branch. And i have done some optimizations.
2. Next, let's continue rpcfs programming support.

[`Wed Sep 28 2022 03:26:39 PM Beijing`]   
1. I cannot resist to make fastrpc to support fuse first before the skelton code generator for `rpcfs`, because fastrpc has some good potentials that could be very useful in the future.
 
[`Sat Sep 24 2022 10:08:56 PM Beijing`]    
1. Released 0.5.0. Congratulations!

[`Mon Sep 19 2022 08:28:10 PM Beijing`]    
1. Continued fixing memory leaks and stability bugs and making optimizations to the 'ipc' code. Now the `fastrpc` module has reach a stable state. There are still one or two minor bugs to fix so far, but it would be very soon to get them fixed. and then we can move on to support for programming with`rpcfs`.

[`Mon Sep 12 2022 03:19:45 PM Beijing`]    
1. The issue of ping token loss is actually a collection of bugs in the uxport, uxstream, and fastrpc. After extensive code review, bug fixes and testing, the issue is almost gone. I will do some futher tests in the next few days to verify.

[`Tue Sep 06  202210:33:13 PM Beijing`]   
1. There is still a difficult bug in the stream module to spot, which caused the initial ping token unreplied from the server per million requests. And therefore, the rpcfs support has to postpone for a while.

[`Sat Sep 03 2022 04:52:48 PM Beijing`]   
1. Merged the `rpc-over-stream` branch to the master branch. The evaluation shows not significant performance gain. But it contains many bug fixes in stream support, and it's pararllel request process architecture should have some superior advantage in some situations. so it is merged.

[`Mon Aug 15 2022 11:02:56 PM Beijing`]    
1. Next 3-4 weeks, I will work on the branch `rpc-over-stream` to evaluate the effect of the solution for performance boost. Good luck to me.

[`Sat Aug 13 2022 09:49:37 AM Beijing`]   
1. Finally we wrapped up the `rpcfs` development. Congratulations!
2. Next I need to evaluate how much the performance optimization for `rpc-frmwrk` affects to the whole architecture of `rpc-frmwrk`. Previously, it was planned to move on to auto code generator for `rpcfs` programming. But the performance optimization will affect it a lot. Therefore it is paused before the evaluation is done.

[`Mon Aug 08 2022 08:49:45 AM Beijing`]
1. Found a way to boost the performace of rpc-frmwrk. It involves development of a new `port` module and some complicated changes in code generator module. And the impact is still under evaluation.
2. It took 10 days to fix bugs, harden the `bridge` security, refact the CFuseStmFile, and make damage tests on the `rpcfs`. And therefore I am now still working on exporting reqfwdr's runtime information to router-specific file `ReqFwdrInfo`. I will wrap up the work in two days so that we can move on to next task, `skelton code generator`.

[`Sat Jul 23 2022 07:56:59 PM Beijing`]   
1. We still have the reqfwdr's runtime information file to implement. It won't take too long before we start to write parsers again.

[`Wed Jul 20 2022 09:34:11 PM Beijing`]   
1. Fixed building issues and made successful building on Raspberry Pi.
2. Continued fixing bugs in `rpcfs`, and fixed some regression bugs.
3. After adding session file and reqfwdr's runtime information file, we will move on to write auto skelton generator for `rpcfs`

[`Fri Jul 08 2022 08:42:04 AM Beijing`]   
1. Added a `InConnections` file to the rpcrouter to show the statistics of the current connections of the rpcrouter.
2. We will add `OutConnections` and `sockets` files to the rpcrouter next. 
3. And then we turn to generate skelton code for the `rpcfs`.

[`Sat Jun 25 2022 01:36:03 PM Beijing`]    
1. Added two more commands `loadl` and `addsp` to the `commands` file. And now the `rpcfs` filesystem can aggregrate new `service point` dynamically. 
2. Next I will integrate an instance of `rpcfs` to the `rpcrouter` for monitoring and control purpose. 

[`Sat Jun 18 2022 09:28:10 PM Beijing`]   
1. Made a performance optimization by polling the stream channels on a dedicated group of loops different from the previous DBus's loop. It will improve the throughtput when there are a large number stream channels and heavy traffics.

[`Wed Jun 15 2022 10:29:58 AM Beijing`]   
1. So far the workflow has covered all the types of the connections rpc-frmwrk supports.
2. And added a `rpcfs` testcase. 
3. Next I will add more mangement and user interface supports to `rpcfs` and `rpcrouter`, including
    * a surrogate program to host multiple `serivce point` objects
    * more commands for the `commands` file
    * a management `rpcfs` for `rpcrouter`
    * Runtime configuration files similiar to the `sysfs` filesystem.

[`Thu Jun 09 2022 07:19:33 PM Beijing`]   
1. Finished adding tests for kerberos connection to the github's workflow.
2. Next is to add tests for websocket connection to the github's workflow as well as some tests for FUSE integration.
3. The second stage of development for `rpcfs` is on the horizon. There are many things to do and I have to take some time to figure out the target to achieve.

[`Wed May 25 2022 10:28:02 PM Beijing`]   
1. Made the individual `service point` of a mounted file system restartable, and reloadable.
2. There are still some more tests to do and the documentation for the FUSE integration.
3. Actually, FUSE integration goes beyond the scope of RPC. It has introduced a new way to build the system when everything is file. It deserves a new project for FUSE integration related stuffs.

[`Fri May 13 2022 05:05:33 PM Beijing`]    
1. Fixed the mysterious request dropping issue. There are still two bugs ahead, but should be easier to handle.

[`Sat May 07 2022 09:21:35 PM Beijing`]    
1. I have encountered a mysterious request dropping issue, and plan to digging into the FUSE kernel module to find some clue. 

[`Tue May 03 2022 12:00:48 PM Beijing`]   
1. the duplicatable 'service directory' has been added.
2. There are still some bugs to fix related to the operations of filesystem so far.

[`Mon Apr 25 2022 10:33:13 PM Beijing`]   
1. Have passed a difficult test for fuse support. 
2. There are still two important requirements to implement before the release, the informational files, and the duplicatable 'service directory'. Examples, and testcases will be added after the release.

[`Thu Apr 14 2022 09:11:27 PM Beijing`]   
1. The fuse integration has passed most of the test cases, and I will merge it to the master branch soon.
2. Next, there should be many documentation work and auto-testcases to write.

[`Wed Mar 30 2022 10:25:04 PM Beijing`]    
1. Finally, the file system interface is code complete. Next let's make it run.

[`Mon Mar 21 2022 07:57:49 PM Beijing`]   
1. Checked in some newly added code for fuse integration. And the file system interface is 60% code complete. A little bit behind the schedule.

[`Mon Feb 28 2022 08:53:32 AM Beijing`]    
1. The ridlc part is 90% code complete, and remain the interface with the file system part.
2. seems the file system is not a trivial work, and it will take a month to get it done.

[`Sat Feb 19 2022 11:47:25 PM Beijing`]   
1. Renamed `Json support` to `Fuse Integration`.
2. And the development of `Fuse Integration` takes two steps, the first is to add fuctions to `ridlc` to generate the proxy/server library for FUSE integration, and the second is to implement the `file system` part over the generated code. The development is on branch `fusesup`.

[`Tue Jan 25 2022 01:38:47 PM Beijing`]   
1. Back from some urgent tasks. Continue the development of Json support.
2. With Json support, the client can send/receive req/resp with a Json string. The server handler will handle and respond the client request in Json string too. 

[`Sat Jan 15 2022 07:03:42 PM Beijing`]   
1. Merged buf2var-dev to the master branch. 
2. The performance gain is about 10%~20%.
3. Next we start the Json support.

[`Wed Jan 05 2022 04:20:20 PM Beijing`]   
1. Added a new branch 'buf2var-dev' for migration from CConfigDb to CConfigDb2.

[`Sun Jan 02 2022 07:53:48 PM Beijing`]   
1. Trying to replace the CConfigDb's properties from BufPtr to Variant, which is an effort for performance improvement. CConfigDb is one of the fundamental data structure, so it will make the system less stable for a while.

[`Sat Jan 01 2022 11:47:41 AM Beijing`]    
1. 新年快乐!
2. Happy New Year!

[`Fri Dec 31 2021 02:39:12 PM Beijing`]    
1. Added all the examples for Python and C++.
2. Fixed some defects in code generators for C++ and Python.
3. Added an SSL test process to the CI workflow.
4. Illustrutions to the design and examples are yet to add.
5. Moving on to the Json support in 2022!

[`Tue Dec 14 2021 10:40:41 PM Beijing`]   
1. Java support is now **released!**, Congratulations!
2. In the next two weeks, I will do some quality improvement work, which includes
   * Get the deb build to work on raspberry pi
   * Fix a defect in python generator.
   * Add more examples for Python and C++. 
   * Add more testcases to the CI workflow.
   * Add some illustrations to the system design and examples.
3. At the beginning of next year, I will start to add Json support, that is to serialize from a json string/file and deserialize to a json string/file, which serves as a preparation for `fuse` support.

[`Tue Dec 07 2021 10:48:54 PM Beijing`]   
1. After the last testcase `sftest` for java is done, the java support is about 95% complete. The last work is to put the Makefile.am in the building process, and get it delivered by the `rpm` and `deb` packages.
2. Next I will make some improvement to expand the coverity of the automated testing process, such as unattented setup of openssl and kerberos.
3. Besides the improvement of testing, i will also add some tutorial documents and language support, especially documents in Chinese.

[`Tue Nov 23 2021 06:21:29 PM Beijing`]   
1. Made some progress on java generator debugging. We are 70% done with the Java support. Java support is coming.

[`Tue Nov 09 2021 02:02:13 PM Beijing`]   
1. Half way through the debugging of java generator. It will take me a week to handle some urgent thing, and there will be less updates in the next four or five days.

[`Thu Nov 04 2021 09:34:51 PM Beijing`]   
1. The Java code generator is almost code complete. But encountered some difficulty with deserializing maps. Fixing it may take a few days.

[`Fri Oct 22 2021 06:18:56 PM Beijing`]   
1. The progress of Java support is faster than python, because I have followed Python module's architecture. I will work out the Java generator before starting to debug.

[`Sun Oct 03 2021 05:21:45 PM Beijing`]    
1. Planning to use SWIG to wrap the classes, because SIP cannot generate JAVA. It seems technically preferable to choosing SWIG at the beginning of Python support, without introducing extra load of learning and development efforts.

[`Sat Oct 02 2021 08:07:17 PM Beijing`]    
1. Decided, I will first implement Java support, and start right away.

[`Fri Oct 01 2021 04:29:54 PM Beijing`]    
1. **0.4.0 is released!**

[`Thu Sep 30 2021 11:26:55 PM Beijing`]    
1. Added scripts to build `deb` and `rpm` package, but still need some tests.

[`Mon Sep 20 2021 11:34:52 PM Beijing`]   
1. Fixed the cumulated bugs in Dockerfile, and made it work independently from any building output.
2. Added scripts to build debian package, and almost done.
3. The first official release 0.4.0 is coming.
4. Json support will start after 0.4.0 is released.

[`Thu Sep 02 2021 02:54:14 PM Beiging`]   
1. Canceled the CBuffer-to-VARIANT change, to keep code clean.
2. Start Json support right away. Json support is to pass the text in Json format as the input/output parameters to the `rpc-frmwrk`. 

[`Tue Aug 31 2021 13:32:00 AM Beijing`]   
1. Python support is now officially released. **Congratulations!**
2. Before starting the JavaScript support, I will do some optimization work to improve the performance at least 20%, which includes at least,
   * New serialization implementation of CConfigDb
   * Replace the CBuffer with a lean structure VARIANT as the CConfigDb's property storage.

[`Sat Aug 28 2021 10:44:47 AM Beijing`]   
1. Python code generator is coming. It should be almost complete. I will add the documentation in the next few days.

[`Fri Aug 06 2021 10:05:20 AM Beijing`]   
1. improved the workflow and docker build, and the test cases can now run automatically.

[`Mon Jul 31 2021 10:23:00 AM Beijing`]   
1. Made the connection limiter to work. Time to merge the `rfcdev` to the master branch.
2. Will add some config options to the config dialog, to wrap up `rfcdev`. It is not perfect yet, since `rfc` is now just a concurrency limiter, to be a complete flow controller, we still need some work on traffics of each individual connection. But at this moment, let's leave this issue behind till it cannot be ignored.
3. Next, we will move on to develop the python's generator.

[`Mon Jul 26 2021 08:20:46 PM Beijing`]   
1. Added a round-robin task-scheduler to the `reqfwdr` as the solution of the starving issue. After having fixed some stable issues, and some old bugs, the starving issue is now eliminated from the `reqfwdr`.
2. still, the connection-limiter on the bridge side is yet to add. Let me do some research to get it done.
3. Did not expected `RFC` last so long.

[`Mon Jul 12 2021 01:54:27 PM Beijing`]   
1. Over the way of tests, it has exposed a starving issue in the `reqfwdr` when a hundred requests arrive at the same time. However, the starving issue could also be an issue of the `bridge`. And I need to address it ASAP before the `rfc` can be released.

[`Mon Jul 05 2021 04:42:26 PM Beijing`]   
1. fixed some stability bugs on the `rfcdev` branch. Since `rfc` is a profound changes, it has exposed some old design issues, and stability issues. Although we have fixed some issues, it still need some more tests.
2. the feature `rfc` still remains one last bit to make it perfect, that is, the algothrim to adaptively and automaticaly adjust the sending window size with the feedback of current load and latency informantion.

[`Mon Jun 21 2021 03:14:14 PM Beijing`]   
1. fixed a message leak bug when the system is under stress testing. 
2. `RFC` is still under testing. Added a stress test to the `rfcdev` branch.

[`Sun Jun 13 2021 11:49:07 PM Beijing`]   
1. Most of the `request flow control` code complete. Now entering debugging stage. It should be ready next week.

[`Sat Jun 05 2021 03:40:07 PM Beijing`]   
1. The scale and volume of the `request flow control` has expanded a lot, including some critical update in `multihop`. I have created another branch for the development.

[`Thu May 27 2021 05:42:27 PM Beijing`]   
1. Completed just half of the `request based flow control`. The other half is the flow control logics in `request forwarder` and per-client connection besides the existing aggregrated connection.
2. Added a workflow, so that we can verify every push now. 

[`Fri May 21 2021 02:31:46 PM Beijing`]   
1. Last week, fixed a memory corruption when web socket was enabled, though it is an old bug in `CBuffer`, just exposed.
2. Also added the missing `active disconnection` on server side if the handshake check fails.
3. The tag 4 is the latest stable version before adding request-based flow control. 
4. The `request based flow control` involves a `mainloop pool` to distribute the io loads, retiring long-wait requests, task scheduling restrictions in `CIfParallelTaskGrp`, and `concurrent request limit update` event.

[`Mon May 10 2021 09:24:59 PM Beijing`]   
1. Let's move on to add `request-based flow control`.

[`Fri May 07 2021 04:12:30 PM Beijing`]   
1. The GUI config tool is almost done. There are still some testing to get it more stable.
2. Now we can deploy `rpc-frmwrk` with ease!

[`Sun Apr 25 2021 03:07:27 PM Beijing`]
1. The request flow control turns out to be big and need some preparations before I can start. So I will first get the GUI config tool done, and at the same time, make some preparations for the flow control.
2. With the aide from GUI config tool and ridl, you can easily deploy the `rpc-frmwrk` and start the development of your `hello, world` application in C++. That will be the first official release of `rpc-frwmrk`.

[`Mon Apr 19 2021 02:14:36 PM Beijing`]   
1. Win10 hosted `Ubuntu` is a nightmare, and the filesystem is so fragile to sustain a single unexpected shutdown or system crash. Switched to `Fedora-33` for luck.

[`Thu Apr 15 2021 10:00:02 PM Beijing`]   
1. The `rpc-frmwrk`'s first code generator for C++ is ready now! Congratulations!

[`Thu Apr 15 2021 12:56:10 PM Beijing`]   
1. Before moving on to the python code generator, I need to add a flow-control mechanism to the request processing pipeline. The taget is to limit performance drop on arrival of huge amount of requests, and let servers, routers, and proxies to coordinate to survive through the overwhelming request surge. 

[`Tue Apr 13 2021 05:29:19 PM Beijing`]   
1. It seems not a good idea to put many irrelevant stuffs to ridl. Maybe a config dialog box is better. Let's keep ridl less complex.

[`Mon Apr 12 2021 09:15:36 PM Beijing`]   
1. `keep-alive` improvement and `no-reply` request are ready now.
2. Found that three more files `router.json`, `rtauth.json`, and `authprxy.json` need to be customized for the generated project. And an `make install` target is needed to ease the deployment efforts or provide hints for deployment. It requires some new statements in the `ridl` gramma.

[`Thu Apr 08 2021 08:37:46 PM Beijing`]   
1. Debugging for cpp generator is still going on. And during debugging I found it is necessary to add a no-reply request to make `keep-alive` support perfect. it should be a quite easy task.
2. So far the cpp generator has generated all the required files successfully. And only the stream support remains to debug.

[`Tue Mar 30 2021 01:28:58 PM Beijing`]   
1. The python's setup.py is still not ready, and so the installation of the whole project is still not perfect at this point. 
2. Changed the project's code name to `rpcf` in source code, to end the naming related chaos. It is also a preparation for building the auto-generated project by ridlc.

[`Thu Mar 25 2021 11:26:39 PM Beijing`]   
1. Suspended the ridlc development and turned to migrate the building system from the raw Makefiles to Autotools mechanism. And there should be still some bugs.  The new building system has the powerful support for installation and distribution, which the old building system cannot compete. And ridlc's output will depend on this building system as well.
2. I can soon resume the ridlc's development.

[`Sat Mar 13 2021 09:25:15 PM Beijing`]   
1. Has finished 70% of the code generator for C++. It needs extensive test though.

[`Sun Jan 31 2021 11:52:16 AM Beijing`]   
1. During preparation for the proxy/server generator, I have added a small feature of `Node Redudancy/Load Balance`. It is a very simple implementation. Load distribution on more advanced strategy relies on more advanced data acquisiton technique, which needs some further development in the future.

[`Sat Jan 23 2021 06:58:41 PM Beijing`]
1. After having fixed some bugs in the streaming related code, test case `sftest` is almost done. I will add some document text and do some further test.
2. After the `sftest` is done, the major development target of Python support has been achieved. As the `todo` list shows, I will do some investigation on the code generator and probably the next step is to add some support about protobuf, to enable multi-languate support, or JAVA support.

[`Sun Jan 17 2021 12:41:10 PM Beijing`]   
1. Completed most of the python support, and now we are fully prepared for writing python version of test case `sftest'.

[`Fri Jan 08 2021 10:17:44 AM Beijing`]   
1. Forked the `server.sip` and `PyRpcServer` from `proxy.sip` and `PyRpcProxy`. 
2. Python version of test case `sftest` will be added after the python server support is completed.

[`Sun Dec 19 2020 12:06:37 AM Beijing`]   
1. The latest changes in CUnixSockStream backed out `sending Progress` when the remote peer has sent out enough messages to fill up the local receiving window. And instead, `sending tokProgress` is performed from `CStreamSyncBase`'s `CIfReadWriteStmTask`, at the very moment the associated pending message is out queue and consumed. This approach prevents the potential stall of stream due to receiving window congestion. 

[`Fri Dec  4 2020 10:54:37 PM Beijing`]   
1. Refacted the streaming interface, and the new version should be less difficult to use than the old one. There will be some tests to the new interface before the Python support can resume.
2. Suprisingly, the `sftest` turns to to be faster than old version with the new streaming interface. The reason is yet to check. Sweet.

[`Fri Nov 27 2020 04:56:52 PM Beijing`]   
1. I need to make the streaming interface less difficult to use before I can move on with Python support.

[`Fri 27 Nov 2020 10:05:19 AM Beijing`]   
1. Helped my father through a heart stent surgery during the last 10 days. Back to this project now!

[`Sat 14 Nov 2020 01:49:44 PM Beijing`]   
1. Added a preliminary version of python support, which can run the echo test so far. 
2. The multithreading issue with Python makes some trouble in evtcli.py. So the event test cannot pass yet. 
3. The python server is yet to develop, as well as streaming support. Anyway, it is a lot easier than the C/C++ version.

[`Thu 22 Oct 2020 10:41:46 AM Beijing`]   
1. Fixed some long standing bugs.
2. Move on to add support for Python.
3. `make install` is deferred, because it is not quite necessary at present.
4. I will add a `tutorial` directory and write some tutorial document.

[`Sat 10 Oct 2020 02:48:54 PM Beijing`]   
1. Fixed the concurrent bug and some long-existing bugs. Now the `rpc-frmwrk` should work stably.
2. As the next step, I will add the `make install` to the build system.

[`Wed 23 Sep 2020 12:24:43 PM Beijing`]   
1. TODO next
    * Fix a concurrent bug in the IO manager
    * Add the build command `make install`
    * Add support for Python

[`Tue 15 Sep 2020 11:37:29 AM Beijing`]   
1. Fixed a bunch of bugs. Now the system works stable. After almost 5 months, Kerberos authentication support is finally ready! :) 

[`Fri 11 Sep 2020 08:25:18 PM Beijing`]   
1. Encountered some concurrent bugs, and fixed some of them. There are still some bugs known to fix, which should be less difficult than the ones fixed this week.   

[`Wed 02 Sep 2020 07:30:53 PM Beijing`]   
1. fixed many bugs, and the test case `sftest` has passed. There are still some test cases to go. Kerberos support is almost ready.

[`Tue 25 Aug 2020 08:19:12 PM Beijing`]   
1. `kinit` now works with rpc-frmwrk through the rpc connection, that is, if the kerberos kdc is behind the firewall, and not open to the internet, the client still has a way to get authenticated.   

[`Tue 11 Aug 2020 10:02:46 PM Beijing`]  
1. The test case `iftest` has passed. There should still be some bugs. Bug fixing and tests continues...   

[`Thu 06 Aug 2020 11:02:28 PM Beijing`]   
1. Still debugging the authentication module. And some progress are made. Many disturbing events happened in the last two weeks, such as 30-hour black-out, and buggy system update degraded the computer performance to 800MHz. Fortunately, everything returns to normal now. Hopefully, next week we will see the authentication support able to work.

[`Sun 26 Jul 2020 08:12:16 PM Beijing`]   
1. Submitted the Kerberos support to the main branch. It is not debugged yet. So it surely cannot work for now. And if you want to give a shot to the framework, please try version 0.3. :)   

[`Sun 12 Jul 2020 11:23:27 AM Beijing`]   
1. still battling with some technical difficulties. The "relative simple" design has turned not as simple as it looks. Anyway, authentication support is coming...  

[`Mon 22 Jun 2020 05:40:08 PM Beijing`]   
1. Discarded the old authentication design, and chose a relative simple design to start over now, `Kerberos` authentication.      

[`Fri 12 Jun 2020 04:58:06 PM Beijing`]   
1. Designing is still in process. Details has made the progress slow.   

[`Sun 24 May 2020 04:49:09 PM Beijing`]   
1. It turns out security stuffs never disappoint you by complexity. Still researching on the different authentication approaches. It would probably take another month to get it done.

[`Wed 13 May 2020 05:23:01 PM Beijing`]   
1. Preferable to `File ACL` as the `access control` model.
2. `Kerberos` requires a lot of resources for deployment, and I will also try to add the `NTLM` as a low-cost alternative.

[`Mon 11 May 2020 09:23:20 PM Beijing`]   
1. Trying to use `GSSAPI's Kerberos implementation` as the authentication mechanism. There are still some technical issues to investigate at this point.
2. It is not decided yet whether to use `File ACL` or `RBAC` as the `access control` model.

[`Tue 28 Apr 2020 02:12:02 PM Beijing`]   
1. The next task is to implement `Authentication and access control` for `rpc-frmwrk`.

[`Sun 26 Apr 2020 06:19:24 PM Beijing`]   
1. `Multihop` is almost completed, but testing will continue to cover more test cases.
2. The next to do could be either `Python support` or `Authentication and access control`. Not decided yet. 

[`Fri 24 Apr 2020 08:15:48 PM Beijing`]   
1. Fixed some major bugs. There are still two or three known bugs, but multihop support is near complete.   

[`Tue 07 Apr 2020 11:15:32 AM Beijing`]   
1. fixed some bugs in the multihop code. It should be more stable now. and more testing and tuning needed.

[`Fri 03 Apr 2020 10:03:06 PM Beijing`]   
1. Streaming for multihop almost works. the performance is not ideal yet. More testing and tuning needed.   

[`Tue 31 Mar 2020 10:11:09 PM Beijing`]   
1. still debugging the multihop related stuffs. Request forwarding and event forwarding work now. Next task is to get streaming to work.   

[`Sat 21 Mar 2020 01:47:35 PM Beijing`]   
1. Updated some `autoconf` releated stuffs to make the cross-compile more efficient.

[`Fri 20 Mar 2020 11:00:16 PM Beijing`]   
1. Submitted a compilable version with multihop support. It will take one or two weeks to get it work.   

[`Sat 14 Mar 2020 09:18:01 PM Beijing`]   
1. Code complete the streaming support for multihop, about 70% done.    

[`Mon 02 Mar 2020 05:36:15 PM Beijing`]
1. Splitted the `CRpcRouter` to smaller classes.   
 
[`Sun 23 Feb 2020 11:22:40 AM Beijing`]   
1. Splitting the CRpcRouter class to four smaller classes to allow better management of shared resources.   

[`Sat 15 Feb 2020 12:02:14 AM Beijing`]   
1. Fixed a bug in the streaming interface.
2. Added [`Concept.md`](https://github.com/zhiming99/rpc-frmwrk/blob/master/Concept.md) as an introduction about the `rpc-frmwrk`.

[`Wed 12 Feb 2020 02:09:38 PM Beijing`]   
1. The `WebSocket` support is completed. And the instructions about the usage is updated in the [`Readme.md`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/wsport/Readme.md).   
2. Next task is `multihop` support, the very important feature!   

[`Mon 10 Feb 2020 02:14:10 PM Beijing`]   
1. `Websocket` support is comming. The new update delivers a working version of `Websocket` port. However, the  `transparent proxy traversal` remains to test.   

[`Fri 07 Feb 2020 07:31:31 PM Beijing`]  
1. Merged the `multihop` branch to `master`.   
2. Added the `WebSocket` support, though not working yet.   
3. After the `WebSocket` is done, I will then choose one to implement between `multihop` and `connection recovery`.

[`Sun 02 Feb 2020 08:30:52 PM Beijing`]   
1. Created a new branch `multihop` for the newly added changes. It will take some time to get stable, and then I will merge back to the `master` branch.

[`Tue 21 Jan 2020 06:53:13 PM Beijing`]   
1. The replacement of `ip address` with `connection handle` has turned out not a trivial change, and has escalated to a major change. I need to bring the pirority of `multihop routing` ahead of the `websocket` support for now. This is a major upgrade of the router module, which enables associations of two or more devices/controlers in a hierarchical tree, and enable the client to `RPC` to a number of remote servers via a `path` string.

[`Mon 06 Jan 2020 12:04:26 PM Beijing`]   
1. I need to replace the `ip address` with a opaque `connection handle` for the upcoming support of websocket, since `ip address` is no longer the only address format to locate the server. It is the bottom half of an earlier change which replaced `ip address` with the `port id` on the `bridge` side. The `reqfwdr` and `dbusprxy` are the target modules for this time.   

[`Wed 01 Jan 2020 09:05:21 AM Beijing`]   
1. Happy New Year! 2020!   

[`Mon 30 Dec 2019 12:57:19 PM Beijing`]   
1. Obosoleted some bad code.

[`Sat 28 Dec 2019 04:11:29 PM Beijing`]   
1. The SSL related bug turns out to be the usage of global buffer without protection. 
2. Now all the known bugs in `sslfido` are fixed, and it should be safe to say the SSL support is more stable than yesterday. :)   

[`Fri 27 Dec 2019 08:51:54 PM Beijing`]   
1. Fixed a bug related to streaming channel setup, which could drop the reponse message and, thus, caused the proxy to wait till timeout. It happens only when the system load is very high.   
2. Also fixed two regression bugs due to earlier code changes, one is caused by no dedicated `irp completion thread` and the other is because the newly added `CTcpStreamPdo2` with incompitable interface to the old `CTcpStreamPdo`. It seems to take a longer time to make the new `tcp port stack` stable.

[`Thu 26 Dec 2019 08:17:28 PM Beijing`]   
1. Further testing between virtual machines revealed a significant performance issue and a SSL bug. The performance issue is fixed. the SSL bug is yet to fix, which should be a concurrent problem.   
2. Websocket support has to put off for several days. 

[`Fri 20 Dec 2019 10:29:02 PM Beijing`]   
1. Now there should still be some minor bugs in `sslfido`, related to the renegotiation. And there are some optimization and enhancement to do. But it does not matter that we can move on the add the support for websocket.   

[`Wed 18 Dec 2019 11:08:13 AM Beijing`]   
1. Finally, get over the last major bug in SSL support. It turned out to be a random packet loss in the CTcpStreamPdo2.  But the error report from OpenSSL as `mac failed`, was confusing and misleading for bug fixing. And most of the time was taken to find out why OpenSSL cannot work multi-threaded, in vain.

[`Fri 13 Dec 2019 07:47:35 PM Beijing`]   
1. Stucked with a bug that SSL randomly failed to decrypt the message and kill the SSL session. It is a bit difficult to fix...    

[`Wed 11 Dec 2019 09:48:08 AM Beijing`]   
1. OpenSSL support is almost done. And there are some minor bugs to fix. 
2. Next task is websocket support.   

[`Sat 30 Nov 2019 02:33:06 PM Beijing`]   
1. Still writing openssl support, and stucked with the SSL's renegotiation. It should be done next week.   

[`Mon 25 Nov 2019 03:58:57 PM Beijing`]   
1. added LZ4 compression option to the router traffic. the `rpcrouter` now accepts command line option `-c` to enable compression on the outbound packets from the CRpcNativeProtoFdo port.
2. SSL support next...

[`Sat 23 Nov 2019 01:38:46 PM Beijing`]   
1. Finally, it took 20 days to make the new `tcp port stack` in place. Both the old `tcp port stack` and the new stack can be useful in different use cases. And therefore let's keep both alive for a period.
2. the lesson learned is that it is always better to make full research in advance than to rewrite afterward.
3. Now we can move on to add the following features, compression, SSL, websocket/http2 support.

[`Sat 09 Nov 2019 10:09:12 AM Beijing`]   
1. Refacting the tcpport. I did not realize that through the days, the tcpport has grown to a big module with about 10000 loc.
2. Continue the design of the support for websock/http.
3. Object addressing and multiple hops between routers are also in the research.

[`Sun 03 Nov 2019 02:05:39 PM Beijing`]   
1. To add the support for websocket, SSL and other unknown protocols, I need to refact the current tcp port implementation, because the present design is too closely coupled without room for the new protocols. It is a bitter decision, but it should worth the effort. Sometime, you may need one step back for 2 steps forward. The good part is that the tcp port implementation is relatively isolated with little changes to other functions.

[`Fri 01 Nov 2019 04:47:07 PM Beijing`]   
1. Changes for Raspberry PI/2 PI is commited. An ARM platform is supported!   

[`Thu 31 Oct 2019 07:36:50 PM Beijing`]   
1. Just get the Raspberry PI/2 to work. the performance is about 15ms per request. Anyway the ARM support is coming soon.
2. The ideal http support is not a trivial task. It may take two months to get it run. Preparing to get hand dirty...
3. Some wonderful feature is brewing, hehe...

[`Thu 24 Oct 2019 10:31:14 AM Beijing`]   
1. IPv6 support is done.
2. It turns out http support depends on the presence of some new features. It needs some time to have a full understanding of the task.
   * Object addressing mechanism
   * multihop routing
   * Session management and access control
   * Site registration and discovery.

[`Mon 21 Oct 2019 05:34:24 PM Beijing`]   
1. Reduced the size of the request packets, by removing some redudant properties from the request configdb.
2. Having difficulty choosing between websocket or http/2, as well as the session manager. So let me add the support for IPv6 first as a warm-up execise.   

[`Thu 17 Oct 2019 07:36:03 PM Beijing`]   
1. Worked a `helloworld` with built-in router as the `btinrt` test, for the curiosity to know the performance of removal of dbus traffic. The outcome shows improvement of the latency from about 1.5ms to about 1ms per echo, as about 30% faster, in sacrifice of the flexibility. Anyway, well worth the effort. Also fixed a hanging bug in router and a segment fault in taskgroup which cannot reproduce in the stand-alone router setup.

[`Wed 09 Oct 2019 09:11:53 AM Beijing`]   
1. Upgraded the synchronous proxy code generation with the new macro, before the start of session manager design. It is now an easy task to write a proxy with just a few lines, and help the developers to focus on server side design. For detail information, please refer to test/helloworld.   

[`Mon 30 Sep 2019 02:53:58 PM Beijing`]
1. Finished rewriting the `sftest` and `stmtest` tests with the stream interface. And fixed many bugs.   
2. Next target is to get session manager to work. It is a premise task before the support for the `websocket` connection.   
3. Happy National Day! :)

[`Tue 17 Sep 2019 09:56:09 PM Beijing`]   
1. Major improvement! This project supports 64bit X86_64 now. Yeah!   

[`Wed 11 Sep 2019 07:16:37 PM Beijing`]   
1. fixed some bugs in the streaming module. It should be more stable now. There should still be a few bugs to fix.       

[`Sun 01 Sep 2019 09:11:20 AM Beijing`]   
1. Still writing some helper classes for the stream interface.

[`Wed 21 Aug 2019 10:21:16 AM Beijing`]   
1. The major streaming bugs are cleared so far. Next is to improve the stream interface for the proxy/server because current API is raw and difficult for third-party development.
2. the second task is to rewrite the file upload/download support via the streaming interface.
3. And the third one is to enable the project 64bit compatible in the next month.

[`Wed 21 Aug 2019 08:13:14 AM Beijing`]   
1. Encountered a concurrency problem in the router's stream flow-control these days. However it should be soon to be fixed.

[`Fri 16 Aug 2019 09:04:06 PM Beijing`]   
1. The stream channel can finally flow smoothly. There should still be some bugs around. Still need sometime to get it stable. Congratulations!   

[`Sun 11 Aug 2019 07:11:55 PM Beijing`]
1. FetchData works now. And moving on to the streaming data transfer over the tcp connection. I can see the light at the end of the tunnel...   

[`Wed 07 Aug 2019 09:48:22 PM Beijing`]   
1. OpenStream works now. Next to get the FetchData to work over the Tcp connection. It should be ok very soon...

[`Fri 02 Aug 2019 01:25:29 PM Beijing`]   
1. Start debugging the remote streaming now. Impressed by the streaming performance, and thinking if it is possible to route the RPC traffic via a streaming channel...   

[`Sat 27 Jul 2019 09:36:00 PM Beijing`]   
1. Made the local streaming work now. There remains some performance issue to solve. And afterwards to move on to the streaming over the router. It should last longer than local streaming.   

[`Sun 21 Jul 2019 02:25:01 PM Beijing`]   
1. Coding is fun and debugging is painful. ...   

[`Sat 20 Jul 2019 01:50:34 PM Beijing`]
1. Streaming support code complete finally. The next step is to get it run.

[`Mon 15 Jul 2019 06:02:27 PM Beijing`]
1. The last piece of code for streaming support is growing complicated than expected. Still trying to weld the two ends between tcp sock and unix sock gracefully inside the router. And struggling with many duplicated code with trivial differences.   

[`Wed 10 Jul 2019 04:02:20 PM Beijing`]
1. Almost code complete with the streaming support, remaining start/stop logics for the streaming channel in the bridge object. 

[`Sat 22 Jun 2019 02:03:53 PM Beijing`]   
1. Still busy designing the `flow control` for the streaming support. It turns out to be more complicated than expected. Just swaying between a simple solution and a complex one.   

[`Thu 13 Jun 2019 06:53:01 AM Beijing`]   
1. the CStreamServer and CStreamProxy are undergoing rewritten now. The unix socket operations will be put into a new port object, and all the I/O related stuffs will go to the port object. and the CStreamServer and CStreamProxy are no longing watching all the unix sockets, and instead, associate with a unix socket port object. The port object handles all the events related to the socket.   

[`Mon 03 Jun 2019 02:24:31 PM Beijing`]   
1. The earlier implementaion of `SEND/FETCH DATA` does not work for `TCP` connection. I need to tear it apart and rewrite it. The issues are:
*    Security problem with `SEND_DATA`. That is the server cannot filter the request before all the data has already uploaded to the server.
*    Both `SEND_DATA` and `FETCH_DATA` are difficult to cancel if the request is still on the way to the server, which is likely to happen over a slow connection.
*    `KEEP_ALIVE` and `OnNotify` becomes complicated.
2. The solution is to use the streaming interface to implement the `SEND_DATA/FETCH_DATA`, so that both the server and the proxy can control the traffic all the time.   
   

[`Sat 01 Jun 2019 05:48:48 PM Beijing`]   
1. Added a Wiki page for performance description.   

[`Mon 27 May 2019 08:13:24 PM Beijing`]   
1. Got sick last week, so long time no update.
2. The test cases, `Event broadcasting`, `KEEP-ALIVE`, `Pause-resume`, `Active-canceling`, and `Async call` are working now over both IRC and RPC, as well as `helloworld`.
3. Next move on to the support for SendData/FetchData RPC. 

[`Sun May 12 14:03:47 Beijing 2019`]   
1. The `communication channel setup` (`EnableRemoteEvent`) is almost OK for tcp connection. There should still be a few `hidden` bugs to fix in the future testing. Anyway the `helloworld` can now work more stable and faster. And both the `bridge` and `forwarder` are more torlerable on the error condtion.   
2. Next I will try to get `File/Big data transfer` work. It should be the last big section of un-debugged part so far.   
3. I will also get the other examples to run over TCP connection.   

[`Fri May 03 13:15:23 Beijing 2019`]   
1. Now the `helloworld` can work over the tcp connection. **Great!**   
2. There are some crash bugs and wrong behavor to fix yet.   
3. Happy International Labour Day 2019.   

[`Wed Apr 24 18:10:46 Beijing 2019`]   
1. The `bridge` side is now in good shape. Next I will take some time to clear some bugs on the `forwarder` side, before moving on to run the helloworld over the tcp connection.   

[`Sun Apr 21 12:36:37 Beijing 2019`]   
1. There is one last memory leak to fix yet. After done, we can wrap up the `EnableRemoteEvent` and move on to debugging client requests.   

[`Thu Apr 11 14:06:43 Beijing 2019`]   
1. Fixed some fatal memory leaks and concurrency bugs. Hopefully, the biggest memory leak will be fixed in the next check-in. But the progress is a bit lagging :(    

[`Wed Mar 27 17:56:33 Beijing 2019`]   
1. Fixed some bugs in the code. There are still some memory leak and concurrency bugs to fix in the `EnableRemoteEvent` and connection setup.

[`Thu Mar 14 20:47:45 Beijing 2019`]   
1. The `EnableRemoteEvent` works on the `Forwarder` side. and The bridge side has successfully received the request. Move on to get the `Bridge` side `EnableRemoteEvent` to work. This should be the last part work before getting through the whole request/response path.   

[`Thu Mar 07 23:35:20 Beijing 2019`]   
1. Fixed the memory leak issues. Move on the get the CRpcControlStream to work.   

[`Tue Mar 05 14:05:40 Beijing 2019`]   
1. Encountered and fixed some bugs in the `CRpcRouter`. There are still some memory leaks to fix. One known bug is that, the `OpenRemotePort` and `CloseRemotePort` need to be sequentially executed rather than in random order. Otherwise, the `CRpcTcpBridgeProxyImpl` and the `CTcpStreamPdo` may not be able to be released properly.   
2. After the leaks are fixed, I will get `EnableRemoteEvent` to work.   

[`Sun Feb 24 14:52:28 Beijing 2019`]   
1. This week, I had some emergency thing to handle. I will get back next week.   

[`Sat Feb 16 17:35:54 Beijing 2019`]   
1. `OpenRemotePort` is done now. The CDBusProxyPdo and CDBusProxyFdo are found lack of the `online/offline` handler for both remote server and remote module, and need to add some code next week. And after that, we can move on to debug `EnableEvent` related stuffs.   

[`Thu Feb 07 21:17:48 Beijing 2019`]   
1. Still debugging the CRpcRouter. The `OpenRemotePort` is almost completed now. And there still need a disconnection handler on the `reqfwdr` side to clean up the context for the unexpectedly disconnected proxy.   

[`Thu Jan 24 17:03:14 Beijing 2019`]   
1. Continued to refact part of the CRpcRouter. Now I expect there are some trivial patches need to be done in the coming days of this week before the refact task is done.   

[`Sat Jan 19 23:43:28 Beijing 2019`]   
1. The redesign and refact work are almost done. Next week we can start the debugging of RPC module hopefully.   

[`Fri Jan 11 08:32:13 Beijing 2019`]   
1. There are several places in the router to redesign for new requirements/discoveries. And the details have appended to the todo.txt.   

[`Tue Jan 01 20:53:12 Beijing 2019`]   
1. This week, I need to redesign the management part of the router for the bridge and reqfwdr's lifecycle management, and add new command to the tcp-level protocol for connection resiliance.   
2. Note that, if you want to run `helloworld`, please use the earlier version `bf852d55ae2342c5f01cc499c59885f50780c550` of `echodesc.json` instead. The latest version is modified for RPC debugging purpose.

[`Tue Dec 18 21:35:54 Beijing 2018`]   
1. Made the proxy ports loaded and work. Next step is to get the req-forwarder to run in a stand-alone process.
2. We have two approaches to deploy the proxy, the compact way, that is, the proxy and the req-forwarder works in the same process, and the normal way, that an rpc-router process runs as a router for all the proxys system-wide. I will first make the normal way work. The significance of compact way is to have better security in communication than the normal way if the device is to deploy in a less trustworthy environment.

[`Fri Dec  7 22:45:23 Beijing 2018`]
1. Finally, I have wrapped up local IPC testing. Now I will proceed to RPC debugging and testing.

[`Mon Dec 03 17:47:55 Beijing 2018`]
1. Added local stream support. It is yet to debug.   

[`Tue Nov 20 13:02:20 BeijingBeijing 2018`]
1. Added message filter support and multiple-interface support. Sorry for not starting the RPC debugging still, because I need to dig more in streaming support which will affect RPC module a bit.   

[`Wed Nov 14 11:24:42 Beijing 2018`]
1. Local `Send/Fetch` is done. And now it is time to move on to the RPC module. For security purpose, I will add the authentication interface sometime after the RPC module is done.   

[`Fri Nov  2 18:24:51 Beijing 2018`]   
1. This week, the `keep-alive` and `Pause/Resume` works.
2. Next week, I will try to get `Send/Fetch file` to work, before I can move on to RPC module.   
   
[ `Tue Oct 23 19:59:39 Beijing 2018` ]   
1. This week, I will get keep-alive to work and move on to make RPC module to work.   
   
[ `Mon Oct 15 18:51:48 Beijing 2018` ]  
1. The dependency of `g_main_loop` from `glib-2.0` is replaced with a simplified `mainloop`.   
2. the glib headers are still needed at compile time. But the glib shared libraries are not required at deploy time.   
   
[ `Thu Oct  4 12:24:00 Beijing 2018` ]
1. After some tests, I cannot find a way to put `libev` in the concurrent environment flawlessly.
So `libev` is not an option any more. [(Reason)](https://github.com/zhiming99/rpc-frmwrk/wiki/Why-libev-cannot-be-used-in-rpc-frmwrk%3F)  
2. I will write a simple mainloop with `poll` to fix it.   

