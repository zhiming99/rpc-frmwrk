
# 用户管理项目介绍

## 项目概述

`usermgmt` 目录下的脚本旨在提供一个简单的用户管理系统。该系统包括用户注册、用户组管理、用户属性修改等功能。通过一系列脚本和工具，管理员可以方便地管理用户和用户组，并进行相关的配置和操作。

## 目录结构

以下是 `usermgmt` 目录的主要结构和文件说明：

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

### 文件说明

- `inituser.sh`：初始化用户环境的脚本。创建必要的目录和文件，并挂载用户注册文件系统。
- `rpcfaddu.sh`：添加新用户的脚本。支持关联 Kerberos 用户名和 OAuth2 用户名，并将用户添加到指定的组。
- `rpcfaddg.sh`：添加新用户组的脚本。
- `rpcfmodu.sh`：修改用户属性的脚本。支持添加或移除用户的 Kerberos 用户名、OAuth2 用户名、用户组和密码。
- `rpcfshow.sh`：显示用户或用户组信息的脚本。支持显示 Kerberos 和 OAuth2 用户关联信息。
- `rpcfrmu.sh`：删除一个或多个用户。
- `rpcfrmg.sh`：删除一个或多个用户组。
- `updattr.py`：管理文件扩展属性的 Python 脚本。支持列出、更新和添加 JSON 数据到文件扩展属性中。

## 使用说明

### 初始化用户环境

运行 `inituser.sh` 脚本来初始化用户环境：

```sh
./inituser.sh
```

### 添加新用户

使用 `rpcfaddu.sh` 脚本添加新用户：

```sh
./rpcfaddu.sh -g <group> -k <kerberos_user> -o <oauth2_user> <username>
```

### 添加新用户组

使用 `rpcfaddg.sh` 脚本添加新用户组：

```sh
./rpcfaddg.sh <group_name>
```

### 修改用户属性

使用 `rpcfmodu.sh` 脚本修改用户属性：

```sh
./rpcfmodu.sh -k <kerberos_user> -o <oauth2_user> -g <group> -p <username>
```

### 显示用户或用户组信息

使用 `rpcfshow.sh` 脚本显示用户或用户组信息：

```sh
./rpcfshow.sh -u <username>
```

### 管理文件扩展属性

使用 `updattr.py` 脚本设置或者打印文件扩展属性：

```sh
python3 updattr.py -l <file_path>
```

### 删除用户

使用 `rpcfrmu.sh` 脚本设置删除一个或多个用户：

```sh
./rpcfrmu.sh <username>
```
### 删除用户组

使用 `rpcfrmg.sh` 脚本设置删除一个或多个用户组：

```sh
./rpcfrmg.sh <groupname>
```
## 命令手册
```
rpcfaddu(1), rpcfmodu(1), rpcfshow(1), updattr(1)
```

## 贡献

欢迎对该项目进行贡献。如果你有任何建议或改进，请提交 Pull Request 或联系项目维护者。

## 许可证

该项目基于 GNU GPLv3 许可证进行分发和使用。详情请参阅 LICENSE 文件。
