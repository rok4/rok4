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

use COMMON::Harvesting;
use COMMON::Node;
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

my $COMMONTEMPDIR;
my $ONTCONFDIR;
my $MERGEMETHOD;

=begin nd
Function: setGlobals

Define and create common working directories
=cut
sub setGlobals {
    $COMMONTEMPDIR = shift;
    $MERGEMETHOD = shift;

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

####################################################################################################
#                                      Group: OVERLAY N TIFF                                       #
####################################################################################################

# Constant: OVERLAYNTIFF_W
use constant OVERLAYNTIFF_W => 3;

my $ONTFUNCTION = <<'ONTFUNCTION';
OverlayNtiff () {
    local config=$1
    local inTemplate=$2

    overlayNtiff -f ${ONT_CONF_DIR}/$config ${OVERLAYNTIFF_OPTIONS}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    rm -f ${TMP_DIR}/$inTemplate
    rm -f ${ONT_CONF_DIR}/$config
}

ONTFUNCTION

my $CEPH_STORAGE_FUNCTIONS = <<'STORAGEFUNCTIONS';
LinkSlab () {
    local target=$1
    local link=$2

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

    work2cache ${TMP_DIR}/$workImgName ${WORK2CACHE_IMAGE_OPTIONS} -pool ${PYR_POOL} $imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    rm -f ${TMP_DIR}/$workImgName

    if [ $workMskName ] ; then
        work2cache ${TMP_DIR}/$workMskName ${WORK2CACHE_MASK_OPTIONS} -pool ${PYR_POOL} $mskName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        rm -f ${TMP_DIR}/$workMskName
    fi
}

PullSlab () {
    local input=$1
    local output=$2

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
}

PullSlab () {
    local input=$1
    local output=$2

    cache2work -c zip $input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

STORAGEFUNCTIONS


=begin nd
Function: linkSlab

Parameters (list):
    node - <JOINCACHE::Node> - Node to treat
=cut
sub linkSlab {
    my $node = shift;
    my $target = shift;
    my $link = shift;

    $node->setCode("LinkSlab $target $link\n");
    $node->writeInScript();
}


=begin nd
Function: convert

Parameters (list):
    node - <JOINCACHE::Node> - Node to treat
=cut
sub convert {
    my $node = shift;

    my $code = "";
    my $nodeName = $node->getWorkBaseName();
    my $inNumber = $node->getSourcesNumber();

    my $sourceImage = $node->getSource(0);
    $code .= sprintf "PullSlab %s ${nodeName}_I.tif\n", $sourceImage->{img};

    my $imgCacheName = $node->getSlabPath("IMAGE", FALSE);
    $code .= "PushSlab ${nodeName}_I.tif $imgCacheName\n\n";

    $node->setCode($code);
    $node->writeInScript();
    
    return TRUE;
}

=begin nd
Function: overlayNtiff

Write commands in the current script to merge N (N could be 1) images according to the merge method. We use *tiff2rgba* to convert into work format and *overlayNtiff* to merge. Masks are treated if needed. Code is store into the node.

If just one input image, overlayNtiff is used to change the image's properties (samples per pixel for example). Mask is not treated (masks have always the same properties and a symbolic link have been created).

Returns:
    A boolean, TRUE if success, FALSE otherwise.

Parameters (list):
    node - <JOINCACHE::Node> - Node to treat
=cut
sub overlayNtiff {
    my $node = shift;

    my $code = "";
    my $nodeName = $node->getWorkBaseName();
    my $inNumber = $node->getSourcesNumber();

    #### Fichier de configuration ####
    my $oNtConfFile = File::Spec->catfile($ONTCONFDIR, "$nodeName.txt");
    
    if (! open CFGF, ">", $oNtConfFile ) {
        ERROR(sprintf "Impossible de creer le fichier $oNtConfFile, en écriture.");
        return FALSE;
    }

    #### Sorties ####

    my $line = File::Spec->catfile($node->getScript()->getTempDir(), $node->getWorkName("I"));
    
    # Pas de masque de sortie si on a juste une image : le masque a été lié symboliquement
    if ($node->getPyramid()->ownMasks() && $inNumber > 1) {
        $line .= " " . File::Spec->catfile($node->getScript->getTempDir(), $node->getWorkName("M"));
    }
    
    printf CFGF "$line\n";
    
    #### Entrées ####
    my $inTemplate = $node->getWorkName("*_*");

    for (my $i = $inNumber - 1; $i >= 0; $i--) {
        # Les images sont dans l'ordre suivant : du dessus vers le dessous
        # Dans le fichier de configuration de overlayNtiff, elles doivent être dans l'autre sens, d'où la lecture depuis la fin.
        my $sourceImage = $node->getSource($i);

        my $inImgName = $node->getWorkName($i."_I");
        my $inImgPath = File::Spec->catfile($node->getScript()->getTempDir(), $inImgName);
        $code .= sprintf "PullSlab %s $inImgName\n", $sourceImage->{img};
        $line = "$inImgPath";

        if (exists $sourceImage->{msk}) {
            my $inMskName = $node->getWorkName($i."_M");
            my $inMskPath = File::Spec->catfile($node->getScript->getTempDir, $inMskName);
            $code .= sprintf "PullSlab %s $inMskName\n", $sourceImage->{msk};
            $line .= " $inMskPath";
        }

        printf CFGF "$line\n";
    }

    close CFGF;

    $code .= "OverlayNtiff $nodeName.txt $inTemplate\n";

    # Final location writting
    my $outImgName = $node->getWorkName("I");
    my $imgCacheName = $node->getSlabPath("IMAGE", FALSE);
    $code .= "PushSlab $outImgName $imgCacheName";
    
    # Pas de masque à tuiler si on a juste une image : le masque a été lié symboliquement
    if ($node->getPyramid()->ownMasks() && $inNumber != 1) {
        my $outMaskName = $node->getWorkName("M");
        my $mskCacheName = $node->getSlabPath("MASK", FALSE);
        $code .= sprintf (" $outMaskName $mskCacheName");
    }

    $code .= "\n\n";

    $node->setCode($code);
    $node->writeInScript();
    
    return TRUE;
}

####################################################################################################
#                                   Group: Export function                                         #
####################################################################################################

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

    # On a précisé une méthode de fusion, on est dans le cas d'un JOINCACHE, on exporte la fonction overlayNtiff
    my $string = sprintf "OVERLAYNTIFF_OPTIONS=\"-c zip -s %s -p %s -b %s -m $MERGEMETHOD",
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

    $string .= "\n";

    return $string;
}
  
1;
__END__
