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

use constant TRUE  => 1;
use constant FALSE => 0;

# Group: Commands' weights and calls

# Constant: MERGE4TIFF_W
use constant MERGE4TIFF_W => 1;
# Constant: MERGENTIFF_W
use constant MERGENTIFF_W => 4;
# Constant: CACHE2WORK_PNG_W
use constant CACHE2WORK_PNG_W => 3;
# Constant: WGET_W
use constant WGET_W => 35;
# Constant: TIFF2TILE_W
use constant TIFF2TILE_W => 0;
# Constant: TIFFCP_W
use constant TIFFCP_W => 0;

=begin nd
Constant: BASHFUNCTIONS
Define bash functions, used to factorize and reduce scripts :
    - Wms2work
    - Cache2work
    - Work2cache
    - MergeNtiff
    - Merge4tiff
=cut
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
            if [ "$fmt" == "png" ] ; then
                if pngcheck -q $nameImg 1>/dev/null ; then break ; fi
            else
                if tiffck $nameImg 1>/dev/null ; then break ; fi
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
        RM_IMGS["$dir.tif"]="1"
        rm -rf $dir
        return
    fi

    if [ "$fmt" == "png" ]||[ "$nbTiles" != "1x1" ] ; then
        montage -geometry $imgSize -tile $nbTiles $dir/*.$fmt __montageOut__ $dir.tif
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    else
        mv $dir/img01.tif $dir.tif
    fi

    rm -rf $dir
}

Cache2work () {
    local imgSrc=$1
    local workName=$2
    local type=$3
    
    if [  "$type" == "png"  ] ; then
        cp $imgSrc $workName.tif
        mkdir $workName
        untile $workName.tif $workName/
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        montage __montageIn__ $workName/*.png __montageOut__ $workName.tif
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        rm -rf $workName/
    elif [  "$type" == "jpg"  ] ; then
        convert $imgSrc __conv__ $workName.tif
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi        
    else
        tiffcp __tcpI__ $imgSrc $workName.tif
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    fi

}

Work2cache () {
    local level=$1
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
                echo "0/$mskName" >> ${LIST_FILE}

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
        rm -f $bgI
    fi
    
    if [ $bgM ] ; then
        rm -f $bgM
    fi
}

Merge4tiff () {
    local imgOut=$1
    local mskOut=$2
    shift 2
    local levelIn=$1
    local imgIn=( 0 $2 $4 $6 $8 )
    local mskIn=( 0 $3 $5 $7 $9 )
    shift 9
    local imgBg=$1
    local mskBg=$2

    local forRM=''

    # Entrées

    local tempDir="${TMP_DIR}"
    if [ "$levelIn" == "$CUT_LEVEL" ] ; then
        tempDir="${COMMON_TMP_DIR}"
    fi
    
    local inM4T=''
    
    if [ $imgBg ] ; then
        forRM="${TMP_DIR}/$imgBg"
        inM4T="$inM4T -ib ${TMP_DIR}/$imgBg"
        if [ $mskBg ] ; then
            forRM="${TMP_DIR}/$mskBg"
            inM4T="$inM4T -mb ${TMP_DIR}/$mskBg"
        fi
    fi
    
    local nbImgs=0
    for i in `seq 1 4`;
    do
        if [ ${imgIn[$i]} != '0' ] ; then
            if [[ ! ${RM_IMGS[${imgIn[$i]}]} ]] ; then
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
        RM_IMGS[${imgOut}]="1"
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
    useMasks - boolean - Do we want use masks to generate images ?
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
    Wms2work "path/image_several_requests" "png" "1024x1024" "4x4" "250000" "http://localhost/wms-vector?LAYERS=BDD_WLD_WM&SERVICE=WMS&VERSION=1.3.0&REQUEST=getMap&FORMAT=image/png&CRS=EPSG:3857&WIDTH=1024&HEIGHT=1024&STYLES=line&BGCOLOR=0x80BBDA&TRANSPARENT=0X80BBDA" $BBOXES
    (end code)

Parameters (list):
    node - <Node> - Node whose image have to be harvested.
    harvesting - <Harvesting> - To use to harvest image.
    suffix - string - Optionnal, suffix to add after the node's name ('BgI' for example), seperated with an underscore.

Returns:
    An array (code, weight), (undef,WGET_W) if error.
=cut
sub wms2work {
    my ($self, $node, $harvesting, $suffix) = @_;
    
    TRACE;
    
    my @imgSize = $self->{pyramid}->getCacheImageSize($node->getLevel); # ie size tile image in pixel !
    my $tms     = $self->{pyramid}->getTileMatrixSet;
    
    my $nodeName = $node->getWorkBaseName($suffix);
    
    my ($xMin, $yMin, $xMax, $yMax) = $node->getBBox;
    
    my $cmd = $harvesting->getCommandWms2work({
        inversion => $tms->getInversion,
        dir => "\${TMP_DIR}/".$nodeName,
        srs => $tms->getSRS,
        bbox => [$xMin, $yMin, $xMax, $yMax],
        width => $imgSize[0],
        height => $imgSize[1]
    });
    
    return ($cmd,WGET_W);
}

=begin nd
Function: cache2work

Copy image from cache to work directory and transform (work format : untiles, uncompressed). Use the 'Cache2work' bash function.

(see cache2work.png)

2 cases:
    - legal tiff compression -> tiffcp
    - png -> untile + montage
    
Examples:
    (start code)
    Cache2work ${PYR_DIR}/IMAGE/19/02/BF/24.tif ${TMP_DIR}/19_398_3136_BgI
    Cache2work ${PYR_DIR}/IMAGE/19/02/BF/24.tif ${TMP_DIR}/19_398_3136_BgI png
    (end code)
    
Parameters (list):
    node - <Node> - Node whose image have to be transfered in the work directory.
    ownMask - boolean - Specify if mask is combined with this node and have to be imported.

Returns:
    An array (code, weight), ("",-1) if error.
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
        $cmd = sprintf ("Cache2work \${PYR_DIR}/%s \${TMP_DIR}/%s %s\n", $fileName, $workBaseName, $self->{pyramid}->getCompression);
        $weight = TIFFCP_W;
    }
    
    #### Rappatriement du masque de donnée (si présent) ####
    
    if ( $ownMask ) {
        # Un masque est associé à l'image que l'on va utiliser, on doit le mettre également au format de travail
        $fileName = File::Spec->catfile($self->{pyramid}->getRootPerType("mask",FALSE),$node->getPyramidName);
        $workBaseName = $node->getWorkBaseName("BgM");
        
        $cmd .= sprintf ("Cache2work \${PYR_DIR}/%s \${TMP_DIR}/%s\n", $fileName , $workBaseName);
        $weight += TIFFCP_W;
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
    
    my $workName  = $node->getWorkName("I");
    my $pyrName = File::Spec->catfile($self->{pyramid}->getRootPerType("data",FALSE),$node->getPyramidName);
    
    $cmd .= sprintf ("Work2cache %s %s %s %s", $node->getLevel, $workDir, $workName, $pyrName);
    $weight += TIFF2TILE_W;
    
    #### Export du masque, si voulu

    if ($self->{useMasks}) {
        $workName  = $node->getWorkName("M");
        $cmd .= sprintf (" %s", $workName);
    }
    
    if ( $self->{pyramid}->ownMasks() ) {
        $pyrName = File::Spec->catfile($self->{pyramid}->getRootPerType("mask",FALSE),$node->getPyramidName);
        
        $cmd .= sprintf (" %s", $pyrName);
        $weight += TIFF2TILE_W;
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
|    MergeNtiff ${MNT_CONF_DIR}/mergeNtiffConfig_19_397_3134.txt

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub mergeNtiff {
    my $self = shift;
    my $node = shift;

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
            
            $workBgM = "?".$node->getWorkName("BgM");
        } else {
            ($c,$w) = $self->cache2work($node,FALSE);
            $code .= $c;
            $weight += $w;
        }
        
        $workBgI = "?".$node->getWorkName("BgI");
    }
    
    my $workImgPath = "?".$node->getWorkName("I");
    my $workMskPath = undef;
    if ($self->{useMasks}) {
        $workMskPath = "?".$node->getWorkName("M");
    }
    
    my $mNtConfFilename = $node->getWorkBaseName.".txt";
    my $mNtConfFile = File::Spec->catfile($self->{mntConfDir}, $mNtConfFilename);
    
    if (! open CFGF, ">", $mNtConfFile ) {
        ERROR(sprintf "Impossible de creer le fichier $mNtConfFile.");
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
        my $imagePath = File::Spec->catfile($nodesource->getScript->getTempDir, $nodesource->getWorkName("I"));
        my $maskPath = undef;
        if ($self->{useMasks}) {
            $maskPath = File::Spec->catfile($nodesource->getScript->getTempDir, $nodesource->getWorkName("M"));
        }
        printf CFGF "%s", $nodesource->exportForMntConf($imagePath, $maskPath);
    }
    
    close CFGF;
    
    $code .= "MergeNtiff $mNtConfFilename";
    $code .= " $workBgI" if (defined $workBgI); # pour supprimer l'image de fond si elle existe
    $code .= " $workBgM" if (defined $workBgM); # pour supprimer le masque de fond si il existe
    $code .= "\n";

    return ($code,$weight);
}

=begin nd
Function: merge4tiff

Compose the 'merge4tiff' command. If we have not 4 children or if children contain nodata, we have to supply a background, a color or an image if exists.

|                   i1  i2
| backGround    +              =  resultImg
|                   i3  i4

(see merge4tiff.png)

Parameters (list):
    node - <Node> - Node to generate thanks to a 'merge4tiff' command.
    harvesting - <Harvesting> - To use to harvest background if necessary. Can be undefined.

Example:
    (start code)
    merge4tiff -g 1 -n FFFFFF  -i1 19_396_3134.tif -i2 19_397_3134.tif -i3 19_396_3135.tif -i4 19_397_3135.tif 18_198_1567.tif
    (end code)

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub merge4tiff {
    my $self = shift;
    my $node = shift;
    my $harvesting = shift;
  
    TRACE;
    
    my ($c, $w);
    my ($code, $weight) = ("",MERGE4TIFF_W);

    # On renseigne dans tous les cas la couleur de nodata, et on donne un fond s'il existe, même s'il y a 4 images,
    # si on utilise les masques
    my $workBgI = undef;
    my $workBgM = undef;
    
    my @childList = $node->getChildren;

    # Gestion du fond : faut-il le récupérer, et si oui, comment ?
    my $imgPath = File::Spec->catfile($self->{pyramid}->getRootPerType("data",TRUE),$node->getPyramidName);
    my $maskPath = File::Spec->catfile($self->{pyramid}->getRootPerType("mask",TRUE),$node->getPyramidName);
    if ( -f $imgPath && ($self->{useMasks} || scalar @childList != 4) ) {
        
        # Il y a dans la pyramide une dalle pour faire image de fond de notre nouvelle dalle.


        if ( $self->{useMasks} && -f $maskPath ) {
            # On a en plus un masque associé à l'image de fond
            ($c,$w) = $self->cache2work($node,TRUE);
            $code .= $c;
            $weight += $w;
            $workBgM = $node->getWorkName("BgM");
        } else {
            if (defined $harvesting) {
                # On a un service WMS pour moissonner le fond, donc on l'utilise
                # Pour cela, on va récupérer le nombre de tuiles (en largeur et en hauteur) du niveau, et
                # le comparer avec le nombre de tuile dans une image (qui est potentiellement demandée à
                # rok4, qui n'aime pas). Si l'image contient plus de tuile que le niveau, on ne demande plus
                # (c'est qu'on a déjà tout ce qu'il faut avec les niveaux inférieurs).

                my $tm = $self->{pyramid}->getTileMatrixSet->getTileMatrix($node->getLevel);

                my $tooWide = ($tm->getMatrixWidth() < $self->{pyramid}->getTilesPerWidth);
                my $tooHigh = ($tm->getMatrixHeight() < $self->{pyramid}->getTilesPerHeight);

                if (! $tooWide && ! $tooHigh) {
                    ($c,$w) = $self->wms2work($node,$harvesting,"BgI");
                    if (! defined $c) {
                        ERROR(sprintf "Cannot harvest the back ground image %s",$node->getWorkBaseName("BgI"));
                        return ("",-1);
                    }

                    $code .= $c;
                    $weight += $w;
                    $workBgI = $node->getWorkName("BgI");
                }

            } else {
                ($c,$w) = $self->cache2work($node,FALSE);
                $code .= $c;
                $weight += $w;
                $workBgI = $node->getWorkName("BgI");
            }
        }
    }
    
    # We compose the 'Merge4tiff' call
    #   - the ouput
    $code .= sprintf "Merge4tiff %s", $node->getWorkName("I");
    if ($self->{useMasks}) {
        $code .= sprintf " %s", $node->getWorkName("M");
    } else {
        $code .= " 0";
    }
    #   - the inputs
    my $inputsLevel = $self->{pyramid}->getTileMatrixSet()->getBelowLevelID($node->getLevel());
    $code .= sprintf " %s", $inputsLevel;
    foreach my $childNode ($node->getPossibleChildren) {
            
        if (defined $childNode){
            
            $code .= sprintf " %s", $childNode->getWorkName("I");
            
            if ($self->{useMasks}){
                $code .= sprintf " %s", $childNode->getWorkName("M");
            } else {
                $code .= " 0";
            }
        } else {
            $code .= " 0 0";
        }
    }
    #   - the background (if it exists)
    if ( defined $workBgI ){
        $code.= " $workBgI";
        if ( defined $workBgM ){
            $code.= " $workBgM";
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

    $configuredFunc =~ s/__montageOut__/$conf_montageOut/g;

    ######## tiffcp ########
    
    my $conf_tcp = "-s -c zip";
    $configuredFunc =~ s/__tcpI__/$conf_tcp/;

    ######## convert ########

    my $conf_convert = "-compress zip";
    if ($spp == 4) {
        $conf_convert .= " -type TrueColorMatte -background none";
    } elsif ($spp == 3) {
        $conf_convert .= " -type TrueColor";
    } elsif ($spp == 1) {
        $conf_convert .= " -type Grayscale";
    }

    $conf_convert .= " -depth $bps";
    
    $configuredFunc =~ s/__conv__/$conf_convert/;
    
    ######## tiff2tile ########
    my $conf_t2t = "";

    # pour les images
    my $compression = $pyr->getCompression;
    
    $conf_t2t .= "-c $compression ";
    if ($pyr->getCompressionOption eq 'crop') {
        $conf_t2t .= "-crop ";
    }

    $conf_t2t .= "-p $ph -b $bps -a $sf -s $spp ";
    $conf_t2t .= sprintf "-t %s %s ",$pyr->getTileMatrixSet->getTileWidth,$pyr->getTileMatrixSet->getTileHeight;

    $configuredFunc =~ s/__t2tI__/$conf_t2t/;
    
    # pour les masques
    $conf_t2t = sprintf "-c zip -p gray -t %s %s -b 8 -a uint -s 1",
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
    return $self->{pyramid}->getNodata; 
}

# Function: getPyramid
sub getPyramid {
    my $self = shift;
    return $self->{pyramid};
}

=begin nd
Function: setConfDir

Store the directory for mergeNtiff configuration files.

Parameters (list):
    mntConfDir - string - Image pyramid to generate
=cut
sub setConfDir {
    my $self = shift;
    my $mntConfDir = shift;

    $self->{mntConfDir} = $mntConfDir;
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
