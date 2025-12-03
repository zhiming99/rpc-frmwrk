# Define the main target for building the Python wheel package
.PHONY: all clean distclean install uninstall

EXTRA_DIST=appmancli/AppManagercli.py appmancli/maincli.py pyproject.toml.in appmancli/apptimer.py setup.py.in

top_dir=$(abs_top_srcdir)
ridlfile=$(top_dir)/monitor/appmon/appmon.ridl

# Default target
all: appmancli/__init__.py dist/appmancli-$(PACKAGE_VERSION)-py3-none-any.whl 
	if [ ! -d ../dist ];then mkdir ../dist; fi
	cp dist/appmancli*.whl ../dist

# Build the Python wheel package
dist/appmancli-$(PACKAGE_VERSION)-py3-none-any.whl: appmancli/AppManagercli.py appmancli/maincli.py appmancli/apptimer.py
	python3 setup.py bdist_wheel
#python3 -m build --wheel .

appmancli/__init__.py:
	@pushd appmancli; \
	bexit=0;if [ ! -f __init__.py ];then bexit=1;fi; \
    if(($$bexit==0));then echo all files are ready;exit 0;fi; \
    if true; then \
        if [ ! -f ./mktpl ];then \
            cp $(top_dir)/ridl/{mktpl,mktplsvr,pymktpl,mktplcli,drvtpl.json,odesctpl2.json,synccfg.py} . ;\
        fi;\
    fi || true; \
	if [ -f Makefile ]; then cp Makefile Makefile_; fi; \
    if [ -f $(top_dir)/ridl/.libs/ridlc ]; then \
        LD_LIBRARY_PATH=$(top_dir)/combase/.libs:$${LD_LIBRARY_PATH} \
        $(top_dir)/ridl/.libs/ridlc --sync_mode IAppStore=sync --sync_mode IDataProducer=sync \
        --services=AppManager --client -pslO . $(ridlfile); \
    else \
        echo ridlc not found. Please build the whole project from the root directory first; exit 1; fi; \
	if [ -f Makefile_ ]; then mv Makefile_ Makefile; fi; \
	if [ -f ./mktpl ];then rm -f mktpl pymktpl mktplsvr mktplcli odesctpl2.json drvtpl.json;fi || true; \
    rm -f *.new; rm -f appmondesc.json driver.json *.md cmdline synccfg.py;\
    popd

# Install the Python package using pip
install:
	if command -v lsb_release >/dev/null 2>&1; then \
        PLATFORM=$$(lsb_release -is 2>/dev/null || echo "Unknown"); \
        if [[ " $${PLATFORM[@]} " =~ " Ubuntu " ]]; then \
            PLATFORM="Ubuntu"; \
        elif [[ " $${PLATFORM[@]} " =~ " Debian " ]]; then  \
            PLATFORM="Debian"; \
        else \
            PLATFORM="Unknown"; \
        fi; \
    else \
        PLATFORM=$$(grep '^ID=' /etc/os-release | cut -d'=' -f2 | tr -d '"'); \
    fi; \
    if [[ "$${PLATFORM}" == "Ubuntu" ]] || [[ "$${PLATFORM}" == "Debian" ]]; then \
        pipopt='--break-system-packages'; \
    else \
        pipopt='';\
    fi; \
    if [[ "$$LD_LIBRARY_PATH" == *libfakeroot* ]]; then \
        exit 0; \
    fi; \
	if [[ "x$${SUDO_USER}" != "x" ]]; then \
        echo su -c 'pip3 install $${pipopt} dist/*.whl' $${SUDO_USER}; \
        su -c "pip3 install --force-reinstall $${pipopt} dist/*.whl" $${SUDO_USER} || true; \
    else \
        pip3 install --force-reinstall $${pipopt} dist/*.whl || true;\
    fi

# Uninstall the Python package
uninstall:
	if command -v lsb_release >/dev/null 2>&1; then \
        PLATFORM=$$(lsb_release -is 2>/dev/null || echo "Unknown"); \
        if [[ " $${PLATFORM[@]} " =~ " Ubuntu " ]]; then \
            PLATFORM= "Ubuntu"; \
        elif [[ " $${PLATFORM[@]} " =~ " Debian " ]]; then  \
            PLATFORM="Debian"; \
        else \
            PLATFORM="Unknown"; \
        fi; \
    else \
        PLATFORM=$$(grep '^ID=' /etc/os-release | cut -d'=' -f2 | tr -d '"'); \
    fi; \
    if [[ "$${PLATFORM}" == "Ubuntu" ]] || [[ "$${PLATFORM}" == "Debian" ]]; then \
        pipopt="--break-system-packages"; \
    fi; \
	if [[ "x$${SUDO_USER}" != "x" ]]; then \
        su -c "pip3 uninstall -y $${pipopt} appmancli" $${SUDO_USER}; \
    else \
        pip3 uninstall -y $${pipopt} appmancli;\
    fi

# Clean up build artifacts
clean:
	rm -rf build dist *.egg-info

# Clean up everything, including generated files
distclean: clean
	rm -rf __pycache__

# Add additional targets if needed
