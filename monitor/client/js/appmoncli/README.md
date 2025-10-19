[中文](./README_cn.md)
### How to Build, Deploy, and Use the Web-based Rpc-Frmwrk Monitor
* First, download the rpc-frmwrk source code to your local machine, and successfully compile and install rpc-frmwrk on the target machine. Currently, the monitor is not included in the rpc-frmwrk installation package, so you need to build and package it in the source directory.
* Make sure the target machine has `nginx` and `npm` installed. `apache` can also be used, but it hasn't been tested yet.
* Use the command `rpcfctl cfg` to configure Rpc-frmwrk, enabling WebSocket, SSL, and SimpAuth options. You can import existing keys or generate self-signed keys in the configuration program. The configuration program can also set up `nginx`. Suppose your configured WebSocket URL is `https://127.0.0.1/rpcf`, and the nginx web root is `/var/www/html/rpcf`. The following instructions will use this configuration. For details on using `rpcfctl cfg`, refer to [this article](../../../../tools/README.md).
* Run the command `rpcfctl initsvr` to set up the monitor runtime environment. This command will create the `user registry`, `application registry`, and an `admin` user.
* In the command line, switch to the directory where this file is located.
* Run the command `../../../../ridl/.libs/ridlc --noreadme --services=AppMonitor -sJO . ../../../../monitor/appmon/appmon.ridl` to generate all the missing skeleton files.
* Run the command `make -f Makefile.skel` to package the `rpc-frwmrk` support library to a few js files. And now the files to deploy include all files under the `dist` directory, all files under `locales`, and the current directory's `appmon.html`, `appmondesc.json`, `appdetail.js`, `i18n-helper.js`. Copy all these files into `/var/www/html/rpcf/`, keeping the directory structure unchanged. For example, use the following script:
   ```
    #!/bin/bash
    sudo cp appmon.html appmondesc.json i18n-helper.js appdetail.js /var/www/html/rpcf/
    if [[ ! -d /var/www/html/rpcf/dist ]]; then
    sudo mkdir /var/www/html/rpcf/dist
    fi
    sudo cp dist/* /var/www/html/rpcf/dist/
    if [[ ! -d /var/www/html/rpcf/locales ]]; then
    sudo mkdir /var/www/html/rpcf/locales
    fi
    sudo cp ./locales/* /var/www/html/rpcf/locales/
   ```
* Now, start the backend programs with the command `rpcfctl startall`.
* Finally, open your browser and enter `https://127.0.0.1/rpcf/appmon.html` in the address bar.
* When prompted for username and password, you can enter `admin` and the account password.
* To integrate your business server based on ridlc generated skeleton, there will be another artile in the near future.
* Below are some user interface screenshots of the monitor program:
* Login page   
   ![login](../../../../pics/mon-login.png)
* Main page   
   ![main](../../../../pics/mon-main.png)
* Application details page   
   ![appdetail](../../../../pics/mon-rpcrouter.png)
* Configuration change page   
   ![setpv](../../../../pics/mon-setpv.png)
* View setpoint log page   
   ![ptlog-chart](../../../../pics/mon-ptlog-chart.png)
