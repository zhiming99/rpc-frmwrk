#!/bin/bash
DBUS_INC=$(echo $(pkg-config --cflags dbus-1 jsoncpp) | sed 's/^-I/, "/g;s/ -I/", "/g;s/\(.*\)$/\1"/' | sed 's:/:\\/:g')
sed "s/XXXXXXXX/$DBUS_INC/" pyproject.toml.tmpl > pyproject.toml
