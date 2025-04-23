# Define the main target for building the Python wheel package
.PHONY: all clean distclean install uninstall genautofile

javasrcpath=./org/rpcf/appmancli/
basefiles= AppManagercli.java maincli.java AppTimerProxy.java

EXTRA_DIST:=$(addprefix $(javasrcpath), $(basefiles))

top_dir=$(abs_top_srcdir)
jarname=appmancli-$(PACKAGE_VERSION).jar
shortform=appmancli.jar
jarpkg=dist/$(jarname)
ridlfile=$(top_dir)/monitor/appmon/appmon.ridl
autofiles=AppManagerclibase.java DeserialArrays.java  DeserialMaps.java  JavaSerialBase.java JavaSerialHelperP.java  JVariant.java  KeyValue.java StructFactory.java
autofiles_path:=$(addprefix $(javasrcpath), $(autofiles))
class_path:=$(addprefix dist/org/rpcf/appmancli, $(autofiles:.java=.class) $(basefiles:.java=.class) )

appmancli_DATA=$(jarpkg)
appmanclidir=$(libdir)/rpcf

# Default target
#all: $(jarpkg)

# Build the Python wheel package
$(jarpkg): $(class_path)
	if [[ "$(MAKECMDGOALS)" =~ "install" ]]; then echo > /dev/null;else \
	if [ ! -d ./dist ]; then mkdir ./dist; fi;\
    CLASSPATH=$(top_dir)/java/rpcbase.jar:/usr/share/java/commons-cli.jar:./:$$CLASSPATH \
    javac $(javasrcpath)*.java -d ./dist; \
	cd dist && ( find . -type f | xargs jar cf $(jarname) );fi

$(class_path) : $(autofiles_path) $(EXTRA_DIST)

$(autofiles_path): genautofile

genautofile:
	@bexit=0;for i in $(autofiles_path); do if [ ! -f $$i ];then bexit=1;break;fi;done; \
    if(($$bexit==0));then echo all files are ready;exit 0;fi; \
    if ! which ridlc > /dev/null; then \
        if [ ! -f ./mktpl ];then \
            cp $(top_dir)/ridl/{mktpl,pymktpl,mktplsvr,mktplcli,drvtpl.json,odesctpl2.json,synccfg.py} . ;\
        fi;\
    fi || true; \
	if [ -f Makefile ]; then cp Makefile Makefile_; fi; \
    if [ -f $(top_dir)/ridl/.libs/ridlc ]; then \
        echo bobo $(ridlfile);\
        cp synccfg.py $(javasrcpath); ls $(javasrcpath);\
        echo LD_LIBRARY_PATH=$(top_dir)/combase/.libs$${LD_LIBRARY_PATH} \
        $(top_dir)/ridl/.libs/ridlc --sync_mode IAppStore=sync --sync_mode IDataProducer=sync \
        --services=AppManager --client --pkgname=org.rpcf.appmancli -jslO . $(ridlfile); \
        LD_LIBRARY_PATH=$(top_dir)/combase/.libs$${LD_LIBRARY_PATH} \
        $(top_dir)/ridl/.libs/ridlc --sync_mode IAppStore=sync --sync_mode IDataProducer=sync \
        --services=AppManager --client --pkgname=org.rpcf.appmancli -jslO . $(ridlfile); \
        rm $(javasrcpath)/synccfg.py;\
    else \
        echo ridlc not found. Please build the whole project from the root directory first; exit 1; fi; \
	if [ -f Makefile_ ]; then mv Makefile_ Makefile; fi; \
	if [ -f ./mktpl ];then rm -f mktpl pymktpl mktplsvr mktplcli odesctpl2.json drvtpl.json;fi || true; \
    rm -f *.new; rm -f appmondesc.json driver.json *.md cmdline synccfg.py;

# Install the Python package using pip
install-data-hook:
	if [[ -z "$(DESTDIR)" ]]; then \
        if [ ! -d $(appmanclidir) ]; then mkdir -p $(appmanclidir); fi ;\
        pushd $(appmanclidir); if [ -f $(shortform) -o -L $(shortform) ]; then rm $(shortform);fi; \
        ln -s $(jarname) $(shortform); popd; \
    fi

# Uninstall the Python package
uninstall-local:
	@echo "Removing $(jarpkg) from $(libdir)/rpcf..."
	if [ -d $(appmanclidir) ]; then pushd $(appmanclidir); if [ -f $(shortform) -o -L $(shortform) ]; then rm $(shortform);fi; popd;fi

# Clean up build artifacts
clean:
	rm -r ./dist/* || rm -f $(javasrcpath)*.class || true
	rm  $(autofiles_path)
# Clean up everything, including generated files
distclean: clean

# Add additional targets if needed
