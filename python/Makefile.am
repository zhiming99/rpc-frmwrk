pysources = README.md gentoml.sh proxy.py proxy.sip pyproject.toml pyproject.toml.tmpl rpcf.sip server.sip

EXTRA_DIST= $(pysources) tests/actcancel/*.py  tests/evtest/*.py tests/iftest/*.py  tests/katest/*.py  tests/prtest/*.py  tests/sftest/*.py  tests/sftest/README.md tests/sftest/sfdesc.json tests/stmtest/*.py tests/stmtest/README.md

PYTHON_EXT=`python3 -c "import importlib.machinery;print('rpcbase',importlib.machinery.EXTENSION_SUFFIXES[0],sep='')"`

all:
	bash gentoml.sh
	sip-build --verbose --build-dir ./sipbuild
	if [ -f ./rpcf/$(PYTHON_EXT) ]; then rm ./rpcf/$(PYTHON_EXT); fi
	ln -s `pwd`/sipbuild/rpcbase/$(PYTHON_EXT) rpcf/

install:
	python3 setup.py install
