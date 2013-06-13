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
File: Harvesting.pm

Class: BE4::Harvesting

Stores parameters and builds WMS request.

Using:
    (start code)
    use BE4::Harvesting;

    # Image width and height not defined

    # Harvesting object creation
    my $objHarvesting = BE4::Harvesting->new({
        wms_layer   => "ORTHO_RAW_LAMB93_PARIS_OUEST",
        wms_url     => "http://localhost/wmts/rok4",
        wms_version => "1.3.0",
        wms_request => "getMap",
        wms_format  => "image/tiff"
    });

    OR

    # Image width and height defined, style, transparent and background color (for a WMS vector)

    # Harvesting object creation
    my $objHarvesting = BE4::Harvesting->new({
        wms_layer   => "BDD_WLD_WM",
        wms_url     => "http://localhost/wmts/rok4",
        wms_version => "1.3.0",
        wms_request => "getMap",
        wms_format  => "image/png",
        wms_bgcolor => "0xFFFFFF",
        wms_transparent  => "FALSE",
        wms_style  => "line",
        max_width  => 1024,
        max_height  => 1024
    });
    (end code)

Attributes:
    URL - string -  Left part of a WMS request, before the *?*.
    VERSION - string - Parameter *VERSION* of a WMS request : "1.3.0".
    REQUEST - string - Parameter *REQUEST* of a WMS request : "getMap"
    FORMAT - string - Parameter *FORMAT* of a WMS request : "image/tiff"
    LAYERS - string - Layer name to harvest, parameter *LAYERS* of a WMS request.
    OPTIONS - string - Contains style, background color and transparent parameters : STYLES=line&BGCOLOR=0xFFFFFF&TRANSPARENT=FALSE for example. If background color is defined, transparent must be 'FALSE'.
    min_size - integer - Used to remove too small harvested images (full of nodata), in bytes. Can be zero (no limit).
    max_width - integer - Max image's pixel width which will be harvested, can be undefined (no limit).
    max_height - integer - Max image's pixel height which will be harvested, can be undefined (no limit).

If *max_width* and *max_height* are not defined, images will be harvested all-in-one. If defined, requested image size have to be a multiple of this size.
=cut

################################################################################

package BE4::Harvesting;

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

# Constant: WMS
# Define allowed values for attribute wms_format.
my %WMS;

# Constant: DEFAULT
# Define default values for attributes.
my %DEFAULT;

################################################################################

BEGIN {}
INIT {
    %WMS = (
        wms_format => ['image/png','image/tiff','image/x-bil;bits=32','image/tiff&format_options=compression:deflate','image/tiff&format_options=compression:lzw','image/tiff&format_options=compression:packbits','image/tiff&format_options=compression:raw'],
    );

    %DEFAULT = (
        min_size => 0,
    );
}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Harvesting constructor. Bless an instance.

Parameters (hash):
    wms_layer - string - Layer to harvest.
    wms_url - string - WMS server url.
    wms_version - string - WMS version.
    wms_request - string - Request's type.
    wms_format - string - Result's format.
    wms_bgcolor - string - Optionnal. Hexadecimal red-green-blue colour value for the background color (white = "0xFFFFFF").
    wms_transparent - boolean - Optionnal.
    wms_style - string - Optionnal.
    min_size - integer - Optionnal. 0 by default
    max_width - integer - Optionnal.
    max_height - integer - Optionnal.
See also:
    <_init>
=cut
sub new {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        URL      => undef,
        VERSION  => undef,
        REQUEST  => undef,
        FORMAT   => undef,
        LAYERS    => undef,
        OPTIONS    => "STYLES=",
        min_size => undef,
        max_width => undef,
        max_height => undef
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init($params));

    return $self;
}

=begin nd
Function: _init

Checks and stores attributes' values.

Parameters (hash):
    wms_layer - string - Layer to harvest.
    wms_url - string - WMS server url.
    wms_version - string - WMS version.
    wms_request - string - Request's type.
    wms_format - string - Result's format.
    wms_bgcolor - string - Optionnal. Hexadecimal red-green-blue colour value for the background color (white = "0xFFFFFF").
    wms_transparent - boolean - Optionnal.
    wms_style - string - Optionnal.
    min_size - integer - Optionnal. 0 by default
    max_width - integer - Optionnal.
    max_height - integer - Optionnal.
=cut
sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # 'max_width' and 'max_height' are optionnal, but if one is defined, the other must be defined
    if (exists($params->{max_width}) && defined ($params->{max_width})) {
        $self->{max_width} = $params->{max_width};
        if (exists($params->{max_height}) && defined ($params->{max_height})) {
            $self->{max_height} = $params->{max_height};
        } else {
            ERROR("If parameter 'max_width' is defined, parameter 'max_height' must be defined !");
            return FALSE ;
        }
    } else {
        if (exists($params->{max_height}) && defined ($params->{max_height})) {
            ERROR("If parameter 'max_height' is defined, parameter 'max_width' must be defined !");
            return FALSE ;
        }
    }
    
    # OPTIONS
    if (exists($params->{wms_style}) && defined ($params->{wms_style})) {
        # "STYLES=" is always present in the options
        $self->{OPTIONS} .= $params->{wms_style};
    }
    
    my $hasBGcolor = FALSE;
    
    if (exists($params->{wms_bgcolor}) && defined ($params->{wms_bgcolor})) {
        if ($params->{wms_bgcolor} !~ m/^0x[a-fA-F0-9]{6}/) {
            ERROR("Parameter 'wms_bgcolor' must be to format '0x' + 6 numbers in hexadecimal format.");
            return FALSE ;
        }
        $self->{OPTIONS} .= "&BGCOLOR=".$params->{wms_bgcolor};
        $hasBGcolor = TRUE;
    }
    
    if (exists($params->{wms_transparent}) && defined ($params->{wms_transparent})) {
        if (uc($params->{wms_transparent}) ne "TRUE" && uc($params->{wms_transparent}) ne "FALSE") {
            ERROR(sprintf "Parameter 'wms_transparent' have to be 'TRUE' or 'FALSE' (%s).",$params->{wms_transparent});
            return FALSE ;
        }
        $self->{OPTIONS} .= "&TRANSPARENT=".uc($params->{wms_bgcolor});
    }
    
    if (! exists($params->{min_size}) || ! defined ($params->{min_size})) {
        $self->{min_size} = $DEFAULT{min_size};
        INFO(sprintf "Default value for 'min_size' : %s", $self->{min_size});
    } else {
        if (int($params->{min_size}) <= 0) {
            ERROR("If 'min_size' is given, it must be strictly positive.");
            return FALSE ;
        }
        $self->{min_size} = int($params->{min_size});
    }

    # Other parameters are mandatory
    # URL
    if (! exists($params->{wms_url}) || ! defined ($params->{wms_url})) {
        ERROR("Parameter 'wms_url' is required !");
        return FALSE ;
    }
    $params->{wms_url} =~ s/http:\/\///;
    # VERSION
    if (! exists($params->{wms_version}) || ! defined ($params->{wms_version})) {
        ERROR("Parameter 'wms_version' is required !");
        return FALSE ;
    }
    # REQUEST
    if (! exists($params->{wms_request}) || ! defined ($params->{wms_request})) {
        ERROR("Parameter 'wms_request' is required !");
        return FALSE ;
    }
    # FORMAT
    if (! exists($params->{wms_format}) || ! defined ($params->{wms_format})) {
        ERROR("Parameter 'wms_format' is required !");
        return FALSE ;
    }
    if (! $self->isWmsFormat($params->{wms_format})) {
        ERROR("Parameter 'wms_format' is not valid !");
        return FALSE ;
    }
    # LAYER
    if (! exists($params->{wms_layer}) || ! defined ($params->{wms_layer})) {
        ERROR("Parameter 'wms_layer' is required !");
        return FALSE ;
    }

    $params->{wms_layer} =~ s/ //;
    my @layers = split (/,/,$params->{wms_layer},-1);
    foreach my $layer (@layers) {
        if ($layer eq '') {
            ERROR(sprintf "Value for 'wms_layer' is not valid (%s) : it must be LAYER[{,LAYER_N}] !",$params->{wms_layer});
            return FALSE ;
        }
        INFO(sprintf "Layer %s will be harvested !",$layer);
    }
    
    # init. params    
    $self->{URL} = $params->{wms_url};
    $self->{VERSION} = $params->{wms_version};
    $self->{REQUEST} = $params->{wms_request};
    $self->{FORMAT} = $params->{wms_format};
    $self->{LAYERS} = $params->{wms_layer};

    return TRUE;
}

####################################################################################################
#                               Group: Request methods                                             #
####################################################################################################

=begin nd
Function: doRequestUrl

From an bbox, determine the request to send to obtain what we want.

Parameters (hash):
    srs - string - Bounding box's SRS
    inversion - boolean - To know if we have to reverse coordinates in the request.
    bbox - double array - Extent of the harvested image
    width - integer - Pixel width of the harvested image
    height - integer - Pixel height of the harvested image

Example:
    (start code)
    # Do a request
    my $request = $objHarvesting->doRequestUrl(
        inversion => TRUE,
        srs => "EPSG:4326",
        bbox => [5,47,6,48],
        width => 4096,
        height => 4096
    );
    
    # $request =
    # http://http://localhost/wmts/rok4?LAYERS=ORTHO_RAW_LAMB93_PARIS_OUEST&SERVICE=WMS&VERSION=1.3.0&
    # REQUEST=getMap&FORMAT=image/tiff&CRS=EPSG:4326&BBOX=47,5,48,6&WIDTH=4096&HEIGHT=4096&STYLES=
    (end code)
=cut
sub doRequestUrl {
    my $self = shift;

    my $args = shift;

    TRACE;

    my $srs       = $args->{srs}          || ( ERROR ("'srs' parameter required !") && return undef );
    my $bbox      = $args->{bbox}         || ( ERROR ("'bbox' parameter required !") && return undef );
    my $image_width = $args->{width} || ( ERROR ("'width' parameter required !") && return undef );
    my $image_height = $args->{height} || ( ERROR ("'height' parameter required !") && return undef );

    my $inversion = $args->{inversion};
    if (! defined $inversion) {
        ERROR ("'inversion' parameter required !");
        return undef;
    }

    my ($xmin, $ymin, $xmax, $ymax)  = @{$bbox};

    my $url = sprintf ("http://%s?LAYERS=%s&SERVICE=WMS&VERSION=%s&REQUEST=%s&FORMAT=%s&CRS=%s",
                        $self->getURL(),
                        $self->getLayers(),
                        $self->getVersion(),
                        $self->getRequest(),
                        $self->getFormat(),
                        $srs);

    if ($inversion) {
        $url .= sprintf ("&BBOX=%s,%s,%s,%s", $ymin, $xmin, $ymax, $xmax);
    } else {
        $url .= sprintf ("&BBOX=%s,%s,%s,%s", $xmin, $ymin, $xmax, $ymax);
    }

    $url .= sprintf ("&WIDTH=%s&HEIGHT=%s&%s", $image_width, $image_height, $self->getOptions);

    return $url;
}

=begin nd
Function: getCommandWms2work

Compose the BBoxes' array and the Wms2work call (bash function), used to obtain wanted image.

Parameters:
    dir - string - directory, to know the final image location and where to write temporary images
    srs - string - Bounding box's SRS
    inversion - boolean - To know if we have to reverse coordinates in the request.
    bbox - double array - Extent of the harvested image
    width - integer - Pixel width of the harvested image
    height - integer - Pixel height of the harvested image

Example:
    (start code)
    # Obtain a "Wms2work" command
    my $cmd = $objHarvesting->getCommandWms2work(
        dir => "path/image_several_requests",
        inversion => FALSE,
        srs => "WGS84",
        bbox => [10018754.17139461632,-2504688.54284865024,12523442.71424327168,0.00000000512],
        width => 4096,
        height =>4096
    );

    # $cmd =
    # BBOXES="10018754.17139461632,-626172.13571215872,10644926.30710678016,0.00000000512
    # 10644926.30710678016,-626172.13571215872,11271098.442818944,0.00000000512
    # 11271098.442818944,-626172.13571215872,11897270.57853110784,0.00000000512
    # 11897270.57853110784,-626172.13571215872,12523442.71424327168,0.00000000512
    # 10018754.17139461632,-1252344.27142432256,10644926.30710678016,-626172.13571215872
    # 10644926.30710678016,-1252344.27142432256,11271098.442818944,-626172.13571215872
    # 11271098.442818944,-1252344.27142432256,11897270.57853110784,-626172.13571215872
    # 11897270.57853110784,-1252344.27142432256,12523442.71424327168,-626172.13571215872
    # 10018754.17139461632,-1878516.4071364864,10644926.30710678016,-1252344.27142432256
    # 10644926.30710678016,-1878516.4071364864,11271098.442818944,-1252344.27142432256
    # 11271098.442818944,-1878516.4071364864,11897270.57853110784,-1252344.27142432256
    # 11897270.57853110784,-1878516.4071364864,12523442.71424327168,-1252344.27142432256
    # 10018754.17139461632,-2504688.54284865024,10644926.30710678016,-1878516.4071364864
    # 10644926.30710678016,-2504688.54284865024,11271098.442818944,-1878516.4071364864
    # 11271098.442818944,-2504688.54284865024,11897270.57853110784,-1878516.4071364864
    # 11897270.57853110784,-2504688.54284865024,12523442.71424327168,-1878516.4071364864"
    #
    # Wms2work "path/image_several_requests" "png" "1024x1024" "4x4" "250000" "http://localhost/wms-vector?LAYERS=BDD_WLD_WM&SERVICE=WMS&VERSION=1.3.0&REQUEST=getMap&FORMAT=image/png&CRS=EPSG:3857&WIDTH=1024&HEIGHT=1024&STYLES=line&BGCOLOR=0x80BBDA&TRANSPARENT=0X80BBDA" $BBOXES
    (end code)
=cut
sub getCommandWms2work {
    my $self = shift;

    my $args = shift;

    TRACE;

    my $dir = $args->{dir} || ( ERROR ("'dir' parameter required !") && return undef );
    my $srs = $args->{srs} || ( ERROR ("'srs' parameter required !") && return undef );
    my $bbox = $args->{bbox} || ( ERROR ("'bbox' parameter required !") && return undef );
    my $max_width = $args->{width} || ( ERROR ("'width' parameter required !") && return undef );
    my $max_height = $args->{height} || ( ERROR ("'height' parameter required !") && return undef );
    
    my $inversion = $args->{inversion};
    if (! defined $inversion) {
        ERROR ("'inversion' parameter required !");
        return undef;
    }
    
    my ($xmin, $ymin, $xmax, $ymax) = @$bbox;
    
    my $imagePerWidth = 1;
    my $imagePerHeight = 1;
    
    # Ground size of an harvested image
    my $groundHeight = $xmax-$xmin;
    my $groundWidth = $ymax-$ymin;
    
    if (defined $self->{max_width} && $self->{max_width} < $max_width) {
        if ($max_width % $self->{max_width} != 0) {
            ERROR(sprintf "Max harvested width (%s) is not a divisor of the image's width (%s) in the request."
                  ,$self->{max_width},$max_width);
            return undef;
        }
        $imagePerWidth = int($max_width/$self->{max_width});
        $groundWidth /= $imagePerWidth;
        $max_width = $self->{max_width};
    }
    
    if (defined $self->{max_height} && $self->{max_height} < $max_height) {
        if ($max_height % $self->{max_height} != 0) {
            ERROR(sprintf "Max harvested height (%s) is not a divisor of the image's height (%s) in the request."
                  ,$self->{max_height},$max_height);
            return undef;
        }
        $imagePerHeight = int($max_height/$self->{max_height});
        $groundHeight /= $imagePerHeight;
        $max_height = $self->{max_height};
    }
    
    my $URL = sprintf ("http://%s?LAYERS=%s&SERVICE=WMS&VERSION=%s&REQUEST=%s&FORMAT=%s&CRS=%s&WIDTH=%s&HEIGHT=%s&%s",
                    $self->getURL, $self->getLayers, $self->getVersion, $self->getRequest, $self->getFormat,
                    $srs, $max_width, $max_height, $self->getOptions);
    my $BBoxesAsString = "\"";
    for (my $i = 0; $i < $imagePerWidth; $i++) {
        for (my $j = 0; $j < $imagePerHeight; $j++) {
            if ($inversion) {
                $BBoxesAsString .= sprintf "%s,%s,%s,%s\n",
                    $ymax-($i+1)*$groundHeight, $xmin+$j*$groundWidth,
                    $ymax-$i*$groundHeight, $xmin+($j+1)*$groundWidth;
            } else {
                $BBoxesAsString .= sprintf "%s,%s,%s,%s\n",
                    $xmin+$j*$groundWidth, $ymax-($i+1)*$groundHeight,
                    $xmin+($j+1)*$groundWidth, $ymax-$i*$groundHeight;
            }
        }
    }
    $BBoxesAsString .= "\"";
    
    my $cmd = "BBOXES=$BBoxesAsString\n";
    
    $cmd .= "Wms2work";
    $cmd .= " \"$dir\"";
    if ($self->getFormat eq "image/png") {
        $cmd .= " \"png\"";
    } else {
        $cmd .= " \"tif\"";
    }
    $cmd .= sprintf " \"%sx%s\"",$max_width,$max_height;
    $cmd .= sprintf " \"%sx%s\"",$imagePerWidth,$imagePerHeight;

    $cmd .= sprintf " \"%s\"",$self->{min_size};

    $cmd .= " \"$URL\"";
    $cmd .= " \$BBOXES\n";
    
    return $cmd;
}

####################################################################################################
#                             Group: Attributes' testers                                           #
####################################################################################################

=begin nd
Function: isWmsFormat

Tests if format value is allowed.

Parameters (list):
    wmsformat - string - Format value to test
=cut
sub isWmsFormat {
    my $self = shift;
    my $wmsformat = shift;

    TRACE;

    return FALSE if (! defined $wmsformat);

    foreach (@{$WMS{wms_format}}) {
        return TRUE if ($wmsformat eq $_);
    }
    ERROR (sprintf "Unknown 'wms_format' (%s) !",$wmsformat);
    return FALSE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getURL
sub getURL {
    my $self = shift;
    return $self->{URL};
}

# Function: getVersion
sub getVersion {
    my $self = shift;
    return $self->{VERSION};
}

# Function: getRequest
sub getRequest {
  my $self = shift;
  return $self->{REQUEST};
}

# Function: getFormat
sub getFormat {
    my $self = shift;
    return $self->{FORMAT};
}

# Function: getLayers
sub getLayers {
    my $self = shift;
    return $self->{LAYERS};
}

# Function: getOptions
sub getOptions {
    my $self = shift;
    return $self->{OPTIONS};
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all harvesting's components. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject BE4::Harvesting :\n";
    $export .= sprintf "\t URL : %s\n",$self->{URL};
    $export .= sprintf "\t VERSION : %s\n",$self->{VERSION};
    $export .= sprintf "\t REQUEST : %s\n",$self->{REQUEST};
    $export .= sprintf "\t FORMAT : %s\n",$self->{FORMAT};
    $export .= sprintf "\t LAYERS : %s\n",$self->{LAYERS};
    $export .= sprintf "\t OPTIONS : %s\n",$self->{OPTIONS};

    $export .= "\t Limits : \n";
    $export .= sprintf "\t\t- Size min : %s\n",$self->{min_size};
    $export .= sprintf "\t\t- Max width (in pixel) : %s\n",$self->{max_width};
    $export .= sprintf "\t\t- Max height (in pixel) : %s\n",$self->{max_height};
    
    return $export;
}

1;
__END__
