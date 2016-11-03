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
File: Level.pm

Class: COMMON::Level

Describe a level in a pyramid.

Using:
    (start code)
    use COMMON::Level;

    my $params = {
        id                => level_5,
        order             => 12,
        dir_image         => "/home/ign/BDORTHO/IMAGE/level_5/",
        dir_mask          => "/home/ign/BDORTHO/MASK/level_5/",
        dir_metadata      => undef,
        compress_metadata => undef,
        type_metadata     => undef,
        size              => [16, 16],
        dir_depth         => 2,
        limits            => [365,368,1026,1035]
    };

    my $objLevel = COMMON::Level->new($params);
    (end code)

Attributes:
    id - string - Level identifiant.
    order - integer - Level order (ascending resolution)
    dir_image - string - Absolute images' directory path for this level.
    dir_mask - string - Absolute mask' directory path for this level.
    dir_metadata - NOT IMPLEMENTED
    compress_metadata - NOT IMPLEMENTED
    type_metadata - NOT IMPLEMENTED
    size - integer array - Number of tile in one image for this level, widthwise and heightwise : [width, height].
    dir_depth - integer - Number of subdirectories from the level root to the image : depth = 2 => /.../LevelID/SUB1/SUB2/IMG.tif, in the images' pyramid.
    limits - integer array - Extrems columns and rows for the level (Extrems tiles which contains data) : [rowMin,rowMax,colMin,colMax]

Limitations:

Metadata not implemented.
=cut

################################################################################

package COMMON::Level;

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
    id - string - Level identifiant
    size - integer array - Number of tile in one image for this level
    limits - integer array - Optionnal. Current level's limits. Set to [undef,undef,undef,undef] if not defined.
    dir_depth - integer - Number of subdirectories from the level root to the image
    dir_image - string - Absolute images' directory path for this level.
    dir_mask - string - Optionnal (if we want to keep mask in the final images' pyramid). Absolute mask' directory path for this level.

See also:
    <_init>
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

        # ABOUT PYRAMID
        desc_path => undef,

        # CAS FICHIER
        dir_depth => undef,
        dir_image => undef,
        dir_mask => undef,

        # CAS OBJET
        prefix_image => undef,
        prefix_mask => undef,
        #    - S3
        bucket_name => undef,
        #    - CEPH
        pool_name => undef,
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
    
    # STOCKAGE TYPE
    if ( defined $this->{dir_depth} ) {
        $this->{type} = "FILE";
    }
    elsif ( defined $this->{bucket_name} ) {
        $this->{type} = "S3";
    }
    elsif ( defined $this->{pool_name} ) {
        $this->{type} = "CEPH";
    }

    return $this;
}

=begin nd
Function: _initValues

Check and store level's attributes values.

Parameters (hash):
    id - string - Level identifiant
    size - integer array - Number of tile in one image for this level
    limits - integer array - Optionnal. Current level's limits. Set to [undef,undef,undef,undef] if not defined.
    dir_depth - integer - Number of subdirectories from the level root to the image
    dir_image - string - Absolute images' directory path for this level.
    dir_mask - string - Optionnal (if we want to keep mask in the final images' pyramid). Absolute mask' directory path for this level.
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
        if (! exists($params->{dir_image})) {
            ERROR ("The parameter 'dir_image' is required");
            return FALSE;
        }
        $this->{dir_image} = $params->{dir_image};

        if (exists $params->{dir_mask} && defined $params->{dir_mask}) {
            $this->{dir_mask} = $params->{dir_mask};
        }
    }
    elsif ( exists $params->{prefix} ) {
        # CAS OBJET
        $this->{prefix_image} = sprintf "%s_IMG_%s", $params->{prefix}, $this->{id};

        if (exists $params->{hasMask} && defined $params->{hasMask} && $params->{hasMask} ) {
            $this->{prefix_mask} = sprintf "%s_MSK_%s", $params->{prefix}, $this->{id};
        }

        if ( exists $params->{bucket_name} ) {
            # CAS S3
            $this->{bucket_name} = $params->{bucket_name};
        }
        elsif ( exists $params->{pool_name} ) {
            # CAS CEPH
            $this->{pool_name} = $params->{pool_name};
        }
        else {
            ERROR("No container name (bucket or pool) for object storage for the level");
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
Function: _initXML

Check and store level's attributes values.

Parameters (hash):
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

    # CAS FICHIER
    my $dirimg = $levelRoot->findvalue('baseDir');
    my $imgprefix = $levelRoot->findvalue('imagePrefix');
    
    if (defined $dirimg && $dirimg ne "" ) {
        $this->{dir_image} = Cwd::realpath(File::Spec->rel2abs( $dirimg , $this->{desc_path} ) );
        
        my $dirmsk = $levelRoot->findvalue('mask/baseDir');
        if (defined $dirmsk && $dirmsk ne "" ) {
            $this->{dir_mask} = Cwd::realpath(File::Spec->rel2abs( $dirimg , $this->{desc_path} ) );
        }

        $this->{dir_depth} = $levelRoot->findvalue('pathDepth');
        if (! defined $this->{dir_depth} || $this->{dir_depth} eq "" ) {
            ERROR ("Cannot extract 'pathDepth' from the XML level");
            return FALSE;
        }
    }
    elsif (defined $imgprefix && $imgprefix ne "" ) {
        $this->{prefix_image} = $imgprefix;

        my $mskprefix = $levelRoot->findvalue('mask/maskPrefix');
        if (defined $mskprefix && $mskprefix ne "" ) {
            $this->{prefix_mask} = $mskprefix;
        }

        my $pool = $levelRoot->findvalue('cephContext/poolName');
        my $bucket = $levelRoot->findvalue('s3Context/bucketName');

        if ( defined $bucket && $bucket ne "" ) {
            # CAS S3
            $this->{bucket_name} = $bucket;
        }
        elsif ( defined $pool && $pool ne "" ) {
            # CAS CEPH
            $this->{pool_name} = $pool;
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
        return (undef, undef, undef, undef);
    }

    my @dirs = File::Spec->splitdir($this->{dir_image});
    my $dir_image_name = $dirs[-2];
    my $dir_data = File::Spec->catdir(@dirs[0..-4]);

    my $dir_mask_name = undef;
    if (defined $this->{dir_mask}) {
        @dirs = File::Spec->splitdir($this->{dir_mask});
        $dir_mask_name = $dirs[-2];
    }

    return ($this->{dir_depth}, $dir_data, $dir_image_name, $dir_mask_name);
}
# Function: getDirsInfo
sub getS3Info {
    my $this = shift;

    if ($this->{type} ne "S3") {
        return (undef, undef, undef);
    }

    return ($this->{bucket_name}, $this->{prefix_image}, $this->{prefix_mask});
}
# Function: getDirsInfo
sub getCephInfo {
    my $this = shift;

    if ($this->{type} ne "CEPH") {
        return (undef, undef, undef);
    }

    return ($this->{pool_name}, $this->{prefix_image}, $this->{prefix_mask});
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
    return $this->{size}->[0];
}

# Function: getLimits
sub getLimits {
    my $this = shift;
    return ($this->{limits}->[0], $this->{limits}->[1], $this->{limits}->[2], $this->{limits}->[3]);
}

# Function: getOrder
sub getOrder {
    my $this = shift;
    return $this->{order};
}

# Function: getTileMatrix
sub getTileMatrix {
    my $this = shift;
    return $this->{tm};
}

# Function: bboxToSlabIndices
sub bboxToSlabIndices {
    my $this = shift;
    my @bbox = @_;

    return $this->{tm}->bboxToIndices(@bbox, $this->{size}->[0], $this->{size}->[1]);
}

# Function: ownMasks
sub ownMasks {
    my $this = shift;
    return (defined $this->{dir_mask} || defined $this->{prefix_mask});
}

=begin nd
Function: getSlabPath

Returns the theoric slab path (file path or object name)

Parameters (list):
    type - string - "IMAGE" ou "MASK"
    col - integer - Slab column
    row - integer - Slab row
=cut
sub getSlabPath {
    my $this = shift;
    my $type = shift;
    my $col = shift;
    my $row = shift;

    if ($this->{type} eq "FILE") {
        my $b36 = COMMON::Base36::indicesToB36Path($col, $row, $this->{dir_depth} + 1);

        if (! defined $type) {
            return "$b36.tif";
        }
        elsif ($type eq "IMAGE") {
            return File::Spec->catdir($this->{dir_image}, "$b36.tif");
        }
        elsif ($type eq "MASK") {
            return File::Spec->catdir($this->{dir_mask}, "$b36.tif");
        }
        else {
            return undef;
        }
    } else {
        if (! defined $type) {
            return sprintf "%s_%s_%s", $this->{id}, $col, $row;
        }
        elsif ($type eq "IMAGE") {
            return sprintf "%s_%s_%s_%s", $this->{prefix_image}, $this->{id}, $col, $row;
        }
        elsif ($type eq "MASK") {
            return sprintf "%s_%s_%s_%s", $this->{prefix_mask}, $this->{id}, $col, $row;
        }
        else {
            return undef;
        }

    }

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

# Function: getDirImage
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
        <mask>
            <baseDir>./BDORTHO/MASK/level_5/</baseDir>
        </mask>
        <pathDepth>2</pathDepth>

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
=cut
sub exportToXML {
    my $this = shift;

    my $string = "    <level>\n";
    $string .= sprintf "        <tileMatrix>%s</tileMatrix>\n", $this->{id};
    $string .= sprintf "        <tilesPerWidth>%s</tilesPerWidth>\n", $this->{size}->[0];
    $string .= sprintf "        <tilesPerHeight>%s</tilesPerHeight>\n", $this->{size}->[1];
    $string .= "        <TMSLimits>\n";

    if (defined $this->{limits}->[0]) {
        $string .= sprintf "            <minTileRow>%s</minTileRow>\n", $this->{limits}->[0];
        $string .= sprintf "            <maxTileRow>%s</maxTileRow>\n", $this->{limits}->[1];
        $string .= sprintf "            <minTileCol>%s</minTileCol>\n", $this->{limits}->[2];
        $string .= sprintf "            <maxTileCol>%s</maxTileCol>\n", $this->{limits}->[3];
    } else {
        $string .= "            <minTileRow>0</minTileRow>\n";
        $string .= "            <maxTileRow>0</maxTileRow>\n";
        $string .= "            <minTileCol>0</minTileCol>\n";
        $string .= "            <maxTileCol>0</maxTileCol>\n";
    }
    $string .= "        </TMSLimits>\n";

    if ($this->{type} eq "FILE") {
        $string .= sprintf "        <baseDir>%s</baseDir>\n", File::Spec->abs2rel($this->{dir_image}, $this->{desc_path});
        $string .= sprintf "        <pathDepth>%s</pathDepth>\n", $this->{dir_depth};
    }
    elsif ($this->{type} eq "S3") {
        $string .= sprintf "        <imagePrefix>%s</imagePrefix>\n", $this->{prefix_image};
        $string .= "        <s3Context>\n";
        $string .= sprintf "            <bucketName>%s</bucketName>\n", $this->{bucket_name};
        $string .= "        </s3Context>\n";
    }
    elsif ($this->{type} eq "CEPH") {
        $string .= sprintf "        <imagePrefix>%s</imagePrefix>\n", $this->{prefix_image};
        $string .= "        <cephContext>\n";
        $string .= sprintf "            <poolName>%s</poolName>\n", $this->{pool_name};
        $string .= "        </cephContext>\n";
    }

    if ($this->ownMasks()) {
        $string .=  "        <mask>\n";

        if (defined $this->{dir_mask}) {
            $string .= sprintf "            <baseDir>%s</baseDir>", File::Spec->abs2rel($this->{dir_mask}, $this->{desc_path});
        }
        elsif (defined $this->{prefix_mask}) {
            $string .= sprintf "            <maskPrefix>%s</maskPrefix>", $this->{prefix_mask};
        }

        $string .=  "            <format>TIFF_ZIP_INT8</format>\n";
        $string .=  "        </mask>\n";
    }


    $string .= "    </level>\n";

    return $string;
}

1;
__END__
