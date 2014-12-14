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

Class: BE4::Level

Describe a level in a pyramid.

Using:
    (start code)
    use BE4::Level;

    my $params = {
        id                => level_5,
        order             => 12,
        dir_image         => "/home/ign/BDORTHO/IMAGE/level_5/",
        dir_nodata        => "/home/ign/BDORTHO/NODATA/level_5/",
        dir_mask          => "/home/ign/BDORTHO/MASK/level_5/",
        dir_metadata      => undef,
        compress_metadata => undef,
        type_metadata     => undef,
        size              => [16, 16],
        dir_depth         => 2,
        limits            => [365,368,1026,1035]
    };

    my $objLevel = BE4::Level->new($params);
    (end code)

Attributes:
    id - string - Level identifiant.
    order - integer - Level order (ascending resolution)
    dir_image - string - Absolute images' directory path for this level.
    dir_nodata - string - Absolute nodata's directory path for this level.
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

package BE4::Level;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

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

# Constant: STRLEVELTMPLT
# Define the template XML for a level in the pyramid's descriptor.
my $STRLEVELTMPLT = <<"TLEVEL";
    <level>
        <tileMatrix>__ID__</tileMatrix>
        <baseDir>__DIRIMG__</baseDir>
<!-- __MASK__ -->
<!-- __MTD__ -->
        <tilesPerWidth>__TILEW__</tilesPerWidth>
        <tilesPerHeight>__TILEH__</tilesPerHeight>
        <pathDepth>__DEPTH__</pathDepth>
        <nodata>
            <filePath>__NODATAPATH__</filePath>
        </nodata>
        <TMSLimits>
            <minTileRow>__MINROW__</minTileRow>
            <maxTileRow>__MAXROW__</maxTileRow>
            <minTileCol>__MINCOL__</minTileCol>
            <maxTileCol>__MAXCOL__</maxTileCol>
        </TMSLimits>
    </level>
<!-- __LEVELS__ -->
TLEVEL

# Constant: STRLEVELTMPLTMASK
# Define the template XML for the mask part of a level in the pyramid's descriptor.
my $STRLEVELTMPLTMASK = <<"TMASK";
        <mask>
            <baseDir>__DIRMASK__</baseDir>
            <format>__FMTMASK__</format>
        </mask>
TMASK

# Constant: STRLEVELTMPLTMORE
# Define the template XML for the metadat part of a level in the pyramid's descriptor.
my $STRLEVELTMPLTMORE = <<"TMTD";
        <metadata type='INT32_DB_LZW'>
            <baseDir>__DIRMTD__</baseDir>
            <format>__FORMATMTD__</format>
        </metadata>
TMTD

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
    order - integer - Level order (ascending resolution)
    size - integer array - Number of tile in one image for this level
    limits - integer array - Optionnal. Current level's limits. Set to [undef,undef,undef,undef] if not defined.
    dir_depth - integer - Number of subdirectories from the level root to the image
    dir_image - string - Absolute images' directory path for this level.
    dir_nodata - string - Absolute nodata's directory path for this level.
    dir_mask - string - Optionnal (if we want to keep mask in the final images' pyramid). Absolute mask' directory path for this level.

See also:
    <_init>
=cut
sub new {
    my $this = shift;
    my $params = shift;
    
    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        id                => undef,
        order             => undef,
        dir_image         => undef,
        dir_nodata        => undef,
        dir_mask          => undef,
        dir_metadata      => undef,
        compress_metadata => undef,
        type_metadata     => undef,
        size              => [],
        dir_depth         => 0,
        limits            => undef
    };
    
    bless($self, $class);
    
    TRACE;
    
    # init. class
    if (! $self->_init($params)) {
        ERROR ("One parameter is missing !");
        return undef;
    }
    
    return $self;
}

=begin nd
Function: _init

Check and store level's attributes values.

Parameters (hash):
    id - string - Level identifiant
    order - integer - Level order (ascending resolution)
    size - integer array - Number of tile in one image for this level
    limits - integer array - Optionnal. Current level's limits. Set to [undef,undef,undef,undef] if not defined.
    dir_depth - integer - Number of subdirectories from the level root to the image
    dir_image - string - Absolute images' directory path for this level.
    dir_nodata - string - Absolute nodata's directory path for this level.
    dir_mask - string - Optionnal (if we want to keep mask in the final images' pyramid). Absolute mask' directory path for this level.
=cut
sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # Mandatory parameters !
    if (! exists($params->{id})) {
        ERROR ("The parameter 'id' is required");
        return FALSE;
    }
    if (! exists($params->{order})) {
        ERROR ("The parameter 'order' is required");
        return FALSE;
    }
    if (! exists($params->{dir_image})) {
        ERROR ("The parameter 'dir_image' is required");
        return FALSE;
    }
    if (! exists($params->{dir_nodata})) {
        ERROR ("The parameter 'dir_nodata' is required");
        return FALSE;
    }
    if (! exists($params->{size})) {
        ERROR ("The parameter 'size' is required");
        return FALSE;
    }
    if (! exists($params->{dir_depth})) {
        ERROR ("The parameter 'dir_depth' is required");
        return FALSE;
    }

    # check values
    if (! scalar ($params->{size})){
        ERROR("List empty to 'size' !");
        return FALSE;
    }
    if (! $params->{dir_depth}){
        ERROR("Value not valid for 'dir_depth' (0 or undef) !");
        return FALSE;
    }
    
    # parameters optional !
    if (exists $params->{dir_mask} && defined $params->{dir_mask}){
        $self->{dir_mask} = $params->{dir_mask};
    }

    if (! exists($params->{limits}) || ! defined($params->{limits})) {
        $params->{limits} = [undef, undef, undef, undef];
    }
    
    # TODO : metadata 
    
    $self->{id}             = $params->{id};
    $self->{order}          = $params->{order};
    $self->{dir_image}      = $params->{dir_image};
    $self->{dir_nodata}     = $params->{dir_nodata};
    $self->{size}           = $params->{size};
    $self->{dir_depth}      = $params->{dir_depth};
    $self->{limits}         = $params->{limits};
    
    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getID
sub getID {
    my $self = shift;
    return $self->{id};
}

# Function: getOrder
sub getOrder {
    my $self = shift;
    return $self->{order};
}

# Function: getDirImage
sub getDirImage {
    my $self = shift;
    return $self->{dir_image};
}

# Function: getDirNodata
sub getDirNodata {
    my $self = shift;
    return $self->{dir_nodata};
}

# Function: getDirMask
sub getDirMask {
    my $self = shift;
    return $self->{dir_mask};
}

# Function: getLimits
sub getLimits {
    my $self = shift;
    return ($self->{limits}[0], $self->{limits}[1], $self->{limits}[2], $self->{limits}[3]);
}

=begin nd
method: updateExtremTiles

Compare old extrems rows/columns with the news and update values.

Parameters (list):
    jMin, jMax, iMin, iMax - integer list - Tiles indices to compare with current extrems
=cut
sub updateExtremTiles {
    my $self = shift;
    my ($jMin,$jMax,$iMin,$iMax) = @_;

    TRACE();
    
    if (! defined $self->{limits}[0] || $jMin < $self->{limits}[0]) {$self->{limits}[0] = $jMin;}
    if (! defined $self->{limits}[1] || $jMax > $self->{limits}[1]) {$self->{limits}[1] = $jMax;}
    if (! defined $self->{limits}[2] || $iMin < $self->{limits}[2]) {$self->{limits}[2] = $iMin;}
    if (! defined $self->{limits}[3] || $iMax > $self->{limits}[3]) {$self->{limits}[3] = $iMax;}
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
method: exportToXML

Insert Level's attributes in the XML template, write in the pyramid's descriptor.

Returns a string to XML format.

Parameter (list):
    descriptorDir - string - Pyramid's descriptor directory, to make relative data's paths.

Example:
    (start code)
    <level>
        <tileMatrix>level_5</tileMatrix>
        <baseDir>./BDORTHO/IMAGE/level_5/</baseDir>
        <mask>
            <baseDir>./BDORTHO/MASK/level_5/</baseDir>
        </mask>
        <tilesPerWidth>16</tilesPerWidth>
        <tilesPerHeight>16</tilesPerHeight>
        <pathDepth>2</pathDepth>
        <nodata>
            <filePath>./BDORTHO/NODATA/level_5/nd.tif</filePath>
        </nodata>
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
    my $self = shift;
    my $descriptorDir = shift;

    my $levelXML = $STRLEVELTMPLT;

    my $id       = $self->{id};
    $levelXML =~ s/__ID__/$id/;

    my $dirimg = File::Spec->abs2rel($self->{dir_image},$descriptorDir);
    $levelXML =~ s/__DIRIMG__/$dirimg/;

    my $pathnd = File::Spec->abs2rel($self->{dir_nodata}."/nd.tif",$descriptorDir);
    $levelXML =~ s/__NODATAPATH__/$pathnd/;

    my $tilew    = $self->{size}[0];
    $levelXML =~ s/__TILEW__/$tilew/;
    my $tileh    = $self->{size}[1];
    $levelXML =~ s/__TILEH__/$tileh/;

    my $depth    =  $self->{dir_depth};
    $levelXML =~ s/__DEPTH__/$depth/;

    my $minrow   =  $self->{limits}[0];
    $levelXML =~ s/__MINROW__/$minrow/;
    my $maxrow   =  $self->{limits}[1];
    $levelXML =~ s/__MAXROW__/$maxrow/;
    my $mincol   =  $self->{limits}[2];
    $levelXML =~ s/__MINCOL__/$mincol/;
    my $maxcol   =  $self->{limits}[3];
    $levelXML =~ s/__MAXCOL__/$maxcol/;

    # mask
    if (defined $self->{dir_mask}) {
        $levelXML =~ s/<!-- __MASK__ -->\n/$STRLEVELTMPLTMASK/;

        my $dirmask = File::Spec->abs2rel($self->{dir_mask},$descriptorDir);
        $levelXML =~ s/__DIRMASK__/$dirmask/;
        
        $levelXML =~ s/__FMTMASK__/TIFF_ZIP_INT8/;
    } else {
        $levelXML =~ s/<!-- __MASK__ -->\n//;
    }
    
    # metadata
    if (defined $self->{dir_metadata}) {
        $levelXML =~ s/<!-- __MTD__ -->/$STRLEVELTMPLTMORE/;

        my $dirmtd   = $self->{dir_metadata};
        $levelXML =~ s/__DIRMTD__/$dirmtd/;

        my $formatmtd = $self->{compress_metadata};
        $levelXML  =~ s/__FORMATMTD__/$formatmtd/;
    } else {
        $levelXML =~ s/<!-- __MTD__ -->\n//;
    }

    return $levelXML;
}

=begin nd
Function: exportForDebug

Returns all level's informations. Useful for debug.

Example:
    (start code)
    Object BE4::Level :
         ID (string) : 16, and order (integer) : 5
         Directories (depth = 2):
                - Images : /home/ign/data/pyramid/IMAGE/16
                - Nodata : /home/ign/data/pyramid/NODATA/16
                - Mask : /home/ign/data/pyramid/MASK/16
         Tile limits :
                - Column min : 783
                - Column max : 797
                - Row min : 6265
                - Row max : 6276
         Tiles per image :
                - widthwise : 16
                - heightwise : 16
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject BE4::Level :\n";
    $export .= sprintf "\t ID (string) : %s, and order (integer) : %s\n", $self->{id}, $self->{order};

    $export .= sprintf "\t Directories (depth = %s): \n",$self->{dir_depth};
    $export .= sprintf "\t\t- Images : %s\n",$self->{dir_image};
    $export .= sprintf "\t\t- Nodata : %s\n",$self->{dir_nodata};
    $export .= sprintf "\t\t- Metadata : %s\n",$self->{dir_metadata} if (defined $self->{dir_metadata});
    $export .= sprintf "\t\t- Mask : %s\n",$self->{dir_mask} if (defined $self->{dir_mask});
    
    $export .= "\t Tile limits : \n";
    $export .= sprintf "\t\t- Column min : %s\n",$self->{limits}[2];
    $export .= sprintf "\t\t- Column max : %s\n",$self->{limits}[3];
    $export .= sprintf "\t\t- Row min : %s\n",$self->{limits}[0];
    $export .= sprintf "\t\t- Row max : %s\n",$self->{limits}[1];
    
    $export .= "\t Tiles per image : \n";
    $export .= sprintf "\t\t- widthwise : %s\n",$self->{size}[0];
    $export .= sprintf "\t\t- heightwise : %s\n",$self->{size}[1];
    
    return $export;
}

1;
__END__
