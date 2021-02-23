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

Class: PYR2PYR::Shell

(see ROK4GENERATION/libperlauto/PYR2PYR_Shell.png)

Provides functions to generate slab for 4head tool.

Using:
    (start code)
    use PYR2PYR::Shell;

    my $scriptInit = PYR2PYR::Shell::getScriptInitialization($pyramid);
    (end code)
=cut

################################################################################

package PYR2PYR::Shell;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

use COMMON::Shell;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

####################################################################################################
#                                     Group: GLOBAL VARIABLES                                      #
####################################################################################################

our $SCRIPTSDIR;
our $PERSONNALTEMPDIR;
our $COMMONTEMPDIR;
our $PARALLELIZATIONLEVEL;
our $SLABLIMIT;

=begin nd
Function: setGlobals

Define and create common working directories
=cut
sub setGlobals {
    $PARALLELIZATIONLEVEL = shift;
    $PERSONNALTEMPDIR = shift;
    $COMMONTEMPDIR = shift;
    $SCRIPTSDIR = shift;
    $SLABLIMIT = shift;

    $COMMONTEMPDIR = File::Spec->catdir($COMMONTEMPDIR,"COMMON");
    
    # Common directory
    if (! -d $COMMONTEMPDIR) {
        DEBUG (sprintf "Create the common temporary directory '%s' !", $COMMONTEMPDIR);
        eval { mkpath([$COMMONTEMPDIR]); };
        if ($@) {
            ERROR(sprintf "Can not create the common temporary directory '%s' : %s !", $COMMONTEMPDIR, $@);
            return FALSE;
        }
    }
    
    return TRUE;
}

# Function: getScriptDirectory
sub getScriptDirectory {
    return $SCRIPTSDIR;
}

# Function: getPersonnalTempDirectory
sub getPersonnalTempDirectory {
    return $PERSONNALTEMPDIR;
}

####################################################################################################
#                                  Group: Shell functions                                          #
####################################################################################################


my $FILE_PUSH = <<'FUNCTION';

PushSlab () {
    local input=$1
    local output=$2

    if [[ "${work}" = "0" ]]; then
        # On regarde si l'image à pousser est la dernière traitée lors d'une exécution précédente
        if [[ "${output}" == "${last_slab}" ]]; then
            echo "Last transfered slab found, now we work"
            work=1
        fi

        return
    fi

    size=`stat -L -c "%s" ${PYR_DIR_SRC}/$input`
    if [ $? != 0 ] ; then echo "$input n'existe pas, on passe" ; return; fi
    if [ "$size" -le "$SLAB_LIMIT" ] ; then
        return
    fi

    local dir=`dirname ${PYR_DIR}/$output`
    if [ ! -d $dir ] ; then mkdir -p $dir ; fi
  
    cp ${PYR_DIR_SRC}/$input ${PYR_DIR}/$output 
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi

    echo "0/${output}" >>${TMP_LIST_FILE}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

my $CEPH_PULL_TMP = <<'FUNCTION';
PullSlab () {
    local input=$1

    if [[ "${work}" == "0" ]]; then
        return
    fi
  
    rados -p ${PYR_POOL_SRC} get $input ${TMP_DIR}/slab.tmp
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

my $CEPH_PULL = <<'FUNCTION';
PullSlab () {
    local input=$1
    local output=$2

    if [[ "${work}" = "0" ]]; then
        # On regarde si l'image à pousser est la dernière traitée lors d'une exécution précédente
        if [[ "${output}" == "${last_slab}" ]]; then
            echo "Last transfered slab found, now we work"
            work=1
        fi

        return
    fi

    local dir=`dirname ${PYR_DIR}/$output`
    if [ ! -d $dir ] ; then mkdir -p $dir ; fi
  
    rados -p ${PYR_POOL_SRC} get $input ${PYR_DIR}/$output 
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi

    echo "0/${output}" >>${TMP_LIST_FILE}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

my $CEPH_PUSH = <<'FUNCTION';

PushSlab () {
    local input=$1
    local output=$2

    if [[ "${work}" = "0" ]]; then
        # On regarde si l'image à pousser est la dernière traitée lors d'une exécution précédente
        if [[ "${output}" == "${last_slab}" ]]; then
            echo "Last transfered slab found, now we work"
            work=1
        fi

        return
    fi

    size=`stat -L -c "%s" ${PYR_DIR_SRC}/$input`
    if [ $? != 0 ] ; then echo "$input n'existe pas, on passe" ; return; fi
    if [ "$size" -le "$SLAB_LIMIT" ] ; then
        return
    fi
  
    rados -p ${PYR_POOL} put $output ${PYR_DIR_SRC}/$input
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi

    echo "0/${output}" >> ${TMP_LIST_FILE}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

my $CEPH_PUSH_TMP = <<'FUNCTION';

PushSlab () {
    local output=$1

    if [[ "${work}" = "0" ]]; then
        # On regarde si l'image à pousser est la dernière traitée lors d'une exécution précédente
        if [[ "${output}" == "${last_slab}" ]]; then
            echo "Last transfered slab found, now we work"
            work=1
        fi

        return
    fi

    size=`stat -L -c "%s" ${TMP_DIR}/slab.tmp`
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    if [ "$size" -le "$SLAB_LIMIT" ] ; then
        return
    fi
  
    rados -p ${PYR_POOL} put $output ${TMP_DIR}/slab.tmp 
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi

    echo "0/${output}" >> ${TMP_LIST_FILE}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

my $S3_PUSH = <<'FUNCTION';

PushSlab () {
    local input=$1
    local output=$2

    if [[ "${work}" = "0" ]]; then
        # On regarde si l'image à pousser est la dernière traitée lors d'une exécution précédente
        if [[ "${output}" == "${last_slab}" ]]; then
            echo "Last transfered slab found, now we work"
            work=1
        fi

        return
    fi

    size=`stat -L -c "%s" ${PYR_DIR_SRC}/$input`
    if [ $? != 0 ] ; then echo "${PYR_DIR_SRC}/$input n'existe pas, on passe" ; return; fi
    if [ "$size" -le "$SLAB_LIMIT" ] ; then
        return
    fi
    
    resource="/${PYR_BUCKET}/${output}"
    contentType="application/octet-stream"
    dateValue=`TZ=GMT date -R`
    stringToSign="PUT\n\n${contentType}\n${dateValue}\n${resource}"

    signature=`echo -en ${stringToSign} | openssl sha1 -hmac ${ROK4_S3_SECRETKEY} -binary | base64`

    curl_options=""
    if [[ ! -z $ROK4_SSL_NO_VERIFY ]]; then
        curl_options="-k"
    fi

    curl $curl_options  --fail -X PUT -T "${PYR_DIR_SRC}/$input" \
     -H "Host: ${HOST}" \
     -H "Date: ${dateValue}" \
     -H "Content-Type: ${contentType}" \
     -H "Authorization: AWS ${ROK4_S3_KEY}:${signature}" \
     ${ROK4_S3_URL}${resource}

    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi

    echo "0/${output}" >> ${TMP_LIST_FILE}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

my $SWIFT_PUSH = <<'FUNCTION';

PushSlab () {
    local input=$1
    local output=$2

    if [[ "${work}" = "0" ]]; then
        # On regarde si l'image à pousser est la dernière traitée lors d'une exécution précédente
        if [[ "${output}" == "${last_slab}" ]]; then
            echo "Last transfered slab found, now we work"
            work=1
        fi

        return
    fi

    size=`stat -L -c "%s" ${PYR_DIR_SRC}/$input`
    if [ $? != 0 ] ; then echo "${PYR_DIR_SRC}/$input n'existe pas, on passe" ; return; fi
    if [ "$size" -le "$SLAB_LIMIT" ] ; then
        return
    fi

    GetSwiftToken
    
    resource="/${PYR_CONTAINER}/${output}"

    curl_options=""
    if [[ ! -z $ROK4_SSL_NO_VERIFY ]]; then
        curl_options="-k"
    fi

    curl $curl_options  --fail -X PUT -T "${PYR_DIR_SRC}/$input"  -H "${SWIFT_TOKEN}"  ${ROK4_SWIFT_PUBLICURL}${resource}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi

    echo "0/${output}" >> ${TMP_LIST_FILE}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

my $SWIFT_PUSH_TMP = <<'FUNCTION';

PushSlab () {
    local output=$1

    if [[ "${work}" = "0" ]]; then
        # On regarde si l'image à pousser est la dernière traitée lors d'une exécution précédente
        if [[ "${output}" == "${last_slab}" ]]; then
            echo "Last transfered slab found, now we work"
            work=1
        fi

        return
    fi
    size=`stat -L -c "%s" ${TMP_DIR}/slab.tmp`
    if [ $? != 0 ] ; then echo "${TMP_DIR}/slab.tmp n'existe pas, on passe" ; return; fi
    if [ "$size" -le "$SLAB_LIMIT" ] ; then
        return
    fi

    GetSwiftToken
    
    resource="/${PYR_CONTAINER}/${output}"

    curl_options=""
    if [[ ! -z $ROK4_SSL_NO_VERIFY ]]; then
        curl_options="-k"
    fi
    curl $curl_options  --fail -X PUT -T "${TMP_DIR}/slab.tmp"  -H "${SWIFT_TOKEN}"  ${ROK4_SWIFT_PUBLICURL}${resource}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi

    echo "0/${output}" >> ${TMP_LIST_FILE}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
FUNCTION

my $PROCESS_PULL_PUSH = <<'FUNCTION';
ProcessSlab () {
    local input=$1
    local output=$2

    PullSlab $input
    PushSlab $output

    print_prog
}
FUNCTION

my $PROCESS_PUSH = <<'FUNCTION';
ProcessSlab () {
    local input=$1
    local output=$2

    PushSlab $input $output

    print_prog
}
FUNCTION

my $PROCESS_PULL = <<'FUNCTION';
ProcessSlab () {
    local input=$1
    local output=$2

    PullSlab $input $output

    print_prog
}
FUNCTION

####################################################################################################
#                                   Group: Export function                                         #
####################################################################################################

my $WORKANDPROG = <<'WORKANDPROG';
progression=-1
progression_file="$0.prog"
lines_count=$(wc -l $0 | cut -d' ' -f1)
start_line=0

print_prog () {
    tmp=$(( (${BASH_LINENO[-2]} - $start_line) * 100 / (${lines_count} - $start_line) ))
    if [[ "$tmp" != "$progression" ]]; then
        progression=$tmp
        echo "$tmp" >$progression_file
    fi
}

work=1

# Test d'existence de la liste temporaire
if [[ -f "${TMP_LIST_FILE}" ]] ; then 
    # La liste existe, ce qui suggère que le script a déjà commencé à tourner
    # On prend la dernière ligne pour connaître la dernière dalle complètement traitée
    
    last_slab=$(tail -n 1 ${TMP_LIST_FILE} | sed "s#^0/##")
    echo "Script ${SCRIPT_ID} recall, work from slab ${last_slab}"
    work=0
fi

WORKANDPROG

=begin nd
Function: getScriptInitialization

Parameters (list):
    pyramid - <COMMON::PyramidRaster> - Pyramid to generate

Returns:
    Global variables and functions to print into script
=cut
sub getScriptInitialization {
    my $pyramidFrom = shift;
    my $pyramidTo = shift;

    my $string = $WORKANDPROG;

    $string .= "SLAB_LIMIT=$SLABLIMIT\n";

    if ( $pyramidFrom->getStorageType() eq "CEPH" ) {
        $string .= sprintf "PYR_POOL_SRC=%s\n", $pyramidFrom->getDataPool();

        if ( $pyramidTo->getStorageType() eq "CEPH" ) {
            $string .= sprintf "PYR_POOL=%s\n", $pyramidTo->getDataPool();
            $string .= $CEPH_PULL_TMP;
            $string .= $CEPH_PUSH_TMP;
            $string .= $PROCESS_PULL_PUSH;
            $string .= $COMMON::Shell::CEPH_BACKUPLIST;
        }
        elsif ( $pyramidTo->getStorageType() eq "FILE" ) {
            $string .= sprintf "PYR_DIR=%s\n", $pyramidTo->getDataDir();
            $string .= $CEPH_PULL;
            $string .= $PROCESS_PULL;
            $string .= $COMMON::Shell::FILE_BACKUPLIST;
        } 
        elsif ( $pyramidTo->getStorageType() eq "S3" ) {
            $string .= sprintf "PYR_BUCKET=%s\n", $pyramidTo->getDataBucket();
            $string .= $S3_PUSH;
            $string .= $PROCESS_PUSH;
            $string .= $COMMON::Shell::S3_BACKUPLIST;
        }        
        elsif ( $pyramidTo->getStorageType() eq "SWIFT" ) {
            $string .= sprintf "ROK4_SWIFT_TOKEN_FILE=\${TMP_DIR}/token.txt\n";
            $string .= sprintf "PYR_CONTAINER=%s\n", $pyramidTo->getDataContainer();
            if (COMMON::ProxyStorage::isSwiftKeystoneAuthentication()) {
                $string .= $COMMON::Shell::SWIFT_KEYSTONE_TOKEN_FUNCTION;
            }
            else {
                $string .= $COMMON::Shell::SWIFT_NATIVE_TOKEN_FUNCTION;
            }
            $string .= $CEPH_PULL_TMP;
            $string .= $SWIFT_PUSH_TMP;
            $string .= $PROCESS_PULL_PUSH;
            $string .= $COMMON::Shell::SWIFT_BACKUPLIST;
        }
    }
    elsif ( $pyramidFrom->getStorageType() eq "FILE" ) {
        $string .= sprintf "PYR_DIR_SRC=%s\n", $pyramidFrom->getDataDir();

        if ( $pyramidTo->getStorageType() eq "CEPH" ) {
            $string .= sprintf "PYR_POOL=%s\n", $pyramidTo->getDataPool();
            $string .= $CEPH_PUSH;
            $string .= $PROCESS_PUSH;
            $string .= $COMMON::Shell::CEPH_BACKUPLIST;
        }
        elsif ( $pyramidTo->getStorageType() eq "FILE" ) {
            $string .= sprintf "PYR_DIR=%s\n", $pyramidTo->getDataDir();
            $string .= $FILE_PUSH;
            $string .= $PROCESS_PUSH;
            $string .= $COMMON::Shell::FILE_BACKUPLIST;
        } 
        elsif ( $pyramidTo->getStorageType() eq "S3" ) {
            $string .= sprintf "PYR_BUCKET=%s\n", $pyramidTo->getDataBucket();
            $string .= $S3_PUSH;
            $string .= $PROCESS_PUSH;
            $string .= $COMMON::Shell::S3_BACKUPLIST;
        }        
        elsif ( $pyramidTo->getStorageType() eq "SWIFT" ) {
            $string .= sprintf "ROK4_SWIFT_TOKEN_FILE=\${TMP_DIR}/token.txt\n";
            $string .= sprintf "PYR_CONTAINER=%s\n", $pyramidTo->getDataContainer();
            if (COMMON::ProxyStorage::isSwiftKeystoneAuthentication()) {
                $string .= $COMMON::Shell::SWIFT_KEYSTONE_TOKEN_FUNCTION;
            }
            else {
                $string .= $COMMON::Shell::SWIFT_NATIVE_TOKEN_FUNCTION;
            }
            $string .= $SWIFT_PUSH;
            $string .= $PROCESS_PUSH;
            $string .= $COMMON::Shell::SWIFT_BACKUPLIST;
        }
    }

    $string .= sprintf "LIST_FILE=\"%s\"\n", $pyramidTo->getListFile();
    $string .= "COMMON_TMP_DIR=\"$COMMONTEMPDIR\"\n";

    $string .= "start_line=\$LINENO\n";
    $string .= "\n";

    return $string;
}

####################################################################################################
#                                     Group: Main function                                         #
####################################################################################################

my $MAIN_SCRIPT = <<'MAINSCRIPT';
#!/bin/bash

################### CODES DE RETOUR #############################
# 0 -> SUCCÈS
# 1 -> ÉCHEC

###################### PARAMÈTRES ###############################
frequency=60
if [[ ! -z $1 ]]; then
    frequency=$1
fi

#################################################################

scripts_directory="__scripts_directory__"
if [[ ! -d "$scripts_directory" ]]; then
    echo "ERREUR $scripts_directory n'existe pas"
    exit 1
fi

SPLITS=()
SPLITS_PIDS=()
SPLITS_END=()
SPLITS_EXITCODE=()
SPLITS_NAME=()
SPLITS_STATUS=()

for (( i = 1; i <= __jobs_number__; i++ )); do
    SPLITS+=("${scripts_directory}/SCRIPT_${i}.sh")
    SPLITS_NAME+=("SCRIPT_${i}.sh")
    SPLITS_END+=("0")
    SPLITS_EXITCODE+=("0")
    SPLITS_STATUS+=("En cours")
done

for s in "${SPLITS[@]}"; do
    (bash $s >$s.log 2>&1) &
    split_pid=$!
    SPLITS_PIDS+=("$split_pid")
done


echo "  INFO Attente de la fin des __jobs_number__ splits PYR2PYR"
first_time="1"
while [[ "0" = "0" ]]; do
    still_one="0"
    for (( i = 0; i < __jobs_number__; i++ )); do
        p=${SPLITS_PIDS[$i]}
        e=${SPLITS_END[$i]}
        n=${SPLITS_NAME[$i]}

        if [[ "$e" = "1" ]]; then
            continue
        fi

        if [[ $(ps -o s,pid,wchan | grep " $p " | grep -v grep) ]] ; then
            still_one="1"
            continue
        fi

        wait $p
        if [[ "$?" = "0" ]]; then
            SPLITS_EXITCODE[$i]="0"
            SPLITS_STATUS[$i]="Succès"
            echo "$n -> Succès"
        else
            SPLITS_EXITCODE[$i]=$?
            SPLITS_STATUS[$i]="Échec"
            echo "$n -> Échec"
        fi

        SPLITS_END[$i]="1"
    done

    if [[ "$still_one" = "0" ]]; then
        break
    fi

    sleep $frequency
done

for (( i = 0; i < __jobs_number__; i++ )); do
    c=${SPLITS_EXITCODE[$i]}
    if [[ "${c}" != "0" ]]; then
        echo "ERREUR Un split au moins a échoué"
        exit 1
    fi
done

echo "  INFO Lancement du finisher PYR2PYR"

bash ${scripts_directory}/SCRIPT_FINISHER.sh >${scripts_directory}/SCRIPT_FINISHER.sh.log 2>&1
if [[ $? != "0" ]]; then
    echo "ERREUR le finisher a échoué"
    exit 1
fi

exit 0

MAINSCRIPT

=begin nd
Function: getMainScript

Get the main script allowing to launch all generation scripts on a same machine

Returns:
    A shell script
=cut
sub getMainScript {

    my $ret = $MAIN_SCRIPT;
    $ret =~ s/__scripts_directory__/$SCRIPTSDIR/;
    $ret =~ s/__jobs_number__/$PARALLELIZATIONLEVEL/g;

    return $ret;
}

1;
__END__
