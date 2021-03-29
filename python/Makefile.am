pysources = README.md gentoml.sh proxy.py proxy.sip pyproject.toml pyproject.toml.tmpl rpcf.sip server.sip

EXTRA_DIST= $(pysources) tests/actcancel/*.py  tests/evtest/*.py tests/iftest/*.py  tests/katest/*.py  tests/prtest/*.py  tests/sftest/*.py  tests/sftest/README.md tests/sftest/sfdesc.json tests/stmtest/*.py tests/stmtest/README.md

PYTHON_EXT=`python getname.py`

all:
	bash gentoml.sh
	sip-build --verbose

inst:
	@echo $(PYTHON_EXT)
