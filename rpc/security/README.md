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
  1. Setup a `kdc( Kerberos domain controller)`. You need to select one of your machines as the kdc server.
  Depending on the linux distribution, the kerberos package name could be slightly different. On a Fedora
  machine, for example, you can `dnf install krb5-server`, and dnf will install all the necessary packages.
