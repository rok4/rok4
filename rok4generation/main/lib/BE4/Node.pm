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
File: Node.pm

Class: BE4::Node

(see ROK4GENERATION/libperlauto/BE4_Node.png)

Describe a node of a <COMMON::QTree> or a <COMMON::NNGraph>. Allow different storage (FileSystem, Ceph, Swift).

Using:
    (start code)
    use BE4::Node

    my $tm = COMMON::TileMatrix->new(...)
    
    my $graph = COMMON::Qtree->new(...)
    #or
    my $graph = COMMON::NNGraph->new(...)
    
    my $node = BE4::Node->new({
        col => 51,
        row => 756,
        tm => $tm,
        graph => $graph
    });
    (end code)

Attributes:
    col - integer - Column, according to the TMS grid.
    row - integer - Row, according to the TMS grid.

    storageType - string - Final storage type for the node : "FILE", "CEPH" or "S3"

    bgImageBasename - string - 
    bgMaskBasename - string - 

    workImageBasename - string - 
    workMaskBasename - string - 
    workExtension - string - extension of the temporary work image, lower case. Default value : tif.

    tm - <COMMON::TileMatrix> - Tile matrix associated to the level which the node belong to.
    graph - <COMMON::NNGraph> or <COMMON::QTree> - Graph which contains the node.
    weight - integer - Node's weight (number of nodes in its descendence)

    script - <COMMON::Script> - Script in which the node will be generated

    sourceNodes - <BE4::Node> array - Nodes from which this node is generated (working for <COMMON::NNGraph>)
    geoImages - <COMMON::GeoImage> array - Source images from which this node (if it belongs to the tree's bottom level) is generated (working for <COMMON::QTree>)
=cut

################################################################################

package BE4::Node;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

use File::Spec ;
use Data::Dumper ;
use COMMON::Base36 ;
use BE4::Shell;

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
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Node constructor. Bless an instance.

Parameters (hash):
    type - string - Final storage type : 'FILE', 'CEPH', 'S3', 'SWIFT'
    col - integer - Node's column
    row - integer - Node's row
    tm - <COMMON::TileMatrix> - Tile matrix of the level which node belong to
    graph - <COMMON::NNGraph> or <COMMON::QTree> - Graph containing the node.

See also:
    <_init>
=cut
sub new {
    my $class = shift;
    my $params = shift;
    
    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        col => undef,
        row => undef,
        tm => undef,
        graph => undef,

        weight => 1,
        script => undef,

        # Sources pour générer ce noeud
        sourceNodes => [],
        geoImages => [],

        bgImageBasename => undef,
        bgMaskBasename => undef,

        workImageBasename => undef,
        workMaskBasename => undef,
        workExtension => "tif", # for images, masks are always tif
        
        # Stockage final de la dalle dans la pyramide pour ce noeud
        storageType => undef
    };
    
    bless($this, $class);
    
    # mandatory parameters !
    if (! defined $params->{col}) {
        ERROR("Node's column is undef !");
        return undef;
    }
    if (! defined $params->{row}) {
        ERROR("Node's row is undef !");
        return undef;
    }
    if (! defined $params->{tm}) {
        ERROR("Node's tile matrix is undef !");
        return undef;
    }
    if (! defined $params->{graph}) {
        ERROR("Node's graph is undef !");
        return undef;
    }

    # init. params    
    $this->{col} = $params->{col};
    $this->{row} = $params->{row};
    $this->{tm} = $params->{tm};
    $this->{graph} = $params->{graph};
    $this->{storageType} = $this->{graph}->getPyramid()->getStorageType();
    $this->{workImageBasename} = sprintf "%s_%s_%s_I", $this->getLevel(), $this->{col}, $this->{row};
        
    return $this;
}

####################################################################################################
#                                Group: Geographic tools                                           #
####################################################################################################

=begin nd
Function: isPointInNodeBbox

Returns a boolean indicating if the point is inside the bbox of the node.
  
Parameters:
    x - double - X coordinate of the point you want to know if it is in
    y - double - Y coordinate of the point you want to know if it is in
=cut
sub isPointInNodeBbox {
    my $this = shift;
    my $x = shift;
    my $y = shift;
    
    my ($xMinNode,$yMinNode,$xMaxNode,$yMaxNode) = $this->getBBox();
    
    if ( $xMinNode <= $x && $x <= $xMaxNode && $yMinNode <= $y && $y <= $yMaxNode ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

=begin nd
Function: isBboxIntersectingNodeBbox

Tests if the provided bbox intersects the bbox of the node.
      
Parameters:
    Bbox - double list - (xmin,ymin,xmax,ymax) : coordinates of the bbox
=cut
sub isBboxIntersectingNodeBbox {
    my $this = shift;
    my ($xMin,$yMin,$xMax,$yMax) = @_;
    my ($xMinNode,$yMinNode,$xMaxNode,$yMaxNode) = $this->getBBox();
    
    if ($xMax > $xMinNode && $xMin < $xMaxNode && $yMax > $yMinNode && $yMin < $yMaxNode) {
        return TRUE;
    } else {
        return FALSE;
    }
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getLevel
sub getLevel {
    my $this = shift;
    return $this->{tm}->getID;
}

# Function: getTM
sub getTM {
    my $this = shift;
    return $this->{tm};
}

# Function: getGraph
sub getGraph {
    my $this = shift;
    return $this->{graph};
}


# Function: getSlabPath
sub getSlabPath {
    my $this = shift;
    my $type = shift;
    my $full = shift;

    return $this->{graph}->getPyramid()->getSlabPath($type, $this->getLevel(), $this->getCol(), $this->getRow(), $full);
}


# Function: getSlabSize
sub getSlabSize {
    my $this = shift;

    return $this->{graph}->getPyramid()->getSlabSize($this->getLevel());
}

# Function: getCol
sub getCol {
    my $this = shift;
    return $this->{col};
}

# Function: getRow
sub getRow {
    my $this = shift;
    return $this->{row};
}

# Function: getStorageType
sub getStorageType {
    my $this = shift;
    return $this->{storageType};
}

########## scripts 

# Function: getScript
sub getScript {
    my $this = shift;
    return $this->{script};
}

=begin nd
Function: setScript

Parameters (list):
    script - <COMMON::Script> - Script to set.
=cut
sub setScript {
    my $this = shift;
    my $script = shift;
    
    if (! defined $script || ref ($script) ne "COMMON::Script") {
        ERROR("We expect to a COMMON::Script object.");
    }
    
    $this->{script} = $script; 
}

########## work files

# Function: setWorkExtension
sub setWorkExtension {
    my $this = shift;
    my $ext = shift;
    
    $this->{workExtension} = lc($ext);
}

# Function: addBgImage
sub addWorkMask {
    my $this = shift;
    $this->{workMaskBasename} = sprintf "%s_%s_%s_M", $this->getLevel, $this->{col}, $this->{row};
}

# Function: getWorkImageName
sub getWorkImageName {
    my $this = shift;
    my $withExtension = shift;
    
    return $this->{workImageBasename}.".".$this->{workExtension} if ($withExtension);
    return $this->{workImageBasename};
}

# Function: getWorkMaskName
sub getWorkMaskName {
    my $this = shift;
    my $withExtension = shift;
    
    return $this->{workMaskBasename}.".tif" if ($withExtension);
    return $this->{workMaskBasename};
}

=begin nd
Function: getWorkBaseName

Returns the work image base name (no extension) : "level_col_row"
=cut
sub getWorkBaseName {
    my $this = shift;
    return (sprintf "%s_%s_%s", $this->getLevel, $this->{col}, $this->{row});
}

########## background files

# Function: addBgImage
sub addBgImage {
    my $this = shift;

    $this->{bgImageBasename} = sprintf "%s_%s_%s_BgI", $this->getLevel, $this->{col}, $this->{row};
}

# Function: getBgImageName
sub getBgImageName {
    my $this = shift;
    my $withExtension = shift;

    return undef if (! defined $this->{bgImageBasename});
    
    return $this->{bgImageBasename}.".tif" if ($withExtension);
    return $this->{bgImageBasename};
}

# Function: addBgMask
sub addBgMask {
    my $this = shift;

    $this->{bgMaskBasename} = sprintf "%s_%s_%s_BgM", $this->getLevel, $this->{col}, $this->{row};
}

# Function: getBgMaskName
sub getBgMaskName {
    my $this = shift;
    my $withExtension = shift;

    return undef if (! defined $this->{bgMaskBasename});
    
    return $this->{bgMaskBasename}.".tif" if ($withExtension);
    return $this->{bgMaskBasename};
}

# Function: getSourceNodes
sub getSourceNodes {
    my $this = shift;
    return $this->{sourceNodes};
}

# Function: getGeoImages
sub getGeoImages {
    my $this = shift;
    return $this->{geoImages};
}

=begin nd
Function: addSourceNodes

Parameters (list):
    nodes - <BE4::Node> array - Source nodes to add
=cut
sub addSourceNodes {
    my $this = shift;
    my @nodes = shift;
    
    push(@{$this->getSourceNodes()},@nodes);
    
    return TRUE;
}

=begin nd
Function: addGeoImages

Parameters (list):
    images - <GeoImage> array - Source images to add
=cut
sub addGeoImages {
    my $this = shift;
    my @images = shift;
    
    push(@{$this->getGeoImages()},@images);
    
    return TRUE;
}

# Function: getUpperLeftTile
sub getUpperLeftTile {
    my $this = shift;

    return (
        $this->{col} * $this->{graph}->getPyramid()->getTilesPerWidth(),
        $this->{row} * $this->{graph}->getPyramid()->getTilesPerHeight()
    );
}

# Function: getBBox
sub getBBox {
    my $this = shift;
    
    my @Bbox = $this->{tm}->indicesToBbox(
        $this->{col},
        $this->{row},
        $this->{graph}->getPyramid()->getTilesPerWidth(),
        $this->{graph}->getPyramid()->getTilesPerHeight()
    );
    
    return @Bbox;
}

=begin nd
Function: getPossibleChildren

Returns a <BE4::Node> array, containing children (length is always 4, with undefined value for children which don't exist), an empty array if the node is a leaf.

Warning:
    Do not mistake with <getChildren>
=cut
sub getPossibleChildren {
    my $this = shift;
    return $this->{graph}->getPossibleChildren($this);
}

=begin nd
Function: getChildren

Returns a <BE4::Node> array, containing real children (max length = 4), an empty array if the node is a leaf.

Warning:
    Do not mistake with <getPossibleChildren>
=cut
sub getChildren {
    my $this = shift;
    return $this->{graph}->getChildren($this);
}


# Function: isAboveCutLevelNode
sub isAboveCutLevelNode {
    my $this = shift;

    if (ref($this->{graph}) ne "COMMON::QTree") {
        return FALSE;
    }

    if ($this->{graph}->getCutLevelID() eq $this->getGraph()->getPyramid()->getTileMatrixSet()->getBelowLevelID($this->getLevel())) {
        return TRUE;
    }

    return FALSE;
}

# Function: isCutLevelNode
sub isCutLevelNode {
    my $this = shift;

    if (ref($this->{graph}) ne "COMMON::QTree") {
        return FALSE;
    }

    if ($this->{graph}->getCutLevelID() eq $this->getLevel()) {
        return TRUE;
    }

    return FALSE;
}

# Function: isTopLevelNode
sub isTopLevelNode {
    my $this = shift;

    if ($this->{graph}->getTopID() eq $this->getLevel()) {
        return TRUE;
    }

    return FALSE;
}

####################################################################################################
#                              Group: Processing functions                                         #
####################################################################################################

=begin nd
Function: mergeNtiff

Use the 'MergeNtiff' bash function. Write a configuration file, with sources.

(see ROK4GENERATION/tools/mergeNtiff.png)

Returns:
    TRUE if success, FALSE if failure
=cut
sub mergeNtiff {
    my $this = shift;

    # Si elle existe, on copie la dalle de la pyramide de base dans le repertoire de travail 
    # en la convertissant du format cache au format de travail: c'est notre image de fond.
    # Si la dalle de la pyramide de base existe, on a créé un lien, donc il existe un fichier
    # correspondant dans la nouvelle pyramide.
    # On fait de même avec le masque de donnée associé, s'il existe.
    my $imgBg = $this->getSlabPath("IMAGE", TRUE);
    if ($this->getGraph()->getPyramid()->ownAncestor() && COMMON::ProxyStorage::isPresent($this->getStorageType(), $imgBg) ) {
        $this->addBgImage();
        
        my $maskBg = $this->getSlabPath("MASK", TRUE);
        
        if ( $BE4::Shell::USEMASK && defined $maskBg && COMMON::ProxyStorage::isPresent($this->getStorageType(), $maskBg) ) {
            # On a en plus un masque associé à l'image de fond
            $this->addBgMask();
        }

        $this->cache2work();
    }

    if ($BE4::Shell::USEMASK) {
        $this->addWorkMask();
    }
    
    my $mNtConfFilename = $this->getWorkBaseName.".txt";
    my $mNtConfFile = File::Spec->catfile($BE4::Shell::MNTCONFDIR, $mNtConfFilename);
    
    if (! open CFGF, ">", $mNtConfFile ) {
        ERROR(sprintf "Impossible de creer le fichier $mNtConfFile");
        return FALSE;
    }
    
    # La premiere ligne correspond à la dalle résultat: La version de travail de la dalle à calculer.
    # Les points d'interrogation permettent de gérer le dossier où écrire les images grâce à une variable
    # Cet export va également ajouter les fonds (si présents) comme premières sources
    printf CFGF $this->exportForMntConf(TRUE, "?");

    my $listGeoImg = $this->getGeoImages;
    foreach my $img (@{$listGeoImg}) {
        printf CFGF "%s", $img->exportForMntConf($BE4::Shell::USEMASK);
    }
    
    close CFGF;
    
    $this->{script}->write("MergeNtiff $mNtConfFilename");
    $this->{script}->write(sprintf " %s", $this->getBgImageName(TRUE)) if (defined $this->getBgImageName()); # pour supprimer l'image de fond si elle existe
    $this->{script}->write(sprintf " %s", $this->getBgMaskName(TRUE)) if (defined $this->getBgMaskName()); # pour supprimer le masque de fond si il existe
    $this->{script}->write("\n");

    return TRUE;
}

=begin nd
Function: cache2work

Copy slab from cache to work directory and transform (work format : untiled, zip-compression). Use the 'PullSlab' bash function.

(see ROK4GENERATION/tools/cache2work.png)

Returns:
    TRUE if success, FALSE if failure
=cut
sub cache2work {
    my $this = shift;
    
    #### Rappatriement de l'image de donnée ####
    
    $this->{script}->write(sprintf "PullSlab %s %s\n", $this->getSlabPath("IMAGE", FALSE), $this->getBgImageName(TRUE));
    
    #### Rappatriement du masque de donnée (si présent) ####
    
    if ( defined $this->getBgMaskName() ) {
        # Un masque est associé à l'image que l'on va utiliser, on doit le mettre également au format de travail
        $this->{script}->write(sprintf "PullSlab %s %s\n", $this->getSlabPath("MASK", FALSE), $this->getBgMaskName(TRUE));
    }
    
    return TRUE;
}

=begin nd
Function: work2cache

Copy image from work directory to cache and transform it (tiled and compressed) thanks to the 'Work2cache' bash function (work2cache).

(see ROK4GENERATION/tools/work2cache.png)

Returns:
    TRUE if success, FALSE if failure
=cut
sub work2cache {
    my $this = shift;
    
    #### Export de l'image

    # Le stockage peut être objet ou fichier
    my $pyrName = $this->getSlabPath("IMAGE", FALSE);

    my $postAction = "none";
    if ($this->isCutLevelNode()) {$postAction = "mv";}
    if ($this->isTopLevelNode()) {$postAction = "rm";}

    $this->{script}->write(sprintf ("PushSlab %s %s %s", $postAction, $this->getWorkImageName(TRUE), $pyrName));
    
    #### Export du masque, si présent

    if ($this->getWorkMaskName()) {
        # On a un masque de travail : on le précise pour qu'il soit potentiellement déplacé dans le temporaire commun ou supprimé
        $this->{script}->write(sprintf (" %s", $this->getWorkMaskName(TRUE)));
        
        # En plus, on veut exporter les masques dans la pyramide, on en précise donc l'emplacement final
        if ( $this->getGraph()->getPyramid()->ownMasks() ) {
            $pyrName = $this->getSlabPath("MASK", FALSE);
            $this->{script}->write(sprintf (" %s", $pyrName));
        }        
    }
    
    $this->{script}->write("\n");

    return TRUE;
}

=begin nd
Function: wms2work

Fetch image corresponding to the node thanks to 'wget', in one or more steps at a time. WMS service is described in the current graph's datasource. Use the 'Wms2work' bash function.

Parameters (list):
    harvesting - <COMMON::Harvesting> - To use to harvest image.

Returns:
    TRUE if success, FALSE if failure
=cut
sub wms2work {
    my $this = shift;
    my $harvesting = shift;

    my ($width, $height) = $this->getSlabSize(); # ie size tile image in pixel !
    my $tms = $this->getGraph()->getPyramid()->getTileMatrixSet();    
    my ($xMin, $yMin, $xMax, $yMax) = $this->getBBox();

    # Calcul de la liste des bbox à moissonner
    my ($grid, @bboxes) = $harvesting->getBboxesList(
        $xMin, $yMin, $xMax, $yMax, 
        $width, $height,
        $tms->getInversion()
    );

    if (scalar @bboxes == 0) {
        ERROR("Impossible de calculer la liste des bboxes à moissonner");
        return FALSE;
    }

    $this->{script}->write(sprintf "BBOXES=\"%s\"\n", join("\n", @bboxes));

    # Écriture de la commande

    my $finalExtension = $harvesting->getHarvestExtension();
    if (scalar @bboxes > 1) {
        $finalExtension = "tif";
    }
    $this->setWorkExtension($finalExtension);

    $this->{script}->write(
        sprintf "Wms2work \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \$BBOXES\n",
            $this->getWorkImageName(FALSE),
            $harvesting->getHarvestExtension(), $finalExtension,
            $harvesting->getMinSize(), $harvesting->getHarvestUrl($tms->getSRS(), $width, $height), $grid
    );
    
    return TRUE;
}

=begin nd
Function: merge4tiff

Use the 'Merge4tiff' bash function.

|     i1  i2
|              =  resultImg
|     i3  i4

(see ROK4GENERATION/tools/merge4tiff.png)

Returns:
    TRUE if success, FALSE if failure
=cut
sub merge4tiff {
    my $this = shift;
    
    my @childList = $this->getChildren();

    # Si elle existe, on copie la dalle de la pyramide de base dans le repertoire de travail 
    # en la convertissant du format cache au format de travail: c'est notre image de fond.
    # Si la dalle de la pyramide de base existe, on a créé un lien, donc il existe un fichier
    # correspondant dans la nouvelle pyramide.
    # On fait de même avec le masque de donnée associé, s'il existe.

    my $imgBg = $this->getSlabPath("IMAGE", TRUE);
    if ($this->getGraph()->getPyramid()->ownAncestor() && ($BE4::Shell::USEMASK || scalar @childList != 4) && COMMON::ProxyStorage::isPresent($this->getStorageType(), $imgBg) ) {
        $this->addBgImage();
        
        my $maskBg = $this->getSlabPath("MASK", TRUE);
        
        if ( $BE4::Shell::USEMASK && defined $maskBg && COMMON::ProxyStorage::isPresent($this->getStorageType(), $maskBg) ) {
            # On a en plus un masque associé à l'image de fond
            $this->addBgMask();
        }
        
        $this->cache2work();
    }
    
    if ($BE4::Shell::USEMASK) {
        $this->addWorkMask();
    } 
    
    # We compose the 'Merge4tiff' call
    #   - the ouput + background
    $this->{script}->write(sprintf "Merge4tiff %s", $this->exportForM4tConf(TRUE));
    
    #   - the children inputs
    if ($this->isAboveCutLevelNode()) {
        $this->{script}->write(" \${COMMON_TMP_DIR}");
    } else {
        $this->{script}->write(" \${TMP_DIR}");
    }

    foreach my $childNode ($this->getPossibleChildren()) {
            
        if (defined $childNode) {
            $this->{script}->write($childNode->exportForM4tConf(FALSE));
        } else {
            $this->{script}->write(" 0 0");
        }
    }
    
    $this->{script}->write("\n");

    return TRUE;
}

=begin nd
Function: decimateNtiff

Use the 'decimateNtiff' bash function. Write a configuration file, with sources.

(see ROK4GENERATION/toolsdecimateNtiff.png)

Example:
|    DecimateNtiff 12_26_17.txt

Returns:
    TRUE if success, FALSE if failure
=cut
sub decimateNtiff {
    my $this = shift;

    # Si elle existe, on copie la dalle de la pyramide de base dans le repertoire de travail 
    # en la convertissant du format cache au format de travail: c'est notre image de fond.
    # Si la dalle de la pyramide de base existe, on a créé un lien, donc il existe un fichier
    # correspondant dans la nouvelle pyramide.
    # On fait de même avec le masque de donnée associé, s'il existe.
    my $imgBg = $this->getSlabPath("IMAGE", TRUE);
    if ($this->getGraph()->getPyramid()->ownAncestor() && COMMON::ProxyStorage::isPresent($this->getStorageType(), $imgBg) ) {
        $this->addBgImage();
        
        my $maskBg = $this->getSlabPath("MASK", TRUE);
        
        if ( $BE4::Shell::USEMASK && defined $maskBg && COMMON::ProxyStorage::isPresent($this->getStorageType(), $maskBg) ) {
            # On a en plus un masque associé à l'image de fond
            $this->addBgMask();
        }
        
        this->cache2work();
    }
    
    if ($BE4::Shell::USEMASK) {
        $this->addWorkMask();
    }
    
    my $dntConf = $this->getWorkBaseName().".txt";
    my $dntConfFile = File::Spec->catfile($BE4::Shell::DNTCONFDIR, $dntConf);
    
    if (! open CFGF, ">", $dntConfFile ) {
        ERROR(sprintf "Impossible de creer le fichier $dntConfFile.");
        return FALSE;
    }
    
    # La premiere ligne correspond à la dalle résultat: La version de travail de la dalle à calculer.
    # Cet export va également ajouter les fonds (si présents) comme premières sources
    printf CFGF $this->exportForDntConf(TRUE, $this->getScript()->getTempDir()."/");
    
    #   - Les noeuds sources (NNGraph)
    foreach my $sourceNode ( @{$this->getSourceNodes()} ) {
        printf CFGF "%s", $sourceNode->exportForDntConf(FALSE, $sourceNode->getScript()->getTempDir()."/");
    }
    
    close CFGF;
    
    $this->{script}->write("DecimateNtiff $dntConf");
    $this->{script}->write(sprintf " %s", $this->getBgImageName(TRUE)) if (defined $this->getBgImageName()); # pour supprimer l'image de fond si elle existe
    $this->{script}->write(sprintf " %s", $this->getBgMaskName(TRUE)) if (defined $this->getBgMaskName()); # pour supprimer le masque de fond si il existe
    $this->{script}->write("\n");

    return TRUE;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForMntConf

Export attributes of the Node for mergeNtiff configuration file. Provided paths will be written as is, so can be relative or absolute (or use environment variables).

Masks and backgrounds are always TIFF images.

Parameters (list):
    exportBg - boolean - Export background files (image + mask) if presents. Considered only for file storage
    prefix - string - String to add before paths, can be undefined.
=cut
sub exportForMntConf {
    my $this = shift;
    my $exportBg = shift;
    my $prefix = shift;
    
    $prefix = "" if (! defined $prefix);


    my @Bbox = $this->getBBox;
    my $output = "";

    $output = sprintf "IMG %s%s.%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $prefix, $this->{workImageBasename}, $this->{workExtension},
        $this->{tm}->getSRS(),
        $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
        $this->getTM()->getResolution(), $this->getTM()->getResolution();

    if (defined $this->{workMaskBasename}) {
        $output .= sprintf "MSK %s%s.tif\n", $prefix,  $this->{workMaskBasename};
    }

    if ($exportBg) {
        if (defined $this->{bgImageBasename}) {
            $output .= sprintf "IMG %s%s.tif\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                $prefix, $this->{bgImageBasename},
                $this->{tm}->getSRS(),
                $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
                $this->getTM()->getResolution(), $this->getTM()->getResolution();
                
            if (defined $this->{bgMaskBasename}) {
                $output .= sprintf "MSK %s%s.tif\n", $prefix, $this->{bgMaskBasename};
            }        
        }
    }

    return $output;
}

=begin nd
Function: exportForDntConf

Export attributes of the Node for decimateNtiff configuration file. Provided paths will be written as is, so can be relative or absolute.

Masks and backgrounds are always TIFF images.

Parameters (list):
    exportBg - boolean - Export background files (image + mask) if presents. Considered only for file storage
    prefix - string - String to add before paths, can be undefined.
=cut
sub exportForDntConf {
    my $this = shift;
    my $exportBg = shift;
    my $prefix = shift;
    
    $prefix = "" if (! defined $prefix);


    my @Bbox = $this->getBBox();
    my $output = "";

    $output = sprintf "IMG %s%s.%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $prefix, $this->{workImageBasename}, $this->{workExtension},
        $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
        $this->getTM()->getResolution(), $this->getTM()->getResolution();

    if (defined $this->{workMaskBasename}) {
        $output .= sprintf "MSK %s%s.tif\n", $prefix,  $this->{workMaskBasename};
    }
    
    if ($exportBg) {
        if (defined $this->{bgImageBasename}) {
            $output .= sprintf "IMG %s%s.tif\t%s\t%s\t%s\t%s\t%s\t%s\n",
                $prefix, $this->{bgImageBasename},
                $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
                $this->getTM()->getResolution(), $this->getTM()->getResolution();
                
            if (defined $this->{bgMaskBasename}) {
                $output .= sprintf "MSK %s%s.tif\n", $prefix, $this->{bgMaskBasename};
            }        
        }
    }

    return $output;
}

=begin nd
Function: exportForM4tConf

Export work files (output) and eventually background (input) of the Node for the Merge4tiff call line.

Parameters (list):
    exportBg - boolean - Export background files (image + mask) if presents. Considered only for file storage
=cut
sub exportForM4tConf {
    my $this = shift;
    my $exportBg = shift;

    my $output = sprintf " %s.%s", $this->{workImageBasename}, $this->{workExtension};

    if (defined $this->{workMaskBasename}) {
        $output .= sprintf " %s.tif", $this->{workMaskBasename};
    } else {
        $output .= " 0"
    }
    
    if ($exportBg) {
    
        if (defined $this->{bgImageBasename}) {
            $output .= sprintf " %s.tif", $this->{bgImageBasename};
            
            if (defined $this->{bgMaskBasename}) {
                $output .= sprintf " %s.tif", $this->{bgMaskBasename};
            } else {
                $output .= " 0"
            }
        } else {
            $output .= " 0 0"
        }
        
    }

    return $output;
}

1;
__END__