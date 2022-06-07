#!/bin/bash
# script adapted from https://github.com/ist-dsi/docker-kerberos
REALM=rpcf.org
SUPPORTED_ENCRYPTION_TYPES="aes256-cts:normal aes128-cts:normal"
KADMIN_PRINCIPAL="kadmin/admi"
KADMIN_PASSWORD="MITiys5K6"

echo "==================================================================================="
echo "==== Kerberos KDC and Kadmin ======================================================"
echo "==================================================================================="
KADMIN_PRINCIPAL_FULL=$KADMIN_PRINCIPAL@$REALM

echo "REALM: $REALM"
echo "KADMIN_PRINCIPAL_FULL: $KADMIN_PRINCIPAL_FULL"
echo "KADMIN_PASSWORD: $KADMIN_PASSWORD"
echo ""

echo "==================================================================================="
echo "==== /etc/krb5.conf ==============================================================="
echo "==================================================================================="

echo '127.0.0.1	kdc.rpcf.org rasp1' >> /etc/hosts
KDC_KADMIN_SERVER=kdc.rpcf.org

tee /etc/krb5.conf <<EOF
[libdefaults]
	default_realm = $REALM

[realms]
	$REALM = {
		kdc_ports = 88,750
		kadmind_port = 749
		kdc = $KDC_KADMIN_SERVER
		admin_server = $KDC_KADMIN_SERVER
	}
EOF
echo ""

echo "==================================================================================="
echo "==== /etc/krb5kdc/kdc.conf ========================================================"
echo "==================================================================================="
tee /etc/krb5kdc/kdc.conf <<EOF
[realms]
	$REALM = {
		acl_file = /etc/krb5kdc/kadm5.acl
		max_renewable_life = 7d 0h 0m 0s
		supported_enctypes = $SUPPORTED_ENCRYPTION_TYPES
		default_principal_flags = +preauth
	}
EOF
echo ""

echo "==================================================================================="
echo "==== /etc/krb5kdc/kadm5.acl ======================================================="
echo "==================================================================================="
tee /etc/krb5kdc/kadm5.acl <<EOF
$KADMIN_PRINCIPAL_FULL *
rasp1/$REALM@$REALM *
zhiming@$REALM *
noPermissions@$REALM X
EOF
echo ""

echo "==================================================================================="
echo "==== Creating realm ==============================================================="
echo "==================================================================================="
#MASTER_PASSWORD=$(tr -cd '[:alnum:]' < /dev/urandom | fold -w30 | head -n1)
MASTER_PASSWORD="MITiys7K8"

# This command also starts the krb5-kdc and krb5-admin-server services
krb5_newrealm <<EOF
$MASTER_PASSWORD
$MASTER_PASSWORD
EOF
echo ""

echo "==================================================================================="
echo "==== Create the principals in the acl ============================================="
echo "==================================================================================="
echo "Adding $KADMIN_PRINCIPAL principal"
kadmin.local -q "delete_principal -force $KADMIN_PRINCIPAL_FULL"
echo ""
kadmin.local -q "addprinc -pw $KADMIN_PASSWORD $KADMIN_PRINCIPAL_FULL"
echo ""

echo "Adding noPermissions principal"
kadmin.local -q "delete_principal -force noPermissions@$REALM"
echo ""
kadmin.local -q "addprinc -pw $KADMIN_PASSWORD noPermissions@$REALM"
echo ""

echo "Adding 'zhiming' principal"
kadmin.local -q "delete_principal -force zhiming@$REALM"
echo ""
kadmin.local -q "addprinc -pw $KADMIN_PASSWORD zhiming@$REALM"
echo ""

echo "Adding 'rasp1' principal"
kadmin.local -q "delete_principal -force rasp1/$REALM@$REALM"
echo ""
kadmin.local -q "addprinc -pw $KADMIN_PASSWORD rasp1/$REALM@$REALM"
echo ""

echo "gen keytable for 'rasp1' & 'zhiming' "
kadmin.local -q "ktadd zhiming@$REALM rasp1/$REALM@$REALM"
echo ""

echo "==================================================================================="
echo "==== Run the services ============================================================="
echo "==================================================================================="
# We want the container to keep running until we explicitly kill it.
# So the last command cannot immediately exit. See
#   https://docs.docker.com/engine/reference/run/#detached-vs-foreground
# for a better explanation.
ls /sbin/k* -l
/sbin/krb5kdc
/sbin/kadmind -nofork
