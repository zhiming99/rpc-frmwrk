### Introduction to oinit
**oinit** is a small tool to perform the `OAuth2 authorization code` login to an rpc-frmwrk server from a non-JS clients, such as a C++, Python or Java client.
It can remember the login information and credentials for next login if the credentials are not expired. And you can also remove the stored login information from the commandline.

### Invoke oinit from command line
* running `python3 oinit.py [options] [<auth url> <client id> <redirect url> <scope>]`
* if you have installed the `python3-oinit` debian package, you can use the following command   
`python3 -m oinit [options] [<auth url> <client id> <redirect url> <scope>]`
* **-d** < description file >. To find the login information from the given desc file
* **-c 1** Using the example container 'django_oa2check_https' for OAuth2 login
* **-c 2** Using the example container 'springboot_oa2check_https' for OAuth2 login
* **-e** to encrypt the login cookie in the storage
* **-r** to remove from a list of stored login information.
* **-f** By default, oinit uses 'chrome' for login. and If `-f` is specified, oinit will use firefox to perform the login.
* If 'authorize url', 'client id', 'redirect url' and 'scope' are specified oinit.py will use the four to perform the OAuth2 login.
* If neither options nor parameters are given, oinit will try to use the stored login info in the registry to perform the OAuth2 login.
And you only need to run this command next time when the access token expired.

### Dependency:
`oinit` depends on the `selenium` for login via UI, which support both `firefox` and `chrome` to do the authentication via the OAuth2 provider's web page. And there fore, you need to make sure your client machine has `chrome` of `firefox` installed. And even more importantly, make sure the browser driver installed which `selenium` uses to communicate with the browsers. `firefox`'s driver is [here](https://github.com/mozilla/geckodriver/releases), and `chrome`'s driver is at this [site](https://developer.chrome.com/docs/chromedriver/downloads). If `oinit` fails to start the browser, it is probably due to the driver missing or unable to find  the driver via the environment variable `PATH` .