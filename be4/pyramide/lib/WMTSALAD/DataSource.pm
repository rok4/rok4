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

Class: WMTSALAD::DataSource

Manage a data source, be it an existing pyramid or a web mapping service (WMS).

Using:
    (start code)
    use WMTSALAD::DataSource;

    # DataSource object creation : 3 cases

    # Real Data and no harvesting : pyramid descriptor

    # No Data, just harvesting WMS (here for a WMS vector) 
    my $objDataSource = BE4::DataSource->new(
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
    
    (end code)

Attributes:
    bottomID - string - Level identifiant, from which data source is used (base level).
    bottomOrder - integer - Level order, from which data source is used (base level).
    topID - string - Level identifiant, to which data source is used. It is calculated in relation to other datasource.
    topOrder - integer - Level order, to which data source is used. It is calculated in relation to other datasource.

    srs - string - SRS of the bottom extent (and ImageSource objects if exists).
    extent - <OGR::Geometry> - Precise extent, in the previous SRS (can be a bbox). It is calculated from the <ImageSource> or supplied in configuration file. 'extent' is mandatory (a bbox or a file which contains a WKT geometry) if there are no images. We have to know area to harvest. If images, extent is calculated thanks to the data.
    list - string - File path, containing a list of image indices (I,J) to harvest.
    bbox - double array - Data source bounding box, in the previous SRS : [xmin,ymin,xmax,ymax].

    pyramidSource - <ImageSource> - Georeferenced images' source.
    harvestSource - <Harvesting> - WMS server. If it is useless, it will be removed.

=cut

################################################################################

package WMTSALAD::DataSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use List::Util qw(min max);

use Geo::GDAL;

# My module
use BE4::ImageSource;
use BE4::Harvesting;

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
    my $this = shift;
    my $level = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
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
        harvestSource => undef,
    };

    bless($self, $class);

    TRACE;

    # load. class
    return undef if (! $self->_load($level,$params));

    return undef if (! $self->computeGlobalInfo());

    return $self;
}
