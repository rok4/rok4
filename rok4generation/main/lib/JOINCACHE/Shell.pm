# Copyright © (2011) Institut national de l'information
#                    géographique et forestière 
# 
# Géoportail SAV <geop_services@geoportail.fr>
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

Class: JOINCACHE::Shell

(see ROK4GENERATION/libperlauto/JOINCACHE_Shell.png)

Configure and assemble commands used to generate raster pyramid's slabs.

All schemes in this page respect this legend :

(see ROK4GENERATION/tools/formats.png)

Using:
    (start code)
    use JOINCACHE::Shell;

    if (! JOINCACHE::Shell::setGlobals($commonTempDir, $mergeMethod)) {
        ERROR ("Cannot initialize Shell commands for JOINCACHE");
        return FALSE;
    }

    my $scriptInit = JOINCACHE::Shell::getScriptInitialization($pyramid);
    (end code)
=cut

################################################################################

package JOINCACHE::Shell;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

use JOINCACHE::Node;
use COMMON::ProxyStorage;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################

use constant TRUE  => 1;
use constant FALSE => 0;

####################################################################################################
#                                     Group: GLOBAL VARIABLES                                      #
####################################################################################################

our $SCRIPTSDIR;
our $USEMASK;
our $COMMONTEMPDIR;
our $PERSONNALTEMPDIR;
our $ONTCONFDIR;
our $MERGEMETHOD;
our $PARALLELIZATIONLEVEL;

=begin nd
Function: setGlobals

Define and create common working directories
=cut
sub setGlobals {
    $PARALLELIZATIONLEVEL = shift;
    $PERSONNALTEMPDIR = shift;
    $COMMONTEMPDIR = shift;
    $SCRIPTSDIR = shift;
    $MERGEMETHOD = shift;
    $USEMASK = shift;

    if (defined $USEMASK && uc($USEMASK) eq "TRUE") {
        $USEMASK = TRUE;
    } else {
        $USEMASK = FALSE;
    }

    $COMMONTEMPDIR = File::Spec->catdir($COMMONTEMPDIR,"COMMON");
    $ONTCONFDIR = File::Spec->catfile($COMMONTEMPDIR,"overlayNtiff");
    
    # Common directory
    if (! -d $COMMONTEMPDIR) {
        DEBUG (sprintf "Create the common temporary directory '%s' !", $COMMONTEMPDIR);
        eval { mkpath([$COMMONTEMPDIR]); };
        if ($@) {
            ERROR(sprintf "Can not create the common temporary directory '%s' : %s !", $COMMONTEMPDIR, $@);
            return FALSE;
        }
    }
    
    # OverlayNtiff configurations directory
    if (! -d $ONTCONFDIR) {
        DEBUG (sprintf "Create the OverlayNtiff configurations directory '%s' !", $ONTCONFDIR);
        eval { mkpath([$ONTCONFDIR]); };
        if ($@) {
            ERROR(sprintf "Can not create the OverlayNtiff configurations directory '%s' : %s !", $ONTCONFDIR, $@);
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
#                                      Group: OVERLAY N TIFF                                       #
####################################################################################################

# Constant: OVERLAYNTIFF_W
use constant OVERLAYNTIFF_W => 3;

my $ONTFUNCTION = <<'ONTFUNCTION';
OverlayNtiff () {
    local config=$1
    local inTemplate=$2

    if [[ "${work}" == "0" ]]; then
        return
    fi

    if [ -f ${MNT_CONF_DIR}/$config ]; then
        overlayNtiff -f ${ONT_CONF_DIR}/$config ${OVERLAYNTIFF_OPTIONS}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        rm -f ${TMP_DIR}/$inTemplate
        rm -f ${ONT_CONF_DIR}/$config
    fi
}

ONTFUNCTION

my $CEPH_STORAGE_FUNCTIONS = <<'STORAGEFUNCTIONS';
LinkSlab () {
    local target=$1
    local link=$2

    if [[ "${work}" == "0" ]]; then
        return
    fi

    # On retire le pool des entrées
    target=`echo -n "$target" | sed "s#${PYR_POOL}/##"`
    link=`echo -n "$link" | sed "s#${PYR_POOL}/##"`

    echo -n "SYMLINK#${target}" | rados -p ${PYR_POOL} put $link /dev/stdin
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

PushSlab () {
    local workImgName=$1
    local imgName=$2
    local workMskName=$3
    local mskName=$4

    if [[ "${work}" = "0" ]]; then
        # On regarde si l'image à pousser est la dernière traitée lors d'une exécution précédente
        if [[ "${imgName}" == "${last_slab}" ]]; then
            echo "Last generated image slab found, now we work"
            work=1
        elif [[ ! -z $mskName && "${mskName}" == "${last_slab}" ]] ; then
            echo "Last generated mask slab found, now we work"
            work=1
        fi

        return
    fi

    work2cache ${TMP_DIR}/$workImgName ${WORK2CACHE_IMAGE_OPTIONS} -pool ${PYR_POOL} $imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    rm -f ${TMP_DIR}/$workImgName

    if [ $workMskName ] ; then
        work2cache ${TMP_DIR}/$workMskName ${WORK2CACHE_MASK_OPTIONS} -pool ${PYR_POOL} $mskName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        rm -f ${TMP_DIR}/$workMskName
    fi

    print_prog
}

PullSlab () {
    local input=$1
    local output=$2

    if [[ "${work}" == "0" ]]; then
        return
    fi

    # On retire le pool du input
    input=`echo -n "$input" | sed "s#${PYR_POOL}/##"`

    cache2work -c zip -pool ${PYR_POOL} $input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
STORAGEFUNCTIONS


my $FILE_STORAGE_FUNCTIONS = <<'STORAGEFUNCTIONS';
LinkSlab () {
    local target=$1
    local link=$2

    if [[ "${work}" == "0" ]]; then
        return
    fi

    mkdir -p $(dirname $link)
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi

    ln -s $target $link
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

PushSlab () {

    local workImgName=$1
    local imgName=$2
    local workMskName=$3
    local mskName=$4

    if [[ "${work}" = "0" ]]; then
        # On regarde si l'image à pousser est la dernière traitée lors d'une exécution précédente
        if [[ "${imgName}" == "${last_slab}" ]]; then
            echo "Last generated image slab found, now we work"
            work=1
        elif [[ ! -z $mskName && "${mskName}" == "${last_slab}" ]] ; then
            echo "Last generated mask slab found, now we work"
            work=1
        fi

        return
    fi

    local dir=`dirname ${PYR_DIR}/$imgName`

    if [ -r ${TMP_DIR}/$workImgName ] ; then rm -f ${PYR_DIR}/$imgName ; fi
    if [ ! -d $dir ] ; then mkdir -p $dir ; fi

    work2cache ${TMP_DIR}/$workImgName ${WORK2CACHE_IMAGE_OPTIONS} ${PYR_DIR}/$imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    rm -f ${TMP_DIR}/$workImgName

    if [ $workMskName ] ; then

        dir=`dirname ${PYR_DIR}/$mskName`

        if [ -r ${TMP_DIR}/$workMskName ] ; then rm -f ${PYR_DIR}/$mskName ; fi
        if [ ! -d $dir ] ; then mkdir -p $dir ; fi

        work2cache ${TMP_DIR}/$workMskName ${WORK2CACHE_MASK_OPTIONS} ${PYR_DIR}/$mskName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        rm -f ${TMP_DIR}/$workMskName
    fi

    print_prog
}

PullSlab () {
    local input=$1
    local output=$2

    if [[ "${work}" == "0" ]]; then
        return
    fi

    cache2work -c zip $input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

STORAGEFUNCTIONS

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


echo "  INFO Attente de la fin des __jobs_number__ splits JOINCACHE"
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

exit 0

MAINSCRIPT

=begin nd
Function: getMainScript

Get the main script allowing to launch all generation scripts on a same machine.

Returns:
    A shell script
=cut
sub getMainScript {

    my $ret = $MAIN_SCRIPT;

    $ret =~ s/__jobs_number__/$PARALLELIZATIONLEVEL/g;
    $ret =~ s/__scripts_directory__/$SCRIPTSDIR/g;

    return $ret;
}

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
    pyramid - <COMMON::PyramidVector> - Pyramid to generate

Returns:
    Global variables and functions to print into script
=cut
sub getScriptInitialization {
    my $pyramid = shift;

    # Variables

    my $string = $WORKANDPROG;

    # On a précisé une méthode de fusion, on est dans le cas d'un JOINCACHE, on exporte la fonction overlayNtiff
    $string .= sprintf "OVERLAYNTIFF_OPTIONS=\"-c zip -s %s -p %s -b %s -m $MERGEMETHOD",
        $pyramid->getImageSpec()->getPixel()->getSamplesPerPixel(),
        $pyramid->getImageSpec()->getPixel()->getPhotometric(),
        $pyramid->getNodata()->getValue();

    if ($MERGEMETHOD eq "ALPHATOP") {
        $string .= " -t 255,255,255";
    }

    $string .= "\"\n";

    $string .= "ONT_CONF_DIR=$ONTCONFDIR\n";

    $string .= sprintf "WORK2CACHE_MASK_OPTIONS=\"-c zip -t %s %s\"\n", $pyramid->getTileMatrixSet()->getTileWidth(), $pyramid->getTileMatrixSet()->getTileHeight();

    $string .= sprintf "WORK2CACHE_IMAGE_OPTIONS=\"-c %s -t %s %s -s %s -b %s -a %s\"\n",
        $pyramid->getImageSpec()->getCompression(),
        $pyramid->getTileMatrixSet()->getTileWidth(), $pyramid->getTileMatrixSet()->getTileHeight(),
        $pyramid->getImageSpec()->getPixel()->getSamplesPerPixel(),
        $pyramid->getImageSpec()->getPixel()->getBitsPerSample(),
        $pyramid->getImageSpec()->getPixel()->getSampleFormat();

    if ($pyramid->getStorageType() eq "FILE") {
        $string .= sprintf "PYR_DIR=%s\n", $pyramid->getDataDir();
    }
    elsif ($pyramid->getStorageType() eq "CEPH") {
        $string .= sprintf "PYR_POOL=%s\n", $pyramid->getDataPool();
    }

    $string .= "COMMON_TMP_DIR=\"$COMMONTEMPDIR\"\n";

    # Fonctions

    if ($pyramid->getStorageType() eq "FILE") {
        $string .= $FILE_STORAGE_FUNCTIONS;
    }
    elsif ($pyramid->getStorageType() eq "CEPH") {
        $string .= $CEPH_STORAGE_FUNCTIONS;
    }

    $string .= $ONTFUNCTION;

    $string .= "start_line=\$LINENO\n";
    $string .= "\n";

    return $string;
}
  
1;
__END__
