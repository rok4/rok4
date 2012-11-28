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

package BE4::Commands;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

use BE4::Harvesting;
use BE4::Level;
use BE4::Script;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constantes

# Booleans
use constant TRUE  => 1;
use constant FALSE => 0;

# Commands' weights
use constant MERGE4TIFF_W => 1;
use constant MERGENTIFF_W => 4;
use constant CACHE2WORK_PNG_W => 3;
use constant WGET_W => 35;
use constant TIFF2TILE_W => 0;
use constant TIFFCP_W => 0;

################################################################################
# Global

my $BASHFUNCTIONS   = <<'FUNCTIONS';

declare -A RM_IMGS

Wms2work () {
    local dir=$1
    local fmt=$2
    local imgSize=$3
    local nbTiles=$4
    local min_size=$5
    local url=$6
    shift 6

    local size=0

    mkdir $dir

    for i in `seq 1 $#`;
    do
        nameImg=`printf "$dir/img%.2d.$fmt" $i`
        local count=0; local wait_delay=1
        while :
        do
            let count=count+1
            wget --no-verbose -O $nameImg "$url&BBOX=$1"
            if gdalinfo $nameImg 1>/dev/null ; then break ; fi
            
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
    
    if [ "$size" -le "$min_size" ] ; then
        RM_IMGS["$dir.tif"]="1"
        rm -rf $dir
        return
    fi

    if [ "$fmt" == "png" ]||[ "$nbTiles" != "1x1" ] ; then
        montage -geometry $imgSize -tile $nbTiles $dir/*.$fmt -depth 8 -define tiff:rows-per-strip=4096 -compress Zip $dir.tif
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    else
        mv $dir/img01.tif $dir.tif
    fi

    rm -rf $dir
}

Cache2work () {
    local imgSrc=$1
    local workName=$2
    local opt=$3
    
    if [  "$opt" == "png"  ] ; then
        cp $imgSrc $workName.tif
        mkdir $workName
        untile $workName.tif $workName/
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        montage __montageIn__ $workName/*.png __montageOut__ $workName.tif
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        rm -rf $workName/
    elif [  "$opt" == "msk"  ] ; then
        tiffcp __tcpM__ $imgSrc $workName.tif
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    else
        tiffcp __tcpI__ $imgSrc $workName.tif
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    fi

}

Work2cache () {
    local after=$1
    local workDir=$2
    local workImgName=$3
    local imgName=$4
    local workMskName=$5
    local mskName=$6
    
    local dir=`dirname ${PYR_DIR}/$imgName`
    
    if [ -r $workDir/$workImgName ] ; then rm -f ${PYR_DIR}/$imgName ; fi
    if [ ! -d $dir ] ; then mkdir -p $dir ; fi
    
    if [[ ! ${RM_IMGS[$workDir/$workImgName]} ]] ; then
        tiff2tile $workDir/$workImgName __t2tI__ ${PYR_DIR}/$imgName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        echo "0/$imgName" >> ${LIST_FILE}
        
        if [ "$after" == "remove" ] ; then
            rm $workDir/$workImgName
        elif [ "$after" == "move" ] ; then
            mv $workDir/$workImgName ${ROOT_TMP_DIR}/$workImgName
        fi
        
        if [ $workMskName ] ; then

            dir=`dirname ${PYR_DIR}/$mskName`
            
            if [ -r $workDir/$workMskName ] ; then rm -f ${PYR_DIR}/$mskName ; fi
            if [ ! -d $dir ] ; then mkdir -p $dir ; fi
            
            tiff2tile $workDir/$workMskName __t2tM__ ${PYR_DIR}/$mskName
            if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
            echo "0/$mskName" >> ${LIST_FILE}
            
            if [ "$after" == "remove" ] ; then
                rm $workDir/$workMskName
            elif [ "$after" == "move" ] ; then
                mv $workDir/$workMskName ${ROOT_TMP_DIR}/$workMskName
            fi
        fi
    fi
}

MergeNtiff () {
    local type=$1
    local config=$2
    local bgI=$3
    local bgM=$4
    
    mergeNtiff -f ${MNT_CONF_DIR}/$config -t $type __mNt__
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    
    rm -f $config
    
    if [ $bgI ] ; then
        rm -f $bgI
    fi
    
    if [ $bgM ] ; then
        rm -f $bgM
    fi
}

Merge4tiff () {
    local imgs=( $1 $3 $5 $7 $9 )
    shift
    local masks=( $1 $3 $5 $7 $9 )
    shift 9
    local bgI=$1
    local bgM=$1

    local forRM=''

    # Entrées
    local inM4T=''
    
    if [ $bgI ] ; then
        forRM="$bgI"
        inM4T="$inM4T -b $bgI"
        if [ $bgM ] ; then
            forRM="$bgM"
            inM4T="$inM4T -mb $bgM"
        fi
    fi
    
    local nbImgs=0
    for i in `seq 1 4`;
    do
        if [ ${imgs[$i]} != '0' ] ; then
            if [[ ! ${RM_IMGS[${imgs[$i]}]} ]] ; then
                inM4T=`printf "$inM4T -i%.1d ${imgs[$i]}" $i`
                
                if [ ${masks[$i]} != '0' ] ; then
                    inM4T=`printf "$inM4T -m%.1d ${masks[$i]}" $i`
                    forRM="$forRM ${masks[$i]}"
                fi
                
                forRM="$forRM ${imgs[$i]}"
                let nbImgs=$nbImgs+1
            else
                echo -e "Merge4tiff : on sait que ${imgs[$i]} a été supprimé normalement, on passe"
            fi
        fi
    done
    
    # Sorties
    local outM4T=''
    
    if [ ${masks[0]} != '0' ] ; then
        outM4T="$outM4T -mo ${masks[0]}"
    fi
    
    outM4T="$outM4T ${imgs[0]}"
    
    # Appel à la commande merge4tiff
    if [ "$nbImgs" -gt 0 ] ; then
        merge4tiff __m4t__ $inM4T $outM4T
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    else
        RM_IMGS[${imgs[0]}]="1"
    fi
    
    # Suppressions
    rm -f $forRM
}

FUNCTIONS

################################################################################

BEGIN {}
INIT {}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    * pyramid : BE4::Pyramid
    * useMasks : Boolean - Specify if we work with masks to generate images.
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;
    
    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        pyramid => undef,
        useMasks => FALSE
    };
    bless($self, $class);

    TRACE;
    
    return undef if (! $self->_init(@_));

    return $self;
}

sub _init {
    my ($self, $pyr, $useMasks) = @_;

    TRACE;

    if (! defined $pyr || ref ($pyr) ne "BE4::Pyramid") {
        ERROR("Can not load Pyramid !");
        return FALSE;
    }
    $self->{pyramid} = $pyr;
    
    if ( $pyr->ownMasks() || (defined $useMasks && uc($useMasks) eq "TRUE") ) {
        $self->{useMasks} = TRUE;
    }

    return TRUE;
}

####################################################################################################
#                                      COMMANDS METHODS                                            #
####################################################################################################

# Group: commands methods

#
=begin nd
method: wms2work

Fetch image corresponding to the node thanks to 'wget', in one or more steps at a time. WMS service is described in the current graph's datasource. Use the 'Wms2work' bash function.

Example:
    Wms2work ${TMP_DIR}/18_8300_5638.tif "http://localhost/wmts/rok4?LAYERS=ORTHO_RAW_LAMB93_D075-E&SERVICE=WMS&VERSION=1.3.0&REQUEST=getMap&FORMAT=image/tiff&CRS=EPSG:3857&BBOX=264166.3697535659936,6244599.462785762557312,266612.354658691633792,6247045.447690888197504&WIDTH=4096&HEIGHT=4096&STYLES="

Parameters:
    node - BE4::Node, whose image have to be harvested.
    harvesting - BE4::Harvesting, to use to harvest image.
    justWeight - boolean, if TRUE, we want to weight node, if FALSE, we want to compute code for the node.
    suffix - optionnal, suffix to add after the node's name ('BgI' for example), seperated with an underscore.
=cut
sub wms2work {
    my ($self, $node, $harvesting, $justWeight, $suffix) = @_;
    
    TRACE;
    
    return ("",WGET_W) if ($justWeight);
    
    my @imgSize = $self->{pyramid}->getCacheImageSize($node->getLevel); # ie size tile image in pixel !
    my $tms     = $self->{pyramid}->getTileMatrixSet;
    
    my $nodeName = $node->getWorkBaseName($suffix);
    
    my ($xMin, $yMin, $xMax, $yMax) = $node->getBBox;
    
    my $cmd = $harvesting->getCommandWms2work({
        inversion => $tms->getInversion,
        dir       => "\${TMP_DIR}/".$nodeName,
        srs       => $tms->getSRS,
        bbox      => [$xMin, $yMin, $xMax, $yMax],
        width => $imgSize[0],
        height => $imgSize[1]
    });
    
    return ($cmd,WGET_W);
}

#
=begin nd
method: cache2work

Copy image from cache to work directory and transform (work format : untiles, uncompressed).
Use the 'Cache2work' bash function.

2 cases:
    - legal tiff compression -> tiffcp
    - png -> untile + montage
    
Examples:
    Cache2work ${PYR_DIR}/IMAGE/19/02/BF/24.tif ${TMP_DIR}/19_398_3136_BgI
    Cache2work ${PYR_DIR}/IMAGE/19/02/BF/24.tif ${TMP_DIR}/19_398_3136_BgI png
    
Parameters:
    node - BE4::Node, whose image have to be transfered in the work directory.
    ownMask - Boolean, specify if mask is combined with this node.
=cut
sub cache2work {
    my ($self, $node, $ownMask) = @_;
    
    #### Rappatriement de l'image de donnée ####

    # On va chercher dans la pyramide l'image, pour s'en servir de fond, d'où le Bg.
    # C'est une image, d'où le I => suffixe BgI
    my $workBaseName = $node->getWorkBaseName("BgI");
    my $fileName = File::Spec->catfile($self->{pyramid}->getRootPerType("data",FALSE),$node->getPyramidName);
    
    my @imgSize   = $self->{pyramid}->getCacheImageSize($node->getLevel); # image size in pixel (4096)
    
    my $cmd = "";
    my $weight = 0;
    
    if ($self->{pyramid}->getCompression eq 'png') {
        # Dans le cas du png, l'opération de copie doit se faire en 3 étapes :
        #       - la copie du fichier dans le dossier temporaire
        #       - le détuilage (untile)
        #       - la fusion de tous les png en un tiff
        $cmd = sprintf ("Cache2work \${PYR_DIR}/%s \${TMP_DIR}/%s png\n", $fileName , $workBaseName);
        $weight = CACHE2WORK_PNG_W;
        
    } else {
        $cmd = sprintf ("Cache2work \${PYR_DIR}/%s \${TMP_DIR}/%s\n", $fileName ,$workBaseName);
        $weight = TIFFCP_W;
    }
    
    #### Rappatriement du masque de donnée (si présent) ####
    
    if ( $ownMask ) {
        # Un masque est associé à l'image que l'on va utiliser, on doit le mettre également au format de travail
        $fileName = File::Spec->catfile($self->{pyramid}->getRootPerType("mask",FALSE),$node->getPyramidName);
        $workBaseName = $node->getWorkBaseName("BgM");
        
        $cmd .= sprintf ("Cache2work \${PYR_DIR}/%s \${TMP_DIR}/%s msk\n", $fileName , $workBaseName);
        $weight += TIFFCP_W;
    }
    
    return ($cmd,$weight);
}

#
=begin nd
method: work2cache

Copy image from work directory to cache and transform it (tiled and compressed) thanks to the 'Work2cache' bash function (tiff2tile).

Example:
    Work2cache ${TMP_DIR}/19_395_3137.tif IMAGE/19/02/AF/Z5.tif

Parameter:
    node - BE4::Node object, whose image have to be transfered in the cache.
    workDir - Work image directory, can be an environment variable.
    after - string, specify if image have to be removed after copy in the cache ("rm"), move in the temporary directory root ("mv"), nothing otherwise
=cut
sub work2cache {
    my $self = shift;
    my $node = shift;
    my $workDir = shift;
    my $after = shift;
    
    my $cmd = "";
    my $weight = 0;
    
    #### Copie de l'image
    
    my $workName  = $node->getWorkName("I");
    my $pyrName = File::Spec->catfile($self->{pyramid}->getRootPerType("data",FALSE),$node->getPyramidName);
    
    $cmd .= sprintf ("Work2cache %s %s %s %s", $after, $workDir, $workName, $pyrName);
    $weight += TIFF2TILE_W;
    
    #### Copie du masque, si voulu
    
    if ($self->{pyramid}->ownMasks()) {
        $workName  = $node->getWorkName("M");
        $pyrName = File::Spec->catfile($self->{pyramid}->getRootPerType("mask",FALSE),$node->getPyramidName);
        
        $cmd .= sprintf (" %s %s", $workName, $pyrName);
        $weight += TIFF2TILE_W;
    }
    
    $cmd .= "\n";
    
    return ($cmd,$weight);
}

#
=begin nd
method: mergeNtiff

Use the 'MergeNtiff' bash function. Write a configuration file, with sources.

Parameters:
    node - BE4::Node to generate thanks to a 'mergeNtiff' command.
    justWeight - boolean, if TRUE, we want to weight node, if FALSE, we want to compute code for the node.
    
Example:
    MergeNtiff image ${MNT_CONF_DIR}/mergeNtiffConfig_19_397_3134.txt
=cut
sub mergeNtiff {
    my $self = shift;
    my $node = shift;
    my $justWeight = shift;

    TRACE;
    
    my ($c, $w);
    my ($code, $weight) = ("",MERGENTIFF_W);
    
    my $workBgI = undef;
    my $workBgM = undef;

    # Si elle existe, on copie la dalle de la pyramide de base dans le repertoire de travail 
    # en la convertissant du format cache au format de travail: c'est notre image de fond.
    # Si la dalle de la pyramide de base existe, on a créé un lien, donc il existe un fichier
    # correspondant dans la nouvelle pyramide.
    # On fait de même avec le masque de donnée associé, s'il existe.
    my $imgPath = File::Spec->catfile($self->{pyramid}->getRootPerType("data",TRUE),$node->getPyramidName);
    
    if ( -f $imgPath ) {
        my $maskPath = File::Spec->catfile($self->{pyramid}->getRootPerType("mask",TRUE),$node->getPyramidName);
        
        if ( $self->{useMasks} && -f $maskPath ) {
            # On a en plus un masque associé à l'image de fond
            ($c,$w) = $self->cache2work($node,TRUE);
            $code .= $c;
            $weight += $w;
            
            # We just want to know weight (we don't already know the script for this node).
            return ("",$weight) if ($justWeight);
            
            $workBgM = File::Spec->catfile($node->getScript->getTempDir,$node->getWorkName("BgM"));
        } else {
            ($c,$w) = $self->cache2work($node,FALSE);
            $code .= $c;
            $weight += $w;
        }
        
        # We just want to know weight (we don't already know the script for this node).
        return ("",$weight) if ($justWeight);
        
        $workBgI = File::Spec->catfile($node->getScript->getTempDir,$node->getWorkName("BgI"));
    }
    
    # We just want to know weight.
    return ("",$weight) if ($justWeight);
    
    my $workImgPath = File::Spec->catfile($node->getScript->getTempDir,$node->getWorkName("I"));
    my $workMskPath = undef;
    if ($self->{useMasks}) {
        $workMskPath = File::Spec->catfile($node->getScript->getTempDir,$node->getWorkName("M"));
    }
    
    my $mergNtiffConfDir = $node->getScript->getMntConfDir;
    my $mergNtiffConfFilename = $node->getWorkBaseName.".txt";
    my $mergNtiffConfFile = File::Spec->catfile($mergNtiffConfDir,$mergNtiffConfFilename);
    
    if (! open CFGF, ">", $mergNtiffConfFile ) {
        ERROR(sprintf "Impossible de creer le fichier $mergNtiffConfFile.");
        return ("",-1);
    }
    
    # La premiere ligne correspond à la dalle résultat: La version de travail de la dalle à calculer.
    printf CFGF $node->exportForMntConf($workImgPath, $workMskPath);

    # Maintenant les dalles en entrée:
    #   - L'éventuelle image de fond (avec l'eventuel masque associé)
    printf CFGF "%s", $node->exportForMntConf($workBgI, $workBgM) if (defined $workBgI);

    #   - Les images source (QTree)
    my $listGeoImg = $node->getGeoImages;
    foreach my $img (@{$listGeoImg}) {
        printf CFGF "%s", $img->exportForMntConf($self->{useMasks});
    }
    #   - Les noeuds source (Graph)
    foreach my $nodesource ( @{$node->getNodeSources()} ) {
        my $filepath = File::Spec->catfile($nodesource->getScript->getTempDir, $nodesource->getWorkName);
        printf CFGF "%s", $nodesource->exportForMntConf($filepath);
    }
    
    close CFGF;
    
    $code .= "MergeNtiff image $mergNtiffConfFilename";
    $code .= " $workBgI" if (defined $workBgI); # pour supprimer l'image de fond si elle existe
    $code .= " $workBgM" if (defined $workBgM); # pour supprimer le masque de fond si il existe
    $code .= "\n";

    return ($code,$weight);
}

#
=begin nd
method: merge4tiff

Compose the 'merge4tiff' command.

|                   i1  i2
| backGround    +              =  resultImg
|                   i3  i4

Parameters:
    node - BE4::Node to generate thanks to a 'merge4tiff' command.
    harvesting - BE4::Harvesting, to use to harvest background if necessary.
    justWeight - boolean, if TRUE, we want to weight node, if FALSE, we want to compute code for the node.

Example:
    merge4tiff -g 1 -n FFFFFF  -i1 ${TMP_DIR}/19_396_3134.tif -i2 ${TMP_DIR}/19_397_3134.tif -i3 ${TMP_DIR}/19_396_3135.tif -i4 ${TMP_DIR}/19_397_3135.tif ${TMP_DIR}/18_198_1567.tif
=cut
sub merge4tiff {
    my $self = shift;
    my $node = shift;
    my $harvesting = shift;
    my $justWeight = shift;
  
    TRACE;
    
    my ($c, $w);
    my ($code, $weight) = ("",MERGE4TIFF_W);

    # On renseigne dans tous les cas la couleur de nodata, et on donne un fond s'il existe, même s'il y a 4 images,
    # si on utilise les masques
    my $workBgI = undef;
    my $workBgM = undef;
    
    my @childList = $node->getChildren;

    if ($self->{useMasks} || scalar @childList != 4) {
        # Pour cela, on va récupérer le nombre de tuiles (en largeur et en hauteur) du niveau, et 
        # le comparer avec le nombre de tuile dans une image (qui est potentiellement demandée à 
        # rok4, qui n'aime pas). Si l'image contient plus de tuile que le niveau, on ne demande plus
        # (c'est qu'on a déjà tout ce qu'il faut avec les niveaux inférieurs).

        my $tm = $self->{pyramid}->getTileMatrixSet->getTileMatrix($node->getLevel);

        my $tooWide = ($tm->getMatrixWidth() < $self->{pyramid}->getTilesPerWidth);
        my $tooHigh = ($tm->getMatrixHeight() < $self->{pyramid}->getTilesPerHeight);
        
        my $imgPath = File::Spec->catfile($self->{pyramid}->getRootPerType("data",TRUE),$node->getPyramidName);

        if (-f $imgPath) {
            # Il y a dans la pyramide une dalle pour faire image de fond de notre nouvelle dalle.

            if ($self->{pyramid}->getCompression eq 'jpg') {
                # On vérifie d'abord qu'on ne veut pas moissonner une zone trop grande
                if ($tooWide || $tooHigh) {
                    WARN(sprintf "The image would have been too high or too wide to harvest it (level %s)",
                         $node->getLevel);
                } else {
                    # On peut et doit chercher l'image de fond sur le WMS
                    $workBgI = $node->getWorkName("BgI");
                    ($c,$w) = $self->wms2work($node,$harvesting,$justWeight,"BgI");
                    if (! defined $c) {
                        ERROR(sprintf "Cannot harvest image for node %s",$node->getWorkName);
                        return FALSE;
                    }
                    
                    $code .= $c;
                    $weight += $w;
                }
            } else {
                
                my $maskPath = File::Spec->catfile($self->{pyramid}->getRootPerType("mask",TRUE),$node->getPyramidName);
        
                if ( $self->{useMasks} && -f $maskPath ) {
                    # On a en plus un masque associé à l'image de fond
                    ($c,$w) = $self->cache2work($node,TRUE);
                    $code .= $c;
                    $weight += $w;                    
                    $workBgM = $node->getWorkName("BgM");
                } else {
                    ($c,$w) = $self->cache2work($node,FALSE);
                    $code .= $c;
                    $weight += $w;
                }
                
                $workBgI = $node->getWorkName("BgI");
            }
        }
    }
    
    return ("",$weight) if ($justWeight);
    
    # We compose the 'Merge4tiff' call
    #   - the ouput
    $code .= sprintf "Merge4tiff \${TMP_DIR}/%s", $node->getWorkName("I");
    if ($self->{useMasks}) {
        $code .= sprintf " \${TMP_DIR}/%s", $node->getWorkName("M");
    } else {
        $code .= " 0";
    }
    #   - the inputs
    foreach my $childNode ($node->getPossibleChildren) {
        if (defined $childNode){
            $code .= sprintf " \${TMP_DIR}/%s", $childNode->getWorkName("I");
            
            if ($self->{useMasks}){
                $code .= sprintf " \${TMP_DIR}/%s", $childNode->getWorkName("M");
            } else {
                $code .= " 0";
            }
        } else {
            $code .= " 0 0";
        }
    }
    #   - the background (if it exists)
    if ( defined $workBgI ){
        $code.= " \${TMP_DIR}/$workBgI";
        if ( defined $workBgM ){
            $code.= " \${TMP_DIR}/$workBgM";
        }
    }
    $code .= "\n";

    return ($code,$weight);
}

####################################################################################################
#                                        PUBLIC METHODS                                            #
####################################################################################################

# Group: public methods

#
=begin nd
method: configureFunctions

Configure bash functions to write in scripts' header thanks to pyramid's components.
=cut
sub configureFunctions {
    my $self = shift;

    TRACE;

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

    if ($self->getNodata->getNoWhite) {
        $conf_mNt .= "-nowhite ";
    }

    my $nd = $self->getNodata->getValue;
    $conf_mNt .= "-n $nd ";

    $configuredFunc =~ s/__mNt__/$conf_mNt/;
    
    ######## merge4tiff ########
    # work compression : deflate
    my $conf_m4t = "-c zip ";

    my $gamma = $pyr->getGamma;
    $conf_m4t .= "-g $gamma ";
    $conf_m4t .= "-n $nd ";

    $configuredFunc =~ s/__m4t__/$conf_m4t/;

    ######## montage ########
    my $conf_montageIn = "";

    $conf_montageIn .= sprintf "-geometry %sx%s",$self->{pyramid}->getTileWidth,$self->{pyramid}->getTileHeight;
    $conf_montageIn .= sprintf " -tile %sx%s",$self->{pyramid}->getTilesPerWidth,$self->{pyramid}->getTilesPerHeight;
    
    $configuredFunc =~ s/__montageIn__/$conf_montageIn/;
    
    my $conf_montageOut = "";
    
    $conf_montageOut .= "-depth $bps ";
    
    $conf_montageOut .= sprintf "-define tiff:rows-per-strip=%s -compress Zip ",$self->{pyramid}->getCacheImageHeight;
    if ($spp == 4) {
        $conf_montageOut .= "-type TrueColorMatte -background none ";
    } elsif ($spp == 3) {
        $conf_montageOut .= "-type TrueColor ";
    } elsif ($spp == 1) {
        $conf_montageOut .= "-type Grayscale ";
    }

    $configuredFunc =~ s/__montageOut__/$conf_montageOut/;

    ######## tiffcp ########
    my $imgS = $self->{pyramid}->getCacheImageHeight;
    
    # pour les images
    my $conf_tcp = "-s -r $imgS -c zip";
    $configuredFunc =~ s/__tcpI__/$conf_tcp/;
    
    # pour les masques
    $conf_tcp = "-s -r $imgS -c packbits";
    $configuredFunc =~ s/__tcpM__/$conf_tcp/;
    
    ######## tiff2tile ########
    my $conf_t2t = "";

    # pour les images
    my $compression = $pyr->getCompression;
    
    $conf_t2t .= "-c $compression ";
    if ($pyr->getCompressionOption eq 'crop') {
        $conf_t2t .= "-crop ";
    }

    $conf_t2t .= "-p $ph ";
    $conf_t2t .= sprintf "-t %s %s ",$pyr->getTileMatrixSet->getTileWidth,$pyr->getTileMatrixSet->getTileHeight;
    $conf_t2t .= "-b $bps ";
    $conf_t2t .= "-a $sf ";
    $conf_t2t .= "-s $spp ";

    $configuredFunc =~ s/__t2tI__/$conf_t2t/;
    
    # pour les masques
    $conf_t2t = sprintf "-c pkb -p gray -t %s %s -b 8 -a uint -s 1",
        $pyr->getTileMatrixSet->getTileWidth,$pyr->getTileMatrixSet->getTileHeight;
    $configuredFunc =~ s/__t2tM__/$conf_t2t/;
    
    return $configuredFunc;
}


####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getNodata {
    my $self = shift;
    return $self->{pyramid}->getNodata; 
}

sub getPyramid {
    my $self = shift;
    return $self->{pyramid};
}
  
1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

BE4::Commands - Compose commands to generate the cache

=head1 SYNOPSIS

    use BE4::Commands;
  
    # Commands object creation
    my $objCommands = BE4::Commands->new(
        $objPyramid, # BE4::Pyramid object
        TRUE # useMasks
    );

=head1 DESCRIPTION

=over 4

=item pyramid

A BE4::Pyramid object.

=item useMasks

A boolean. If TRUE, all generating tools (mergeNtiff, merge4tiff...) use masks if present and generate a result mask. This processing is longer, that's why default behaviour is without mask.

=back

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-NoData.html">BE4::NoData</A></li>
<li><A HREF="./lib-BE4-Pyramid.html">BE4::Pyramid</A></li>
<li><A HREF="./lib-BE4-Harvesting.html">BE4::Harvesting</A></li>
<li><A HREF="./lib-BE4-Script.html">BE4::Script</A></li>
</ul>

=end html

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHOR

Satabin Théo, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
