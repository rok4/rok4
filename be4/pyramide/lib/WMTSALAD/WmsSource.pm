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
File: WmsSource.pm

Class: WMTSALAD::WmsSource



Using:
    (start code)
    use WMTSALAD::WmsSource;

    
    (end code)

Attributes:
    
=cut

################################################################################

package WMTSALAD::WmsSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

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

<WMTSALAD::WmsSource's> constructor.

Using:
    (start code)
    my wmsSource = WMTSALAD::WmsSource->new( {
        wms_url             =>  "http://target.server.net/wms"
        wms_timeout         =>  60
        wms_retry           =>  10
        wms_version         =>  "1.3.0"
        wms_layers          =>  "LAYER_1,LAYER_2,LAYER_3"
        wms_styles          =>  "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3"
        wms_format          =>  "image/png"
        wms_crs             =>  "EPSG:2154"
        wms_extent          =>  "634500,6855000,636800,6857700"
        wms_channels        =>  3
        wms_nodata          =>  "0xFFA2FA"
    } )
    (end code)

Parameters:
    params - hash reference, containing the following properties :
        {
            wms_url - string - WMS server's URL
            wms_proxy - string - proxy's URL (opt)
            wms_timeout - int - waiting time before timeout, in seconds (opt)
            wms_retry - int - max number of tries (opt)
            wms_interval - int - time interval between tries, in seconds  (opt)
            wms_user - string - authentification username on the WMS server (opt)
            wms_password - string - authentification password (opt)
            wms_referer - string - authentification referer (opt)
            wms_userAgent - string - authentifcation user agent (opt)
            wms_version - string - version number
            wms_layers - string - comma separated layers list
            wms_styles - string - comma separated styles list, matching the layers list
            wms_crs - string - coordinate reference system (opt)
            wms_format - string - output image format (opt)
            wms_channels - int - output image channels number
            wms_nodata - string - value of the no_data / background color
            wms_extent - string - data bounding box, in the following format : minx,miny,maxx,maxxy
            wms_option - string - WMS request options (opt)
        }

Returns:
    The newly created WmsSource object. 'undef' in case of failure.
    
=cut
sub new() {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;

    # IMPORTANT : if modification, think to update natural documentation (just above)
    # see config/pyramids/pyramid.xsd to get the list of parameters, as used by Rok4.
    my $self = {
        wms_url => undef,
        wms_proxy => undef,
        wms_timeout => undef,
        wms_retry => undef,
        wms_interval => undef,
        wms_user => undef,
        wms_password => undef,
        wms_referer => undef,
        wms_userAgent => undef,
        wms_version => undef,
        wms_layers => undef,
        wms_styles => undef,
        wms_crs => undef,
        wms_format => undef,
        wms_channels => undef,
        wms_nodata => undef,
        wms_extent => undef,
        wms_option => undef,
    };

    bless($self, $class);

    if (!defined $file) {
        return undef;
    }

    if (!$self->_load($params)) {
        ERROR("Could not load pyramid source.");
        return undef;
    }

    return $self;
}

=begin nd

Function: _load

<WMTSALAD::WmsSource's> constructor's annex. Checks parameters passed to 'new', 
then load them in the new WmsSource object.

Using:
    (start code)
    _load( {
        wms_url             =>  "http://target.server.net/wms"
        wms_timeout         =>  60
        wms_retry           =>  10
        wms_version         =>  "1.3.0"
        wms_layers          =>  "LAYER_1,LAYER_2,LAYER_3"
        wms_styles          =>  "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3"
        wms_format          =>  "image/png"
        wms_crs             =>  "EPSG:2154"
        wms_extent          =>  "634500,6855000,636800,6857700"
        wms_channels        =>  3
        wms_nodata          =>  "0xFFA2FA"
    } )
    (end code)

Parameters:
    params - hash reference, containing the following properties :
        {
            wms_url - string - WMS server's URL
            wms_proxy - string - proxy's URL (opt)
            wms_timeout - int - waiting time before timeout, in seconds (opt)
            wms_retry - int - max number of tries (opt)
            wms_interval - int - time interval between tries, in seconds  (opt)
            wms_user - string - authentification username on the WMS server (opt)
            wms_password - string - authentification password (opt)
            wms_referer - string - authentification referer (opt)
            wms_userAgent - string - authentifcation user agent (opt)
            wms_version - string - version number
            wms_layers - string - comma separated layers list
            wms_styles - string - comma separated styles list, matching the layers list
            wms_crs - string - coordinate reference system (opt)
            wms_format - string - output image format (opt)
            wms_channels - int - output image channels number
            wms_nodata - string - value of the no_data / background color
            wms_extent - string - data bounding box, in the following format : minx,miny,maxx,maxxy
            wms_option - string - WMS request options (opt)       
        }

Returns:
    TRUE in case of success, FALSE in case of failure.
    
=cut
sub _load() {
    my $self = shift;
    my $params = shift;

    # Test if the mandatory parameters are defined
    ## server URL
    if (!exists $params->{wms_url} || !defined $params->{wms_url} || $params->{wms_url} eq '') {
        ERROR("Undefined WMS server URL.");
        return FALSE;
    }
    ## WMS version
    if (!exists $params->{wms_version} || !defined $params->{wms_version} || $params->{wms_version} eq '') {
        ERROR("Undefined WMS protocol version.");
        return FALSE;
    }
    ## Layers list
    if (!exists $params->{wms_layers} || !defined $params->{wms_layers} || $params->{wms_layers} eq '') {
        ERROR("Undefined layers list.");
        return FALSE;
    }
    ## Layer styles list
    if (!exists $params->{wms_styles} || !defined $params->{wms_styles} || $params->{wms_styles} eq '') {
        ERROR("Undefined layer styles list.");
        return FALSE;
    } elsif (scalar (split (',',$params->{wms_styles})) != scalar (split (',',$params->{wms_layers})) ) {
        ERROR("The number of layer styles does not match the number of layers.");
        return FALSE;
    }
    ## Number of color channels
    if (!exists $params->{wms_channels} || !defined $params->{wms_channels} || $params->{wms_channels} eq '') {
        ERROR("Undefined images' number of channels.");
        return FALSE;
    }
    ## Background color
    if (!exists $params->{wms_nodata} || !defined $params->{wms_nodata} || $params->{wms_nodata} eq '') {
        ERROR("Undefined nodata value / background color.");
        return FALSE;
    }
    ## Bounding box
    if (!exists $params->{wms_extent} || !defined $params->{wms_extent} || $params->{wms_extent} eq '') {
        ERROR("Undefined data extent.");
        return FALSE;
    } elsif (scalar (split (',',$params->{wms_styles})) != 4) {
        ERROR("Wrong coordinates count in extent. Extent should be 'min_x,min_y,max_x,max_y'");
        return FALSE;
    }



    return TRUE;
}

####################################################################################################
#                                        Group: Tests                                              #
####################################################################################################





####################################################################################################
#                                        Group: Output                                             #
####################################################################################################

=begin nd

Function: write

=cut
sub write() {
    return TRUE;
}
