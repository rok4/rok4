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
File: PyramidRasterOD.pm

Class: COMMON::PyramidRasterOD

(see ROK4GENERATION/libperlauto/COMMON_PyramidRasterOD.png)

Store all informations about a raster pyramid, whatever the storage type.

Using:
    (start code)
    use COMMON::PyramidRasterOD;
    (end code)

Attributes:
    type - string - READ (pyramid load from a descriptor) or WRITE ("new" pyramid, create from values)

    name - string - Pyramid's name
    desc_path - string - Directory in which we write the pyramid's descriptor

    image_width - integer - Number of tile in an pyramid's image, widthwise.
    image_height - integer - Number of tile in an pyramid's image, heightwise.

    pyrImgSpec - <COMMON::PyramidRasterSpec> - Pyramid's image's components
    tms - <COMMON::TileMatrixSet> - Pyramid's images will be cutted according to this TMS grid.
    nodata - <COMMON::NoData> - Information about nodata (like its value)
    levels - <COMMON::LevelRasterOD> hash - Key is the level ID, the value is the <COMMON::LevelRasterOD> object. Define levels present in the pyramid.

    persistent - boolean - Precise if we want to store slabs generated on fly
    data_path - string - Directory in which we write the pyramid's data if storage
    dir_depth - integer - Number of subdirectories from the level root to the image if storage

=cut

################################################################################

package COMMON::PyramidRasterOD;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use XML::LibXML;

use File::Basename;
use File::Spec;
use File::Path;
use File::Copy;
use Tie::File;
use Cwd;
use Devel::Size qw(size total_size);

use Data::Dumper;

use COMMON::LevelRasterOD;
use COMMON::NoData;
use COMMON::PyramidRasterSpec;
use COMMON::ProxyStorage;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT = qw();

################################################################################
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

# Constant: DEFAULT
# Define default values for attributes dir_depth, image_width and image_height.
my %DEFAULT = (
    dir_depth => 2,
    image_width => 16,
    image_height => 16
);

################################################################################

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Pyramid constructor. Bless an instance.

Parameters (list):
    type - string - VALUES
    params - hash - All parameters about the new pyramid, "pyramid" section of the wmtsalad configuration file
       or
    type - string - DESCRIPTOR
    params - string - Path to the pyramid's descriptor to load

See also:
    <_readDescriptor>, <_load>
=cut
sub new {
    my $class = shift;
    my $type = shift;
    my $params = shift;

    $class = ref($class) || $class;

    # IMPORTANT : if modification, think to update natural documentation (just above)

    my $this = {
        type => undef,

        name => undef,
        desc_path => undef,

        pyrImgSpec => undef,
        tms => undef,
        nodata => undef,
        levels => {},

        # Si stockage (forcément fichier)
        persistent => FALSE,
        data_path => undef,
        image_width  => undef,
        image_height => undef,
        dir_depth => undef
    };

    bless($this, $class);

    if ($type eq "DESCRIPTOR") {

        $params = File::Spec->rel2abs($params);

        # Le paramètre est le chemin du descripteur de pyramide, on en tire 'name' et 'desc_path'
        if (! -f $params) {
            ERROR ("XML file does not exist: $params !");
            return undef;
        }

        # Cette pyramide est donc en lecture
        $this->{type} = "READ";

        $this->{name} = File::Basename::basename($params);
        $this->{name} =~ s/\.pyr$//i;
        $this->{desc_path} = File::Basename::dirname($params);

        # On remplit params avec les paramètres issus du parsage du XML
        $params = {};
        $params->{desc_path} = $this->{desc_path};
        $params->{name} = $this->{name};
        if (! $this->_readDescriptor($params)) {
            ERROR ("Cannot extract informations from pyramid descriptor");
            return undef;
        }

    } else {
        # On crée une pyramide à partir de ses caractéristiques
        # Cette pyramide est donc une nouvelle pyramide, à écrire
        $this->{type} = "WRITE";

        # Pyramid pyr_name, desc path
        if (! exists $params->{pyr_name} || ! defined $params->{pyr_name}) {
            ERROR ("The parameter 'pyr_name' is required!");
            return undef;
        }
        $this->{name} = $params->{pyr_name};
        $this->{name} =~ s/\.pyr$//i;

        if (! exists $params->{pyr_desc_path} || ! defined $params->{pyr_desc_path}) {
            ERROR ("The parameter 'pyr_desc_path' is required!");
            return undef;
        }
        $this->{desc_path} = File::Spec->rel2abs($params->{pyr_desc_path});
    }

    if ( ! $this->_load($params) ) {return undef;}

    return $this;
}

=begin nd
Function: _load
=cut
sub _load {
    my $this   = shift;
    my $params = shift;

    if (! defined $params ) {
        ERROR ("Parameters argument required (null) !");
        return FALSE;
    }


    # TMS
    if (! exists $params->{tms_name} || ! defined $params->{tms_name}) {
        ERROR ("The parameter 'tms_name' is required!");
        return FALSE;
    }
    # On chargera l'objet TMS plus tard on ne mémorise pour le moment que son nom.
    $this->{tms} = $params->{tms_name};
    $this->{tms} =~ s/\.TMS$//i;


    if (exists $params->{persistent} && uc($params->{persistent}) eq "TRUE") {
        $this->{persistent} = TRUE;

        if (! exists $params->{pyr_data_path} || ! defined $params->{pyr_data_path}) {
            ERROR ("The parameter 'pyr_data_path' is required!");
            return undef;
        }
        $this->{data_path} = File::Spec->rel2abs($params->{pyr_data_path});

        if (! exists $params->{dir_depth} || ! defined $params->{dir_depth}) {
            ERROR ("The parameter 'dir_depth' is required!");
            return undef;
        }
        $this->{dir_depth} = $params->{dir_depth};

        # image_width
        if (exists $params->{image_width} && defined $params->{image_width}) {
            $this->{image_width} = $params->{image_width};
        }
        
        # image_height
        if (exists $params->{image_height} && defined $params->{image_height}) {
            $this->{image_height} = $params->{image_height};
        }
    }

    
    # PyrImageSpec
    my $pyrImgSpec = COMMON::PyramidRasterSpec->new($params);

    if (! defined $pyrImgSpec) {
        ERROR ("Can not load specification of pyramid's images !");
        return FALSE;
    }

    $this->{pyrImgSpec} = $pyrImgSpec;

    # NoData
    if (! exists $params->{color} || ! defined $params->{color}) {
        ERROR ("The parameter 'color' is required!");
        return FALSE;
    }

    ##### create NoData !
    my $objNodata = COMMON::NoData->new({
        pixel   => $this->{pyrImgSpec}->getPixel(),
        value   => $params->{color}
    });

    if (! defined $objNodata) {
        ERROR ("Can not load NoData !");
        return FALSE;
    }
    $this->{nodata} = $objNodata;

    return TRUE;
}


=begin nd
Function: _readDescriptor
=cut
sub _readDescriptor {
    my $this   = shift;
    my $params = shift;

    my $pyrDescFile = File::Spec->catfile($this->{desc_path},$this->{name}.".pyr");

    # read xml pyramid
    my $parser  = XML::LibXML->new();
    my $xmltree =  eval { $parser->parse_file($pyrDescFile); };

    if (! defined ($xmltree) || $@) {
        ERROR (sprintf "Can not read the XML file $pyrDescFile : %s !", $@);
        return FALSE;
    }

    my $root = $xmltree->getDocumentElement;

    # NODATA
    my $tagnodata = $root->findnodes('nodataValue')->to_literal;
    if ($tagnodata eq '') {
        ERROR (sprintf "Can not extract 'nodata' from the XML file $pyrDescFile !");
        return FALSE;
    }
    $params->{color} = $tagnodata;
    
    # PHOTOMETRIC
    my $tagphotometric = $root->findnodes('photometric')->to_literal;
    if ($tagphotometric eq '') {
        ERROR (sprintf "Can not extract 'photometric' from the XML file $pyrDescFile !");
        return FALSE;
    }
    $params->{photometric} = $tagphotometric;

    # INTERPOLATION    
    my $taginterpolation = $root->findnodes('interpolation')->to_literal;
    if ($taginterpolation eq '') {
        ERROR (sprintf "Can not extract 'interpolation' from the XML file $pyrDescFile !");
        return FALSE;
    }
    $params->{interpolation} = $taginterpolation;

    # TMS
    my $tagtmsname = $root->findnodes('tileMatrixSet')->to_literal;
    if ($tagtmsname eq '') {
        ERROR (sprintf "Can not extract 'tileMatrixSet' from the XML file $pyrDescFile !");
        return FALSE;
    }
    $params->{tms_name} = $tagtmsname;

    # FORMAT
    my $tagformat = $root->findnodes('format')->to_literal;
    if ($tagformat eq '') {
        ERROR (sprintf "Can not extract 'format' in the XML file $pyrDescFile !");
        return FALSE;
    }

    my ($comp, $sf, $bps) = COMMON::PyramidRasterSpec::decodeFormat($tagformat);
    if (! defined $comp) {
        ERROR (sprintf "Can not decode the pyramid format");
        return FALSE;
    }

    $params->{sampleformat} = $sf;
    $params->{compression} = $comp;
    $params->{bitspersample} = $bps;

    # SAMPLESPERPIXEL  
    my $tagsamplesperpixel = $root->findnodes('channels')->to_literal;
    if ($tagsamplesperpixel eq '') {
        ERROR (sprintf "Can not extract 'channels' in the XML file $pyrDescFile !");
        return FALSE;
    } 
    $params->{samplesperpixel} = $tagsamplesperpixel;

    # load pyramid level
    my @levels = $root->getElementsByTagName('level');

    my $oneLevelId;
    my $persistent;

    foreach my $v (@levels) {

        my $tagtm = $v->findvalue('tileMatrix');

        my $objLevel = COMMON::LevelRasterOD->new("XML", $v, $this->{desc_path});
        if (! defined $objLevel) {
            ERROR(sprintf "Can not load the pyramid level : '%s'", $tagtm);
            return FALSE;
        }

        # On vérifie que les niveaux sont tous persistents ou aucun
        if(defined $persistent && $objLevel->isPersistent() != $persistent) {
            ERROR("All level have to be onFly or onDemand");
            return FALSE;
        }
        $persistent = $objLevel->isPersistent();

        $this->{levels}->{$tagtm} = $objLevel;

        $oneLevelId = $tagtm;
    }

    if (defined $oneLevelId) {
        if ($persistent) {
            $params->{persistent} = "TRUE";
            $params->{image_width}  = $this->{levels}->{$oneLevelId}->getImageWidth();
            $params->{image_height} = $this->{levels}->{$oneLevelId}->getImageHeight();
            ($params->{dir_depth}, $params->{pyr_data_path}) = $this->{levels}->{$oneLevelId}->getDirsInfo();
        }

    } else {
        # On a aucun niveau dans la pyramide à charger, il va donc nous manquer des informations : on sort en erreur
        ERROR("No level in the pyramid's descriptor $pyrDescFile");
        return FALSE;
    }

    return TRUE;
}

####################################################################################################
#                                Group: Common getters                                             #
####################################################################################################

# Function: getDataDir
sub getDataDir {
    my $this = shift;    
    return File::Spec->catfile($this->{data_path}, $this->{name});
}

# Function: getLevels
sub getLevels {
    my $this = shift;
    return values %{$this->{levels}};
}

# Function: getTileMatrixSet
sub getTileMatrixSet {
    my $this = shift;
    return $this->{tms};
}


####################################################################################################
#                                        Group: Update pyramid                                     #
####################################################################################################


=begin nd
Function: bindTileMatrixSet
=cut
sub bindTileMatrixSet {
    my $this = shift;
    my $tmsPath = shift;

    # 1 : Créer l'objet TileMatrixSet
    my $tmsFile = File::Spec->catdir($tmsPath, $this->{tms}.".tms");
    $this->{tms} = COMMON::TileMatrixSet->new($tmsFile, TRUE);
    if (! defined $this->{tms}) {
        ERROR("Cannot create a TileMatrixSet object from the file $tmsFile");
        return FALSE;
    }

    # 2 : Lier le TileMatrix à chaque niveau de la pyramide
    while (my ($id, $level) = each(%{$this->{levels}}) ) {
        if (! $level->bindTileMatrix($this->{tms})) {
            ERROR("Cannot bind a TileMatrix to pyramid's level $id");
            return FALSE;
        }
    }

    return TRUE;
}


=begin nd
Function: addLevel
=cut
sub addLevel {
    my $this = shift;
    my $level = shift;
    my $extent = shift;
    my $sources = shift;

    if ($this->{type} eq "READ") {
        ERROR("Cannot add level to 'read' pyramid");
        return FALSE;        
    }

    if (exists $this->{levels}->{$level}) {
        ERROR("Cannot add level $level in pyramid : already exists");
        return FALSE;
    }

    my @bbox = split (',', $extent);
    my @limits = $this->{tms}->getTileMatrix($level)->bboxToIndices(@bbox, 1, 1);

    my $levelParams = {};
    if ($this->{persistent}) {
        # On doit ajouter un niveau avec stockage fichier
        $levelParams = {
            persistent => $this->{persistent},

            tm => $this->{tms}->getTileMatrix($level),
            limits => \@limits,
            sources => $sources,

            size => [$this->{image_width}, $this->{image_height}],
            dir_data => $this->getDataDir(),
            dir_depth => $this->{dir_depth}
        };
    }
    else {
        # On doit ajouter un niveau sans stockage
        $levelParams = {
            persistent => $this->{persistent},

            tm => $this->{tms}->getTileMatrix($level),
            limits => \@limits,
            sources => $sources
        };
    }

    $this->{levels}->{$level} = COMMON::LevelRasterOD->new("VALUES", $levelParams, $this->{desc_path});

    if (! defined $this->{levels}->{$level}) {
        ERROR("Cannot create a Level object for level $level");
        return FALSE;
    }

    return TRUE;
}


=begin nd
Function: updateTMLimits
=cut
sub updateTMLimits {
    my $this = shift;
    my ($level,@bbox) = @_;
        
    $this->{levels}->{$level}->updateLimitsFromBbox(@bbox);
}

####################################################################################################
#                                      Group: Write functions                                     #
####################################################################################################

=begin nd
Function: writeDescriptor
=cut
sub writeDescriptor {
    my $this = shift;
    my $force = shift;

    if (! defined $force) {
        $force = FALSE;     
    }

    if ($this->{type} eq "READ") {
        ERROR("Cannot write descriptor of 'read' pyramid");
        return FALSE;        
    }

    my $descPath = File::Spec->catdir($this->{desc_path}, $this->{name}.".pyr");

    if (! $force && -f $descPath) {
        ERROR("New pyramid descriptor ('$descPath') exist, can not overwrite it !");
        return FALSE;
    }

    if (! open FILE, ">:encoding(UTF-8)", $descPath ){
        ERROR(sprintf "Cannot open the pyramid descriptor %s to write",$descPath);
        return FALSE;
    }

    my $string = "<?xml version='1.0' encoding='UTF-8'?>\n";
    $string .= "<Pyramid>\n";
    $string .= sprintf "    <tileMatrixSet>%s</tileMatrixSet>\n", $this->{tms}->getName();
    $string .= sprintf "    <format>%s</format>\n", $this->{pyrImgSpec}->getFormatCode();
    $string .= sprintf "    <channels>%s</channels>\n", $this->{pyrImgSpec}->getPixel()->getSamplesPerPixel();
    $string .= sprintf "    <nodataValue>%s</nodataValue>\n", $this->{nodata}->getValue();
    $string .= sprintf "    <interpolation>%s</interpolation>\n", $this->{pyrImgSpec}->getInterpolation();
    $string .= sprintf "    <photometric>%s</photometric>\n", $this->{pyrImgSpec}->getPixel()->getPhotometric();


    my @orderedLevels = sort {$a->getOrder <=> $b->getOrder} ( values %{$this->{levels}});

    for (my $i = scalar @orderedLevels - 1; $i >= 0; $i--) {
        # we write levels in pyramid's descriptor from the top to the bottom
        $string .= $orderedLevels[$i]->exportToXML();
    }

    $string .= "</Pyramid>";

    print FILE $string;

    close(FILE);

    return TRUE
}


####################################################################################################
#                                   Group: Clone function                                          #
####################################################################################################

=begin nd
Function: clone

Clone object. Recursive clone only for levels. Other object attributes are just referenced.
=cut
sub clone {
    my $this = shift;
    
    my $clone = { %{ $this } };
    bless($clone, 'COMMON::PyramidRasterOD');
    delete $clone->{levels};

    while (my ($id, $level) = each(%{$this->{levels}}) ) {
        $clone->{levels}->{$id} = $level->clone();
    }

    return $clone;
}

1;
__END__
