pysources = README.md gentoml.sh rpcf/proxy.py proxy.sip pyproject.toml pyproject.toml.tmpl rpcf.sip server.sip setup.py

EXTRA_DIST= $(pysources) tests/actcancel/*.py  tests/evtest/*.py tests/iftest/*.py  tests/katest/*.py  tests/prtest/*.py  tests/sftest/*.py  tests/sftest/README.md tests/sftest/sfdesc.json tests/stmtest/*.py tests/stmtest/README.md

if SIP4
SIPVER=4
endif

if SIPBUILD
SIPVER=5
endif

PYTHON_EXT1=`python3 -c "import importlib.machinery;print('rpcbase',importlib.machinery.EXTENSION_SUFFIXES[0],sep='')"`
PYTHON_EXT=$(PYTHON_EXT1).$(VERSION)

all:
	@if echo "$(COMMON_LINK)" | grep "latomic"; then export ARMBUILD="1"; else export ARMBUILD="0";fi;\
	if [ "x$(SIPVER)" == "x5" ]; then \
        bash gentoml.sh $(libdir);\
		sip-build --verbose --build-dir ./sipbuild; \
        rm rpcf/$(PYTHON_EXT1)*;\
        cp `pwd`/sipbuild/rpcbase/$(PYTHON_EXT1) rpcf/$(PYTHON_EXT) ;\
		cd rpcf; ln -s $(PYTHON_EXT) $(PYTHON_EXT1);\
    elif [ "x$(SIPVER)" == "x4" ]; then \
        export CC=$(CC); \
		export CXX=$(CXX); \
		export PKG_CONFIG=$(PKG_CONFIG);\
		if [ "x$(PKG_CONFIG_LIBDIR)" != "x" ];then export PKG_CONFIG_LIBDIR=$(PKG_CONFIG_LIBDIR);fi;\
		if [ "x$(PKG_CONFIG_PATH)" != "x" ]; then export PKG_CONFIG_PATH=$(PKG_CONFIG_PATH);fi;\
		if [ "x$(PKG_CONFIG_SYSROOT_DIR)" != "x" ];then export PKG_CONFIG_SYSROOT_DIR=$(PKG_CONFIG_SYSROOT_DIR);fi;\
		export SYSROOT=$(with_sysroot);\
        python3 build-4.9.py $(libdir); \
        rm rpcf/$(PYTHON_EXT1)*;\
        cp `pwd`/sip4build/rpcbase.so rpcf/$(PYTHON_EXT); \
		cd rpcf; ln -s $(PYTHON_EXT) $(PYTHON_EXT1);\
    fi

install: build_dpkg
	if [ "x$${SUDO_USER}" == "x" ];then \
		python3 setup.py bdist_wheel; \
	else \
		su -c "python3 setup.py bdist_wheel" $${SUDO_USER} ;\
	fi;
	@export pkg=./dist/`ls -1t dist | head -n 1`; \
	echo installing $${pkg} ....;\
	if [ "x$(DESTDIR)" == "x" ]; then \
		echo destdir is $(DESTDIR);\
		if [ "x$${SUDO_USER}" == "x" ];then \
			pip3 install --user $${pkg};\
		else \
			su -c "pip3 install --user $${pkg}" $${SUDO_USER};\
		fi; \
	else\
		cp $${pkg} $(DESTDIR)/$(libdir)/rpcf/; \
	fi

uninstall:
	@if [ "x$(DESTDIR)" == "x" ]; then \
		if [ "x$${SUDO_USER}" == "x" ];then \
			pip3 uninstall rpcf;\
		else \
			su -c "pip3 uninstall $${pkg}" $${SUDO_USER};\
		fi; \
	fi

build_dpkg:
	@export pkg=./dist/`ls -1t dist | head -n 1`;\
	if [ ! "x$(DESTDIR)" == "x" ]; then \
		bash blddpkg.sh $(DESTDIR) $(prefix) $${pkg}; \
	fi
