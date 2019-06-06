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

(see ROK4GENERATION/libperlauto/WMTSALAD_WmsSource.png)

Using:
    (start code)
    use WMTSALAD::WmsSource;

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
    } );

    $wmsSource->writeInXml();    
    (end code)

Attributes:
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
    
=cut

################################################################################

package WMTSALAD::WmsSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use XML::LibXML;
use COMMON::Array;
use COMMON::CheckUtils;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constants
use constant TRUE  => 1;
use constant FALSE => 0;


# Constant: WMSVERSION
# Define allowed values for attribute wms_version
my @WMSVERSION = ('1.1.1', '1.3.0');

# Constant: WMSFORMAT
# Define allowed values for attribute wms_format
my @WMSFORMAT = ('image/png','image/jpeg');
# 'image/x-bil;bits=32',
# 'image/tiff',
# 'image/tiff&format_options=compression:deflate',
# 'image/tiff&format_options=compression:lzw',
# 'image/tiff&format_options=compression:packbits',
# 'image/tiff&format_options=compression:raw'

################################################################################

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################


=begin nd
Constructor: new

<WMTSALAD::WmsSource> constructor.    
=cut
sub new {
    my $class = shift;
    my $type = shift;
    my $params = shift;

    $class = ref($class) || $class;

    # IMPORTANT : if modification, think to update natural documentation (just above)
    # see config/pyramids/pyramid.xsd to get the list of parameters, as used by Rok4.

    $params->{type} = 'WMS';

    my $this = {
        url => undef,
        proxy => undef,
        timeout => undef,
        retry => undef,
        interval => undef,
        user => undef,
        password => undef,
        referer => undef,
        userAgent => undef,
        version => undef,
        layers => undef,
        styles => undef,
        crs => undef,
        format => undef,
        channels => undef,
        nodata => undef,
        extent => undef,
        option => undef
    };

    bless($this, $class);

    if ($type eq "XML") {
        if ( ! $this->_loadXML($params) ) {
            ERROR("Cannot load WmsSource from XML");
            return undef;
        }
    } else {
        if (! $this->_loadValues($params)) {
            ERROR ("Cannot load WmsSource from values");
            return undef;
        }
    }


    return $this;
}

=begin nd
Function: _loadValues    
=cut
sub _loadValues {
    my $this = shift;
    my $params = shift;

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
    } elsif (! defined COMMON::Array::isInArray($params->{wms_version}, @WMSVERSION)) {
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
        if (! defined COMMON::Array::isInArray($params->{wms_format}, @WMSFORMAT)) {
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


=begin nd
Function: _loadXML
    sourceRoot - <XML::LibXML::Element> - XML node of the WMS source (from the pyramid's descriptor)
=cut
sub _loadXML {
    my $this   = shift;
    my $sourceRoot = shift;

    # Mandatory

    $this->{url} = $sourceRoot->findvalue('url');
    if (! defined $this->{url} || $this->{url} eq "" ) {
        ERROR ("Cannot extract 'url' from the XML level source");
        return FALSE;
    }

    $this->{version} = $sourceRoot->findvalue('wms/version');
    if (! defined $this->{version} || $this->{version} eq "" ) {
        ERROR ("Cannot extract 'wms/version' from the XML level source");
        return FALSE;
    }

    $this->{layers} = $sourceRoot->findvalue('wms/layers');
    if (! defined $this->{layers} || $this->{layers} eq "" ) {
        ERROR ("Cannot extract 'wms/layers' from the XML level source");
        return FALSE;
    }

    $this->{styles} = $sourceRoot->findvalue('wms/styles');
    if (! defined $this->{styles} || $this->{styles} eq "" ) {
        ERROR ("Cannot extract 'wms/styles' from the XML level source");
        return FALSE;
    }

    $this->{channels} = $sourceRoot->findvalue('wms/channels');
    if (! defined $this->{channels} || $this->{channels} eq "" ) {
        ERROR ("Cannot extract 'wms/channels' from the XML level source");
        return FALSE;
    }

    $this->{nodata} = $sourceRoot->findvalue('wms/noDataValue');
    if (! defined $this->{nodata} || $this->{nodata} eq "" ) {
        ERROR ("Cannot extract 'wms/noDataValue' from the XML level source");
        return FALSE;
    }

    my $bboxNode = $sourceRoot->findnodes('wms/bbox')->[0];
    if (! defined $bboxNode) {
        ERROR ("Cannot extract node 'wms/bbox' from the XML level source");
        return FALSE;
    }
    $this->{extent} = sprintf "%s,%s,%s,%s",
        $bboxNode->getAttribute("minx"),$bboxNode->getAttribute("miny"),
        $bboxNode->getAttribute("maxx"),$bboxNode->getAttribute("maxy");

    # Optionnal

    $this->{proxy} = $sourceRoot->findvalue('proxy');
    $this->{timeout} = $sourceRoot->findvalue('timeout');
    $this->{retry} = $sourceRoot->findvalue('retry');
    $this->{interval} = $sourceRoot->findvalue('interval');
    $this->{user} = $sourceRoot->findvalue('user');
    $this->{password} = $sourceRoot->findvalue('password');
    $this->{referer} = $sourceRoot->findvalue('referer');
    $this->{userAgent} = $sourceRoot->findvalue('userAgent');

    $this->{crs} = $sourceRoot->findvalue('wms/crs');
    $this->{format} = $sourceRoot->findvalue('wms/format');
    $this->{option} = $sourceRoot->findvalue('wms/option');

    return TRUE;
}


####################################################################################################
#                                        Group: Output                                             #
####################################################################################################

=begin nd
Function: exportToXML
=cut
sub exportToXML {
    my $this = shift;

    my $string =       "            <webService>\n";
    $string .= sprintf "                <url>%s</url>\n", $this->{url};

    $string .= sprintf "                <proxy>%s</proxy>\n", $this->{proxy} if (defined $this->{proxy});
    $string .= sprintf "                <timeout>%s</timeout>\n", $this->{timeout} if (defined $this->{timeout});
    $string .= sprintf "                <retry>%s</retry>\n", $this->{retry} if (defined $this->{retry});
    $string .= sprintf "                <interval>%s</interval>\n", $this->{interval} if (defined $this->{interval});
    $string .= sprintf "                <user>%s</user>\n", $this->{user} if (defined $this->{user});
    $string .= sprintf "                <password>%s</password>\n", $this->{password} if (defined $this->{password});
    $string .= sprintf "                <referer>%s</referer>\n", $this->{referer} if (defined $this->{referer});
    $string .= sprintf "                <userAgent>%s</userAgent>\n", $this->{userAgent} if (defined $this->{userAgent});

    $string .= sprintf "                <wms>\n";
    $string .= sprintf "                    <version>%s</version>\n", $this->{version};
    $string .= sprintf "                    <layers>%s</layers>\n", $this->{layers};
    $string .= sprintf "                    <styles>%s</styles>\n", $this->{styles};
    $string .= sprintf "                    <channels>%s</channels>\n", $this->{channels};
    $string .= sprintf "                    <noDataValue>%s</noDataValue>\n", $this->{nodata};
    $string .= sprintf "                    <bbox minx=\"%s\" miny=\"%s\" maxx=\"%s\" maxy=\"%s\" />\n", split(/,/,$this->{extent});

    $string .= sprintf "                    <crs>%s</crs>\n", $this->{crs} if (defined $this->{crs});
    $string .= sprintf "                    <format>%s</format>\n", $this->{format} if (defined $this->{format});
    $string .= sprintf "                    <option>%s</option>\n", $this->{option} if (defined $this->{option});

    $string .= sprintf "                </wms>\n";
    $string .=         "            </webService>\n";

    return $string;
}

1;
__END__
