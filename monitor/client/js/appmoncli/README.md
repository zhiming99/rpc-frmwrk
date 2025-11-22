[中文](./README_cn.md)

### Features
- This monitor can observe all applications deployed under rpc-frmwrk on a server, providing real-time metrics and the ability to change runtime parameters.
- The monitor can also observe devices connected to rpc-frmwrk via multihop gateways.
- The monitor uses username+password login. The backend can use the built-in SimpAuth authentication or a third-party OAuth2 provider.

### Deploy and use the web-based Rpc-Frmwrk monitor
- Ensure the target machine has `nginx` installed. `Apache` can also be used but has not been tested.
- After rpc-frmwrk is installed and configured, the packaged UI `appmonui.tar.gz` can be found under `/usr/local/etc/rpcf/` or `/etc/rpcf`. For information on configuring rpc-frmwrk, see [this article](../../../../tools/README_cn.md#rpc-frmwrk配置工具).
- Run `rpcfctl initsvr` to initialize the monitor runtime environment. This command creates the user registry, the application registry, and an `admin` user.
- Extract `appmonui.tar.gz` into the directory served by nginx.
- Start rpc-frmwrk services with `rpcfctl startall`.
- Open your browser and go to `https://127.0.0.1/rpcf/appmon.html`. If you use self-signed certificates, ignore the browser warning.
- When prompted for username and password, use `admin` and the account password.

If you want your business servers to be manageable from the monitor:
- When generating server framework code with `ridlc`, add the `-m <app-name>` option to update server-side code. This option only affects server-side files.
- Register the application with `rpcfctl addapp <app-name>`.
- Run the server once so it registers its startup information and instance metadata.
- After that, the application can be monitored and managed via the web UI.
- The Docker image `zhiming99/django-oa2cli-cgi/general:SimpAuth-https-2` contains a demo of the monitor; you can download it with `docker pull`.

### Screenshots
- Login page  
  ![login](../../../../pics/mon-login.png)
- Main page  
  ![main](../../../../pics/mon-main.png)
- Application detail page  
  ![appdetail](../../../../pics/mon-rpcrouter.png)
- Configuration change page  
  ![setpv](../../../../pics/mon-setpv.png)
- View setpoint log page  
  ![ptlog-chart](../../../../pics/mon-ptlog-chart.png)


