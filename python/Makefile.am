pysources = README.md gentoml.sh proxy.py proxy.sip pyproject.toml pyproject.toml.tmpl rpcfrmwrk.sip server.sip

EXTRA_DIST= $(pysources) tests/actcancel/*.py  tests/evtest/*.py tests/iftest/*.py  tests/katest/*.py  tests/prtest/*.py  tests/sftest/*.py  tests/sftest/README.md tests/sftest/sfdesc.json tests/stmtest/*.py tests/stmtest/README.md

build:
	bash gentoml.sh pyprogect.toml.tmpl
	sip-build --verbose

inst:
	bash gentoml.sh pyprogect.toml.tmpl
	sip-install --verbose
