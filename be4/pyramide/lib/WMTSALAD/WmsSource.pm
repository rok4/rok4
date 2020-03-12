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

    my wmsSource = WMTSALAD::WmsSource->new( {
        level               =>  7,
        order               =>  0,
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
    } );

    $wmsSource->writeInXml();    
    (end code)

Attributes:
    level - string - the level ID for this source in the tile matrix sytem (TMS) (inherited from <WMTSALAD::DataSource>
    order - positive integer (starts at 0) - the priority order for this source at this level (inherited from <WMTSALAD::DataSource>
    url - string - WMS server's URL
    proxy - string - proxy's URL (opt)
    timeout - int - waiting time before timeout, in seconds (opt)
    retry - int - max number of tries (opt)
    interval - int - time interval between tries, in seconds (opt)
    user - string - authentification username on the WMS server (opt)
    password - string - authentification password (opt)
    referer - string - authentification referer (opt)
    userAgent - string - authentifcation user agent (opt)
    version - string - version number
    layers - string - comma separated layers list
    styles - string - comma separated styles list, matching the layers list
    crs - string - coordinate reference system (opt)
    format - string - output image format (opt)
    channels - int - output image channels number
    nodata - string - value of the no_data / background color
    extent - string - data bounding box, in the following format : minx,miny,maxx,maxxy
    option - string - WMS request options (opt)

Related:
    <WMTSALAD::DataSource> - Mother class
    
=cut

################################################################################

package WMTSALAD::WmsSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use XML::LibXML;
use COMMON::CheckUtils;

require Exporter;

use parent qw(WMTSALAD::DataSource Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constants
use constant TRUE  => 1;
use constant FALSE => 0;

# Constant: WMS
# Define allowed values for attributes wms_format and wms_version.
my %WMS;

################################################################################

BEGIN {}
INIT {
    %WMS = (
        format => [
            'image/png',
            'image/jpeg',
            # 'image/x-bil;bits=32',
            # 'image/tiff',
            # 'image/tiff&format_options=compression:deflate',
            # 'image/tiff&format_options=compression:lzw',
            # 'image/tiff&format_options=compression:packbits',
            # 'image/tiff&format_options=compression:raw'
        ],
        version => [
            '1.1.1', 
            '1.3.0'
        ],
    );
}
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
        level               =>  7,
        order               =>  0,
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
            level - string - the level ID for this source in the tile matrix sytem (TMS) (inherited from <WMTSALAD::DataSource>
            order - positive integer (starts at 0) - the priority order for this source at this level (inherited from <WMTSALAD::DataSource>
            wms_url - string - WMS server's URL
            wms_proxy - string - proxy's URL (opt)
            wms_timeout - int - waiting time before timeout, in seconds (opt)
            wms_retry - int - max number of tries (opt)
            wms_interval - int - time interval between tries, in seconds  (opt)
            wms_user - string - authentification username on the WMS server (opt)
            wms_password - string - authentification password (opt)
            wms_referer - string - authentification referer (opt)
            wms_userAgent - string - authentification user agent (opt)
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
    my $class = shift;
    my $params = shift;

    $class = ref($class) || $class;

    # IMPORTANT : if modification, think to update natural documentation (just above)
    # see config/pyramids/pyramid.xsd to get the list of parameters, as used by Rok4.

    $params->{type} = 'WMS';

    my $this = $class->SUPER::new($params);
        $this->{url} = undef;
        $this->{proxy} = undef;
        $this->{timeout} = undef;
        $this->{retry} = undef;
        $this->{interval} = undef;
        $this->{user} = undef;
        $this->{password} = undef;
        $this->{referer} = undef;
        $this->{userAgent} = undef;
        $this->{version} = undef;
        $this->{layers} = undef;
        $this->{styles} = undef;
        $this->{crs} = undef;
        $this->{format} = undef;
        $this->{channels} = undef;
        $this->{nodata} = undef;
        $this->{extent} = undef;
        $this->{option} = undef;

    bless($this, $class);

    if (!$this->_init($params)) {
        ERROR("Could not load WMS source.");
        return undef;
    }

    return $this;
}

=begin nd

Function: _init

<WMTSALAD::WmsSource's> constructor's annex. Checks parameters passed to 'new', 
then load them in the new WmsSource object.

Using:
    (start code)
    _init( {
        level               =>  7,
        order               =>  0,
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
            level - string - the level ID for this source in the tile matrix sytem (TMS) (inherited from <WMTSALAD::DataSource>
            order - positive integer (starts at 0) - the priority order for this source at this level (inherited from <WMTSALAD::DataSource>
            wms_url - string - WMS server's URL
            wms_proxy - string - proxy's URL (opt)
            wms_timeout - int - waiting time before timeout, in seconds (opt)
            wms_retry - int - max number of tries (opt)
            wms_interval - int - time interval between tries, in seconds  (opt)
            wms_user - string - authentification username on the WMS server (opt)
            wms_password - string - authentification password (opt)
            wms_referer - string - authentification referer (opt)
            wms_userAgent - string - authentification user agent (opt)
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
sub _init() {
    my $this = shift;
    my $params = shift;

    return FALSE if(!$this->SUPER::_init($params));

    foreach my $key (keys %{$params}) {
        chomp $params->{$key};
    }

    #### Mandatory parameters ####
    
    ## server URL
    if (!exists $params->{wms_url} || !defined $params->{wms_url} || $params->{wms_url} eq '' || $params->{wms_url} eq 'http://') {
        ERROR("Undefined WMS server URL.");
        return FALSE;
    }
    $params->{wms_url} =~ s/^http:\/\///;
    $this->{url} = $params->{wms_url};


    ## WMS version
    if (!exists $params->{wms_version} || !defined $params->{wms_version} || $params->{wms_version} eq '') {
        ERROR("Undefined WMS protocol version.");
        return FALSE;
    } elsif (! $this->isWmsVersion($params->{wms_version})) {
        return FALSE;
    }
    $this->{version} = $params->{wms_version};


    ## Layers list
    if (!exists $params->{wms_layers} || !defined $params->{wms_layers} || $params->{wms_layers} eq '') {
        ERROR("Undefined layers list.");
        return FALSE;
    }
    $this->{layers} = $params->{wms_layers};


    ## Layer styles list
    if (!exists $params->{wms_styles} || !defined $params->{wms_styles} || $params->{wms_styles} eq '') {
        ERROR("Undefined layer styles list.");
        return FALSE;
    }
    my @styles = split (',',$params->{wms_styles});
    my @layers = split (',',$params->{wms_layers});
    if (scalar (@styles) != scalar (@layers) ) {
        ERROR("The number of layer styles does not match the number of layers.");
        return FALSE;
    }
    $this->{styles} = $params->{wms_styles};


    ## Number of color channels
    if (!exists $params->{wms_channels} || !defined $params->{wms_channels} || $params->{wms_channels} eq '') {
        ERROR("Undefined images' number of channels.");
        return FALSE;
    } elsif ( ! COMMON::CheckUtils::isStrictPositiveInt($params->{wms_channels})) {
        ERROR("Channels number must be a strict positive integer.");
        return FALSE;
    }
    $this->{channels} = $params->{wms_channels};


    ## Background color
    if (!exists $params->{wms_nodata} || !defined $params->{wms_nodata} || $params->{wms_nodata} eq '') {
        ERROR("Undefined nodata value / background color.");
        return FALSE;
    }
    $this->{nodata} = $params->{wms_nodata};


    ## Bounding box
    if (!exists $params->{wms_extent} || !defined $params->{wms_extent} || $params->{wms_extent} eq '') {
        ERROR("Undefined data extent.");
        return FALSE;
    } 
    my @coords = split (',',$params->{wms_extent});
    if (scalar (@coords) != 4) {
        ERROR("Wrong coordinates count in extent. Extent should be 'min_x,min_y,max_x,max_y'");
        return FALSE;
    }
    foreach my $coord (@coords) {
        if (! COMMON::CheckUtils::isNumber($coord)) {
            ERROR("Extent coordinates should be decimal numbers.");
            return FALSE;
        }
    }
    if (($coords[0] >= $coords[2]) || ($coords[1] >= $coords[3])) {
        ERROR("Wrong coordinates order in extent. Extent should be 'min_x,min_y,max_x,max_y'");
        return FALSE;
    }
    $this->{extent} = $params->{wms_extent};



    #### Optional parameters ####

    ## wms_proxy - (opt)
    if (exists $params->{wms_proxy} && defined $params->{wms_proxy} && $params->{wms_proxy} ne '') {
        $this->{proxy} = $params->{wms_proxy};
    }

    ## wms_timeout - (opt)
    if (exists $params->{wms_timeout} && defined $params->{wms_timeout} && $params->{wms_timeout} ne '') {
        if (! COMMON::CheckUtils::isPositiveInt($params->{wms_timeout})) {
            ERROR("Request timeout value must be a positive integer.");
            return FALSE;
        }
        $this->{timeout} = $params->{wms_timeout};
    }

    ## wms_retry - (opt)
    if (exists $params->{wms_retry} && defined $params->{wms_retry} && $params->{wms_retry} ne '') {
        if (! COMMON::CheckUtils::isPositiveInt($params->{wms_retry})) {
            ERROR("Request retry number must be a positive integer.");
            return FALSE;
        }
        $this->{retry} = $params->{wms_retry};
    }

    ## wms_interval - (opt)
    if (exists $params->{wms_interval} && defined $params->{wms_interval} && $params->{wms_interval} ne '') {
        if (! COMMON::CheckUtils::isPositiveInt($params->{wms_interval})) {
            ERROR("Request retry interval delay must be a positive integer.");
            return FALSE;
        }
        $this->{interval} = $params->{wms_interval};
    }

    ## wms_user - (opt)
    if (exists $params->{wms_user} && defined $params->{wms_user} && $params->{wms_user} ne '') {
        $this->{user} = $params->{wms_user};
    }

    ## wms_password - (opt)
    if (exists $params->{wms_password} && defined $params->{wms_password} && $params->{wms_password} ne '') {
        $this->{password} = $params->{wms_password};
    }

    ## wms_referer - (opt)
    if (exists $params->{wms_referer} && defined $params->{wms_referer} && $params->{wms_referer} ne '') {
        $this->{referer} = $params->{wms_referer};
    }

    ## wms_userAgent - (opt)
    if (exists $params->{wms_userAgent} && defined $params->{wms_userAgent} && $params->{wms_userAgent} ne '') {
        $this->{userAgent} = $params->{wms_userAgent};
    }

    ## wms_crs - (opt)
    if (exists $params->{wms_crs} && defined $params->{wms_crs} && $params->{wms_crs} ne '') {
        $this->{crs} = $params->{wms_crs};
    }

    ## wms_format - (opt)
    if (exists $params->{wms_format} && defined $params->{wms_format} && $params->{wms_format} ne '') {
        if (! $this->isWmsFormat($params->{wms_format})) {
            return FALSE;
        }
        $this->{format} = $params->{wms_format};
    }  

    ## wms_option - (opt)
    if (exists $params->{wms_option} && defined $params->{wms_option} && $params->{wms_option} ne '') {
        $this->{option} = $params->{wms_option};
    }


    return TRUE;
}

####################################################################################################
#                                        Group: Tests                                              #
####################################################################################################

=begin nd
Function: isWmsFormat

Tests if format value is allowed.

Parameters (list):
    wmsformat - string - Format value to test
=cut
sub isWmsFormat {
    my $this = shift;
    my $wmsformat = shift;

    TRACE;

    return FALSE if (! defined $wmsformat);

    foreach (@{$WMS{format}}) {
        return TRUE if ($wmsformat eq $_);
    }
    ERROR (sprintf "Unknown 'wms_format' (%s).",$wmsformat);
    return FALSE;
}

=begin nd
Function: isWmsVersion

Tests if WMS version value is allowed.

Parameters (list):
    wmsversion - string - Version value to test
=cut
sub isWmsVersion {

    my $this = shift;
    my $wmsversion = shift;

    TRACE;

    if (! defined $wmsversion) {
        ERROR("Undefined 'wms_version'.");
        return FALSE;
    }

    foreach (@{$WMS{version}}) {
        return TRUE if ($wmsversion eq $_);
    }
    ERROR (sprintf "Unknown or unsupported 'wms_version' (%s).",$wmsversion);
    return FALSE;
}


####################################################################################################
#                                        Group: Output                                             #
####################################################################################################

=begin nd

Function: writeInXml

Writes the 'webService' element node in the pyramid descriptor file. This function needs to know where to write (see parameters).

Using:
    (start code)
    $wmsSource->writeInXml(xmlDocument, parentNode);
    (end code)

Parameters:
    xmlDocument - <XML::LibXML::Document> - The xml document where the 'webService' node will be written. (i.e. the interface for the descriptor file)
    parentNode - <XML::LibXML::Element> - The parent node where the 'webService' element will be nested. (ex: the 'sources' element node)

Returns:
    1 (TRUE) if success. 0 (FALSE) if an error occured.

=cut
sub writeInXml() {
    my $this = shift;
    my $xmlDoc = shift;
    my $sourcesNode = shift;

    my $webServiceEl = $xmlDoc->createElement("webService");
    $sourcesNode->appendChild($webServiceEl);
    $webServiceEl->appendTextChild("url", $this->{url});
    if (defined $this->{proxy}) { $webServiceEl->appendTextChild("proxy", $this->{proxy}); }
    if (defined $this->{timeout}) { $webServiceEl->appendTextChild("timeout", $this->{timeout}); }
    if (defined $this->{retry}) { $webServiceEl->appendTextChild("retry", $this->{retry}); }
    if (defined $this->{interval}) { $webServiceEl->appendTextChild("interval", $this->{interval}); }
    if (defined $this->{user}) { $webServiceEl->appendTextChild("user", $this->{user}); }
    if (defined $this->{password}) { $webServiceEl->appendTextChild("password", $this->{password}); }
    if (defined $this->{referer}) { $webServiceEl->appendTextChild("referer", $this->{referer}); }
    if (defined $this->{userAgent}) { $webServiceEl->appendTextChild("userAgent", $this->{userAgent}); }
    my $wmsEl = $xmlDoc->createElement("wms");
    $webServiceEl->appendChild($wmsEl);
    $wmsEl->appendTextChild("version", $this->{version});
    $wmsEl->appendTextChild("layers", $this->{layers});
    $wmsEl->appendTextChild("styles", $this->{styles});
    if (defined $this->{crs}) { $wmsEl->appendTextChild("crs", $this->{crs}); }
    if (defined $this->{format}) { $wmsEl->appendTextChild("format", $this->{format}); }
    $wmsEl->appendTextChild("channels", $this->{channels});
    $wmsEl->appendTextChild("noDataValue", $this->{nodata});
    my $boundingBoxEl = $xmlDoc->createElement("bbox");
    $wmsEl->appendChild($boundingBoxEl);
    my @boundingBox = split (",", $this->{extent});
    $boundingBoxEl->setAttribute("minx", $boundingBox[0]);
    $boundingBoxEl->setAttribute("miny", $boundingBox[1]);
    $boundingBoxEl->setAttribute("maxx", $boundingBox[2]);
    $boundingBoxEl->setAttribute("maxy", $boundingBox[3]);
    if (defined $this->{option}) { $wmsEl->appendTextChild("option", $this->{option}); }

    return TRUE;
}

1;