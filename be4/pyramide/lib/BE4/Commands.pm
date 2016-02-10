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

Class: BE4::Commands

Configure and assemble commands used to generate pyramid's images.

All schemes in this page respect this legend :

(see formats.png)

Using:
    (start code)
    use BE4::Commands;

    # Commands object creation
    my $objCommands = BE4::Commands->new(
        $objPyramid, # BE4::Pyramid object
        TRUE, # useMasks
    );
    (end code)

Attributes:
    pyramid - <Pyramid> - Allowed to know output format specifications and configure commands.
    mntConfDir - string - Directory, where to write mergeNtiff configuration files.
    dntConfDir - string - Directory, where to write decimateNtiff configuration files.
    useMasks - boolean - If TRUE, all generating tools (mergeNtiff, merge4tiff...) use masks if present and generate a resulting mask. This processing is longer, that's why default behaviour is without mask.
=cut

################################################################################

package BE4::Commands;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

use COMMON::Harvesting;
use BE4::Level;
use BE4::Script;

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
# Constant: CACHE2WORK_W
use constant CACHE2WORK_W => 1;
# Constant: WGET_W
use constant WGET_W => 35;
# Constant: TIFF2TILE_W
use constant TIFF2TILE_W => 1;

=begin nd
Constant: BASHFUNCTIONS
Define bash functions, used to factorize and reduce scripts :
    - Wms2work
    - Cache2work
    - Work2cache
    - MergeNtiff
    - Merge4tiff
    - DecimateNtiff
=cut
my $BASHFUNCTIONS   = <<'FUNCTIONS';

declare -A RM_IMGS

Wms2work () {
    local dir=$1
    local harvest_ext=$2
    local final_ext=$3
    local nbTiles=$4
    local min_size=$5
    local url=$6
    shift 6

    local size=0

    mkdir $dir

    for i in `seq 1 $#`;
    do
        nameImg=`printf "$dir/img%.5d.$harvest_ext" $i`
        local count=0; local wait_delay=1
        while :
        do
            let count=count+1
            wget --no-verbose -O $nameImg "$url&BBOX=$1"

            if [ $? == 0 ] ; then
                if [ "$harvest_ext" == "png" ] ; then
                    if pngcheck $nameImg 1>/dev/null ; then break ; fi
                else
                    if tiffck $nameImg 1>/dev/null ; then break ; fi
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
    
    if [ "$size" -le "$min_size" ] ; then
        RM_IMGS["$dir.$final_ext"]="1"

        rm -rf $dir
        return
    fi

    if [ "$nbTiles" != "1 1" ] ; then
        composeNtiff -g $nbTiles -s $dir/ -c zip $dir.$final_ext
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    else
        mv $dir/img00001.$harvest_ext $dir.$final_ext
    fi

    rm -rf $dir
}

Cache2work () {
    local imgSrc=$1
    local workBaseName=$2

    cache2work __c2w__ $imgSrc $workBaseName.tif
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

Work2cache () {
    local level=$1
    local workDir=$2
    local workImgName=$3
    local imgName=$4
    local workMskName=$5
    local mskName=$6
    
    
    if [[ ! ${RM_IMGS[$workDir/$workImgName]} ]] ; then
        
        local dir=`dirname ${PYR_DIR}/$imgName`
        
        if [ -r $workDir/$workImgName ] ; then rm -f ${PYR_DIR}/$imgName ; fi
        if [ ! -d $dir ] ; then mkdir -p $dir ; fi
            
        tiff2tile $workDir/$workImgName __t2tI__ ${PYR_DIR}/$imgName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        echo "0/$imgName" >> ${TMP_LIST_FILE}
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        
        if [ "$level" == "$TOP_LEVEL" ] ; then
            rm $workDir/$workImgName
        elif [ "$level" == "$CUT_LEVEL" ] ; then
            mv $workDir/$workImgName ${COMMON_TMP_DIR}/
        fi
        
        if [ $workMskName ] ; then
            
            if [ $mskName ] ; then
                
                dir=`dirname ${PYR_DIR}/$mskName`
                
                if [ -r $workDir/$workMskName ] ; then rm -f ${PYR_DIR}/$mskName ; fi
                if [ ! -d $dir ] ; then mkdir -p $dir ; fi
                    
                tiff2tile $workDir/$workMskName __t2tM__ ${PYR_DIR}/$mskName
                if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
                echo "0/$mskName" >> ${TMP_LIST_FILE}
                
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
    local bgI=$2
    local bgM=$3
    
    mergeNtiff -f ${MNT_CONF_DIR}/$config -r ${TMP_DIR}/ __mNt__
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    
    rm -f ${MNT_CONF_DIR}/$config
    
    if [ $bgI ] ; then
        rm -f ${TMP_DIR}/$bgI
    fi
    
    if [ $bgM ] ; then
        rm -f ${TMP_DIR}/$bgM
    fi
}

DecimateNtiff () {
    local config=$1
    local bgI=$2
    local bgM=$3
    
    decimateNtiff -f ${DNT_CONF_DIR}/$config __dNt__
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    
    rm -f ${DNT_CONF_DIR}/$config
    
    if [ $bgI ] ; then
        rm -f ${TMP_DIR}/$bgI
    fi
    
    if [ $bgM ] ; then
        rm -f ${TMP_DIR}/$bgM
    fi
}

Merge4tiff () {
    local imgOut=$1
    local mskOut=$2
    shift 2
    local imgBg=$1
    local mskBg=$2
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
    pyr - <Pyramid> - Image pyramid to generate
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

    TRACE;

    if (! defined $pyr || ref ($pyr) ne "BE4::Pyramid") {
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
Function: wms2work

Fetch image corresponding to the node thanks to 'wget', in one or more steps at a time. WMS service is described in the current graph's datasource. Use the 'Wms2work' bash function.

Example:
    (start code)
    BBOXES="10018754.17139461632,-626172.13571215872,10644926.30710678016,0.00000000512
    10644926.30710678016,-626172.13571215872,11271098.442818944,0.00000000512
    11271098.442818944,-626172.13571215872,11897270.57853110784,0.00000000512
    11897270.57853110784,-626172.13571215872,12523442.71424327168,0.00000000512
    10018754.17139461632,-1252344.27142432256,10644926.30710678016,-626172.13571215872
    10644926.30710678016,-1252344.27142432256,11271098.442818944,-626172.13571215872
    11271098.442818944,-1252344.27142432256,11897270.57853110784,-626172.13571215872
    11897270.57853110784,-1252344.27142432256,12523442.71424327168,-626172.13571215872
    10018754.17139461632,-1878516.4071364864,10644926.30710678016,-1252344.27142432256
    10644926.30710678016,-1878516.4071364864,11271098.442818944,-1252344.27142432256
    11271098.442818944,-1878516.4071364864,11897270.57853110784,-1252344.27142432256
    11897270.57853110784,-1878516.4071364864,12523442.71424327168,-1252344.27142432256
    10018754.17139461632,-2504688.54284865024,10644926.30710678016,-1878516.4071364864
    10644926.30710678016,-2504688.54284865024,11271098.442818944,-1878516.4071364864
    11271098.442818944,-2504688.54284865024,11897270.57853110784,-1878516.4071364864
    11897270.57853110784,-2504688.54284865024,12523442.71424327168,-1878516.4071364864"
    #
    Wms2work "path/image_several_requests" "png" "tif" "4 4" "250000" "http://localhost/wms-vector?LAYERS=BDD_WLD_WM&SERVICE=WMS&VERSION=1.3.0&REQUEST=getMap&FORMAT=image/png&CRS=EPSG:3857&WIDTH=1024&HEIGHT=1024&STYLES=line&BGCOLOR=0x80BBDA&TRANSPARENT=0X80BBDA" $BBOXES
    (end code)

Parameters (list):
    node - <Node> - Node whose image have to be harvested.
    harvesting - <Harvesting> - To use to harvest image.

Returns:
    An array (code, weight), (undef,WGET_W) if error.
=cut
sub wms2work {
    my ($self, $node, $harvesting) = @_;
    
    TRACE;
    
    my @imgSize = $self->{pyramid}->getCacheImageSize($node->getLevel); # ie size tile image in pixel !
    my $tms     = $self->{pyramid}->getTileMatrixSet;
    
    my $nodeName = $node->getWorkImageName();
    
    my ($xMin, $yMin, $xMax, $yMax) = $node->getBBox();
    
    my ($cmd, $finalExtension) = $harvesting->getCommandWms2work({
        inversion => $tms->getInversion,
        dir => "\${TMP_DIR}/".$nodeName,
        srs => $tms->getSRS,
        bbox => [$xMin, $yMin, $xMax, $yMax],
        width => $imgSize[0],
        height => $imgSize[1]
    });
    
    if (! defined $cmd) {
        return (undef, WGET_W);
    }

    $node->setWorkExtension($finalExtension);
    
    return ($cmd, WGET_W);
}

=begin nd
Function: cache2work

Copy image from cache to work directory and transform (work format : untiled, zip-compression). Use the 'Cache2work' bash function.

(see cache2work.png)
    
Examples:
    (start code)
    Cache2work ${PYR_DIR}/IMAGE/19/02/BF/24.tif ${TMP_DIR}/19_398_3136_BgI
    Cache2work ${PYR_DIR}/IMAGE/19/02/BF/24.tif ${TMP_DIR}/19_398_3136_BgM
    (end code)
    
Parameters (list):
    node - <Node> - Node whose image have to be transfered in the work directory.

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub cache2work {
    my ($self, $node) = @_;
    
    #### Rappatriement de l'image de donnée ####
    my $fileName = File::Spec->catfile($self->{pyramid}->getDirImage(),$node->getPyramidName);
    
    my $cmd = "";
    my $weight = 0;
    
    $cmd = sprintf "Cache2work \${PYR_DIR}/%s \${TMP_DIR}/%s\n", $fileName, $node->getBgImageName();
    $weight = CACHE2WORK_W;
    
    #### Rappatriement du masque de donnée (si présent) ####
    
    if ( defined $node->getBgMaskName() ) {
        # Un masque est associé à l'image que l'on va utiliser, on doit le mettre également au format de travail
        $fileName = File::Spec->catfile($self->{pyramid}->getDirMask(),$node->getPyramidName);
        
        $cmd .= sprintf ("Cache2work \${PYR_DIR}/%s \${TMP_DIR}/%s\n", $fileName , $node->getBgMaskName());
        $weight += CACHE2WORK_W;
    }
    
    return ($cmd,$weight);
}

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
    
    my $pyrName = File::Spec->catfile($self->{pyramid}->getDirImage(),$node->getPyramidName());
    
    $cmd .= sprintf ("Work2cache %s %s %s %s", $node->getLevel, $workDir, $node->getWorkImageName(TRUE), $pyrName);
    $weight += TIFF2TILE_W;
    
    #### Export du masque, si présent

    if ($node->getWorkMaskName()) {
        # On a un masque de travail : on le précise pour qu'il soit potentiellement déplacé dans le temporaire commun ou supprimé
        $cmd .= sprintf (" %s", $node->getWorkMaskName(TRUE));
        
        # En plus, on veut exporter les masques dans la pyramide, on en précise donc l'emplacement final
        if ( $self->{pyramid}->ownMasks() ) {
            $pyrName = File::Spec->catfile($self->{pyramid}->getDirMask(),$node->getPyramidName());
            
            $cmd .= sprintf (" %s", $pyrName);
            $weight += TIFF2TILE_W;
        }        
    }
    
    $cmd .= "\n";
    
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

    TRACE;
    
    my ($c, $w);
    my ($code, $weight) = ("",MERGENTIFF_W);

    # Si elle existe, on copie la dalle de la pyramide de base dans le repertoire de travail 
    # en la convertissant du format cache au format de travail: c'est notre image de fond.
    # Si la dalle de la pyramide de base existe, on a créé un lien, donc il existe un fichier
    # correspondant dans la nouvelle pyramide.
    # On fait de même avec le masque de donnée associé, s'il existe.
    my $imgPath = File::Spec->catfile($self->{pyramid}->getDirImage(TRUE),$node->getPyramidName());
    
    if ( -f $imgPath ) {
        $node->addBgImage();
        
        my $maskPath = File::Spec->catfile($self->{pyramid}->getDirMask(TRUE),$node->getPyramidName());
        
        if ( $self->{useMasks} && -f $maskPath ) {
            # On a en plus un masque associé à l'image de fond
            $node->addBgMask();
        }
        
        ($c,$w) = $self->cache2work($node);
        $code .= $c;
        $weight += $w;
    }
    
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
    $code .= sprintf " %s", $node->getBgImageName(TRUE) if (defined $node->getBgImageName()); # pour supprimer l'image de fond si elle existe
    $code .= sprintf " %s", $node->getBgMaskName(TRUE) if (defined $node->getBgMaskName()); # pour supprimer le masque de fond si il existe
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

    TRACE;
    
    my ($c, $w);
    my ($code, $weight) = ("",DECIMATENTIFF_W);

    # Si elle existe, on copie la dalle de la pyramide de base dans le repertoire de travail 
    # en la convertissant du format cache au format de travail: c'est notre image de fond.
    # Si la dalle de la pyramide ancêtre existe, on a créé un lien, donc il existe un fichier
    # correspondant dans la nouvelle pyramide.
    # On fait de même avec le masque de donnée associé, s'il existe.
    my $imgPath = File::Spec->catfile($self->{pyramid}->getDirImage(TRUE),$node->getPyramidName());
    
    if ( -f $imgPath ) {
        $node->addBgImage();
        
        my $maskPath = File::Spec->catfile($self->{pyramid}->getDirMask(TRUE),$node->getPyramidName());
        
        if ( $self->{useMasks} && -f $maskPath ) {
            # On a en plus un masque associé à l'image de fond
            $node->addBgMask();
        }
        
        ($c,$w) = $self->cache2work($node);
        $code .= $c;
        $weight += $w;
    }
    
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
    $code .= sprintf " %s", $node->getBgImageName(TRUE) if (defined $node->getBgImageName()); # pour supprimer l'image de fond si elle existe
    $code .= sprintf " %s", $node->getBgMaskName(TRUE) if (defined $node->getBgMaskName()); # pour supprimer le masque de fond si il existe
    $code .= "\n";

    return ($code,$weight);
}

=begin nd
Function: merge4tiff

Use the 'Merge4tiff' bash function.

|                   i1  i2
| backGround    +              =  resultImg
|                   i3  i4

(see merge4tiff.png)

Parameters (list):
    node - <Node> - Node to generate thanks to a 'merge4tiff' command.

Example:
|    

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub merge4tiff {
    my $self = shift;
    my $node = shift;
  
    TRACE;
    
    my ($c, $w);
    my ($code, $weight) = ("",MERGE4TIFF_W);

    # On renseigne dans tous les cas la couleur de nodata, et on donne un fond s'il existe, même s'il y a 4 images,
    # si on utilise les masques
    my $workBgI = undef;
    my $workBgM = undef;
    
    my @childList = $node->getChildren;

    # Gestion du fond : faut-il le récupérer, et si oui, comment ?
    my $imgPath = File::Spec->catfile($self->{pyramid}->getDirImage(TRUE),$node->getPyramidName);
    
    if ( -f $imgPath && ($self->{useMasks} || scalar @childList != 4) ) {
        # Il y a dans la pyramide une dalle pour faire image de fond de notre nouvelle dalle.
        # On détuile l'image de fond (et éventuellement le masque), pour pouvoir travailler avec.
        
        $node->addBgImage();
        
        my $maskPath = File::Spec->catfile($self->{pyramid}->getDirMask(TRUE),$node->getPyramidName);

        if ( $self->{useMasks} && -f $maskPath ) {
            # On a en plus un masque associé à l'image de fond
            $node->addBgMask();
        }
        
        ($c,$w) = $self->cache2work($node);
        $code .= $c;
        $weight += $w;
    }
    
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

    ######## cache2work ########
    
    my $conf_c2w = "-c zip";
    $configuredFunc =~ s/__c2w__/$conf_c2w/;
    
    ######## tiff2tile ########
    my $conf_t2t = "";

    # pour les images
    my $compression = $pyr->getCompression;
    
    $conf_t2t .= "-c $compression ";
    if ($pyr->getCompressionOption eq 'crop') {
        $conf_t2t .= "-crop ";
    }

    $conf_t2t .= sprintf "-t %s %s ",$pyr->getTileMatrixSet->getTileWidth,$pyr->getTileMatrixSet->getTileHeight;

    $configuredFunc =~ s/__t2tI__/$conf_t2t/;
    
    # pour les masques
    $conf_t2t = sprintf "-c zip -t %s %s",
        $pyr->getTileMatrixSet->getTileWidth,$pyr->getTileMatrixSet->getTileHeight;
    $configuredFunc =~ s/__t2tM__/$conf_t2t/;
    
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

    $export .= "\nObject BE4::Commands :\n";
    $export .= "\t Use masks\n" if $self->{useMasks};
    $export .= "\t Doesn't use masks\n" if (! $self->{useMasks});
    $export .= "\t Export masks\n" if $self->{pyramid}->ownMasks();
    $export .= "\t Doesn't export masks\n" if (! $self->{pyramid}->ownMasks());

    return $export;
}
  
1;
__END__
