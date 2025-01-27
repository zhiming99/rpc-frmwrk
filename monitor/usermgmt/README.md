# User Management Project Introduction

## Project Overview

The scripts in the `usermgmt` directory aim to provide a simple user management system. This system includes user registration, user group management, user attribute modification, and other functions. Through a series of scripts and tools, administrators can easily manage users and user groups and perform related configurations and operations.

## Directory Structure

Below is the main structure and file description of the `usermgmt` directory:

```
usermgmt/
├── inituser.sh
├── rpcfaddu.sh
├── rpcfaddg.sh
├── rpcfmodu.sh
├── rpcfshow.sh
├── rpcfrmu.sh
├── rpcfrmg.sh
└── updattr.py
```

### File Descriptions

- `inituser.sh`: Script to initialize the user environment. Creates necessary directories and files, and mounts the user registry file system.
- `rpcfaddu.sh`: Script to add new users. Supports associating Kerberos usernames and OAuth2 usernames, and adding users to specified groups.
- `rpcfaddg.sh`: Script to add new user groups.
- `rpcfmodu.sh`: Script to modify user attributes. Supports adding or removing Kerberos association, OAuth2 association, user groups, and passwords.
- `rpcfshow.sh`: Script to display user or user group information. Supports displaying Kerberos and OAuth2 association information.
- `rpcfrmu.sh`: Script to remove one or more users.
- `rpcfrmg.sh`: Script to remove one or more user groups.
- `updattr.py`: Python script to manage file extended attributes. Supports listing, updating, and adding JSON data to file extended attributes.

## Usage Instructions

### Initialize User Environment

Run the `inituser.sh` script to initialize the user environment:

```sh
./inituser.sh
```

### Add New User

Use the `rpcfaddu.sh` script to add a new user:

```sh
./rpcfaddu.sh -g <group> -k <kerberos_user> -o <oauth2_user> <username>
```

### Add New User Group

Use the `rpcfaddg.sh` script to add a new user group:

```sh
./rpcfaddg.sh <group_name>
```

### Modify User Attributes

Use the `rpcfmodu.sh` script to modify user attributes:

```sh
./rpcfmodu.sh -k <kerberos_user> -o <oauth2_user> -g <group> -p <username>
```

### Display User or User Group Information

Use the `rpcfshow.sh` script to display user or user group information:

```sh
./rpcfshow.sh -u <username>
```

### Manage File Extended Attributes

Use the `updattr.py` script to set or print file extended attributes:

```sh
python3 updattr.py -l <file_path>
```

### Remove Users

Use the `rpcfrmu.sh` script to remove one or more users:

```sh
./rpcfrmu.sh <username>
```

### Remove User Groups

Use the `rpcfrmg.sh` script to remove one or more user groups:

```sh
./rpcfrmg.sh <groupname>
```

## Command Manuals
```
rpcfaddu(1), rpcfmodu(1), rpcfshow(1), updattr(1)
```

## Contribution

Contributions to this project are welcome. If you have any suggestions or improvements, please submit a Pull Request or contact the project maintainers.

## License

This project is distributed and used under the GNU GPLv3 license. For details, please refer to the LICENSE file.