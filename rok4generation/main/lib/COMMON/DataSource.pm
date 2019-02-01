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
File: DataSource.pm

Class: COMMON::DataSource

(see COMMON_DataSource.png)

Manage a data source, physical (image files) or virtual (WMS server) or both.

Using:
    (start code)
    use COMMON::DataSource;

    # DataSource object creation : 3 cases

    # Real Data and no harvesting : native SRS and lossless compression
    my $objDataSource = COMMON::DataSource->new(
        "19",
        {
            srs => "IGNF:LAMB93",
            path_image => "/home/ign/DATA/BDORTHO"
        }
    );

    # No Data, just harvesting (here for a WMS vector) 
    my $objDataSource = COMMON::DataSource->new(
        "19",
        {
            srs => IGNF:WGS84G,
            extent => /home/ign/SHAPE/WKTPolygon.txt,

            wms_layer   => "tp:TRONCON_ROUTE",
            wms_url => "http://geoportail/wms/",
            wms_version => "1.3.0",
            wms_request => "getMap",
            wms_format  => "image/png",
            wms_bgcolor => "0xFFFFFF",
            wms_transparent  => "FALSE",
            wms_style  => "line",
            min_size => 9560,
            max_width => 1024,
            max_height => 1024
        }
    );
    
    # No Data, just harvesting provided images
    my $objDataSource = COMMON::DataSource->new(
        "19",
        {
            srs => IGNF:WGS84G,
            list => /home/ign/listIJ.txt,

            wms_layer   => "tp:TRONCON_ROUTE",
            wms_url => "http://geoportail/wms/",
            wms_version => "1.3.0",
            wms_request => "getMap",
            wms_format  => "image/png",
            wms_bgcolor => "0xFFFFFF",
            wms_transparent  => "FALSE",
            wms_style  => "line",
            min_size => 9560,
            max_width => 1024,
            max_height => 1024
        }
    );

    # Real Data and harvesting : reprojection or lossy compression
    my $objDataSource = COMMON::DataSource->new(
        "19",
        {
            srs => "IGNF:LAMB93",
            path_image => "/home/ign/DATA/BDORTHO"
            wms_layer => "ORTHO_XXX",
            wms_url => "http://geoportail/wms/",
            wms_version => "1.3.0",
            wms_request => "getMap",
            wms_format => "image/tiff"
        }
    );
    (end code)

Attributes:
    bottomID - string - Level identifiant, from which data source is used (base level).
    bottomOrder - integer - Level order, from which data source is used (base level).
    topID - string - Level identifiant, to which data source is used. It is calculated in relation to other datasource.
    topOrder - integer - Level order, to which data source is used. It is calculated in relation to other datasource.

    srs - string - SRS of the bottom extent (and ImageSource objects if exists).
    extent - <OGR::Geometry> - Precise extent, in the previous SRS (can be a bbox). It is calculated from the <ImageSource> or supplied in configuration file. 'extent' is mandatory (a bbox or a file which contains a WKT geometry) if there are no images. We have to know area to harvest. If images, extent is calculated thanks data.
    list - string - File path, containing a list of image indices (I,J) to harvest.
    bbox - double array - Data source bounding box, in the previous SRS : [xmin,ymin,xmax,ymax].

    imageSource - <COMMON::ImageSource> - Georeferenced images' source.
    harvesting - <COMMON::Harvesting> - WMS server.
    databaseSource - <COMMON::DatabaseSource> - PostgreSQL server.
=cut

################################################################################

package COMMON::DataSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use List::Util qw(min max);

# My module
use COMMON::ImageSource;
use COMMON::Harvesting;
use COMMON::DatabaseSource;
use COMMON::ProxyGDAL;

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

DataSource constructor. Bless an instance.

Parameters (list):
    level - string - Base level (bottom) for this data source.
    params - hash - Data source parameters (see <_load> for details).

See also:
    <_load>, <computeGlobalInfo>
=cut
sub new {
    my $class = shift;
    my $level = shift;
    my $params = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $this = {
        # Global information
        bottomID => undef,
        bottomOrder => undef,
        topID => undef,
        topOrder => undef,
        bbox => undef,
        list => undef,
        extent => undef,
        srs => undef,
        # Image source
        imageSource => undef,
        # Harvesting
        harvesting => undef,
        # Database
        databaseSource => undef
    };

    bless($this, $class);

    # load. class
    return undef if (! $this->_load($level,$params));
    return undef if (! $this->computeGlobalInfo());

    return $this;
}

=begin nd
Function: _load

Sorts parameters, relays to concerned constructors and stores results.

(see datasource.png)

Parameters (list):
    level - string - Base level (bottom) for this data source.
    params - hash - Data source parameters :
    (start code)
            # common part
            srs - string

            # image source part
            path_image          - string
            preprocess_command  - string
            preprocess_opt_beg  - string
            preprocess_opt_mid  - string
            preprocess_opt_end  - string
            preprocess_tmp_dir  - string

            # harvesting part
            wms_layer - string
            wms_url - string
            wms_version - string
            wms_request - string
            wms_format - string
            wms_bgcolor - string
            wms_transparent - string
            wms_style - string
            min_size - string
            max_width - string
            max_height - string
    (end code)
    This hash is directly and entirely relayed to <ImageSource::new> (even though only common and harvesting parts will be used) and harvesting part is directly relayed to <COMMON::Harvesting::new> (see parameters' meaning).
=cut
sub _load {
    my $this   = shift;
    my $level = shift;
    my $params = shift;
    
    return FALSE if (! defined $params);

    if (! defined $level || $level eq "") {
        ERROR("A data source have to be defined with a level !");
        return FALSE;
    }
    $this->{bottomID} = $level;

    if (! exists $params->{srs} || ! defined $params->{srs}) {
        ERROR("A data source have to be defined with the 'srs' parameter !");
        return FALSE;
    }
    $this->{srs} = $params->{srs};

    # bbox is optionnal if we have an ImageSource (checked in computeGlobalInfo)
    if (exists $params->{extent} && defined $params->{extent}) {
        $this->{extent} = $params->{extent};
    }
    
    if (exists $params->{list} && defined $params->{list}) {
        $this->{list} = $params->{list};
    }

    # ImageSource is optionnal
    my $imagesource = undef;
    if (exists $params->{path_image}) {
        $imagesource = COMMON::ImageSource->new($params);
        if (! defined $imagesource) {
            ERROR("Cannot create the ImageSource object");
            return FALSE;
        }
    }
    $this->{imageSource} = $imagesource;

    # Harvesting is optionnal, but if we have 'wms_layer' parameter, we suppose that we have others
    my $harvesting = undef;
    if (exists $params->{wms_layer}) {
        $harvesting = COMMON::Harvesting->new($params);
        if (! defined $harvesting) {
            ERROR("Cannot create the Harvesting object");
            return FALSE;
        }
    }
    $this->{harvesting} = $harvesting;

    # DatabaseSource is optionnal, but if we have 'db' parameter, we suppose that we have others
    my $database = undef;
    if (exists $params->{db}) {
        $database = COMMON::DatabaseSource->new($params);
        if (! defined $database) {
            ERROR("Cannot create the DatabaseSource object");
            return FALSE;
        }
    }
    $this->{databaseSource} = $database;
    
    if (! defined $harvesting && ! defined $imagesource && ! defined $database) {
        ERROR("A data source must have a ImageSource OR a Harvesting OR a DatabaseSource !");
        return FALSE;
    }
    
    return TRUE;
}

=begin nd
Function: computeGlobalInfo

Reads the srs, manipulates extent and bounding box.

If an extent is supplied (no image source), 2 cases are possible :
    - extent is a bbox, as xmin,ymin,xmax,ymax
    - extent is a file path, file contains a complex polygon, WKT format.

We generate an OGR Geometry from the supplied extent or the image source bounding box.
=cut
sub computeGlobalInfo {
    my $this = shift;

    # Bounding polygon
    if (defined $this->{imageSource}) {
        # We have real images for source, bbox will be calculated from them.

        my @BBOX = $this->{imageSource}->computeBBox();
        $this->{extent} = sprintf "%s,%s,%s,%s", $BBOX[0], $BBOX[1], $BBOX[2], $BBOX[3];
    }
    
    if (defined $this->{extent}) {
        # On a des images, une bbox ou une géométrie WKT pour définir la zone de génération

        my $WKTextent;

        $this->{extent} =~ s/ //;
        my @limits = split (/,/,$this->{extent},-1);
        
        my $extent;
        if (scalar @limits == 4) {
            # user supplied a BBOX
            $extent = COMMON::ProxyGDAL::geometryFromString("BBOX", $this->{extent});
            if (! defined $extent) {
                ERROR(sprintf "Cannot create a OGR geometry from the bbox %s", $this->{extent});
                return FALSE ;
            }

        }
        else {
            # user supplied a file which contains bounding polygon
            $extent = COMMON::ProxyGDAL::geometryFromFile($this->{extent});
            if (! defined $extent) {
                ERROR(sprintf "Cannot create a OGR geometry from the file %s", $this->{extent});
                return FALSE ;
            }
        }
        $this->{extent} = $extent;

        my ($xmin,$ymin,$xmax,$ymax) = COMMON::ProxyGDAL::getBbox($this->{extent});

        if (! defined $xmin) {
            ERROR("Cannot calculate bbox from the OGR Geometry");
            return FALSE;
        }

        $this->{bbox} = [$xmin,$ymin,$xmax,$ymax];
        
    } elsif (defined $this->{list}) {
        # On a fourni un fichier contenant la liste des images (I et J) à générer
        
        my $file = $this->{list};
        
        if (! -e $file) {
            ERROR("Parameter 'list' value have to be an existing file ($file)");
            return FALSE ;
        }
        
        
    } else {
        ERROR("'extent' or 'list' required in the sources configuration file if no image source !");
        return FALSE ;
    }

    return TRUE;

}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getSRS
sub getSRS {
    my $this = shift;
    return $this->{srs};
}

# Function: getType

=begin nd
Function: getType

A datasource with a database source and no harvesting source will be considered as a VECTOR source. It's a raster source otherwise
=cut
sub getType {
    my $this = shift;

    if (defined $this->{databaseSource} && ! defined $this->{harvesting}) {
        return "VECTOR";
    } else {
        return "RASTER";
    }
}

# Function: getExtent
sub getExtent {
    my $this = shift;
    return $this->{extent};
}

# Function: getList
sub getList {
    my $this = shift;
    return $this->{list};
}

# Function: getHarvesting
sub getHarvesting {
    my $this = shift;
    return $this->{harvesting};
}

# Function: getDatabaseSource
sub getDatabaseSource {
    my $this = shift;
    return $this->{databaseSource};
}

# Function: getImages
sub getImages {
    my $this = shift;
    return $this->{imageSource}->getImages();
}

# Function: hasImages
sub hasImages {
    my $this = shift;
    return (defined $this->{imageSource});
}

# Function: hasHarvesting
sub hasHarvesting {
    my $this = shift;
    return (defined $this->{harvesting});
}

# Function: hasDatabase
sub hasDatabase {
    my $this = shift;
    return (defined $this->{databaseSource});
}

# Function: getBottomID
sub getBottomID {
    my $this = shift;
    return $this->{bottomID};
}

# Function: getTopID
sub getTopID {
    my $this = shift;
    return $this->{topID};
}

# Function: getBottomOrder
sub getBottomOrder {
    my $this = shift;
    return $this->{bottomOrder};
}

# Function: getTopOrder
sub getTopOrder {
    my $this = shift;
    return $this->{topOrder};
}

# Function: getPixel
sub getPixel {
    my $this = shift;

    if (! defined $this->{imageSource}) {return undef;}
    
    return $this->{imageSource}->getPixel();
}

=begin nd
Function: setBottomOrder

Parameters (list):
    bottomOrder - integer - Bottom level order to set
=cut
sub setBottomOrder {
    my $this = shift;
    my $bottomOrder = shift;
    $this->{bottomOrder} = $bottomOrder;
}

=begin nd
Function: setTopOrder

Parameters (list):
    topOrder - integer - Top level order to set
=cut
sub setTopOrder {
    my $this = shift;
    my $topOrder = shift;
    $this->{topOrder} = $topOrder;
}

=begin nd
Function: setTopID

Parameters (list):
    topID - string - Top level identifiant to set
=cut
sub setTopID {
    my $this = shift;
    my $topID = shift;
    $this->{topID} = $topID;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all informations about the data source. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $this = shift ;
    
    my $export = "";
    
    $export .= sprintf "\n Object COMMON::DataSource :\n";
    $export .= sprintf "\t Extent: %s\n",$this->{extent};
    $export .= sprintf "\t Levels ID (order):\n";
    $export .= sprintf "\t\t- bottom : %s (%s)\n",$this->{bottomID},$this->{bottomOrder};
    $export .= sprintf "\t\t- top : %s (%s)\n",$this->{topID},$this->{topOrder};

    $export .= sprintf "\t Data :\n";
    $export .= sprintf "\t\t- SRS : %s\n",$this->{srs};
    $export .= "\t\t- We have images\n" if (defined $this->{imageSource});
    $export .= "\t\t- We have a WMS service\n" if (defined $this->{harvesting});
    $export .= "\t\t- We have a database\n" if (defined $this->{databaseSource});
    
    if (defined $this->{bbox}) {
        $export .= "\t\t Bbox :\n";
        $export .= sprintf "\t\t\t- xmin : %s\n",$this->{bbox}[0];
        $export .= sprintf "\t\t\t- ymin : %s\n",$this->{bbox}[1];
        $export .= sprintf "\t\t\t- xmax : %s\n",$this->{bbox}[2];
        $export .= sprintf "\t\t\t- ymax : %s\n",$this->{bbox}[3];
    }
    
    if (defined $this->{list}) {
        $export .= sprintf "\t\t List file : %s\n", $this->{list};
    }
    
    return $export;
}

1;
__END__
