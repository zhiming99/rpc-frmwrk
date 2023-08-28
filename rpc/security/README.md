### Introduction to the rpc-frmwrk's Authentication support
The authentication process is the process to allow only valid users to access the rpc-frmwrk's service.
Currently the rpc-frmwrk relies only on the `Kerberos 5` authentication framework. In the future, 
we will add more authentication approaches besides the `Kerberos 5`. This document also tries to give some
cookbook guide for users with no experience to `Kerberos` in advance. If you are timid about reading through
the long document, please scroll to the bottom to find some magic solution.

#### What does `Kerberos 5` do in rpc-frmwrk?
The following are some services, `Kerberos` can provides, and of course, it can provide far more
beyond the list:
 1. user/service account management, adding/modify/delete the user or service account.
 2. Granting tickets to the legistimate users, which could be used by the user to access all the
    services, within a `realm( similiar to domain )` without repeatedly typing passwords.
 3. Setup the secure sessions between the client and the service servers, and provide
    encryption/signature services for the communications of the session.
 4. Credential management and key caches, including credential's lifecycle management,
    providing storage for keys and credentials.

#### Setup Kerberos
Please don't be scared, setup kerberos for authentication is not as difficult as you thought. And it can
be deployed on a network as simple as of only two connected devices. Here are the steps to make Kerberos
to work with `rpc-frmwrk` on such a simple network.
##### 1. Setup `kdc( Kerberos domain controller)`. You may want to select one of your machines as the kdc server.
  Depending on the linux distribution, the kerberos package name could be slightly different.
  * On a Fedora machine, you can `dnf install krb5-server`, and dnf will install all the necessary packages for you.
  * On a Raspberry Pi, you can use `apt install krb5-kdc` and apt will install all the necessary packages for you.
  * After the successful installation, you need first to select a good name for the default realm, in our case, 
  `rpcf.org`.
  * And then make some changes to the `/etc/krb5.conf`. If your kdc does not have a public DNS entry, just put the ip
  address as the kdc address in the section `[realms]`, or add the domain name to `/etc/hosts`.The following is a sample
  `krb5.conf` for reference.  

```
    [logging]
        default = FILE:/var/log/krb5libs.log
        kdc = FILE:/var/log/krb5kdc.log
        admin_server = FILE:/var/log/kadmind.log

    [libdefaults]
        dns_lookup_realm = false
        ticket_lifetime = 24h
        renew_lifetime = 7d
        forwardable = true
        rdns = false
        pkinit_anchors = FILE:/etc/pki/tls/certs/ca-bundle.crt
        spake_preauth_groups = edwards25519
        dns_canonicalize_hostname = fallback
        default_realm = rpcf.org
        default_ccache_name = KEYRING:persistent:%{uid}

    [realms]
    rpcf.org = {
        kdc = kdc.rpcf.org
        admin_server = kdc.rpcf.org
    }

    [domain_realm]
    .rpcf.org = rpcf.org
    rpcf.org = rpcf.org
```
  * Create the kdc database, on the kdc host, with command `sudo kdb5_util create -r rpcf.org -s`.
  * Add the user account with `kadmin.local` on your kdc machine. `kadmin.local` does not require password and
  can only be used locally, while `kadmin` is a network version. Suppose you have a linux account name `foo`, and you can add a
  user account `foo` to the kerberos's user database. The fully qualified account name is `foo@rpcf.org`, which must be put
  to the proxy's description file as the `user name` later, if authentication is enabled.
  
 * Add the service principal to the kerberos's user database with the `kadmin.local` as well. The service principal, looks like
  `rasp1/rpcf.org`. `rasp1` is the machine name, and `rpcf.org` after the slash is instance name to tell the service is
  `rpc-frmwrk`. Note that, `Kerberos` uses the term `principal` to represent an entity that participate in the `Kerberos's`
  authentication process, and you can just assume it the same as a `name`.
  
  * Start `krb5kdc`, the KDC server, and `kadmind` the admin deamon. And the kdc setup is done now. If you want advanced
  and exhaustive configuration options, please refer to the official document [here](https://web.mit.edu/kerberos/krb5-devel/doc/admin/install_kdc.html#install-and-configure-the-master-kdc)
  
  * BTW, You can use `kpasswd` to change each existing account's password.
  
##### 2. Setup the client machines. Client machines are those devices via which the users can access the the RPC services.
  * On a Fedora machine, you can `dnf install krb5-workstation`, and dnf will install all the necessary packages for you.
  * On a Raspberry Pi, you can `apt install krb5-user`, and apt will install all the necessary packages for you,
      and help you configure the kerberos.
  * Put the same `krb5.conf` as the one on the `kdc server` to the directory `/etc`. (Note: if the client is a different linux distribution, you may want to tweak the `krb5.conf`, as can be quite different from the one on the `kdc server`)
  * Type `kinit foo` to authenticate with the remote `kdc`. According to the `ticket_lifetime` option above, the ticket will
  last one day, and when it expires, the login session will ends.
  * In some environment when you cannot access `kdc` directly, `rpc-frmwrk` can provide a `kdc` communication channel for `kdc` 
  access via the RPC connection, thus you can use `kinit`, `kadmin` as usual. The approach is to symbolic link `libauth.so`
  under the directory, `/usr/lib/x86_64-linux-gnu/krb5/plugins/libkrb5`(ubuntu) or `/usr/lib64/krb5/plugins/libkrb5`(fedora), for example. the directory name could vary from different distributions or architectures.
  * The official document is at [here](https://web.mit.edu/kerberos/krb5-devel/doc/admin/install_clients.html)
  
##### 3. Setup the service server, and in our case, the `rpc-frmwrk bridge` with authentication`
  * The installation is the same as we do on the client machines, that is, the first two steps.
  * Then, unlike the client machines, the service server needs a `key table` to authenticate to the `KDC`. The `key table`
  can be generated from the service server, with `kadmin` and `ktadd` subcommand. When `ktadd` is asking service principal for the `key table`,
  in our case, `rasp1/rpcf.org`. The file name is `krb5.keytab` and make sure its **ownership and access right** properly set, otherwise the `bridge` may fail to start. The The official document is at [here](https://web.mit.edu/kerberos/krb5-devel/doc/admin/install_appl_srv.html)

##### 4. Enable authentication for `rpc-frmwrk`
  * In the [`driver.json`](https://github.com/zhiming99/rpc-frmwrk/blob/master/ipc/driver.json), the section for `RpcTcpBusPort`,
  you can find the configurations for each listening port. And add attribue `HasAuth:"true"` to the port, and all the incoming connections from
  that port will be authenticated first before normal `RPC` requests can be serviced.
  
  * In the [`rtauth.json`](https://github.com/zhiming99/rpc-frmwrk/blob/master/test/router/rtauth.json), the section for
  `RpcRouterBridgeAuthImpl`, you can add the authentication infomantion as the service server. It looks like 
 ```
             "AuthInfo" :
            {
                "AuthMech" : "krb5",
                "ServiceName" : "rasp1@rpcf.org",
                "Realm" : "rpcf.org",
                "SignMessage" : "true"
            }
 ```
  * Note that the `ServiceName` is in the form of `service@host`, that is `rasp1` is the service name and `rpcf.org` is the domain name of the of the host. If your host is not registered with the DNS, put the host name to the `hosts` file. If you uses `rpcfg.py` to initialized the local Kerberos server, the tool will take care it for you.

  * In the `helloworld's` description file, [`hwdesc.json`](https://github.com/zhiming99/rpc-frmwrk/blob/master/test/helloworld/hwdesc.json),
  for example, add the following lines,
  ```
            "AuthInfo" :
            {
                "AuthMech" : "krb5",
                "UserName" : "foo@rpcf.org",
                "ServiceName" : "rasp1@rpcf.org",
                "Realm" : "rpcf.org",
                "SignMessage" : "true"
            }
  ```
  * The messages of the authenticated session is usually encrypted over the network. And it can be signed instead, which can be preferable if you want to use SSL as the encryption method. You need to add `SignMessage:"true"` to both `rtauth.json` and `hwdesc.json` as mentioned above to enable this feature. According to my test, such a combination of kerberos signature plus SSL encryption delivers better performance than the encryption with `kerberos` only.
  ```
            "AuthInfo" :
            {
                "AuthMech" : "krb5",
                ... ...
                "SignMessage" : "true"
            }
  ```
  * To enable SSL, please refer to this [`Readme.md`](https://github.com/zhiming99/rpc-frmwrk/blob/master/rpc/sslport/Readme.md)
  
5. Start the rpcrouter with `-a` option, which is the authentication flag.

##### 5. More information
1. The communication of an authenticated session is encrypted or signed throughout the session's lifecycle.
2. The duration for authenticating process can last for about 2 minutes, if the process cannot complete during this period, the bridge side will reset the connection.
3. If the service ticket expires, the session will ends in 10 minutes.
4. Train yourself to get used to `kinit` and `klist`, which can be used frequently as the login method. `kinit`, as mentioned above, is to use the password to get the `ticket granting ticket`, which will be used to acquire the other `sevice tickes` when the client is trying to access some service. And `klist` is to list the tickets for an account, and the tickets include both `ticket granting ticket` and `service tickets`. You can check the timestamp to know if the ticket is expired, and need to login again.
5. Make sure the firewall not block the `kerberos` ports, especially port 88 on your `kdc` machine, for the access from service servers.
##### 6. Using rpcfg.py 
1.
[`rpcfg.py`](https://github.com/zhiming99/rpc-frmwrk/tree/master/tools#rpc-router-config-tool) has some useful buttons on the `security page` to make the learning process much easier by automatically setting the Kerberos environment. The taget use cases are for small business environment, where the simple authentication is enough to meet the user's requiremnt. If this is the case, deploying a working environment can be as fast as within a few minutes. For a big organization with established authentication infrastructure, it is better for you to read through the document and contact the adminstrator to find the solution.


