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
File: Commands.pm

Class: BE4CEPH::Commands

Configure and assemble commands used to generate pyramid's images.

All schemes in this page respect this legend :

(see formats.png)

Using:
    (start code)
    use BE4CEPH::Commands;

    # Commands object creation
    my $objCommands = BE4CEPH::Commands->new(
        $objPyramid, # BE4::Pyramid object
        TRUE, # useMasks
    );
    (end code)

Attributes:
    pyramid - <BE4CEPH::Pyramid> - Allowed to know output format specifications and configure commands.
    mntConfDir - string - Directory, where to write mergeNtiff configuration files.
    dntConfDir - string - Directory, where to write decimateNtiff configuration files.
    useMasks - boolean - If TRUE, all generating tools (mergeNtiff, merge4tiff...) use masks if present and generate a resulting mask. This processing is longer, that's why default behaviour is without mask.
=cut

################################################################################

package BE4CEPH::Commands;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

use COMMON::Harvesting;
use BE4CEPH::Level;
use COMMON::GraphScript;
use COMMON::GraphNode;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################

use constant TRUE  => 1;
use constant FALSE => 0;

# Group: Commands' weights and calls

# Constant: MERGE4TIFF_W
use constant MERGE4TIFF_W => 1;
# Constant: MERGENTIFF_W
use constant MERGENTIFF_W => 4;
# Constant: MERGENTIFF_W
use constant DECIMATENTIFF_W => 3;
# Constant: WGET_W
use constant WGET_W => 35;
# Constant: TIFF2TILE_W
use constant TIFF2TILE_W => 1;

=begin nd
Constant: BASHFUNCTIONS
Define bash functions, used to factorize and reduce scripts :
    - Wms2work
    - StoreImage
    - StoreTiles
    - MergeNtiff
    - Merge4tiff
    - DecimateNtiff
=cut
my $BASHFUNCTIONS   = <<'FUNCTIONS';

declare -A RM_IMGS

StoreImage () {
    local level=$1
    local workDir=$2
    local workImgName=$3
    local imgName=$4
    local workMskName=$5
    local mskName=$6
    
    
    if [[ ! ${RM_IMGS[$workDir/$workImgName]} ]] ; then
             
        tiff2tile $workDir/$workImgName __t2tI__ -pool ${PYR_POOL} $imgName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        echo "$imgName" >> ${TMP_LIST_FILE}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        if [ "$level" == "$TOP_LEVEL" ] ; then
            rm $workDir/$workImgName
        elif [ "$level" == "$CUT_LEVEL" ] ; then
            mv $workDir/$workImgName ${COMMON_TMP_DIR}/
        fi
        
        if [ $workMskName ] ; then
            
            if [ $mskName ] ; then
                    
                tiff2tile $workDir/$workMskName __t2tM__ -pool ${PYR_POOL} $mskName
                if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
                echo "$mskName" >> ${TMP_LIST_FILE}
                
            fi
            
            if [ "$level" == "$TOP_LEVEL" ] ; then
                rm $workDir/$workMskName
            elif [ "$level" == "$CUT_LEVEL" ] ; then
                mv $workDir/$workMskName ${COMMON_TMP_DIR}/
            fi
        fi
    fi
}
StoreTiles () {
    local level=$1
    local workDir=$2
    local workImgName=$3
    local imgName=$4

    shift 4

    local imgI=$1
    local imgJ=$2
    local tilesW=$3
    local tilesH=$4

    shift 4

    local workMskName=$1
    local mskName=$2
    
    local imin=$imgI*$tilesW
    local imax=$imgI*$tilesW+$tilesW-1
    local jmin=$imgJ*$tilesH
    local jmax=$imgJ*$tilesH+$tilesH-1
    
    if [[ ! ${RM_IMGS[$workDir/$workImgName]} ]] ; then
             
        tiff2tile $workDir/$workImgName __t2tI__ -ij $imgI $imgJ -pool ${PYR_POOL} $imgName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        for i in `seq $imin $imax` ; do 
            for j in `seq $imin $imax` ; do 
                echo "${imgName}_i_j" >> ${TMP_LIST_FILE}
                if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
            done
        done
        
        if [ "$level" == "$TOP_LEVEL" ] ; then
            rm $workDir/$workImgName
        elif [ "$level" == "$CUT_LEVEL" ] ; then
            mv $workDir/$workImgName ${COMMON_TMP_DIR}/
        fi
        
        if [ $workMskName ] ; then
            
            if [ $mskName ] ; then
                    
                tiff2tile $workDir/$workMskName __t2tM__ -ij $imgI $imgJ -pool ${PYR_POOL} $mskName
                if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
                for i in `seq $imin $imax` ; do 
                    for j in `seq $imin $imax` ; do 
                        echo "${mskName}_i_j" >> ${TMP_LIST_FILE}
                        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
                    done
                done
                
            fi
            
            if [ "$level" == "$TOP_LEVEL" ] ; then
                rm $workDir/$workMskName
            elif [ "$level" == "$CUT_LEVEL" ] ; then
                mv $workDir/$workMskName ${COMMON_TMP_DIR}/
            fi
        fi
    fi
}

MergeNtiff () {
    local config=$1
    
    mergeNtiff -f ${MNT_CONF_DIR}/$config -r ${TMP_DIR}/ __mNt__
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    
    rm -f ${MNT_CONF_DIR}/$config
}

DecimateNtiff () {
    local config=$1
    
    decimateNtiff -f ${DNT_CONF_DIR}/$config __dNt__
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    
    rm -f ${DNT_CONF_DIR}/$config
}

Merge4tiff () {
    local imgOut=$1
    local mskOut=$2
    shift 2
    local levelIn=$1
    local imgIn=( 0 $2 $4 $6 $8 )
    local mskIn=( 0 $3 $5 $7 $9 )
    shift 9

    local forRM=''

    # Entrées

    local tempDir="${TMP_DIR}"
    if [ "$levelIn" == "$CUT_LEVEL" ] ; then
        tempDir="${COMMON_TMP_DIR}"
    fi
    
    local inM4T=''
    
    local nbImgs=0
    for i in `seq 1 4`;
    do
        if [ ${imgIn[$i]} != '0' ] ; then
            if [[ -f ${tempDir}/${imgIn[$i]} ]] ; then
                forRM="$forRM ${tempDir}/${imgIn[$i]}"
                inM4T=`printf "$inM4T -i%.1d ${tempDir}/${imgIn[$i]}" $i`
                
                if [ ${mskIn[$i]} != '0' ] ; then
                    inM4T=`printf "$inM4T -m%.1d ${tempDir}/${mskIn[$i]}" $i`
                    forRM="$forRM ${tempDir}/${mskIn[$i]}"
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
        merge4tiff __m4t__ $inM4T $outM4T
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    else
        RM_IMGS[${TMP_DIR}/${imgOut}]="1"
    fi
    
    # Suppressions
    rm $forRM
}

FUNCTIONS

################################################################################

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Commands constructor. Bless an instance.

Parameters (list):
    pyr - <BE4CEPH::Pyramid> - Image pyramid to generate
    useMasks - string - Do we want use masks to generate images ?
=cut
sub new {
    my $this = shift;
    my $pyr = shift;
    my $useMasks = shift;
    
    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        pyramid => undef,
        mntConfDir => undef,
        dntConfDir => undef,
        useMasks => FALSE,
    };

    bless($self, $class);

    if (! defined $pyr || ref ($pyr) ne "BE4CEPH::Pyramid") {
        ERROR("Can not load Pyramid !");
        return undef;
    }
    $self->{pyramid} = $pyr;

    if ( $self->{pyramid}->ownMasks() || (defined $useMasks && uc($useMasks) eq "TRUE") ) {
        $self->{useMasks} = TRUE;
    }

    return $self;
}

####################################################################################################
#                               Group: Commands methods                                            #
####################################################################################################


=begin nd
Function: work2cache

Copy image from work directory to cache and transform it (tiled and compressed) thanks to the 'Work2cache' bash function (tiff2tile).

(see work2cache.png)

Example:
|    Work2cache ${TMP_DIR}/19_395_3137.tif IMAGE/19/02/AF/Z5.tif

Parameter:
    node - <Node> - Node whose image have to be transfered in the cache.
    workDir - string - Work image directory, can be an environment variable.

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub work2cache {
    my $self = shift;
    my $node = shift;
    my $workDir = shift;
    
    my $cmd = "";
    my $weight = 0;
    
    #### Export de l'image

    if ($self->{pyramid}->storeTiles()) {

        my $pyrName = sprintf "%s_IMG_%s", $self->{pyramid}->getNewName(), $node->getLevel();
        
        $cmd .= sprintf "StoreTiles %s %s %s %s %s %s %s %s",
            $node->getLevel, $workDir, $node->getWorkImageName(TRUE), $pyrName,
            $node->getCol, $node->getRow, $self->{pyramid}->getTilesPerWidth, $self->{pyramid}->getTilesPerHeight;

        $weight += TIFF2TILE_W;
        
        #### Export du masque, si présent

        if ($node->getWorkMaskName()) {
            # On a un masque de travail : on le précise pour qu'il soit potentiellement déplacé dans le temporaire commun ou supprimé
            $cmd .= sprintf (" %s", $node->getWorkMaskName(TRUE));
            
            # En plus, on veut exporter les masques dans la pyramide, on en précise donc l'emplacement final
            if ( $self->{pyramid}->ownMasks() ) {
                $pyrName = sprintf "%s_MSK_%s", $self->{pyramid}->getNewName(), $node->getPyramidName();
                
                $cmd .= sprintf (" %s", $pyrName);
                $weight += TIFF2TILE_W;
            }        
        }
        
        $cmd .= "\n";
    } else {
    
        my $pyrName = sprintf "%s_IMG_%s", $self->{pyramid}->getNewName(), $node->getPyramidName();
        
        $cmd .= sprintf ("StoreImage %s %s %s %s", $node->getLevel, $workDir, $node->getWorkImageName(TRUE), $pyrName);
        $weight += TIFF2TILE_W;
        
        #### Export du masque, si présent

        if ($node->getWorkMaskName()) {
            # On a un masque de travail : on le précise pour qu'il soit potentiellement déplacé dans le temporaire commun ou supprimé
            $cmd .= sprintf (" %s", $node->getWorkMaskName(TRUE));
            
            # En plus, on veut exporter les masques dans la pyramide, on en précise donc l'emplacement final
            if ( $self->{pyramid}->ownMasks() ) {
                $pyrName = sprintf "%s_MSK_%s", $self->{pyramid}->getNewName(), $node->getPyramidName();
                
                $cmd .= sprintf (" %s", $pyrName);
                $weight += TIFF2TILE_W;
            }        
        }
        
        $cmd .= "\n";
    }

    return ($cmd,$weight);
}

=begin nd
Function: mergeNtiff

Use the 'MergeNtiff' bash function. Write a configuration file, with sources.

(see mergeNtiff.png)

Parameters (list):
    node - <Node> - Node to generate thanks to a 'mergeNtiff' command.
    
Example:
|    MergeNtiff 19_397_3134.txt

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub mergeNtiff {
    my $self = shift;
    my $node = shift;
    
    my ($c, $w);
    my ($code, $weight) = ("",MERGENTIFF_W);
    
    if ($self->{useMasks}) {
        $node->addWorkMask();
    }
    
    my $mNtConfFilename = $node->getWorkBaseName.".txt";
    my $mNtConfFile = File::Spec->catfile($self->{mntConfDir}, $mNtConfFilename);
    
    if (! open CFGF, ">", $mNtConfFile ) {
        ERROR(sprintf "Impossible de creer le fichier $mNtConfFile.");
        return ("",-1);
    }
    
    # La premiere ligne correspond à la dalle résultat: La version de travail de la dalle à calculer.
    # Les points d'interrogation permettent de gérer le dossier où écrire les images grâce à une variable
    # Cet export va également ajouter les fonds (si présents) comme premières sources
    printf CFGF $node->exportForMntConf(TRUE, "?");

    #   - Les images sources (QTree)
    my $listGeoImg = $node->getGeoImages;
    foreach my $img (@{$listGeoImg}) {
        printf CFGF "%s", $img->exportForMntConf($self->{useMasks});
    }
    
    close CFGF;
    
    $code .= "MergeNtiff $mNtConfFilename";
    $code .= "\n";

    return ($code,$weight);
}

=begin nd
Function: decimateNtiff

Use the 'decimateNtiff' bash function. Write a configuration file, with sources.

(see decimateNtiff.png)

Parameters (list):
    node - <Node> - Node to generate thanks to a 'decimateNtiff' command.
    
Example:
|    DecimateNtiff 12_26_17.txt

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub decimateNtiff {
    my $self = shift;
    my $node = shift;
    
    my ($c, $w);
    my ($code, $weight) = ("",DECIMATENTIFF_W);
    
    if ($self->{useMasks}) {
        $node->addWorkMask();
    }
    
    my $dNtConfFilename = $node->getWorkBaseName.".txt";
    my $dNtConfFile = File::Spec->catfile($self->{dntConfDir}, $dNtConfFilename);
    
    if (! open CFGF, ">", $dNtConfFile ) {
        ERROR(sprintf "Impossible de creer le fichier $dNtConfFile.");
        return ("",-1);
    }
    
    # La premiere ligne correspond à la dalle résultat: La version de travail de la dalle à calculer.
    # Cet export va également ajouter les fonds (si présents) comme premières sources
    printf CFGF $node->exportForDntConf(TRUE, $node->getScript()->getTempDir()."/");
    
    #   - Les noeuds sources (NNGraph)
    foreach my $nodesource ( @{$node->getNodeSources()} ) {
        printf CFGF "%s", $nodesource->exportForDntConf(FALSE, $nodesource->getScript()->getTempDir()."/");
    }
    
    close CFGF;
    
    $code .= "DecimateNtiff $dNtConfFilename";
    $code .= "\n";

    return ($code,$weight);
}

=begin nd
Function: merge4tiff

Use the 'Merge4tiff' bash function.

|     i1  i2
|              =  resultImg
|     i3  i4

(see merge4tiff.png)

Parameters (list):
    node - <COMMON::GraphNode> - Node to generate thanks to a 'merge4tiff' command.

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub merge4tiff {
    my $self = shift;
    my $node = shift;
    
    my ($c, $w);
    my ($code, $weight) = ("",MERGE4TIFF_W);
    
    my @childList = $node->getChildren;
    
    if ($self->{useMasks}) {
        $node->addWorkMask();
    }
    
    # We compose the 'Merge4tiff' call
    #   - the ouput + background
    $code .= sprintf "Merge4tiff %s", $node->exportForM4tConf(TRUE);
    
    #   - the children inputs
    my $inputsLevel = $self->{pyramid}->getTileMatrixSet()->getBelowLevelID($node->getLevel());
    
    $code .= sprintf " %s", $inputsLevel;
    foreach my $childNode ($node->getPossibleChildren()) {
            
        if (defined $childNode) {
            $code .= $childNode->exportForM4tConf(FALSE);
        } else {
            $code .= " 0 0";
        }
    }
    
    $code .= "\n";

    return ($code,$weight);
}

=begin nd
Function: configureFunctions

Configure bash functions to write in scripts' header thanks to pyramid's components.
=cut
sub configureFunctions {
    my $self = shift;

    my $pyr = $self->{pyramid};
    my $configuredFunc = $BASHFUNCTIONS;

    ######## mergeNtiff ########
    # work compression : deflate
    my $conf_mNt = "-c zip ";

    my $ip = $pyr->getInterpolation;
    $conf_mNt .= "-i $ip ";
    my $spp = $pyr->getSamplesPerPixel;
    $conf_mNt .= "-s $spp ";
    my $bps = $pyr->getBitsPerSample;
    $conf_mNt .= "-b $bps ";
    my $ph = $pyr->getPhotometric;
    $conf_mNt .= "-p $ph ";
    my $sf = $pyr->getSampleFormat;
    $conf_mNt .= "-a $sf ";

    my $nd = $self->getNodata->getValue;
    $conf_mNt .= "-n $nd ";

    $configuredFunc =~ s/__mNt__/$conf_mNt/;
    
    ######## decimateNtiff ########
    # work compression : deflate
    my $conf_dNt = "-c zip ";

    $conf_dNt .= "-n $nd ";

    $configuredFunc =~ s/__dNt__/$conf_dNt/;
    
    ######## merge4tiff ########
    # work compression : deflate
    my $conf_m4t = "-c zip ";

    my $gamma = $pyr->getGamma;
    $conf_m4t .= "-g $gamma ";
    $conf_m4t .= "-n $nd ";

    $configuredFunc =~ s/__m4t__/$conf_m4t/;
    
    ######## tiff2tile ########
    my $conf_t2t = "";

    # pour les images
    my $compression = $pyr->getCompression;
    
    $conf_t2t .= "-c $compression ";
    if ($pyr->getCompressionOption eq 'crop') {
        $conf_t2t .= "-crop ";
    }


    $conf_t2t .= sprintf "-t %s %s ", $pyr->getTileMatrixSet->getTileWidth,$pyr->getTileMatrixSet->getTileHeight;

    $configuredFunc =~ s/__t2tI__/$conf_t2t/g;
    
    # pour les masques
    $conf_t2t = sprintf "-c zip -t %s %s",
        $pyr->getTileMatrixSet->getTileWidth, $pyr->getTileMatrixSet->getTileHeight;
    $configuredFunc =~ s/__t2tM__/$conf_t2t/g;
    
    return $configuredFunc;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getNodata
sub getNodata {
    my $self = shift;
    return $self->{pyramid}->getNodata();
}

# Function: getPyramid
sub getPyramid {
    my $self = shift;
    return $self->{pyramid};
}

=begin nd
Function: setConfDir

Store the directory for mergeNtiff and decimateNtiff configuration files.

Parameters (list):
    mntConfDir - string - mergeNtiff configurations' directory
    dntConfDir - string - decimateNtiff configurations' directory
=cut
sub setConfDir {
    my $self = shift;
    my $mntConfDir = shift;
    my $dntConfDir = shift;

    $self->{mntConfDir} = $mntConfDir;
    $self->{dntConfDir} = $dntConfDir;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all commands' informations. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;

    my $export = "";

    $export .= "\nObject BE4CEPH::Commands :\n";
    $export .= "\t Use masks\n" if $self->{useMasks};
    $export .= "\t Doesn't use masks\n" if (! $self->{useMasks});
    $export .= "\t Export masks\n" if $self->{pyramid}->ownMasks();
    $export .= "\t Doesn't export masks\n" if (! $self->{pyramid}->ownMasks());

    return $export;
}
  
1;
__END__
