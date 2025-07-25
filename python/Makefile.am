pysources = README.md gentoml.sh rpcf/proxy.py rpcf/__init__.py proxy.sip pyproject.toml.tmpl rpcf.sip server.sip setup.py build-4.9.py blddpkg.sh stripsym.sh rpcf/iolib.py rpcf/serijson.py fastrpc.sip fuseif.sip

EXTRA_DIST= $(pysources) tests/actcancel/*.py  tests/evtest/*.py tests/iftest/*.py  tests/katest/*.py  tests/prtest/*.py  tests/sftest/*.py  tests/sftest/README.md tests/sftest/sfdesc.json tests/stmtest/*.py tests/stmtest/README.md

if SIP4
SIPVER=4
endif

if SIPBUILD
SIPVER=5
endif

if ENABLE_FUSE3
LINK_FUSE=1
else
LINK_FUSE=0
endif

PYTHON_EXT1=`python3 -c "import importlib.machinery;print('rpcbase',importlib.machinery.EXTENSION_SUFFIXES[0],sep='')"`
PYTHON_EXT=$(PYTHON_EXT1).$(VERSION)

all:
	@if echo "$(COMMON_LINK)" | grep "latomic"; then export ARMBUILD="1"; else export ARMBUILD="0";fi;\
	if [ "x$(SIPVER)" == "x5" ]; then \
        FUSE3="$(LINK_FUSE)" bash gentoml.sh $(libdir);\
		sip-build --verbose --build-dir ./sipbuild; \
        rm -f rpcf/rpcbase*.so*;\
		sofile=$$(find ./sipbuild -name $(PYTHON_EXT1));\
        cp `pwd`/$${sofile} rpcf/$(PYTHON_EXT) ;\
		cd rpcf; ln -s $(PYTHON_EXT) $(PYTHON_EXT1);\
    elif [ "x$(SIPVER)" == "x4" ]; then \
        export FUSE3="$(LINK_FUSE)";\
        export CC=$(CC); \
		export CXX=$(CXX); \
		export PKG_CONFIG=$(PKG_CONFIG);\
		if [ "x$(PKG_CONFIG_LIBDIR)" != "x" ];then export PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR);fi;\
		if [ "x$(PKG_CONFIG_PATH)" != "x" ]; then export PKG_CONFIG_PATH=$(PKG_CONFIG_PATH);fi;\
		if [ "x$(PKG_CONFIG_SYSROOT_DIR)" != "x" ];then export PKG_CONFIG_SYSROOT_DIR=$(PKG_CONFIG_SYSROOT_DIR);fi;\
		export SYSROOT=$(with_sysroot); export DBGBUILD=0;\
        if grep 'CPPFLAGS.*\-O0 \-ggdb \-DDEBUG' Makefile > /dev/null; then export DBGBUILD=1;fi;\
        python3 build-4.9.py $(libdir); \
        rm -f rpcf/rpcbase*.so*;\
        cp `pwd`/sip4build/rpcbase.so rpcf/$(PYTHON_EXT); \
		cd rpcf; ln -s $(PYTHON_EXT) $(PYTHON_EXT1);\
    fi

genpipopt:
	$(eval pipopt=$(shell os_name=$$(cat /etc/issue | awk '{print $$1}');\
    if [ "x$${os_name}" == "xDebian" ]; then \
        os_version=$$(cat /etc/issue | awk '{print $$3}');\
        echo $$(if (($${os_version} > 11 ));then echo '--break-system-packages';else echo ''; fi); \
    elif [ "x$${os_name}" == "xUbuntu" ]; then \
        os_version=$$(cat /etc/issue | awk '{print $$2}' | awk -F'.' '{print $$1}');\
        echo $$(if (($${os_version} > 22 ));then echo '--break-system-packages';else echo ''; fi); \
    fi))
	@if [ "x${pipopt}" != "x" ]; then \
        echo Warning: break-system-packages enabled; \
    fi;

install: genpipopt
	@if [ "x$(DESTDIR)" != "x" ];then \
        cp rpcf/$(PYTHON_EXT) ./$(PYTHON_EXT).bak; \
        symdir=`pwd`/../bldpkg/symlibs/dbgsyms;\
        if [ ! -d $${symdir} ];then mkdir -p $${symdir};fi;\
        bash ./stripsym.sh rpcf/$(PYTHON_EXT) $${symdir};\
    fi;
	@if [ "x$${SUDO_USER}" == "x" ];then \
		python3 setup.py bdist_wheel; \
	else \
		su -c "python3 setup.py bdist_wheel" $${SUDO_USER} ;\
	fi;
	@export pkg=./dist/`ls -1t dist | head -n 1`; \
	echo installing $${pkg} ....;\
	if [ "x$(DESTDIR)" == "x" ]; then \
		echo destdir is $(DESTDIR);\
		if [ "x$${SUDO_USER}" == "x" ];then \
			pip3 install ${pipopt} --force-reinstall --user $${pkg};\
		else \
			su -c "pip3 install ${pipopt} --force-reinstall --user $${pkg}" $${SUDO_USER};\
		fi; \
	else\
        mv ./$(PYTHON_EXT).bak rpcf/$(PYTHON_EXT); \
		cp $${pkg} $(DESTDIR)/$(libdir)/rpcf/;\
		bash blddpkg.sh `pwd`/../ $(libdir) $${pkg}; \
	fi

uninstall: genpipopt
	@if [ "x$(DESTDIR)" == "x" ]; then \
		if [ "x$${SUDO_USER}" == "x" ];then \
			pip3 uninstall -y ${pipopt} @PACKAGE_NAME@;\
		else \
			su -c "pip3 uninstall -y ${pipopt} @PACKAGE_NAME@" $${SUDO_USER};\
		fi; \
	fi

