# Copyright © (2011) Institut national de l'information
#                    géographique et forestière 
# 
# Géoportail SAV <contact.geoservices@ign.fr>
# 
# This software is a computer program whose purpose is to publish geographic
# data using OGC WMS and WMTS protocol.
# 
# This software is governed by the CeCILL-C license under French law and
# abiding by the rules of distribution of free software.  You can  use, 
# modify and/ or redistribute the software under the terms of the CeCILL-C
# license as circulated by CEA, CNRS and INRIA at the following URL
# "http://www.cecill.info". 
# 
# As a counterpart to the access to the source code and  rights to copy,
# modify and redistribute granted by the license, users are provided only
# with a limited warranty  and the software's author,  the holder of the
# economic rights,  and the successive licensors  have only  limited
# liability. 
# 
# In this respect, the user's attention is drawn to the risks associated
# with loading,  using,  modifying and/or developing or reproducing the
# software by the user in light of its specific status of free software,
# that may mean  that it is complicated to manipulate,  and  that  also
# therefore means  that it is reserved for developers  and  experienced
# professionals having in-depth computer knowledge. Users are therefore
# encouraged to load and test the software's suitability as regards their
# requirements in conditions enabling the security of their systems and/or 
# data to be ensured and,  more generally, to use and operate it in the 
# same conditions as regards security. 
# 
# The fact that you are presently reading this means that you have had
# 
# knowledge of the CeCILL-C license and that you accept its terms.

################################################################################

=begin nd
File: Shell.pm

Class: BE4::Shell

(see ROK4GENERATION/libperlauto/BE4_Shell.png)

Configure and assemble commands used to generate raster pyramid's slabs.

All schemes in this page respect this legend :

(see ROK4GENERATION/tools/formats.png)

Using:
    (start code)
    use BE4::Shell;

    if (! BE4::Shell::setGlobals($commonTempDir, $useMasks)) {
        ERROR ("Cannot initialize Shell commands for BE4");
        return FALSE;
    }

    my $scriptInit = BE4::Shell::getScriptInitialization($pyramid);
    (end code)
=cut

################################################################################

package COMMON::Shell;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################

use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
#                            Group: SWIFT                                      #
################################################################################

our $SWIFT_KEYSTONE_TOKEN_FUNCTION = <<'FUNCTION';
SWIFT_TOKEN_DATE=0
SWIFT_TOKEN=""
GetSwiftToken () {
    
    tokenAge=$(( $(date +"%s") - ${SWIFT_TOKEN_DATE} ))
    if [[ ! -z $SWIFT_TOKEN && "$tokenAge" -lt "3600" ]]; then
        return
    fi

    curl_options=""
    if [[ ! -z $ROK4_SSL_NO_VERIFY ]]; then
        curl_options="-k"
    fi

    SWIFT_TOKEN=$(curl -s -i $curl_options \
        -H "Content-Type: application/json" \
        -X POST \
        -d '
    {   "auth": {
            "scope": {
                "project": {"id": "'$ROK4_KEYSTONE_PROJECTID'"}
            },
            "identity": {
                "methods": ["password"],
                "password": {
                    "user": {
                        "domain": {"name": "'$ROK4_KEYSTONE_DOMAINID'"},
                        "password": "'$ROK4_SWIFT_PASSWD'",
                        "name": "'$ROK4_SWIFT_USER'"
                    }
                }
            }
        }
    }' ${ROK4_SWIFT_AUTHURL} | grep "X-Subject-Token")

    # trailing new line removal
    # sed options :
    #   :a = create label a to jump back.
    #   N  = append the next line of input into the pattern space.
    #   $! = if it's not the last line...
    #       ba = jump back to label a
    SWIFT_TOKEN=$(echo "$SWIFT_TOKEN" | sed -E '/^[[:space:]]*$/d' | sed -E 's/^[[:space:]]+//g' | sed -E 's/[[:space:]]+$//g' | sed -E ':a;N;$!ba;s/[\n\r]//g' | sed -E 's/X-Subject-Token/X-Auth-Token/')
    export SWIFT_TOKEN
    
    SWIFT_TOKEN_DATE=$(date +"%s")
    export SWIFT_TOKEN_DATE

    echo ${SWIFT_TOKEN} > ${ROK4_SWIFT_TOKEN_FILE}
    sed -E '/^[[:space:]]*$/d' -i ${ROK4_SWIFT_TOKEN_FILE}
    sed -E ':a;N;$!ba;s/[\n\r]//g' -i ${ROK4_SWIFT_TOKEN_FILE}
}
FUNCTION

our $SWIFT_NATIVE_TOKEN_FUNCTION = <<'FUNCTION';
SWIFT_TOKEN_DATE=0
SWIFT_TOKEN=""
GetSwiftToken (){

    tokenAge=$(( $(date +"%s") - ${SWIFT_TOKEN_DATE} ))
    if [[ ! -z $SWIFT_TOKEN && "$tokenAge" -lt "3600" ]]; then
        return
    fi

    curl_options=""
    if [[ ! -z $ROK4_SSL_NO_VERIFY ]]; then
        curl_options="-k"
    fi

    SWIFT_TOKEN=$(curl -s -i $curl_options \
        -H "X-Storage-User: '${ROK4_SWIFT_ACCOUNT}':'${ROK4_SWIFT_USER}'" \
        -H "X-Storage-Pass: '${ROK4_SWIFT_PASSWD}'" \
        -H "X-Auth-User: '${ROK4_SWIFT_ACCOUNT}':'${ROK4_SWIFT_USER}'" \
        -H "X-Auth-Key: '${ROK4_SWIFT_PASSWD}'" \
        -X GET \
    }' ${ROK4_SWIFT_AUTHURL} | grep "X-Auth-Token")

    # trailing new line removal
    # sed options :
    #   :a = create label a to jump back.
    #   N  = append the next line of input into the pattern space.
    #   $! = if it's not the last line...
    #       ba = jump back to label a
    SWIFT_TOKEN=$(echo "$SWIFT_TOKEN" | sed -E '/^[[:space:]]*$/d' | sed -E 's/^[[:space:]]+//g' | sed -E 's/[[:space:]]+$//g' | sed -E ':a;N;$!ba;s/[\n\r]//g')
    export SWIFT_TOKEN
    
    SWIFT_TOKEN_DATE=$(date +"%s")
    export SWIFT_TOKEN_DATE

    echo $SWIFT_TOKEN > ${ROK4_SWIFT_TOKEN_FILE}
    sed -E '/^[[:space:]]*$/d' -i ${ROK4_SWIFT_TOKEN_FILE}
    sed -E ':a;N;$!ba;s/[\n\r]//g' -i ${ROK4_SWIFT_TOKEN_FILE}
}
FUNCTION

our $SWIFT_BACKUPLIST = <<'FUNCTION';
BackupListFile () {
    local objectName=$(basename ${LIST_FILE})

    resource="/${PYR_CONTAINER}/${objectName}"
    
    GetSwiftToken

    curl_options=""
    if [[ ! -z $ROK4_SSL_NO_VERIFY ]]; then
        curl_options="-k"
    fi

    curl $curl_options --fail -X PUT -T "${LIST_FILE}" -H "${SWIFT_TOKEN}" "${ROK4_SWIFT_PUBLICURL}${resource}"
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

################################################################################
#                            Group: S3                                         #
################################################################################

our $S3_BACKUPLIST = <<'FUNCTION';
HOST=$(echo ${ROK4_S3_URL} | sed 's!.*://!!' | sed 's!:[0-9]\+$!!')
BackupListFile () {
    local objectName=$(basename ${LIST_FILE})

    resource="/${PYR_BUCKET_DST}/${objectName}"
    contentType="application/octet-stream"
    dateValue=`TZ=GMT date -R`
    stringToSign="PUT\n\n${contentType}\n${dateValue}\n${resource}"

    signature=`echo -en ${stringToSign} | openssl sha1 -hmac ${ROK4_S3_SECRETKEY} -binary | base64`

    curl_options=""
    if [[ ! -z $ROK4_SSL_NO_VERIFY ]]; then
        curl_options="-k"
    fi

    curl $curl_options --fail -X PUT -T "${LIST_FILE}" \
     -H "Host: ${HOST}" \
     -H "Date: ${dateValue}" \
     -H "Content-Type: ${contentType}" \
     -H "Authorization: AWS ${ROK4_S3_KEY}:${signature}" \
     ${ROK4_S3_URL}${resource}

    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

################################################################################
#                            Group: CEPH                                       #
################################################################################

our $CEPH_BACKUPLIST = <<'FUNCTION';
BackupListFile () {
    local objectName=$(basename ${LIST_FILE})
    rados -p ${PYR_POOL} put ${objectName} ${LIST_FILE}
}
FUNCTION

################################################################################
#                            Group: FILE                                       #
################################################################################

our $FILE_BACKUPLIST = <<'FUNCTION';
BackupListFile () {
    bn=$(basename ${LIST_FILE})
    if [ "$(stat -c "%d:%i" ${LIST_FILE})" != "$(stat -c "%d:%i" ${PYR_DIR}/$bn)" ]; then
        cp ${LIST_FILE} ${PYR_DIR}/
    else
        echo "List file is already locate to the backup destination"
    fi
}
FUNCTION
  
1;
__END__
