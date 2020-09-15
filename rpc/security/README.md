### Introduction to the rpc-frmwrk's Authentication support
The authentication process is the process to allow only valid users to access the rpc-frmwrk's service.
Currently the rpc-frmwrk relies only on the `Kerberos 5` authentication framework. In the future, 
we may add more authentication approaches besides the `Kerberos 5`.   

#### What does `Kerberos 5` do in rpc-frmwrk?
The following are some services, `Kerberos` can provides, and of course, it can provide far more
beyond the list:
 1. user/service account management, adding/modify/delete the user or service account.
 2. Grant tickets to the legistimate users, which could be used by the user to access all the
    services, within a `realm( similiar to domain )` without repeatedly typing passwords.
 3. Setup the secure sessions between the client and the services, and provide encryption/signature
    services for the session.
 4. Credential management and key caches, including credential's lifecycle management,
    providing storage for keys and credentials.

#### Setup Kerberos
Please don't be scared, setup kerberos for authentication is not as difficult as you thought. And it can
be deployed on a network as simple as with just two connected devices. Here are the steps to make Kerberos
to work with `rpc-frmwrk` on such a simple network.
1. Setup `kdc( Kerberos domain controller)`. You may want to select one of your machines as the kdc server.
  Depending on the linux distribution, the kerberos package name could be slightly different.
  * On a Fedora machine, you can `dnf install krb5-server`, and dnf will install all the necessary packages for you.
  * On a Raspberry Pi, you can use `apt install krb5-kdc` and apt will install all the necessary packages for you.
  * After the successful installation, you need to made changes to the `/etc/krb5.conf` and add the user and service accounts.
  and the following is a sample krb5 for reference. If your kdc does not have a dns entry, use the ip address as in the
  `[realms]` section instead or add the domain name in `/etc/hosts`.
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
        default_realm = rpcfrmwrk.org
        default_ccache_name = KEYRING:persistent:%{uid}

    [realms]
    rpcfrmwrk.org = {
        kdc = kdc.rpcfrmwrk.org
        admin_server = kdc.rpcfrmwrk.org
    }

    [domain_realm]
    .rpcfrmwrk.org = rpcfrmwrk.org
    rpcfrmwrk.org = rpcfrmwrk.org
```
  * Add the user account with `kadmin.local` on your kdc machine. `kadmin.local` does not require password and
  can only be used locally, while `kadmin` is a network version. Suppose you have a linux account name `foo`, and you can add a
  user account `foo` to the kerberos's user database. The fully qualified account name is `foo@rpcfrmwrk.org`, which must be put
  to the proxy's description file as the `user name` later, if authentication is enabled.
  
 * Add the service principal to the kerberos's user database with the `kadmin.local` as well. The service principal, looks like
  `rasp1/rpcfrmwrk.org`. `rasp1` is the machine name, and `rpcfrmwrk.org` after the slash is instance name to tell the service is
  `rpc-frmwrk`. Note that, `Kerberos` uses the term `principal` to represent an entity that participate in the `Kerberos's`
  authentication activity.
  
  * Start `krb5kdc`, the KDC server, and `kadmind` the adminstration deamon. And the kdc setup is done now. If you want advanced
  and exhaustive configuration options, please refer to the official document at [here](https://web.mit.edu/kerberos/krb5-devel/doc/admin/install_kdc.html#install-and-configure-the-master-kdc)
  
2. Setup the client machines. Client machines are those devices via which the users can access the the RPC servers.
  * On a Fedora machine, you can `dnf install krb5-workstation`, and dnf will install all the necessary packages for you.
  * On a Raspberry Pi, you can `apt install krb5-user`, and apt will install all the necessary packages for you,
      and help you configure the kerberos.

