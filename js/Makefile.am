jslib_files=rpc/protopkt.js \
combase/configdb.js combase/factory.js combase/msgmatch.js combase/objbase.js \
combase/cbuffer.js combase/defines.js combase/enums.js combase/SERI_HEADER_BASE.js \
dbusmsg/LICENSE dbusmsg/align.js dbusmsg/constants.js dbusmsg/dbus-buffer.js \
dbusmsg/header-signature.json dbusmsg/marshall.js dbusmsg/marshallers.js \
dbusmsg/message.js dbusmsg/signature.js dbusmsg/unmarshall.js \
rpc/bridge.js rpc/dmsg.js combase/iomsg.js rpc/ioworker.js rpc/handshak.js \
rpc/enablevtrmt.js ipc/enablevt.js ipc/iomgr.js ipc/proxy.js \
rpc/fwrdreqrmt.js ipc/fwrdreq.js combase/seribase.js rpc/fwrdevtrmt.js \
ipc/fwrdevt.js ipc/cancelrq.js ipc/keepalive.js rpc/keepalivermt.js \
ipc/openstm.js rpc/openstmrmt.js rpc/stream.js \
ipc/stmwrite.js ipc/stmevt.js rpc/stmwritermt.js

js_files=index.html index.js webpack.config.js test/evtest.js test/asynctst.js \
test/actcancel.js test/katest.js test/stmtest.js $(jslib_files)

EXTRA_DIST=$(js_files)

js_confdir=$(sysconfdir)/rpcf
dist_js_conf_DATA=rpcbase.js.tar.gz
BUILT_SOURCES=rpcbase.js.tar.gz

rpcbase.js.tar.gz:
	tar zcf rpcbase.js.tar.gz $(jslib_files)

install-data-hook:
	if [ ! -d $(DESTDIR)$(js_confdir)/jslib ]; then mkdir -p $(DESTDIR)$(js_confdir)/jslib; fi;
	tar xf $(DESTDIR)$(js_confdir)/rpcbase.js.tar.gz -C $(DESTDIR)$(js_confdir)/jslib
	rm $(DESTDIR)$(js_confdir)/rpcbase.js.tar.gz

uninstall-hook:
	if [ -d $(js_confdir)/jslib ];then cd $(js_confdir) && rm -rf ./jslib;fi;
