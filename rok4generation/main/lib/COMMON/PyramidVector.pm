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
File: PyramidVector.pm

Class: COMMON::PyramidVector

(see ROK4GENERATION/libperlauto/COMMON_PyramidVector.png)

Store all informations about a vector pyramid, whatever the storage type.

Using:
    (start code)
    use COMMON::PyramidVector;

    # To create a new FILE pyramid, "to write"
    my $newPyramid = COMMON::PyramidVector->new("VALUES", {
        pyr_desc_path => "/path/to/descriptors/directory",
        pyr_name_new => "TOTO",

        tms_name => "PM",

        image_width => 16,
        image_height => 16,

        pyr_data_path => "/path/to/data/directory",
        dir_depth => 2
    });

    if (! defined $newPyramid) {
        ERROR("Cannot create the new file pyramid");
    }

    if (! $newPyramid->bindTileMatrixSet("/path/to/tms/directory")) {
        ERROR("Can not bind the TMS to new file pyramid");
    }

    # To load an existing pyramid, "to read"
    my $readPyramid = COMMON::PyramidVector->new("DESCRIPTOR", "/path/to/an/existing/pyramid.pyr");

    if (! defined $readPyramid) {
        ERROR("Cannot load the pyramid");
    }

    if (! $readPyramid->bindTileMatrixSet("/path/to/tms/directory")) {
        ERROR("Can not bind the TMS to loaded pyramid");
    }
    (end code)

Attributes:
    type - string - READ (pyramid load from a descriptor) or WRITE ("new" pyramid, create from values)
    own_ancestor - boolean - Precise if pyramid own an ancestor (only for new pyramid)

    name - string - Pyramid's name
    desc_path - string - Directory in which we write the pyramid's descriptor
    content_path - string - Path to the content's list

    image_width - integer - Number of tile in an pyramid's image, widthwise.
    image_height - integer - Number of tile in an pyramid's image, heightwise.

    tms - <COMMON::TileMatrixSet> - Pyramid's images will be cutted according to this TMS grid.
    levels - <COMMON::LevelVector> hash - Key is the level ID, the value is the <COMMON::LevelVector> object. Define levels present in the pyramid.

    storage_type - string - Storage type of data : FILE, S3, SWIFT or CEPH

    data_path - string - Directory in which we write the pyramid's data if FILE storage type
    dir_depth - integer - Number of subdirectories from the level root to the image if FILE storage type : depth = 2 => /.../LevelID/SUB1/SUB2/IMG.tif

    data_bucket - string - Name of the (existing) S3 bucket, where to store data if S3 storage type

    data_container - string - Name of the (existing) Swift container, where to store data if Swift storage type
    keystone_connection - boolean - For swift storage, keystone authentication or not ?

    data_pool - string - Name of the (existing) CEPH pool, where to store data if CEPH storage type

    cachedList - string hash - If loaded, list content in an hash.
    listCached - boolean - Precise if the list has been loaded
    cachedListModified - boolean - Precise if cached list has been modified
=cut

################################################################################

package COMMON::PyramidVector;

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

use Data::Dumper;

use COMMON::LevelVector;
use COMMON::Array;
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
    params - hash - All parameters about the new pyramid, "pyramid" section of the be4 configuration file
       or
    type - string - DESCRIPTOR
    params - string - Path to the pyramid's descriptor to load

    ancestor - <COMMON::PyramidVector> - Optionnal, to provide if we want to use parameters from ancestor 

See also:
    <_readDescriptor>, <_load>
=cut
sub new {
    my $class = shift;
    my $type = shift;
    my $params = shift;
    my $ancestor = shift;

    $class = ref($class) || $class;

    # IMPORTANT : if modification, think to update natural documentation (just above)

    my $this = {
        type => undef,
        own_ancestor => FALSE,

        name => undef,
        desc_path => undef,
        content_path  => undef,

        # OUT
        image_width  => undef,
        image_height => undef,

        tms => undef,
        levels => {},

        storage_type => undef,
        # Pyramide FICHIER
        data_path => undef,
        dir_depth => undef,

        # Pyramide S3
        data_bucket => undef,
        
        # Pyramide SWIFT
        data_container => undef,
        keystone_connection => FALSE,

        # Pyramide CEPH
        data_pool => undef,

        # Cached list
        cachedList => {},
        listCached => FALSE,
        cachedListModified => FALSE
    };

    bless($this, $class);

    if ($type eq "DESCRIPTOR") {

        $params = File::Spec->rel2abs($params);

        # Le paramètre est le chemin du descripteur de pyramide, on en tire 'name' et 'desc_path'
        if (! -f $params) {
            ERROR ("XML file does not exist: $params !");
            return undef;
        }

        # Cette pyramide est donc en lecture, on ne tient pas compte d'un éventuel ancêtre (qu'on ne devrait pas avoir)
        $this->{type} = "READ";
        $ancestor = undef;

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

        # Pyramid pyr_name_new, desc path
        if (! exists $params->{pyr_name_new} || ! defined $params->{pyr_name_new}) {
            ERROR ("The parameter 'pyr_name_new' is required!");
            return undef;
        }
        $this->{name} = $params->{pyr_name_new};
        $this->{name} =~ s/\.(pyr|PYR)$//;

        if (! exists $params->{pyr_desc_path} || ! defined $params->{pyr_desc_path}) {
            ERROR ("The parameter 'pyr_desc_path' is required!");
            return undef;
        }
        $this->{desc_path} = File::Spec->rel2abs($params->{pyr_desc_path});

        if (exists $params->{pyr_data_path} && defined $params->{pyr_data_path}) {

            #### CAS D'UNE PYRAMIDE FICHIER
            $this->{storage_type} = "FILE";
            $this->{data_path} = File::Spec->rel2abs($params->{pyr_data_path});

        }

        elsif (exists $params->{pyr_data_bucket_name} && defined $params->{pyr_data_bucket_name}) {

            #### CAS D'UNE PYRAMIDE S3
            $this->{storage_type} = "S3";
            $this->{data_bucket} = $params->{pyr_data_bucket_name};

        }

        elsif (exists $params->{pyr_data_container_name} && defined $params->{pyr_data_container_name}) {

            #### CAS D'UNE PYRAMIDE SWIFT
            $this->{storage_type} = "SWIFT";
            $this->{data_container} = $params->{pyr_data_container_name};

            if ( exists $params->{keystone_connection} && defined $params->{keystone_connection} && uc($params->{keystone_connection}) eq "TRUE" ) {
                $this->{keystone_connection} = TRUE;                
            }
        }

        elsif (exists $params->{pyr_data_pool_name} && defined $params->{pyr_data_pool_name}) {

            #### CAS D'UNE PYRAMIDE CEPH
            $this->{storage_type} = "CEPH";
            $this->{data_pool} = $params->{pyr_data_pool_name};
        }

        else {
            ERROR("No storage provided for the new pyramid");
            return undef;
        }

    }

    if ( ! $this->_load($params,$ancestor) ) {return undef;}

    return $this;
}

=begin nd
Function: _load
=cut
sub _load {
    my $this   = shift;
    my $params = shift;
    my $ancestor = shift;

    if (! defined $params ) {
        ERROR ("Parameters argument required (null) !");
        return FALSE;
    }

    if (defined $ancestor && $ancestor->getStorageType() ne $this->getStorageType()) {
        ERROR ("An ancestor is provided for the pyramid, and its storage type is not the same than the new pyramid");
        return FALSE;
    }

    if (defined $ancestor) {
        INFO("We have an ancestor, all parameters are picked from this pyramid");
        # les valeurs sont récupérées de l'ancêtre pour s'assurer la cohérence
        $this->{own_ancestor} = TRUE;
        
        $this->{tms} = $ancestor->getTileMatrixSet()->getName();
        $this->{image_width} = $ancestor->getTilesPerWidth();
        $this->{image_height} = $ancestor->getTilesPerHeight();

        if (defined $ancestor->getDirDepth()) {
            $this->{dir_depth} = $ancestor->getDirDepth();
        }
    } else {

        # dir_depth
        if ($this->{storage_type} eq "FILE" && (! exists $params->{dir_depth} || ! defined $params->{dir_depth})) {
            $params->{dir_depth} = $DEFAULT{dir_depth};
            INFO(sprintf "Default value for 'dir_depth' : %s", $params->{dir_depth});
        }
        $this->{dir_depth} = $params->{dir_depth};

        # TMS
        if (! exists $params->{tms_name} || ! defined $params->{tms_name}) {
            ERROR ("The parameter 'tms_name' is required!");
            return FALSE;
        }
        # On chargera l'objet TMS plus tard on ne mémorise pour le moment que son nom.
        $this->{tms} = $params->{tms_name};
        $this->{tms} =~ s/\.TMS$//i;
        
        # image_width
        if (! exists $params->{image_width} || ! defined $params->{image_width}) {
            $params->{image_width} = $DEFAULT{image_width};
            INFO(sprintf "Default value for 'image_width' : %s", $params->{image_width});
        }
        $this->{image_width} = $params->{image_width};

        # image_height
        if (! exists $params->{image_height} || ! defined $params->{image_height}) {
            $params->{image_height} = $DEFAULT{image_height};
            INFO(sprintf "Default value for 'image_height' : %s", $params->{image_height});
        }
        $this->{image_height} = $params->{image_height};
    }

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

    # Read tag value of tileMatrixSet and format, MANDATORY

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

    # load pyramid level
    my @levels = $root->getElementsByTagName('level');

    my $oneLevelId;
    my $storageType = undef;
    foreach my $v (@levels) {

        my $tagtm = $v->findvalue('tileMatrix');

        my $objLevel = COMMON::LevelVector->new("XML", $v, $this->{desc_path});
        if (! defined $objLevel) {
            ERROR(sprintf "Can not load the pyramid level : '%s'", $tagtm);
            return FALSE;
        }

        # On vérifie que tous les niveaux ont le même type de stockage
        if(defined $storageType && $objLevel->getStorageType() ne $storageType) {
            ERROR(sprintf "All level have to own the same storage type (%s -> %s != %s)", $tagtm, $objLevel->getStorageType(), $storageType);
            return FALSE;
        }
        $storageType = $objLevel->getStorageType();

        $this->{levels}->{$tagtm} = $objLevel;

        $oneLevelId = $tagtm;

        # same for each level
    }

    if (defined $oneLevelId) {
        $params->{image_width}  = $this->{levels}->{$oneLevelId}->getImageWidth();
        $params->{image_height} = $this->{levels}->{$oneLevelId}->getImageHeight();

        $this->{storage_type} = $storageType;

        if ($storageType eq "FILE") {
            my ($dd, $dp) = $this->{levels}->{$oneLevelId}->getDirsInfo();
            $params->{dir_depth} = $dd;
            $this->{data_path} = $dp;
        }
        elsif ($storageType eq "S3") {
            $this->{data_bucket} = $this->{levels}->{$oneLevelId}->getS3Info();
        }
        elsif ($storageType eq "SWIFT") {
            ($this->{data_container}, $this->{keystone_connection}) = $this->{levels}->{$oneLevelId}->getSwiftInfo();
        }
        elsif ($storageType eq "CEPH") {
            $this->{data_pool} = $this->{levels}->{$oneLevelId}->getCephInfo();
        }

    } else {
        # On a aucun niveau dans la pyramide à charger, il va donc nous manquer des informations : on sort en erreur
        ERROR("No level in the pyramid's descriptor $pyrDescFile");
        return FALSE;
    }

    return TRUE;
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
    $this->{tms} = COMMON::TileMatrixSet->new($tmsFile);
    if (! defined $this->{tms}) {
        ERROR("Cannot create a TileMatrixSet object from the file $tmsFile");
        return FALSE;
    }

    # Une pyramide vecteur ne peux être faite que sur un TMS Quadtree 3857 ou 4326 (limites de tippecanoe)
    if ( !(uc($this->{tms}->getSRS()) eq "EPSG:3857" && $this->{tms}->isQTree()) && 
        !(uc($this->{tms}->getSRS()) eq "EPSG:4326" && $this->{tms}->isQTree()))
    {
        ERROR("TMS ($tmsFile) is not handled for a vector pyramid (tippecanoe)");
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
    my $ancestor = shift;
    my $dbs = shift;

    if ($this->{type} eq "READ") {
        ERROR("Cannot add level to 'read' pyramid");
        return FALSE;        
    }

    if (exists $this->{levels}->{$level}) {
        ERROR("Cannot add level $level in pyramid : already exists");
        return FALSE;
    }

    my $levelParams = {};
    if (defined $this->{data_path}) {
        # On doit ajouter un niveau stockage fichier
        $levelParams = {
            id => $level,
            tm => $this->{tms}->getTileMatrix($level),
            size => [$this->{image_width}, $this->{image_height}],

            dir_data => $this->getDataDir(),
            dir_depth => $this->{dir_depth},

            tables => $dbs->getTables()
        };
    }
    elsif (defined $this->{data_pool}) {
        # On doit ajouter un niveau stockage ceph
        $levelParams = {
            id => $level,
            tm => $this->{tms}->getTileMatrix($level),
            size => [$this->{image_width}, $this->{image_height}],

            prefix => $this->{name},
            pool_name => $this->{data_pool},

            tables => $dbs->getTables()
        };
    }
    elsif (defined $this->{data_bucket}) {
        # On doit ajouter un niveau stockage s3
        $levelParams = {
            id => $level,
            tm => $this->{tms}->getTileMatrix($level),
            size => [$this->{image_width}, $this->{image_height}],

            prefix => $this->{name},
            bucket_name => $this->{data_bucket},

            tables => $dbs->getTables()
        };
    }
    elsif (defined $this->{data_container}) {
        # On doit ajouter un niveau stockage swift
        $levelParams = {
            id => $level,
            tm => $this->{tms}->getTileMatrix($level),
            size => [$this->{image_width}, $this->{image_height}],

            prefix => $this->{name},
            container_name => $this->{data_container},
            keystone_connection => $this->{keystone_connection},

            tables => $dbs->getTables()
        };
    }

    # Niveau ancêtre, potentiellement non défini, pour en reprendre les limites et les tables
    if (defined $ancestor) {
        my $ancestorLevel = $ancestor->getLevel($level);
        if (defined $ancestorLevel) {
            my ($rowMin,$rowMax,$colMin,$colMax) = $ancestorLevel->getLimits();
            $levelParams->{limits} = [$rowMin,$rowMax,$colMin,$colMax];
        }
    }

    $this->{levels}->{$level} = COMMON::LevelVector->new("VALUES", $levelParams, $this->{desc_path});

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


=begin nd
Function: updateStorageInfos
=cut
sub updateStorageInfos {
    my $this = shift;
    my $params = shift;

    $this->{name} = $params->{pyr_name_new};
    $this->{desc_path} = $params->{pyr_desc_path};
    $this->{content_path} = File::Spec->catfile($this->{desc_path}, $this->{name}.".list");

    my $updateLevelParams = {
        desc_path => $this->{desc_path}
    };

    if (defined $params->{pyr_data_path}) {
        $this->{storage_type} = "FILE";
        $this->{data_path} = File::Spec->rel2abs($params->{pyr_data_path});

        if ( exists $params->{dir_depth} && defined $params->{dir_depth} && COMMON::CheckUtils::isStrictPositiveInt($params->{dir_depth})) {
            $this->{dir_depth} = $params->{dir_depth};
        } else {
            $this->{dir_depth} = 2;
        }

        $this->{dir_depth} = $params->{dir_depth};

        $this->{data_bucket} = undef;
        $this->{data_container} = undef;
        $this->{data_pool} = undef;

        $updateLevelParams->{dir_depth} = $this->{dir_depth};
        $updateLevelParams->{dir_data} = $this->getDataDir();
    }
    elsif (defined $params->{pyr_data_pool_name}) {
        $this->{storage_type} = "CEPH";
        $this->{data_pool} = $params->{pyr_data_pool_name};

        $this->{data_path} = undef;
        $this->{dir_depth} = undef;
        $this->{data_bucket} = undef;
        $this->{data_container} = undef;

        $updateLevelParams->{prefix} = $this->{name};
        $updateLevelParams->{pool_name} = $this->{data_pool};
    }
    elsif (defined $params->{pyr_data_bucket_name}) {
        $this->{storage_type} = "S3";
        $this->{data_bucket} = $params->{pyr_data_bucket_name};

        $this->{data_path} = undef;
        $this->{dir_depth} = undef;
        $this->{data_container} = undef;
        $this->{data_pool} = undef;

        $updateLevelParams->{prefix} = $this->{name};
        $updateLevelParams->{bucket_name} = $this->{data_bucket};
    }
    elsif (defined $params->{pyr_data_container_name}) {
        $this->{storage_type} = "SWIFT";
        $this->{data_container} = $params->{pyr_data_container_name};

        if ( exists $params->{keystone_connection} && defined $params->{keystone_connection} && uc($params->{keystone_connection}) eq "TRUE" ) {
            $this->{keystone_connection} = TRUE;
        } else {
            $this->{keystone_connection} = FALSE;
        }

        $this->{data_path} = undef;
        $this->{dir_depth} = undef;
        $this->{data_bucket} = undef;
        $this->{data_pool} = undef;

        $updateLevelParams->{prefix} = $this->{name};
        $updateLevelParams->{container_name} = $this->{data_container};
        $updateLevelParams->{keystone_connection} = $this->{keystone_connection};
    }


    while (my ($id, $level) = each(%{$this->{levels}}) ) {
        if (! $level->updateStorageInfos($updateLevelParams)) {
            ERROR("Cannot update storage infos for the pyramid's level $id");
            return FALSE;
        }
    }
}

####################################################################################################
#                                      Group: Pyramids comparison                                  #
####################################################################################################

=begin nd
Function: checkCompatibility

We control values, in order to have the same as the final pyramid.

Compatibility = it's possible to convert (different compression or samples per pixel).

Equals = all format's parameters are the same (not the content).

Return 0 if pyramids is not consistent, 1 if compatibility but not equals, 2 if equals

Parameters (list):
    other - <COMMON::PyramidVector> - Pyramid to compare
=cut
sub checkCompatibility {
    my $this = shift;
    my $other = shift;

    if ($this->getStorageType() ne $other->getStorageType()) {
        return 0;
    }

    if ($this->getStorageType() eq "FILE") {
        if ($this->getDirDepth() != $other->getDirDepth()) {
            return 0;
        }
    }

    if ($this->getTilesPerWidth() != $other->getTilesPerWidth()) {
        return 0;
    }
    if ($this->getTilesPerHeight() != $other->getTilesPerHeight()) {
        return 0;
    }

    if ($this->getTileMatrixSet()->getName() ne $other->getTileMatrixSet()->getName()) {
        return 0;
    }

    if ($this->getFormatCode() ne $other->getFormatCode()) {
        return 0;
    }

    return 2;
}

####################################################################################################
#                                      Group: Write functions                                     #
####################################################################################################


=begin nd
Function: writeDescriptor

Back up it at the end
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

    if (! -d $this->{desc_path}) {
        eval { mkpath([$this->{desc_path}]); };
        if ($@) {
            ERROR(sprintf "Can not create the pyramid descriptor directory '%s' : %s !", $this->{desc_path} , $@);
            return FALSE;
        }
    }

    if (! open FILE, ">:encoding(UTF-8)", $descPath ){
        ERROR(sprintf "Cannot open the pyramid descriptor %s to write",$descPath);
        return FALSE;
    }

    my $string = "<?xml version='1.0' encoding='UTF-8'?>\n";
    $string .= "<Pyramid>\n";
    $string .= sprintf "    <tileMatrixSet>%s</tileMatrixSet>\n", $this->{tms}->getName();
    $string .= sprintf "    <format>%s</format>\n", $this->getFormatCode();


    my @orderedLevels = sort {$a->getOrder <=> $b->getOrder} ( values %{$this->{levels}});

    for (my $i = scalar @orderedLevels - 1; $i >= 0; $i--) {
        # we write levels in pyramid's descriptor from the top to the bottom
        $string .= $orderedLevels[$i]->exportToXML();
    }

    $string .= "</Pyramid>";

    print FILE $string;

    close(FILE);

    $this->backupDescriptor();

    return TRUE
}


=begin nd
Function: backupDescriptor

Pyramid's descriptor is stored into the object storage with data. Nothing done if FILE storage

This file have to be written before calling this function
=cut
sub backupDescriptor {
    my $this = shift;

    my $descFile = $this->getDescriptorFile();

    if ($this->{storage_type} eq "FILE") {
        INFO("On ne sauvegarde pas le descripteur de pyramide en mode fichier car des chemins sont en relatif et n'ont pas de sens si le fichier est ailleurs");
    } else {
        my $backupDescFile = sprintf "%s/%s.pyr", $this->getDataRoot(), $this->getName();
        COMMON::ProxyStorage::copy("FILE", $descFile, $this->{storage_type}, $backupDescFile);
    }
}


=begin nd
Function: backupList

Pyramid's list is stored into the data storage : in the data directory or in the object tray

This file have to be written before calling this function
=cut
sub backupList {
    my $this = shift;

    my $listFile = $this->getListFile();

    my $backupList;
    if ($this->{storage_type} eq "FILE") {
        $backupList = sprintf "%s/%s.list", $this->getDataDir(), $this->getName();
    } else {
        $backupList = sprintf "%s/%s.list", $this->getDataRoot(), $this->getName();
    }

    COMMON::ProxyStorage::copy("FILE", $listFile, $this->{storage_type}, $backupList);
}


####################################################################################################
#                                Group: Common getters                                             #
####################################################################################################

# Function: ownAncestor
sub ownAncestor {
    my $this = shift;
    return $this->{own_ancestor};
}

# Function: getFormatCode
sub getFormatCode {
    my $this = shift;
    return "TIFF_PBF_MVT";
}

# Function: keystoneConnection
sub keystoneConnection {
    my $this = shift;
    return $this->{keystone_connection};
}

# Function: getName
sub getName {
    my $this = shift;    
    return $this->{name};
}

# Function: getDescriptorFile
sub getDescriptorFile {
    my $this = shift;    
    return File::Spec->catfile($this->{desc_path}, $this->{name}.".pyr");
}

# Function: getDescriptorDir
sub getDescriptorDir {
    my $this = shift;    
    return $this->{desc_path};
}

# Function: getListFile
sub getListFile {
    my $this = shift;
    
    if (! defined $this->{content_path}) {
        $this->{content_path} = File::Spec->catfile($this->{desc_path}, $this->{name}.".list");
    }
    
    return $this->{content_path};
}


# Function: getTileMatrixSet
sub getTileMatrixSet {
    my $this = shift;
    return $this->{tms};
}


=begin nd
Function: getSlabPath

Returns the theoric slab path, undef if the level is not present in the pyramid

Parameters (list):
    type - string - IMAGE
    level - string - Level ID
    col - integer - Slab column
    row - integer - Slab row
    full - boolean - In file storage case, precise if we want full path or juste the end (without data root)
=cut
sub getSlabPath {
    my $this = shift;
    my $type = shift;
    my $level = shift;
    my $col = shift;
    my $row = shift;
    my $full = shift;

    if (! exists $this->{levels}->{$level}) {
        return undef;
    }

    return $this->{levels}->{$level}->getSlabPath($type, $col, $row, $full);
}

# Function: getTilesPerWidth
sub getTilesPerWidth {
    my $this = shift;
    return $this->{image_width};
}

# Function: getTilesPerHeight
sub getTilesPerHeight {
    my $this = shift;
    return $this->{image_height};
}

=begin nd
Function: getSlabSize

Returns the pyramid's image's pixel width and height as the double list (width, height), for a given level.

Parameters (list):
    level - string - Level ID
=cut
sub getSlabSize {
    my $this = shift;
    my $level = shift;
    return ($this->getSlabWidth($level), $this->getSlabHeight($level));
}

=begin nd
Function: getSlabWidth

Returns the pyramid's image's pixel width, for a given level.

Parameters (list):
    level - string - Level ID
=cut
sub getSlabWidth {
    my $this = shift;
    my $level = shift;

    return $this->{image_width} * $this->{tms}->getTileWidth($level);
}

=begin nd
Function: getSlabHeight

Returns the pyramid's image's pixel height, for a given level.

Parameters (list):
    level - string - Level ID
=cut
sub getSlabHeight {
    my $this = shift;
    my $level = shift;

    return $this->{image_height} * $this->{tms}->getTileHeight($level);
}

# Function: getLevel
sub getLevel {
    my $this = shift;
    my $level = shift;
    return $this->{levels}->{$level};
}

# Function: getBottomOrder
sub getBottomOrder {
    my $this = shift;

    my $order = undef;
    while (my ($levelID, $level) = each(%{$this->{levels}})) {
        my $o = $level->getOrder();
        if (! defined $order || $o < $order) {
            $order = $o;
        }
    }

    return $order;
}

# Function: getTopOrder
sub getTopOrder {
    my $this = shift;

    my $order = undef;
    while (my ($levelID, $level) = each(%{$this->{levels}})) {
        my $o = $level->getOrder();
        if (! defined $order || $o > $order) {
            $order = $o;
        }
    }

    return $order;
}

# Function: getLevels
sub getLevels {
    my $this = shift;
    return values %{$this->{levels}};
}

=begin nd
Function: hasLevel

Precises if the provided level exists in the pyramid.

Parameters (list):
    levelID - string - Identifiant of the asked level
=cut
sub hasLevel {
    my $this = shift;
    my $levelID = shift;

    if (defined $levelID && exists $this->{levels}->{$levelID}) {
        return TRUE;
    }

    return FALSE;
}


####################################################################################################
#                                Group: Storage getters                                            #
####################################################################################################

# Function: getStorageType
sub getStorageType {
    my $this = shift;
    return $this->{storage_type};
}

# Function: getDataRoot
sub getDataRoot {
    my $this = shift;

    if (defined $this->{data_path}) {
        return $this->{data_path};
    }
    elsif (defined $this->{data_pool}) {
        return $this->{data_pool};
    }
    elsif (defined $this->{data_bucket}) {
        return $this->{data_bucket};
    }
    elsif (defined $this->{data_container}) {
        return $this->{data_container};
    }
    return undef;
}

### FILE

# Function: getDataDir
sub getDataDir {
    my $this = shift;    
    return File::Spec->catfile($this->{data_path}, $this->{name});
}

# Function: getDirDepth
sub getDirDepth {
    my $this = shift;
    return $this->{dir_depth};
}

### S3

# Function: getDataBucket
sub getDataBucket {
    my $this = shift;    
    return $this->{data_bucket};
}

### SWIFT

# Function: getDataContainer
sub getDataContainer {
    my $this = shift;    
    return $this->{data_container};
}

### CEPH

# Function: getDataPool
sub getDataPool {
    my $this = shift;    
    return $this->{data_pool};
}

####################################################################################################
#                                     Group: List tools                                            #
####################################################################################################

=begin nd
Function: loadList

Read the list and store content in an hash as following :
|   level => {
|       col_row => full slab path (file or object)
|   }
=cut
sub loadList {
    my $this = shift;

    if ($this->{cachedListModified}) {
        ERROR("Cached list have been modified, we don't erase this modification loading the list from the file");
        return FALSE;
    }

    if ($this->{listCached}) {
        DEBUG("List have already been loaded");
        return TRUE;
    }

    my $listFile = $this->getListFile();

    if (! open LIST, "<", $listFile) {
        ERROR("Cannot open pyramid list file (to load content in cache) : $listFile");
        return FALSE;
    }

    # Dans le cas objet, pour passer du type présent dans le nom de l'objet au type générique
    my %objectTypeConverter = (
        IMG => "IMAGE"
    );

    # Lecture des racines
    my %roots;
    while( my $line = <LIST> ) {
        chomp $line;

        if ($line eq "#") {
            # separator between caches' roots and images
            last;
        }
        
        $line =~ s/\s+//g; # we remove all spaces
        my @tmp = split(/=/,$line,-1);
        
        if (scalar @tmp != 2) {
            ERROR(sprintf "Wrong formatted pyramid list (root definition) : %s",$line);
            return FALSE;
        }
        
        $roots{$tmp[0]} = $tmp[1];
    }

    while( my $line = <LIST> ) {
        chomp $line;

        # On reconstitue le chemin complet à l'aide des racines de l'index
        $line =~ m/^(\d+)\/.+/;
        my $index = $1;
        my $root = $roots{$index};
        my $fullline = $line;
        $fullline =~ s/^(\d+)/$root/;

        # On va vouloir déterminer le niveau, la colonne et la ligne de la dalle, ainsi que le type (IMAGE)
        # Cette extraction diffère selon que l'on est en mode fichier ou objet

        my ($type, $level, $col, $row);

        # Cas fichier
        if ($this->getStorageType() eq "FILE") {
            # Une ligne du fichier c'est
            # Cas fichier : 0/IMAGE/15/AB/CD/EF.tif
            my @parts = split("/", $line);
            # La première partie est toujours l'index de la racine, déjà traitée
            shift(@parts);
            # Dans le cas d'un stockage fichier, le premier élément du chemin est maintenant le type de donnée
            $type = shift(@parts);
            # et le suivant est le niveau
            $level = shift(@parts);

            ($col,$row) = $this->{levels}->{$level}->getFromSlabPath($line);

        }
        # Cas objet
        else {
            # Une ligne du fichier c'est
            # Cas objet : 0/PYRAMID_IMG_15_15656_5423

            # Dans le cas d'un stockage objet, on a un nom d'objet de la forme BLA/BLA_BLA_DATATYPE_LEVEL_COL_ROW
            # DATATYPE vaut MSK ou IMG, à convertir en IMAGE
            my @p = split("_",$line);
            $col = $p[-2];
            $row = $p[-1];
            $level = $p[-3];
            $type = $objectTypeConverter{$p[-4]};
        }
        
        if (exists $this->{cachedList}->{$level}->{$type}->{"${col}_${row}"}) {
            WARN("The list contains twice the same slab : $type, $level, $col, $row");
        }
        $this->{cachedList}->{$level}->{$type}->{"${col}_${row}"} = $fullline;
    }

    close(LIST);

    $this->{listCached} = TRUE;

    return TRUE;
}



=begin nd
Function: getLevelsSlabs

Returns the cached list content for all levels.
=cut
sub getLevelsSlabs {
    my $this = shift;

    return $this->{cachedList};
} 

=begin nd
Function: getLevelSlabs

Returns the cached list content for one level.

Parameters (list):
    level - string - Identifiant of the asked level
=cut
sub getLevelSlabs {
    my $this = shift;
    my $level = shift;

    return $this->{cachedList}->{$level};
} 


=begin nd
Function: containSlab

Precises if the provided slab belongs to the pyramid, using the cached list. Returns the full slab path if present, undef otherwise

Parameters (list):
    type - strong - IMAGE
    level - string - Identifiant of the asked level
    col - integer - Column indice
    row - integer - Row indice
=cut
sub containSlab {
    my $this = shift;
    my $type = shift;
    my $level = shift;
    my $col = shift;
    my $row = shift;

    return $this->{cachedList}->{$level}->{$type}->{"${col}_${row}"};
    # undef if not exists
} 


=begin nd
Function: modifySlab

Replace the full slab path with the local full path. This modification can be made persistent with <flushCachedList>.

Parameters (list):
    type - strong - IMAGE
    level - string - Identifiant of the asked level
    col - integer - Column indice
    row - integer - Row indice
=cut
sub modifySlab {
    my $this = shift;
    my $type = shift;
    my $level = shift;
    my $col = shift;
    my $row = shift;

    my $path = $this->getSlabPath($type, $level, $col, $row, TRUE);

    if (! $this->{listCached}) {
        ERROR("We cannot modified cached list beacuse the list have not been loaded");
        return FALSE;
    }

    $this->{cachedListModified} = TRUE;

    $this->{cachedList}->{$level}->{$type}->{"${col}_${row}"} = $path;

    return TRUE;
}


=begin nd
Function: deleteSlab

Delete the slab path from the cached list. This modification can be made persistent with <flushCachedList>.

Parameters (list):
    type - strong - IMAGE
    level - string - Identifiant of the asked level
    col - integer - Column indice
    row - integer - Row indice
=cut
sub deleteSlab {
    my $this = shift;
    my $type = shift;
    my $level = shift;
    my $col = shift;
    my $row = shift;

    delete $this->{cachedList}->{$level}->{$type}->{"${col}_${row}"};

    return TRUE;
}


=begin nd
Function: flushCachedList

Save cached list in the original file. If no modification, do nothing.
=cut
sub flushCachedList {
    my $this = shift;

    if (! $this->{listCached}) {
        ERROR("We cannot flush an unloaded list");
        return FALSE;
    }

    if (! $this->{cachedListModified}) {
        WARN("The cached list have not been modified, it's useless to flush it");
        return TRUE;
    }

    my $listFile = $this->getListFile();

    if (! open LIST, ">", $listFile) {
        ERROR("Cannot open pyramid list file (to flush cached content) : $listFile");
        return FALSE;
    }

    my %roots = ();

    if ($this->{storage_type} eq "FILE") {
        $roots{$this->getDataDir()} = 0;
    } else {
        $roots{$this->getDataRoot()} = 0;
    }

    foreach my $l (keys(%{$this->{cachedList}})) {
        while (my ($slabKey, $slabPath) = each(%{$this->{cachedList}->{$l}->{IMAGE}})) {

            my ($COL, $ROW) = split(/_/, $slabKey);

            # Cas fichier
            if ($this->{storage_type} eq "FILE") {

                my $pathEnd = $this->getSlabPath("IMAGE", $l, $COL, $ROW, FALSE);
                my $root = $slabPath;
                $root =~ s/\/$pathEnd$//;
                
                my $rootInd;

                if (! exists $roots{$root}) {
                    $rootInd = scalar(keys(%roots));
                    $roots{$root} = $rootInd;
                } else {
                    $rootInd = $roots{$root};
                }

                printf LIST "$rootInd/$pathEnd\n";
            }
            # Cas objet
            else {
                my $path = $slabPath;
                $path =~ s/^[^\/]+\///;
                printf LIST "0/$path\n";
            }
        }
    }

    close(LIST);

    # On va pouvoir écrire les racines maintenant
    my @LISTHDR;

    if (! tie @LISTHDR, 'Tie::File', $listFile) {
        ERROR("Cannot flush the header of the cache list : $listFile");
        return FALSE;
    }

    unshift @LISTHDR,"#\n";
    
    while ( my ($root,$rootInd) = each(%roots) ) {
        unshift @LISTHDR,(sprintf "%s=%s", $rootInd, $root);
    }
    
    untie @LISTHDR;

    $this->{cachedListModified} = FALSE;

    return TRUE;
} 

=begin nd
Function: getCachedListStats

Prints the memory size of the cached list hash.
=cut
sub getCachedListStats {
    my $this = shift;

    my $nb = scalar(keys %{$this->{cachedList}});
    my $size = total_size($this->{cachedList});

    my $ret = "Stats :\n\t $size bytes\n";
    # $ret .= "\t $size bytes\n";
    # $ret .= sprintf "\t %s bytes per cached slab\n", $size / $nb;

    return $ret;
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
    bless($clone, 'COMMON::PyramidVector');
    delete $clone->{levels};

    while (my ($id, $level) = each(%{$this->{levels}}) ) {
        $clone->{levels}->{$id} = $level->clone();
    }

    return $clone;
}


1;
__END__
