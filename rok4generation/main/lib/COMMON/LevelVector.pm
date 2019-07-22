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
File: LevelVector.pm

Class: COMMON::LevelVector

(see ROK4GENERATION/libperlauto/COMMON_LevelVector.png)

Describe a level in a vector pyramid.

Using:
    (start code)
    use COMMON::LevelVector;
    use COMMON::DatabaseSource;

    # From values
    my $valuesLevel = COMMON::LevelVector->new("VALUES",{
        id => "12",
        tm => $tms->getTileMatrix("12"),
        size => [16, 16],

        prefix => "TOTO_",
        bucket_name => "MyBucket",

        tables => $objDatabaseSource->getTables()
    });

    # From XML element
    my $xmlLevel = COMMON::LevelVector->new("XML", $xmlElement);
    $xmlLevel->bindTileMatrix($tms)

    (end code)

Attributes:
    id - string - Level identifiant.
    order - integer - Level order (ascending resolution)
    tm - <COMMON::TileMatrix> - Binding Tile Matrix to this level
    type - string - Storage type of data : FILE, S3 or CEPH

    size - integer array - Number of tile in one image for this level, widthwise and heightwise : [width, height].
    limits - integer array - Extrems columns and rows for the level (Extrems tiles which contains data) : [rowMin,rowMax,colMin,colMax]

    tables - string hash - Informations about tables included in the level.

    desc_path - string - Directory path of the pyramid's descriptor containing this level

    dir_depth - integer - Number of subdirectories from the level root to the image if FILE storage type : depth = 2 => /.../LevelID/SUB1/SUB2/IMG.tif
    dir_image - string - Directory in which we write the pyramid's images if FILE storage type

    prefix_image - string - Prefix used to name the image objects, in CEPH or S3 storage (contains the pyramid's name and the level's id)

    bucket_name - string - Name of the (existing) S3 bucket, where to store data if S3 storage type
    
    container_name - string - Name of the (existing) SWIFT container, where to store data if SWIFT storage type
    keystone_connection - boolean - For swift storage, keystone authentication or not ?

    pool_name - string - Name of the (existing) CEPH pool, where to store data if CEPH storage type
=cut

################################################################################

package COMMON::LevelVector;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use XML::LibXML;
use File::Spec;
use Cwd;
use Data::Dumper;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

use COMMON::TileMatrix;
use COMMON::TileMatrixSet;
use COMMON::Base36;

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

Level constructor. Bless an instance.

Parameters (hash):
    type - string - XML
    params - <XML::LibXML::Element> - XML node of the level (from the pyramid's descriptor)
        or
    type - string - VALUES
    params - string hash - Hash containg all needed level informations

    descDirectory - string - Directory path of the pyramid's descriptor containing this level

See also:
    <_loadXML>, <_loadValues>
=cut
sub new {
    my $class = shift;
    my $type = shift;
    my $params = shift;
    my $descDirectory = shift;
    
    $class= ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        id => undef,
        order => undef,
        tm => undef,
        type => undef,

        size => [],
        limits => undef, # rowMin,rowMax,colMin,colMax

        tables => undef,

        # ABOUT PYRAMID
        desc_path => undef,

        # CAS FICHIER
        dir_depth => undef,
        dir_image => undef,

        # CAS OBJET
        prefix_image => undef,
        #    - S3
        bucket_name => undef,
        #    - SWIFT
        container_name => undef,
        keystone_connection => FALSE,
        #    - CEPH
        pool_name => undef
    };
    
    bless($this, $class);

    if (! defined $descDirectory) {
        ERROR ("The 'desc_path' is required to create a level");
        return undef;
    }
    $this->{desc_path} = $descDirectory;

    if ($type eq "XML") {
        if ( ! $this->_loadXML($params) ) {
            ERROR("Cannot load Level XML");
            return undef;
        }
    } else {
        if (! $this->_loadValues($params)) {
            ERROR ("One parameter is missing !");
            return undef;
        }
    }
    
    # STOCKAGE TYPE AND ENVIRONMENT VARAIBLES CONTROLS
    if ( defined $this->{dir_depth} ) {
        $this->{type} = "FILE";
    }
    elsif ( defined $this->{bucket_name} ) {
        $this->{type} = "S3";

        if (! COMMON::ProxyStorage::checkEnvironmentVariables("S3")) {
            ERROR("Environment variable is missing for a S3 storage");
            return FALSE;
        }
    }
    elsif ( defined $this->{pool_name} ) {
        $this->{type} = "CEPH";

        if (! COMMON::ProxyStorage::checkEnvironmentVariables("CEPH")) {
            ERROR("Environment variable is missing for a CEPH storage");
            return FALSE;
        }
    }
    elsif ( defined $this->{container_name} ) {
        $this->{type} = "SWIFT";

        if (! COMMON::ProxyStorage::checkEnvironmentVariables("SWIFT")) {
            ERROR("Environment variable is missing for a SWIFT storage");
            return FALSE;
        }
    } else {
        ERROR ("Cannot identify the storage type for the COMMON::LevelVector");
        return undef;
    }

    return $this;
}

=begin nd
Function: _initValues

Check and store level's attributes values.

Parameter:
    params - hash - Hash containg all needed level informations
=cut
sub _loadValues {
    my $this   = shift;
    my $params = shift;
  
    return FALSE if (! defined $params);
    
    # PARTIE COMMUNE
    if (! exists($params->{id})) {
        ERROR ("The parameter 'id' is required");
        return FALSE;
    }
    $this->{id} = $params->{id};

    if (! exists($params->{tm})) {
        ERROR ("The parameter 'tm' is required");
        return FALSE;
    }
    if (ref ($params->{tm}) ne "COMMON::TileMatrix") {
        ERROR ("The parameter 'tm' is not a COMMON:TileMatrix");
        return FALSE;
    }
    $this->{tm} = $params->{tm};
    $this->{order} = $params->{tm}->getOrder();

    if (! exists($params->{size})) {
        ERROR ("The parameter 'size' is required");
        return FALSE;
    }

    if (! exists($params->{tables})) {
        ERROR ("The parameter 'tables' is required");
        return FALSE;
    }
    $this->{tables} = $params->{tables};

    # check values
    if (! scalar ($params->{size})){
        ERROR("List empty to 'size' !");
        return FALSE;
    }
    $this->{size} = $params->{size};
    
    if (! exists($params->{limits}) || ! defined($params->{limits})) {
        $params->{limits} = [undef, undef, undef, undef];
    }
    $this->{limits} = $params->{limits};


    # STOCKAGE    
    if ( exists $params->{dir_depth} ) {
        # CAS FICHIER
        if (! exists($params->{dir_depth})) {
            ERROR ("The parameter 'dir_depth' is required");
            return FALSE;
        }
        if (! $params->{dir_depth}){
            ERROR("Value not valid for 'dir_depth' (0 or undef) !");
            return FALSE;
        }
        $this->{dir_depth} = $params->{dir_depth};

        if (! exists($params->{dir_data})) {
            ERROR ("The parameter 'dir_data' is required");
            return FALSE;
        }


        $this->{dir_image} = File::Spec->catdir($params->{dir_data}, "IMAGE", $this->{id});

    }
    elsif ( exists $params->{prefix} ) {
        # CAS OBJET
        $this->{prefix_image} = sprintf "%s_IMG_%s", $params->{prefix}, $this->{id};

        if ( exists $params->{bucket_name} ) {
            # CAS S3
            $this->{bucket_name} = $params->{bucket_name};
        }
        elsif ( exists $params->{pool_name} ) {
            # CAS CEPH
            $this->{pool_name} = $params->{pool_name};
        }
        elsif ( exists $params->{container_name} ) {
            # CAS SWIFT
            $this->{container_name} = $params->{container_name};
            
            if ( exists $params->{keystone_connection} && defined $params->{keystone_connection} && $params->{keystone_connection}) {
                $this->{keystone_connection} = TRUE;                
            }
        }
        else {
            ERROR("No container name (bucket or pool or container) for object storage for the level");
            return FALSE;        
        }
    }
    else {
        ERROR("No storage (neither file nor object) for the level");
        return FALSE;        
    }

    return TRUE;
}

=begin nd
Function: _loadXML

Extract level's information from the XML element

Parameter:
    levelRoot - <XML::LibXML::Element> - XML node of the level (from the pyramid's descriptor)
=cut
sub _loadXML {
    my $this   = shift;
    my $levelRoot = shift;

    $this->{id} = $levelRoot->findvalue('tileMatrix');
    if (! defined $this->{id} || $this->{id} eq "" ) {
        ERROR ("Cannot extract 'tileMatrix' from the XML level");
        return FALSE;
    }

    $this->{size} = [ $levelRoot->findvalue('tilesPerWidth'), $levelRoot->findvalue('tilesPerHeight') ];
    if (! defined $this->{size}->[0] || $this->{size}->[0] eq "" ) {
        ERROR ("Cannot extract image's tilesize from the XML level");
        return FALSE;
    }

    $this->{limits} = [
        $levelRoot->findvalue('TMSLimits/minTileRow'),
        $levelRoot->findvalue('TMSLimits/maxTileRow'),
        $levelRoot->findvalue('TMSLimits/minTileCol'),
        $levelRoot->findvalue('TMSLimits/maxTileCol')
    ];
    if (! defined $this->{limits}->[0] || $this->{limits}->[0] eq "") {
        ERROR ("Cannot extract extrem tiles from the XML level");
        return FALSE;
    }

    my @tables = $levelRoot->getElementsByTagName('table');
    foreach my $t (@tables) {

        my $tablename = $t->findvalue('name');
        my $geometry = $t->findvalue('geometry');

        $this->{tables}->{$tablename} = {
            geometry => $geometry,
            attributes => {}
        };

        my @atts = $t->getElementsByTagName('attribute');
        foreach my $a (@atts) {
            my $attname = $a->findvalue('name');
            my $atttype = $a->findvalue('type');
            my $min = $a->findvalue('min');
            my $max = $a->findvalue('max');
            my $count = $a->findvalue('count');
            my $values = $a->findvalue('values');

            $this->{tables}->{$tablename}->{attributes}->{$attname} = {
                type => $atttype,
                count => $count
            };

            if (defined $min && $min ne "") {
                $this->{tables}->{$tablename}->{attributes}->{$attname}->{min} = $min;
            }

            if (defined $max && $max ne "") {
                $this->{tables}->{$tablename}->{attributes}->{$attname}->{max} = $max;
            }

            if (defined $values && $values ne "") {
                $values =~ s/^"//g;
                $values =~ s/"$//g;
                $this->{tables}->{$tablename}->{attributes}->{$attname}->{values} = split(/","/, $values);
            }
        }
    }

    # CAS FICHIER
    my $dirimg = $levelRoot->findvalue('baseDir');
    my $imgprefix = $levelRoot->findvalue('imagePrefix');
    
    if (defined $dirimg && $dirimg ne "" ) {
        $this->{dir_image} = File::Spec->rel2abs(File::Spec->rel2abs( $dirimg , $this->{desc_path} ) );

        $this->{dir_depth} = $levelRoot->findvalue('pathDepth');
        if (! defined $this->{dir_depth} || $this->{dir_depth} eq "" ) {
            ERROR ("Cannot extract 'pathDepth' from the XML level");
            return FALSE;
        }
    }
    elsif (defined $imgprefix && $imgprefix ne "" ) {
        $this->{prefix_image} = $imgprefix;

        my $pool = $levelRoot->findvalue('cephContext/poolName');
        my $bucket = $levelRoot->findvalue('s3Context/bucketName');
        my $container = $levelRoot->findvalue('swiftContext/containerName');

        if ( defined $bucket && $bucket ne "" ) {
            # CAS S3
            $this->{bucket_name} = $bucket;
        }
        elsif ( defined $pool && $pool ne "" ) {
            # CAS CEPH
            $this->{pool_name} = $pool;
        }
        elsif ( defined $container && $container ne "" ) {
            # CAS SWIFT
            $this->{container_name} = $container;
            my $ks = $levelRoot->findvalue('swiftContext/keystoneConnection');
            if ( defined $ks && uc($ks) eq "TRUE" ) {
                $this->{keystone_connection} = TRUE;
            }
        }
        else {
            ERROR("No container name (bucket or pool) for object storage for the level");
            return FALSE;        
        }
    }

    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################


# Function: getID
sub getID {
    my $this = shift;
    return $this->{id};
}

# Function: getDirDepth
sub getDirDepth {
    my $this = shift;
    return $this->{dir_depth};
}

# Function: getDirImage
sub getDirImage {
    my $this = shift;
    return $this->{dir_image};
}

# Function: getDirsInfo
sub getDirsInfo {
    my $this = shift;

    if ($this->{type} ne "FILE") {
        return (undef, undef);
    }

    my @dirs = File::Spec->splitdir($this->{dir_image});
    pop(@dirs);pop(@dirs);pop(@dirs);
    my $dir_data = File::Spec->catdir(@dirs);

    return ($this->{dir_depth}, $dir_data);
}
# Function: getS3Info
sub getS3Info {
    my $this = shift;

    if ($this->{type} ne "S3") {
        return undef;
    }

    return $this->{bucket_name};
}
# Function: getSwiftInfo
sub getSwiftInfo {
    my $this = shift;

    if ($this->{type} ne "SWIFT") {
        return undef;
    }

    return ($this->{container_name}, $this->{keystone_connection});
}
# Function: getCephInfo
sub getCephInfo {
    my $this = shift;

    if ($this->{type} ne "CEPH") {
        return undef;
    }

    return $this->{pool_name};
}

# Function: getStorageType
sub getStorageType {
    my $this = shift;
    return $this->{type};
}

# Function: getImageWidth
sub getImageWidth {
    my $this = shift;
    return $this->{size}->[0];
}

# Function: getImageHeight
sub getImageHeight {
    my $this = shift;
    return $this->{size}->[1];
}

=begin nd
Function: getLimits

Return extrem tiles as an array: rowMin,rowMax,colMin,colMax
=cut
sub getLimits {
    my $this = shift;
    return ($this->{limits}->[0], $this->{limits}->[1], $this->{limits}->[2], $this->{limits}->[3]);
}

# Function: getOrder
sub getOrder {
    my $this = shift;
    return $this->{order};
}

# Function: getTables
sub getTables {
    my $this = shift;
    return $this->{tables};
}

# Function: getTileMatrix
sub getTileMatrix {
    my $this = shift;
    return $this->{tm};
}

=begin nd
Function: bboxToSlabIndices

Returns the extrem slab's indices from a bbox in a list : ($rowMin, $rowMax, $colMin, $colMax).

Parameters (list):
    xMin,yMin,xMax,yMax - bounding box
=cut
sub bboxToSlabIndices {
    my $this = shift;
    my @bbox = @_;

    return $this->{tm}->bboxToIndices(@bbox, $this->{size}->[0], $this->{size}->[1]);
}

=begin nd
Function: slabIndicesToBbox

Returns the bounding box from the slab's column and row.

Parameters (list):
    col,row - Slab's column and row
=cut
sub slabIndicesToBbox {
    my $this = shift;
    my $col = shift;
    my $row = shift;

    return $this->{tm}->indicesToBbox($col, $row, $this->{size}->[0], $this->{size}->[1]);
}

=begin nd
Function: getSlabPath

Returns the theoric slab path (file path or object name)

Parameters (list):
    type - string - "IMAGE"
    col - integer - Slab column
    row - integer - Slab row
    full - boolean - In file storage case, precise if we want full path or juste the end (without data root). In object storage case, precise if we want full path (with the container name) or just the object name.
=cut
sub getSlabPath {
    my $this = shift;
    my $type = shift;
    my $col = shift;
    my $row = shift;
    my $full = shift;

    if ($this->{type} eq "FILE") {
        my $b36 = COMMON::Base36::indicesToB36Path($col, $row, $this->{dir_depth} + 1);

        if ($type eq "IMAGE") {
            if (defined $full && ! $full) {
                return File::Spec->catdir("IMAGE", $this->{id}, "$b36.tif");
            }
            return File::Spec->catdir($this->{dir_image}, "$b36.tif");
        }
        else {
            return undef;
        }
    }
    elsif ($this->{type} eq "S3") {
        if ($type eq "IMAGE") {
            if (defined $full && ! $full) {
                return sprintf "%s_%s_%s", $this->{prefix_image}, $col, $row;
            }
            return sprintf "%s/%s_%s_%s", $this->{bucket_name}, $this->{prefix_image}, $col, $row;
        }
        else {
            return undef;
        }
    }    
    elsif ($this->{type} eq "SWIFT") {
        if ($type eq "IMAGE") {
            if (defined $full && ! $full) {
                return sprintf "%s_%s_%s", $this->{prefix_image}, $col, $row;
            }
            return sprintf "%s/%s_%s_%s", $this->{container_name}, $this->{prefix_image}, $col, $row;
        }
        else {
            return undef;
        }
    }
    elsif ($this->{type} eq "CEPH") {
        if ($type eq "IMAGE") {
            if (defined $full && ! $full) {
                return sprintf "%s_%s_%s", $this->{prefix_image}, $col, $row;
            }
            return sprintf "%s/%s_%s_%s", $this->{pool_name}, $this->{prefix_image}, $col, $row;
        }
        else {
            return undef;
        }
    } else {
        return undef;
    }

}

=begin nd
Function: getFromSlabPath

Extract column and row from a slab path

Parameter (list):
    path - string - Path to decode, to obtain slab's column and row

Returns:
    Integers' list, (col, row)
=cut
sub getFromSlabPath {
    my $this = shift;
    my $path = shift;

    if ($this->{type} eq "FILE") {
        # 1 on ne garde que la partie finale propre à l'indexation de la dalle
        my @parts = split("/", $path);

        my @finalParts;
        for (my $i = -1 - $this->{dir_depth}; $i < 0; $i++) {
            push(@finalParts, $parts[$i]);
        }
        # 2 on enlève l'extension
        $path = join("/", @finalParts);
        $path =~ s/(\.tif|\.tiff|\.TIF|\.TIFF)//;
        return COMMON::Base36::b36PathToIndices($path);

    } else {
        my @parts = split("_", $path);
        return ($parts[-2], $parts[-1]);
    }

}


=begin nd
Function: updateStorageInfos
=cut
sub updateStorageInfos {
    my $this = shift;
    my $params = shift;

    $this->{desc_path} = $params->{desc_path};

    if (exists $params->{dir_depth}) {
        $this->{type} = "FILE";
        $this->{dir_depth} = $params->{dir_depth};

        $this->{dir_image} = File::Spec->catdir($params->{dir_data}, "IMAGE", $this->{id});

        $this->{prefix_image} = undef;
        $this->{bucket_name} = undef;
        $this->{container_name} = undef;
        $this->{keystone_connection} = FALSE;
        $this->{pool_name} = undef;
    } elsif ( exists $params->{pool_name} ) {
        $this->{type} = "CEPH";
        $this->{pool_name} = $params->{pool_name};
        $this->{prefix_image} = sprintf "%s_IMG_%s", $params->{prefix}, $this->{id};
        $this->{dir_depth} = undef;
        $this->{dir_image} = undef;
        $this->{bucket_name} = undef;
        $this->{container_name} = undef;
        $this->{keystone_connection} = FALSE;
    } elsif ( exists $params->{bucket_name} ) {
        $this->{type} = "S3";
        $this->{bucket_name} = $params->{bucket_name};
        $this->{prefix_image} = sprintf "%s_IMG_%s", $params->{prefix}, $this->{id};
        $this->{dir_depth} = undef;
        $this->{dir_image} = undef;
        $this->{container_name} = undef;
        $this->{keystone_connection} = FALSE;
        $this->{pool_name} = undef;
    } elsif ( exists $params->{container_name} ) {
        $this->{type} = "SWIFT";
        $this->{container_name} = $params->{container_name};
        $this->{prefix_image} = sprintf "%s_IMG_%s", $params->{prefix}, $this->{id};
        if ( exists $params->{keystone_connection} && defined $params->{keystone_connection} && $params->{keystone_connection}) {
            $this->{keystone_connection} = TRUE;
        } else {
            $this->{keystone_connection} = FALSE;
        }
        $this->{dir_depth} = undef;
        $this->{dir_image} = undef;
        $this->{bucket_name} = undef;
        $this->{pool_name} = undef;
    }

    return TRUE;
}

=begin nd
method: updateLimits

Compare old extrems rows/columns with the news and update values.

Parameters (list):
    rowMin, rowMax, colMin, colMax - integer list - Tiles indices to compare with current extrems
=cut
sub updateLimits {
    my $this = shift;
    my ($rowMin,$rowMax,$colMin,$colMax) = @_;

    if (! defined $this->{limits}->[0] || $rowMin < $this->{limits}->[0]) {$this->{limits}->[0] = $rowMin;}
    if (! defined $this->{limits}->[1] || $rowMax > $this->{limits}->[1]) {$this->{limits}->[1] = $rowMax;}
    if (! defined $this->{limits}->[2] || $colMin < $this->{limits}->[2]) {$this->{limits}->[2] = $colMin;}
    if (! defined $this->{limits}->[3] || $colMax > $this->{limits}->[3]) {$this->{limits}->[3] = $colMax;}
}

=begin nd
method: updateLimitsFromSlab
=cut
sub updateLimitsFromSlab {
    my $this = shift;
    my ($col,$row) = @_;

    $this->updateLimits(
        $row * $this->{size}->[1], $row * $this->{size}->[1] + ($this->{size}->[1] - 1),
        $col * $this->{size}->[0], $col * $this->{size}->[0] + ($this->{size}->[0] - 1)
    );
}


=begin nd
method: updateLimitsFromBbox
=cut
sub updateLimitsFromBbox {
    my $this = shift;
    my ($xmin, $ymin, $xmax, $ymax) = @_;

    my $colMin = $this->{tm}->xToColumn($xmin);
    my $colMax = $this->{tm}->xToColumn($xmax);
    my $rowMin = $this->{tm}->yToRow($ymax);
    my $rowMax = $this->{tm}->yToRow($ymin);

    $this->updateLimits($rowMin,$rowMax,$colMin,$colMax);
}

=begin nd
method: bindTileMatrix

For levels loaded from an XML element, we have to link the Tile Matrix (we only have the level ID).

We control if level exists in the provided TMS, and we calculate the level's order in this TMS.

Parameter:
    tms - <COMMON::TileMatrixSet> - TMS containg the Tile Matrix to link to this level.
=cut
sub bindTileMatrix {
    my $this = shift;
    my $tms = shift;

    if (defined $this->{tm} ) {
        # le tm est déjà présent pour le niveau
        return TRUE;
    }

    $this->{tm} = $tms->getTileMatrix($this->{id});
    if (! defined $this->{tm}) {
        ERROR(sprintf "Cannot find a level with the id %s in the TMS", $this->{id});
        return FALSE;
    }

    $this->{order} = $tms->getOrderfromID($this->{id});

    return TRUE;
}

####################################################################################################
#                                  Group: BBOX tools                                               #
####################################################################################################

=begin nd
Function: intersectBboxIndices

Intersects provided indices bbox with the extrem tiles of this source level. Provided list is directly modified.

Parameters (list):
    bbox - list reference - Bounding box to intersect with the level's limits : (colMin,rowMin,colMax,rowMax).
=cut
sub intersectBboxIndices {
    my $this = shift;
    my $bbox = shift;

    $bbox->[0] = max($bbox->[0], $this->{limits}[2]);
    $bbox->[1] = max($bbox->[1], $this->{limits}[0]);
    $bbox->[2] = min($bbox->[2], $this->{limits}[3]);
    $bbox->[3] = min($bbox->[3], $this->{limits}[1]);
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
method: exportToXML

Export Level's attributes in XML format.

Example:
    (start code)
    <level>
        <tileMatrix>level_5</tileMatrix>

        <baseDir>./BDORTHO/IMAGE/level_5/</baseDir>
        <pathDepth>2</pathDepth>
        <table>
            <name>batiment</name>
            <attribute>
                <name>hauteur</name>
                <type>integer</type>
            </attribute>
            <attribute>
                <name>type</name>
                <type>varchar</type>
            </attribute>
            <attribute>
                <name>surface</name>
                <type>double</type>
            </attribute>
        </table>
        <tilesPerWidth>16</tilesPerWidth>
        <tilesPerHeight>16</tilesPerHeight>
        <TMSLimits>
            <minTileRow>365</minTileRow>
            <maxTileRow>368</maxTileRow>
            <minTileCol>1026</minTileCol>
            <maxTileCol>1035</maxTileCol>
        </TMSLimits>
    </level>
    (end code)

Parameter:
    tiles_storage - boolean - If tiles are stored individually, we put 0 for tilesPerWidth and tilesPerHeight
=cut
sub exportToXML {
    my $this = shift;

    my $string =                   "    <level>\n";
    $string .= sprintf             "        <tileMatrix>%s</tileMatrix>\n", $this->{id};

    $string .= sprintf             "        <tilesPerWidth>%s</tilesPerWidth>\n", $this->{size}->[0];
    $string .= sprintf             "        <tilesPerHeight>%s</tilesPerHeight>\n", $this->{size}->[1];

    $string .=                     "        <TMSLimits>\n";

    if (defined $this->{limits}->[0]) {
        $string .= sprintf         "            <minTileRow>%s</minTileRow>\n", $this->{limits}->[0];
        $string .= sprintf         "            <maxTileRow>%s</maxTileRow>\n", $this->{limits}->[1];
        $string .= sprintf         "            <minTileCol>%s</minTileCol>\n", $this->{limits}->[2];
        $string .= sprintf         "            <maxTileCol>%s</maxTileCol>\n", $this->{limits}->[3];
    } else {
        $string .=                 "            <minTileRow>0</minTileRow>\n";
        $string .=                 "            <maxTileRow>0</maxTileRow>\n";
        $string .=                 "            <minTileCol>0</minTileCol>\n";
        $string .=                 "            <maxTileCol>0</maxTileCol>\n";
    }
    $string .=                     "        </TMSLimits>\n";

    if ($this->{type} eq "FILE") {
        $string .= sprintf         "        <baseDir>%s</baseDir>\n", File::Spec->abs2rel($this->{dir_image}, $this->{desc_path});
        $string .= sprintf         "        <pathDepth>%s</pathDepth>\n", $this->{dir_depth};
    }
    elsif ($this->{type} eq "S3") {
        $string .= sprintf         "        <imagePrefix>%s</imagePrefix>\n", $this->{prefix_image};
        $string .=                 "        <s3Context>\n";
        $string .= sprintf         "            <bucketName>%s</bucketName>\n", $this->{bucket_name};
        $string .=                 "        </s3Context>\n";
    }
    elsif ($this->{type} eq "SWIFT") {
        $string .= sprintf         "        <imagePrefix>%s</imagePrefix>\n", $this->{prefix_image};
        $string .=                 "        <swiftContext>\n";
        $string .= sprintf         "            <containerName>%s</containerName>\n", $this->{container_name};

        if ($this->{keystone_connection}) {
            $string .=             "            <keystoneConnection>TRUE</keystoneConnection>\n";
        }

        $string .=                 "        </swiftContext>\n";
    }
    elsif ($this->{type} eq "CEPH") {
        $string .= sprintf         "        <imagePrefix>%s</imagePrefix>\n", $this->{prefix_image};
        $string .=                 "        <cephContext>\n";
        $string .= sprintf         "            <poolName>%s</poolName>\n", $this->{pool_name};
        $string .=                 "        </cephContext>\n";
    }

    foreach my $table (keys(%{$this->{tables}})) {
        $string .=                 "        <table>\n";
        $string .= sprintf         "            <name>%s</name>\n", $this->{tables}->{$table}->{final_name};
        $string .= sprintf         "            <geometry>%s</geometry>\n", $this->{tables}->{$table}->{geometry}->{type};
        while (my ($att, $hash) = each(%{$this->{tables}->{$table}->{attributes_analysis}})) {
            $string .=             "            <attribute>\n";
            $string .=             "                <name>$att</name>\n";
            $string .= sprintf     "                <type>%s</type>\n", $hash->{type};
            $string .= sprintf     "                <count>%s</count>\n", $hash->{count};

            if (exists $hash->{min}) {
                $string .= sprintf "                <min>%s</min>\n", $hash->{min};
                $string .= sprintf "                <max>%s</max>\n", $hash->{max};
            }

            if (exists $hash->{values} && scalar @{$hash->{values}} != 0) {
                $string .= sprintf "                <values>\"%s\"</values>\n", join("\",\"", @{$hash->{values}});
            }

            $string .=             "            </attribute>\n";
        }
        $string .=                 "        </table>\n";
    }


    $string .=                     "    </level>\n";

    return $string;
}


####################################################################################################
#                                   Group: Clone function                                          #
####################################################################################################

=begin nd
Function: clone

Clone object.
=cut
sub clone {
    my $this = shift;

    my $clone = { %{ $this } };
    bless($clone, 'COMMON::LevelVector');

    return $clone;
}

1;
__END__
