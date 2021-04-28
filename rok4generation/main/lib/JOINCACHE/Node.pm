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
File: Node.pm

Class: JOINCACHE::Node

(see ROK4GENERATION/libperlauto/JOINCACHE_Node.png)

Descibe a node

Using:
    (start code)
    use JOINCACHE::Node

    my $node = JOINCACHE::Node->new(51, 756, "12", 2);
    (end code)

Attributes:
    col - integer - Column
    row - integer - Row
    level - string - Level's identifiant
    pyramid - <COMMON::PyramidRaster> - Pyramid which node belong to
    script - <COMMON::Script> - Script in which the node will be generated
    sources - hash array - Source images from which this node is generated. One image source :
|               img - string - Absolute path to the image
|               msk - string - Absolute path to the associated mask (optionnal)
|               sourcePyramid - <COMMON::PyramidRaster> - Pyramid which image belong to
=cut

################################################################################

package JOINCACHE::Node;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

use File::Spec;
use Data::Dumper;

use COMMON::Base36;
use COMMON::ProxyGDAL;
use COMMON::PyramidRaster;

use JOINCACHE::Shell;


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

################################################################################

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Node constructor. Bless an instance.

Parameters (list):
    level - string - Node's level ID
    col - integer - Node's column
    row - integer - Node's row
    pyramid - <COMMON::PyramidRaster> - Pyramid which node belong to
    sourcePyramids - array - Pyramids and limits to use as sources
    mainSourceIndice - integer - First source indice to use for the node

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
        level => undef,
        pyramid => undef,
        script => undef,
        sources => []
    };
    
    bless($this, $class);
    
    # init. class
    if (! $this->_init($params)) {
        ERROR("Node initialization failed.");
        return undef;
    }
    if (! $this->_load($params)){
        ERROR("Node loading failed.");
        return undef ;
    }
    
    return $this;
}

=begin nd
Function: _init

Check and store node's attributes values.

Parameters (list):
    level - string - Node's level ID
    col - integer - Node's column
    row - integer - Node's row
=cut
sub _init {
    my $this = shift;
    my $params = shift;
    
    # mandatory parameters !
    if (! exists $params->{level} || ! defined $params->{level}) {
        ERROR("Node's level is undefined !");
        return FALSE;
    }
    if (! exists $params->{col} || ! defined $params->{col}) {
        ERROR("Node's column is undefined !");
        return FALSE;
    }
    if (! exists $params->{row} || ! defined $params->{row}) {
        ERROR("Node's row is undefined !");
        return FALSE;
    }
    if (! exists $params->{pyramid} || ! defined $params->{pyramid}) {
        ERROR("Node's pyramid is undefined !");
        return FALSE;
    }
    
    # init. params
    $this->{col} = $params->{col};
    $this->{row} = $params->{row};
    $this->{level} = $params->{level};
    $this->{pyramid} = $params->{pyramid};
    
    return TRUE;
}

sub _load {
    my $this = shift;
    my $params = shift;

    if (! exists $params->{sourcePyramids} || ! defined $params->{sourcePyramids}) {
        ERROR("Source pyramids is undefined !");
        return FALSE;
    }
    if (! exists $params->{mainSourceIndice} || ! defined $params->{mainSourceIndice}) {
        ERROR("Main source's indice is undefined !");
        return FALSE;
    }

    my $mainLevel = $params->{sourcePyramids}->[$params->{mainSourceIndice}]->{pyr}->getLevel($this->{level});

    # On vérifie que cette dalle appartient bien à l'étendue de la source principale
    # Si c'est une bbox qui était fournie comme extent, on va économiser un intersect GDAL couteux

    if ($params->{sourcePyramids}->[$params->{mainSourceIndice}]->{provided} eq "WKTFILE") {
        my @slabBBOX = $mainLevel->slabIndicesToBbox($this->{col}, $this->{row});
        my $slabOGR = COMMON::ProxyGDAL::geometryFromBbox(@slabBBOX);

        if (! COMMON::ProxyGDAL::isIntersected($slabOGR, $params->{sourcePyramids}->[$params->{mainSourceIndice}]->{extent})) {
            return TRUE;
        }
    } else {
        # Extent était une bbox, on va pouvoir plus simplement vérifier les coordonnées des dalles sans passer par GDAL
        # bboxes dans la source contient forcément une seule bbox, celle fournie.

        my ($ROWMIN, $ROWMAX, $COLMIN, $COLMAX) = @{$params->{sourcePyramids}->[$params->{mainSourceIndice}]->{extrem_slabs}};

        if ($this->{col} < $COLMIN || $this->{col} > $COLMAX || $this->{row} < $ROWMIN || $this->{row} > $ROWMAX) {
            return TRUE;
        }
    }

    # On traite séparément le cas de la source principale (la plus prioritaire car :
    #   - on sait que la dalle cherchée appartient à l'extent de cette source (vérifiée juste au dessus)
    #   - si on ne trouve pas la dalle pour cette source, on arrête là. On reviendra éventuellement sur cette dalle après
    #   - si la méthode de fusion est REPLACE, on ne va pas chercher plus loin

    my $pyr = $params->{sourcePyramids}->[$params->{mainSourceIndice}]->{pyr};
    my $imgPath = $pyr->containSlab("IMAGE", $this->{level}, $this->{col}, $this->{row});
    if ( defined $imgPath ) {
        # L'image existe, voyons également si elle a un masque associé
        my %sourceSlab = (
            img => $imgPath,
            compatible => ($this->{pyramid}->checkCompatibility($pyr) == 2)
        );

        if ($JOINCACHE::Shell::USEMASK && $mainLevel->ownMasks()) {

            my $mskPath = $pyr->containSlab("MASK", $this->{level}, $this->{col}, $this->{row});
            if ( defined $mskPath ) {
                $sourceSlab{msk} = $mskPath;
            }
        }

        push @{$this->{sources}}, \%sourceSlab;
    } else {
        return TRUE;
    }

    if ($JOINCACHE::Shell::MERGEMETHOD eq 'REPLACE') {
        return TRUE;
    }

    for (my $ind = $params->{mainSourceIndice} + 1; $ind < scalar @{$params->{sourcePyramids}}; $ind++) {
        $pyr = $params->{sourcePyramids}->[$ind]->{pyr};
        my $sourceLevel = $pyr->getLevel($this->{level});

        if ($params->{sourcePyramids}->[$ind]->{provided} eq "WKTFILE") {
            my @slabBBOX = $sourceLevel->slabIndicesToBbox($this->{col}, $this->{row});
            my $slabOGR = COMMON::ProxyGDAL::geometryFromBbox(@slabBBOX);

            if (! COMMON::ProxyGDAL::isIntersected($slabOGR, $params->{sourcePyramids}->[$ind]->{extent})) {
                next;
            }
        } else {
            # Extent était une bbox, on va pouvoir plus simplement vérifier les coordonnées des dalles sans passer par GDAL
            # bboxes dans la source contient forcément une seule bbox, celle fournie.

            my ($ROWMIN, $ROWMAX, $COLMIN, $COLMAX) = @{$params->{sourcePyramids}->[$ind]->{extrem_slabs}};

            if ($this->{col} < $COLMIN || $this->{col} > $COLMAX || $this->{row} < $ROWMIN || $this->{row} > $ROWMAX) {
                next;
            }
        }

        
        my $imgPath = $pyr->containSlab("IMAGE", $this->{level}, $this->{col}, $this->{row});
        if ( defined $imgPath ) {
            # L'image existe, voyons également si elle a un masque associé
            my %sourceSlab = (
                img => $imgPath,
                compatible => ($this->{pyramid}->checkCompatibility($pyr) == 2)
            );

            if ($JOINCACHE::Shell::USEMASK && $sourceLevel->ownMasks()) {

                my $mskPath = $pyr->containSlab("MASK", $this->{level}, $this->{col}, $this->{row});
                if ( defined $mskPath ) {
                    $sourceSlab{msk} = $mskPath;
                }
            }

            push @{$this->{sources}}, \%sourceSlab;
        }
    }

    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getPyramid
sub getPyramid {
    my $this = shift;
    return $this->{pyramid};
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

# Function: getLevel
sub getLevel {
    my $this = shift;
    return $this->{level};
}

=begin nd
Function: setScript

Parameters (list):
    script - <Script> - Script to set.
=cut
sub setScript {
    my $this = shift;
    my $script = shift;

    if (! defined $script || ref ($script) ne "COMMON::Script") {
        ERROR("We expect to have a COMMON::Script object.");
    }

    $this->{script} = $script;
}

=begin nd
Function: getWorkBaseName

Returns the work image base name (no extension) : "level_col_row", or "level_col_row_suffix" if suffix is defined.

Parameters (list):
    suffix - string - Optionnal, suffix to add to the work name
=cut
sub getWorkBaseName {
    my $this = shift;
    my $suffix = shift;
    
    # si un suffix est précisé
    return (sprintf "%s_%s_%s_%s", $this->{level}, $this->{col}, $this->{row}, $suffix) if (defined $suffix);
    # si pas de suffix
    return (sprintf "%s_%s_%s", $this->{level}, $this->{col}, $this->{row});
}

=begin nd
Function: getWorkName

Returns the work image name : "level_col_row.tif", or "level_col_row_suffix.tif" if suffix is defined.

Parameters (list):
    prefix - string - Optionnal, suffix to add to the work name
=cut
sub getWorkName {
    my $this = shift;
    my $suffix = shift;
    
    return $this->getWorkBaseName($suffix).".tif";
}

# Function: getSources
sub getSources {
    my $this = shift;
    return $this->{sources};
}

=begin nd
Function: getSource

Parameters (list):
    ind - integer - Index of the wanted source image

Returns
    A source image, as an hash :
|               img - string - Path to the image (object or file)
|               msk - string - Path to the associated mask (optionnal, object or file)
|               sourcePyramid - <COMMON::PyramidRaster> - Pyramid which image belong to
=cut
sub getSource {
    my $this = shift;
    my $ind = shift;
    return $this->{sources}->[$ind];
}

# Function: getSourcesNumber
sub getSourcesNumber {
    my $this = shift;
    return scalar @{$this->{sources}};
}

# Function: getScript
sub getScript {
    my $this = shift;
    return $this->{script};
}

# Function: getSlabPath
sub getSlabPath {
    my $this = shift;
    my $type = shift;
    my $full = shift;

    return $this->{pyramid}->getSlabPath($type, $this->getLevel(), $this->getCol(), $this->getRow(), $full);
}

####################################################################################################
#                              Group: Processing functions                                         #
####################################################################################################


=begin nd
Function: linkSlab

Parameters (list):
    target - string - Path to slab to link
    link - string - Path to symbolic slab
=cut
sub linkSlab {
    my $this = shift;
    my $target = shift;
    my $link = shift;

    $this->{script}->write("LinkSlab $target $link\n");
}


=begin nd
Function: convert
=cut
sub convert {
    my $this = shift;

    my $nodeName = $this->getWorkBaseName();
    my $inNumber = $this->getSourcesNumber();

    my $sourceImage = $this->getSource(0);
    $this->{script}->write(sprintf "PullSlab %s ${nodeName}_I.tif\n", $sourceImage->{img});

    my $imgCacheName = $this->getSlabPath("IMAGE", FALSE);
    $this->{script}->write("PushSlab ${nodeName}_I.tif $imgCacheName\n\n");
    
    return TRUE;
}

=begin nd
Function: overlayNtiff

Write commands in the current script to merge N (N could be 1) images according to the merge method. We use *tiff2rgba* to convert into work format and *overlayNtiff* to merge. Masks are treated if needed. Code is store into the node.

If just one input image, overlayNtiff is used to change the image's properties (samples per pixel for example). Mask is not treated (masks have always the same properties and a symbolic link have been created).

Returns:
    A boolean, TRUE if success, FALSE otherwise.
=cut
sub overlayNtiff {
    my $this = shift;

    my $nodeName = $this->getWorkBaseName();
    my $inNumber = $this->getSourcesNumber();

    #### Fichier de configuration ####
    my $oNtConfFile = File::Spec->catfile($JOINCACHE::Shell::ONTCONFDIR, "$nodeName.txt");
    
    if (! open CFGF, ">", $oNtConfFile ) {
        ERROR(sprintf "Impossible de creer le fichier $oNtConfFile, en écriture.");
        return FALSE;
    }

    #### Sorties ####

    my $line = File::Spec->catfile($this->getScript()->getTempDir(), $this->getWorkName("I"));
    
    # Pas de masque de sortie si on a juste une image : le masque a été lié symboliquement
    if ($this->getPyramid()->ownMasks() && $inNumber > 1) {
        $line .= " " . File::Spec->catfile($this->getScript->getTempDir(), $this->getWorkName("M"));
    }
    
    printf CFGF "$line\n";
    
    #### Entrées ####
    my $inTemplate = $this->getWorkName("*_*");

    for (my $i = $inNumber - 1; $i >= 0; $i--) {
        # Les images sont dans l'ordre suivant : du dessus vers le dessous
        # Dans le fichier de configuration de overlayNtiff, elles doivent être dans l'autre sens, d'où la lecture depuis la fin.
        my $sourceImage = $this->getSource($i);

        my $inImgName = $this->getWorkName($i."_I");
        my $inImgPath = File::Spec->catfile($this->getScript()->getTempDir(), $inImgName);
        $this->{script}->write(sprintf "PullSlab %s $inImgName\n", $sourceImage->{img});
        $line = "$inImgPath";

        if (exists $sourceImage->{msk}) {
            my $inMskName = $this->getWorkName($i."_M");
            my $inMskPath = File::Spec->catfile($this->getScript->getTempDir, $inMskName);
            $this->{script}->write(sprintf "PullSlab %s $inMskName\n", $sourceImage->{msk});
            $line .= " $inMskPath";
        }

        printf CFGF "$line\n";
    }

    close CFGF;

    $this->{script}->write("OverlayNtiff $nodeName.txt $inTemplate\n");

    # Final location writting
    my $outImgName = $this->getWorkName("I");
    my $imgCacheName = $this->getSlabPath("IMAGE", FALSE);
    $this->{script}->write("PushSlab $outImgName $imgCacheName");
    
    # Pas de masque à tuiler si on a juste une image : le masque a été lié symboliquement
    if ($this->getPyramid()->ownMasks() && $inNumber != 1) {
        my $outMaskName = $this->getWorkName("M");
        my $mskCacheName = $this->getSlabPath("MASK", FALSE);
        $this->{script}->write(sprintf (" $outMaskName $mskCacheName"));
    }

    $this->{script}->write("\n\n");
    
    return TRUE;
}

1;
__END__