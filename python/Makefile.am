pysources = README.md gentoml.sh proxy.py proxy.sip pyproject.toml pyproject.toml.tmpl rpcf.sip server.sip

EXTRA_DIST= $(pysources) tests/actcancel/*.py  tests/evtest/*.py tests/iftest/*.py  tests/katest/*.py  tests/prtest/*.py  tests/sftest/*.py  tests/sftest/README.md tests/sftest/sfdesc.json tests/stmtest/*.py tests/stmtest/README.md

PYTHON_EXT=`python3 -c "import importlib.machinery;print('rpcbase',importlib.machinery.EXTENSION_SUFFIXES[0],sep='')"`

#bash gentoml.sh
#sip-build --verbose

all:
	bash gentoml.sh
	sip-build --verbose
	ln -s `pwd`/build/rpcbase/$(PYTHON_EXT) rpcf/

install:
	python3 setup.py install
