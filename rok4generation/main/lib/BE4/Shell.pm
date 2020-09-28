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

package BE4::Shell;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

use COMMON::Harvesting;
use BE4::Node;
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

our $COMMONTEMPDIR;
our $PERSONNALTEMPDIR;
our $SCRIPTSDIR;
our $MNTCONFDIR;
our $DNTCONFDIR;
our $USEMASK;
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
    $USEMASK = shift;

    if (defined $USEMASK && uc($USEMASK) eq "TRUE") {
        $USEMASK = TRUE;
    } else {
        $USEMASK = FALSE;
    }

    $COMMONTEMPDIR = File::Spec->catdir($COMMONTEMPDIR,"COMMON");
    $MNTCONFDIR = File::Spec->catfile($COMMONTEMPDIR,"mergeNtiff");
    $DNTCONFDIR = File::Spec->catfile($COMMONTEMPDIR,"decimateNtiff");
    
    # Common directory
    if (! -d $COMMONTEMPDIR) {
        DEBUG (sprintf "Create the common temporary directory '%s' !", $COMMONTEMPDIR);
        eval { mkpath([$COMMONTEMPDIR]); };
        if ($@) {
            ERROR(sprintf "Can not create the common temporary directory '%s' : %s !", $COMMONTEMPDIR, $@);
            return FALSE;
        }
    }
    
    # MergeNtiff configurations directory
    if (! -d $MNTCONFDIR) {
        DEBUG (sprintf "Create the MergeNtiff configurations directory '%s' !", $MNTCONFDIR);
        eval { mkpath([$MNTCONFDIR]); };
        if ($@) {
            ERROR(sprintf "Can not create the MergeNtiff configurations directory '%s' : %s !", $MNTCONFDIR, $@);
            return FALSE;
        }
    }
    
    # DecimateNtiff configurations directory
    if (! -d $DNTCONFDIR) {
        DEBUG (sprintf "Create the DecimateNtiff configurations directory '%s' !", $DNTCONFDIR);
        eval { mkpath([$DNTCONFDIR]); };
        if ($@) {
            ERROR(sprintf "Can not create the DecimateNtiff configurations directory '%s' : %s !", $DNTCONFDIR, $@);
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
#                                        Group: MERGE N TIFF                                       #
####################################################################################################

my $MNTFUNCTION = <<'MNTFUNCTION';
MergeNtiff () {
    local config=$1
    local bgI=$2
    local bgM=$3

    if [[ "${work}" == "0" ]]; then
        return
    fi
    
    if [ -f ${MNT_CONF_DIR}/$config ]; then
        mergeNtiff -f ${MNT_CONF_DIR}/$config -r ${TMP_DIR}/ ${MERGENTIFF_OPTIONS}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    
        rm -f ${MNT_CONF_DIR}/$config
    fi

    if [ $bgI ] ; then
        rm -f ${TMP_DIR}/$bgI
    fi
    
    if [ $bgM ] ; then
        rm -f ${TMP_DIR}/$bgM
    fi
}
MNTFUNCTION

####################################################################################################
#                                        Group: CACHE TO WORK                                      #
####################################################################################################

my $FILE_C2WFUNCTION = <<'C2WFUNCTION';
PullSlab () {
    if [[ "${work}" == "0" ]]; then
        return
    fi
    local input=$1
    local output=$2

    cache2work -c zip ${PYR_DIR}/$input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
C2WFUNCTION

my $S3_C2WFUNCTION = <<'C2WFUNCTION';
PullSlab () {
    if [[ "${work}" == "0" ]]; then
        return
    fi
    local input=$1
    local output=$2

    cache2work -c zip -bucket ${PYR_BUCKET} $input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
C2WFUNCTION

my $SWIFT_C2WFUNCTION = <<'C2WFUNCTION';
PullSlab () {
    if [[ "${work}" == "0" ]]; then
        return
    fi
    local input=$1
    local output=$2

    cache2work -c zip -container ${PYR_CONTAINER} ${KEYSTONE_OPTION} $input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
C2WFUNCTION

my $CEPH_C2WFUNCTION = <<'C2WFUNCTION';
PullSlab () {
    if [[ "${work}" == "0" ]]; then
        return
    fi
    local input=$1
    local output=$2

    cache2work -c zip -pool ${PYR_POOL} $input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
C2WFUNCTION

####################################################################################################
#                                        Group: WORK TO CACHE                                      #
####################################################################################################

my $S3_W2CFUNCTION = <<'W2CFUNCTION';
BackupListFile () {
    echo "List file back up to do"
}

PushSlab () {
    local postAction=$1
    local workImgName=$2
    local imgName=$3
    local workMskName=$4
    local mskName=$5

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
    
    if [[ ! ${RM_IMGS[${TMP_DIR}/$workImgName]} ]] ; then
             
        work2cache ${TMP_DIR}/$workImgName ${WORK2CACHE_IMAGE_OPTIONS} -bucket ${PYR_BUCKET} $imgName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        echo "0/$imgName" >> ${TMP_LIST_FILE}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        if [ "$postAction" == "rm" ] ; then
            rm ${TMP_DIR}/$workImgName
        elif [ "$postAction" == "mv" ] ; then
            mv ${TMP_DIR}/$workImgName ${COMMON_TMP_DIR}/
        fi
        
        if [ $workMskName ] ; then
            
            if [ $mskName ] ; then
                    
                work2cache ${TMP_DIR}/$workMskName ${WORK2CACHE_MASK_OPTIONS} -bucket ${PYR_BUCKET} $mskName
                if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
                echo "0/$mskName" >> ${TMP_LIST_FILE}
                
            fi
            
            if [ "$postAction" == "rm" ] ; then
                rm ${TMP_DIR}/$workMskName
            elif [ "$postAction" == "mv" ] ; then
                mv ${TMP_DIR}/$workMskName ${COMMON_TMP_DIR}/
            fi
        fi
    fi
}
W2CFUNCTION

my $SWIFT_W2CFUNCTION = <<'W2CFUNCTION';
BackupListFile () {
    echo "List file back up to do"
}

PushSlab () {
    local postAction=$1
    local workImgName=$2
    local imgName=$3
    local workMskName=$4
    local mskName=$5

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
    
    if [[ ! ${RM_IMGS[${TMP_DIR}/$workImgName]} ]] ; then
             
        work2cache ${TMP_DIR}/$workImgName ${WORK2CACHE_IMAGE_OPTIONS} -container ${PYR_CONTAINER} ${KEYSTONE_OPTION} $imgName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        echo "0/$imgName" >> ${TMP_LIST_FILE}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        if [ "$postAction" == "rm" ] ; then
            rm ${TMP_DIR}/$workImgName
        elif [ "$postAction" == "mv" ] ; then
            mv ${TMP_DIR}/$workImgName ${COMMON_TMP_DIR}/
        fi
        
        if [ $workMskName ] ; then
            
            if [ $mskName ] ; then
                    
                work2cache ${TMP_DIR}/$workMskName ${WORK2CACHE_MASK_OPTIONS} -container ${PYR_CONTAINER} ${KEYSTONE_OPTION} $mskName
                if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
                echo "0/$mskName" >> ${TMP_LIST_FILE}
                
            fi
            
            if [ "$postAction" == "rm" ] ; then
                rm ${TMP_DIR}/$workMskName
            elif [ "$postAction" == "mv" ] ; then
                mv ${TMP_DIR}/$workMskName ${COMMON_TMP_DIR}/
            fi
        fi
    fi
}
W2CFUNCTION


my $CEPH_W2CFUNCTION = <<'W2CFUNCTION';
BackupListFile () {
    local objectName=`basename ${LIST_FILE}`
    rados -p ${PYR_POOL} put ${objectName} ${LIST_FILE}
}

PushSlab () {
    local postAction=$1
    local workImgName=$2
    local imgName=$3
    local workMskName=$4
    local mskName=$5

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
    
    
    if [[ ! ${RM_IMGS[${TMP_DIR}/$workImgName]} ]] ; then
             
        work2cache ${TMP_DIR}/$workImgName ${WORK2CACHE_IMAGE_OPTIONS} -pool ${PYR_POOL} $imgName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        echo "0/$imgName" >> ${TMP_LIST_FILE}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        if [ "$postAction" == "rm" ] ; then
            rm ${TMP_DIR}/$workImgName
        elif [ "$postAction" == "mv" ] ; then
            mv ${TMP_DIR}/$workImgName ${COMMON_TMP_DIR}/
        fi
        
        if [ $workMskName ] ; then
            
            if [ $mskName ] ; then
                    
                work2cache ${TMP_DIR}/$workMskName ${WORK2CACHE_MASK_OPTIONS} -pool ${PYR_POOL} $mskName
                if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
                echo "0/$mskName" >> ${TMP_LIST_FILE}
                
            fi
            
            if [ "$postAction" == "rm" ] ; then
                rm ${TMP_DIR}/$workMskName
            elif [ "$postAction" == "mv" ] ; then
                mv ${TMP_DIR}/$workMskName ${COMMON_TMP_DIR}/
            fi
        fi
    fi

    print_prog
}
W2CFUNCTION


my $FILE_W2CFUNCTION = <<'W2CFUNCTION';
BackupListFile () {
    bn=$(basename ${LIST_FILE})
    if [ "$(stat -c "%d:%i" ${LIST_FILE})" != "$(stat -c "%d:%i" ${PYR_DIR}/$bn)" ]; then
        cp ${LIST_FILE} ${PYR_DIR}/
    else
        echo "List file is already locate to the backup destination"
    fi
}

PushSlab () {
    local postAction=$1
    local workImgName=$2
    local imgName=$3
    local workMskName=$4
    local mskName=$5

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
        
    if [[ ! ${RM_IMGS[${TMP_DIR}/$workImgName]} ]] ; then
        
        local dir=`dirname ${PYR_DIR}/$imgName`
        
        if [ -r ${TMP_DIR}/$workImgName ] ; then rm -f ${PYR_DIR}/$imgName ; fi
        if [ ! -d $dir ] ; then mkdir -p $dir ; fi
            
        work2cache ${TMP_DIR}/$workImgName ${WORK2CACHE_IMAGE_OPTIONS} ${PYR_DIR}/$imgName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        echo "0/$imgName" >> ${TMP_LIST_FILE}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        if [ "$postAction" == "rm" ] ; then
            rm ${TMP_DIR}/$workImgName
        elif [ "$postAction" == "mv" ] ; then
            mv ${TMP_DIR}/$workImgName ${COMMON_TMP_DIR}/
        fi
        
        if [ $workMskName ] ; then
            
            if [ $mskName ] ; then
                
                dir=`dirname ${PYR_DIR}/$mskName`
                
                if [ -r ${TMP_DIR}/$workMskName ] ; then rm -f ${PYR_DIR}/$mskName ; fi
                if [ ! -d $dir ] ; then mkdir -p $dir ; fi
                    
                work2cache ${TMP_DIR}/$workMskName ${WORK2CACHE_MASK_OPTIONS} ${PYR_DIR}/$mskName
                if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
                echo "0/$mskName" >> ${TMP_LIST_FILE}
                
            fi
            
            if [ "$postAction" == "rm" ] ; then
                rm ${TMP_DIR}/$workMskName
            elif [ "$postAction" == "mv" ] ; then
                mv ${TMP_DIR}/$workMskName ${COMMON_TMP_DIR}/
            fi
        fi
    fi

    print_prog
}
W2CFUNCTION

####################################################################################################
#                                        Group: HARVEST IMAGE                                      #
####################################################################################################

my $HARVESTFUNCTION = <<'HARVESTFUNCTION';
Wms2work () {
    local workName=$1
    local harvestExtension=$2
    local finalExtension=$3
    local minSize=$4
    local url=$5
    local grid=$6
    shift 6

    if [[ "${work}" == "0" ]]; then
        return
    fi

    local size=0

    mkdir -p ${TMP_DIR}/harvesting/

    for i in `seq 1 $#`;
    do
        nameImg=`printf "${TMP_DIR}/harvesting/img%.5d.$harvestExtension" $i`
        local count=0; local wait_delay=1
        while :
        do
            let count=count+1
            wget --no-verbose -O $nameImg "$url&BBOX=$1"

            if [ $? == 0 ] ; then
                checkWork $nameImg 2>/dev/null
                if [ $? == 0 ] ; then
                    break
                fi
            fi
            
            echo "Failure $count : wait for $wait_delay s"
            sleep $wait_delay
            let wait_delay=wait_delay*2
            if [ 3600 -lt $wait_delay ] ; then 
                let wait_delay=3600
            fi
        done
        let size=`stat -c "%s" $nameImg`+$size

        shift
    done
    
    if [ "$size" -le "$minSize" ] ; then
        RM_IMGS["${TMP_DIR}/${workName}.${finalExtension}"]="1"
        rm -rf ${TMP_DIR}/harvesting/
        return
    fi

    if [ "$grid" != "1 1" ] ; then
        composeNtiff -g $grid -s ${TMP_DIR}/harvesting/ -c zip ${TMP_DIR}/${workName}.${finalExtension}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    else
        mv ${TMP_DIR}/harvesting/img00001.${harvestExtension} ${TMP_DIR}/${workName}.${finalExtension}
    fi

    rm -rf ${TMP_DIR}/harvesting/
}
HARVESTFUNCTION

####################################################################################################
#                                        Group: MERGE 4 TIFF                                       #
####################################################################################################

my $M4TFUNCTION = <<'M4TFUNCTION';
Merge4tiff () {
    local imgOut=$1
    local mskOut=$2
    shift 2
    local imgBg=$1
    local mskBg=$2
    shift 2
    local directoryIn=$1
    local imgIn=( 0 $2 $4 $6 $8 )
    local mskIn=( 0 $3 $5 $7 $9 )
    shift 9

    if [[ "${work}" == "0" ]]; then
        return
    fi
    
    local forRM=''

    # Entrées   
    local inM4T=''

    if [ $imgBg != '0'  ] ; then
        forRM="$forRM ${TMP_DIR}/$imgBg"
        inM4T="$inM4T -ib ${TMP_DIR}/$imgBg"
        if [ $mskBg != '0'  ] ; then
            forRM="$forRM ${TMP_DIR}/$mskBg"
            inM4T="$inM4T -mb ${TMP_DIR}/$mskBg"
        fi
    fi
    
    local nbImgs=0
    for i in `seq 1 4`;
    do
        if [ ${imgIn[$i]} != '0' ] ; then
            if [[ -f ${directoryIn}/${imgIn[$i]} ]] ; then
                forRM="$forRM ${directoryIn}/${imgIn[$i]}"
                inM4T=`printf "$inM4T -i%.1d ${directoryIn}/${imgIn[$i]}" $i`
                
                if [ ${mskIn[$i]} != '0' ] ; then
                    inM4T=`printf "$inM4T -m%.1d ${directoryIn}/${mskIn[$i]}" $i`
                    forRM="$forRM ${directoryIn}/${mskIn[$i]}"
                fi
                
                let nbImgs=$nbImgs+1
            fi
        fi
    done
    
    # Sorties
    local outM4T=''
    
    if [ ${mskOut} != '0' ] ; then
        outM4T="-mo ${TMP_DIR}/${mskOut}"
    fi
    
    outM4T="$outM4T -io ${TMP_DIR}/${imgOut}"
    
    # Appel à la commande merge4tiff
    if [ "$nbImgs" -gt 0 ] ; then
        merge4tiff ${MERGE4TIFF_OPTIONS} $inM4T $outM4T
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    else
        RM_IMGS[${TMP_DIR}/${imgOut}]="1"
    fi
    
    # Suppressions
    rm $forRM
}
M4TFUNCTION

####################################################################################################
#                                        Group: DECIMATE N TIFF                                    #
####################################################################################################

my $DNTFUNCTION = <<'DNTFUNCTION';
DecimateNtiff () {
    local config=$1
    local bgI=$2
    local bgM=$3

    if [[ "${work}" == "0" ]]; then
        return
    fi
    
    if [ -f ${DNT_CONF_DIR}/$config ]; then
        decimateNtiff -f ${DNT_CONF_DIR}/$config ${DECIMATENTIFF_OPTIONS}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        rm -f ${DNT_CONF_DIR}/$config
    fi
    
    if [ $bgI ] ; then
        rm -f ${TMP_DIR}/$bgI
    fi
    
    if [ $bgM ] ; then
        rm -f ${TMP_DIR}/$bgM
    fi
}
DNTFUNCTION

####################################################################################################
#                                     Group: Main function                                         #
####################################################################################################

my $MAIN_SCRIPT_NNGRAPH = <<'MAINSCRIPT';
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

for level in __level_ids__ ; do 

    SPLITS=()
    SPLITS_PIDS=()
    SPLITS_END=()
    SPLITS_EXITCODE=()
    SPLITS_NAME=()
    SPLITS_STATUS=()
    
    for (( i = 1; i <= __jobs_number__; i++ )); do
        SPLITS+=("${scripts_directory}/LEVEL_${level}_SCRIPT_${i}.sh")
        SPLITS_NAME+=("LEVEL_${level}_SCRIPT_${i}.sh")
        SPLITS_END+=("0")
        SPLITS_EXITCODE+=("0")
        SPLITS_STATUS+=("En cours")
    done

    for s in "${SPLITS[@]}"; do
        (bash $s >$s.log 2>&1) &
        split_pid=$!
        SPLITS_PIDS+=("$split_pid")
    done


    echo "  INFO Attente de la fin des __jobs_number__ splits BE4 pour le niveau $level"
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

done

echo "  INFO Lancement du finisher BE4"

bash ${scripts_directory}/SCRIPT_FINISHER.sh >${scripts_directory}/SCRIPT_FINISHER.sh.log 2>&1
if [[ $? != "0" ]]; then
    echo "ERREUR le finisher a échoué"
    exit 1
fi

exit 0

MAINSCRIPT

my $MAIN_SCRIPT_QTREE = <<'MAINSCRIPT';
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


echo "  INFO Attente de la fin des __jobs_number__ splits BE4"
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

echo "  INFO Lancement du finisher BE4"

bash ${scripts_directory}/SCRIPT_FINISHER.sh >${scripts_directory}/SCRIPT_FINISHER.sh.log 2>&1
if [[ $? != "0" ]]; then
    echo "ERREUR le finisher a échoué"
    exit 1
fi

exit 0

MAINSCRIPT

=begin nd
Function: getMainScript

Get the main script allowing to launch all generation scripts on a same machine. Different script for QTree and NNgraph generation.

Parameters (list):
    pyramid - <COMMON::PyramidRaster> - Pyramid to generate, to know the TMS type and levels

Returns:
    A shell script
=cut
sub getMainScript {
    my $pyramid = shift;

    my $ret = "";
    if ($pyramid->getTileMatrixSet()->isQTree()) {
        $ret = $MAIN_SCRIPT_QTREE;
    } else {
        $ret = $MAIN_SCRIPT_NNGRAPH;
        
        my @levels = $pyramid->getOrderedLevels();
        my @quotedLevels = ();
        foreach my $l (@levels) {
            push(@quotedLevels, sprintf("\"%s\"", $l->getID()));
        }

        my $stringLevels = join(" ", @quotedLevels);
        $ret =~ s/__level_ids__/$stringLevels/g;
    }

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
    temp - string - Temporary directory

Returns:
    Global variables and functions to print into script
=cut
sub getScriptInitialization {
    my $pyramid = shift;

    # Variables

    my $string = $WORKANDPROG;

    $string .= sprintf "MERGENTIFF_OPTIONS=\"-c zip -i %s -s %s -b %s -a %s -n %s\"\n",
        $pyramid->getImageSpec()->getInterpolation(),
        $pyramid->getImageSpec()->getPixel()->getSamplesPerPixel(),
        $pyramid->getImageSpec()->getPixel()->getBitsPerSample(),
        $pyramid->getImageSpec()->getPixel()->getSampleFormat(),
        $pyramid->getNodata()->getValue();
    $string .= "MNT_CONF_DIR=$MNTCONFDIR\n";

    $string .= sprintf "WORK2CACHE_MASK_OPTIONS=\"-c zip -t %s %s\"\n", $pyramid->getTileMatrixSet()->getTileWidth(), $pyramid->getTileMatrixSet()->getTileHeight();

    $string .= sprintf "WORK2CACHE_IMAGE_OPTIONS=\"-c %s -t %s %s -s %s -b %s -a %s",
        $pyramid->getImageSpec()->getCompression(),
        $pyramid->getTileMatrixSet()->getTileWidth(), $pyramid->getTileMatrixSet()->getTileHeight(),
        $pyramid->getImageSpec()->getPixel()->getSamplesPerPixel(),
        $pyramid->getImageSpec()->getPixel()->getBitsPerSample(),
        $pyramid->getImageSpec()->getPixel()->getSampleFormat();

    if ($pyramid->getImageSpec()->getCompressionOption() eq 'crop') {
        $string .= " -crop\"\n";
    } else {
        $string .= "\"\n";
    }

    if ($pyramid->getTileMatrixSet()->isQTree()) {
        $string .= sprintf "MERGE4TIFF_OPTIONS=\"-c zip -g %s -n %s -s %s -b %s -a %s\"\n",
            $pyramid->getImageSpec()->getGamma(),
            $pyramid->getNodata()->getValue(),
            $pyramid->getImageSpec()->getPixel()->getSamplesPerPixel(),
            $pyramid->getImageSpec()->getPixel()->getBitsPerSample(),
            $pyramid->getImageSpec()->getPixel()->getSampleFormat();
    } else {
        $string .= sprintf "DECIMATENTIFF_OPTIONS=\"-c zip -n %s\"\n", $pyramid->getNodata()->getValue();
        $string .= "DNT_CONF_DIR=$DNTCONFDIR\n";
    }

    if ($pyramid->getStorageType() eq "FILE") {
        $string .= sprintf "PYR_DIR=%s\n", $pyramid->getDataDir();
    }
    elsif ($pyramid->getStorageType() eq "CEPH") {
        $string .= sprintf "PYR_POOL=%s\n", $pyramid->getDataPool();
    }
    elsif ($pyramid->getStorageType() eq "S3") {
        $string .= sprintf "PYR_BUCKET=%s\n", $pyramid->getDataBucket();
    }
    elsif ($pyramid->getStorageType() eq "SWIFT") {
        $string .= sprintf "PYR_CONTAINER=%s\n", $pyramid->getDataContainer();
        if ($pyramid->keystoneConnection()) {
            $string .= "KEYSTONE_OPTION=\"-ks\"\n";
        } else {
            $string .= "KEYSTONE_OPTION=\"\"\n";
        }
    }

    $string .= sprintf "LIST_FILE=\"%s\"\n", $pyramid->getListFile();
    $string .= "COMMON_TMP_DIR=\"$COMMONTEMPDIR\"\n";

    # Fonctions

    $string .= "\n# Pour mémoriser les dalles supprimées\n";
    $string .= "declare -A RM_IMGS\n";
    if ($pyramid->getStorageType() eq "FILE") {
        $string .= $FILE_C2WFUNCTION;
        $string .= $FILE_W2CFUNCTION;
    }
    elsif ($pyramid->getStorageType() eq "CEPH") {
        $string .= $CEPH_C2WFUNCTION;
        $string .= $CEPH_W2CFUNCTION;
    }
    elsif ($pyramid->getStorageType() eq "S3") {
        $string .= $S3_C2WFUNCTION;
        $string .= $S3_W2CFUNCTION;
    }
    elsif ($pyramid->getStorageType() eq "SWIFT") {
        $string .= $SWIFT_C2WFUNCTION;
        $string .= $SWIFT_W2CFUNCTION;
    }

    $string .= $MNTFUNCTION;
    $string .= $HARVESTFUNCTION;

    if ($pyramid->getTileMatrixSet()->isQTree()) {
        $string .= $M4TFUNCTION;
    } else {
        $string .= $DNTFUNCTION;
    }

    $string .= "start_line=\$LINENO\n";
    $string .= "\n";

    return $string;
}

  
1;
__END__
