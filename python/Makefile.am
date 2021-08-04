pysources = README.md gentoml.sh rpcf/proxy.py proxy.sip pyproject.toml pyproject.toml.tmpl rpcf.sip server.sip setup.py

EXTRA_DIST= $(pysources) tests/actcancel/*.py  tests/evtest/*.py tests/iftest/*.py  tests/katest/*.py  tests/prtest/*.py  tests/sftest/*.py  tests/sftest/README.md tests/sftest/sfdesc.json tests/stmtest/*.py tests/stmtest/README.md

PYTHON_EXT=`python3 -c "import importlib.machinery;print('rpcbase',importlib.machinery.EXTENSION_SUFFIXES[0],sep='')"`

if SIP4
SIPVER=4
endif

if SIPBUILD
SIPVER=5
endif

PYTHON_EXT=`python3 -c "import importlib.machinery;print('rpcbase',importlib.machinery.EXTENSION_SUFFIXES[0],sep='')"`

all:
	@if [ "x$(SIPVER)" == "x5" ]; then \
        bash gentoml.sh;\
		sip-build --verbose --build-dir ./sipbuild; \
        if [ -f ./rpcf/$(PYTHON_EXT) ]; then rm ./rpcf/$(PYTHON_EXT); fi;\
        ln -s `pwd`/sipbuild/rpcbase/$(PYTHON_EXT) rpcf/ ;\
    elif [ "x$(SIPVER)" == "x4" ]; then \
        python3 build-4.9.py; \
        if [ -f ./rpcf/$(PYTHON_EXT) ]; then rm ./rpcf/$(PYTHON_EXT); fi;\
        ln -s `pwd`/sip4build/rpcbase.so rpcf/$(PYTHON_EXT); \
    fi

install:
	python3 setup.py install
