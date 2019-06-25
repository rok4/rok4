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

Class: COMMON::Harvesting

(see ROK4GENERATION/libperlauto/COMMON_Harvesting.png)

Stores parameters and builds WMS request.

Using:
    (start code)
    use COMMON::Harvesting;

    # Image width and height not defined

    # Harvesting object creation
    my $objHarvesting = COMMON::Harvesting->new({
        wms_layer   => "ORTHO_RAW_LAMB93_PARIS_OUEST",
        wms_url     => "http://localhost/wmts/rok4",
        wms_version => "1.3.0",
        wms_format  => "image/tiff"
    });

    OR

    # Image width and height defined, style, transparent and background color (for a WMS vector)

    # Harvesting object creation
    my $objHarvesting = COMMON::Harvesting->new({
        wms_layer   => "BDD_WLD_WM",
        wms_url     => "http://localhost/wmts/rok4",
        wms_version => "1.3.0",
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
    FORMAT - string - Parameter *FORMAT* of a WMS request : "image/tiff"
    LAYERS - string - Layer name to harvest, parameter *LAYERS* of a WMS request.
    OPTIONS - string - Contains style, background color and transparent parameters : STYLES=line&BGCOLOR=0xFFFFFF&TRANSPARENT=FALSE for example. If background color is defined, transparent must be 'FALSE'.
    min_size - integer - Used to remove too small harvested images (full of nodata), in bytes. Can be zero (no limit).
    max_width - integer - Max image's pixel width which will be harvested, can be undefined (no limit).
    max_height - integer - Max image's pixel height which will be harvested, can be undefined (no limit).

If *max_width* and *max_height* are not defined, images will be harvested all-in-one. If defined, requested image size have to be a multiple of this size.
=cut

################################################################################

package COMMON::Harvesting;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

use COMMON::Array;

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

# Constant: FORMATS
# Define allowed values for attribute wms format
my @FORMATS = ('image/png','image/tiff','image/jpeg','image/x-bil;bits=32','image/tiff&format_options=compression:deflate','image/tiff&format_options=compression:lzw','image/tiff&format_options=compression:packbits','image/tiff&format_options=compression:raw');

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
    my $class = shift;
    my $params = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $this = {
        URL      => undef,
        VERSION  => undef,
        FORMAT   => undef,
        LAYERS    => undef,
        OPTIONS    => "STYLES=",
        min_size => 0,
        max_width => undef,
        max_height => undef
    };

    bless($this, $class);


    # init. class
    return undef if (! $this->_init($params));

    return $this;
}

=begin nd
Function: _init

Checks and stores attributes' values.

Parameters (hash):
    wms_layer - string - Layer to harvest.
    wms_url - string - WMS server url.
    wms_version - string - WMS version.
    wms_format - string - Result's format.
    wms_bgcolor - string - Optionnal. Hexadecimal red-green-blue colour value for the background color (white = "0xFFFFFF").
    wms_transparent - boolean - Optionnal.
    wms_style - string - Optionnal.
    min_size - integer - Optionnal. 0 by default
    max_width - integer - Optionnal.
    max_height - integer - Optionnal.
=cut
sub _init {
    my $this   = shift;
    my $params = shift;
    
    return FALSE if (! defined $params);
    
    # 'max_width' and 'max_height' are optionnal, but if one is defined, the other must be defined
    if (exists($params->{max_width}) && defined ($params->{max_width})) {
        $this->{max_width} = $params->{max_width};
        if (exists($params->{max_height}) && defined ($params->{max_height})) {
            $this->{max_height} = $params->{max_height};
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
        $this->{OPTIONS} .= $params->{wms_style};
    }
        
    if (exists($params->{wms_bgcolor}) && defined ($params->{wms_bgcolor})) {
        if ($params->{wms_bgcolor} !~ m/^0x[a-fA-F0-9]{6}/) {
            ERROR("Parameter 'wms_bgcolor' must be to format '0x' + 6 numbers in hexadecimal format.");
            return FALSE ;
        }
        $this->{OPTIONS} .= "&BGCOLOR=".$params->{wms_bgcolor};
    }
    
    if (exists($params->{wms_transparent}) && defined ($params->{wms_transparent})) {
        if (uc($params->{wms_transparent}) ne "TRUE" && uc($params->{wms_transparent}) ne "FALSE") {
            ERROR(sprintf "Parameter 'wms_transparent' have to be 'TRUE' or 'FALSE' (%s).",$params->{wms_transparent});
            return FALSE ;
        }
        $this->{OPTIONS} .= "&TRANSPARENT=".uc($params->{wms_transparent});
    }
    
    if (exists($params->{min_size}) && defined ($params->{min_size})) {
        if (int($params->{min_size}) <= 0) {
            ERROR("If 'min_size' is given, it must be strictly positive.");
            return FALSE ;
        }
        $this->{min_size} = int($params->{min_size});
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
    # FORMAT
    if (! exists($params->{wms_format}) || ! defined ($params->{wms_format})) {
        ERROR("Parameter 'wms_format' is required !");
        return FALSE ;
    }
    if (! defined COMMON::Array::isInArray($params->{wms_format}, @FORMATS)) {
        ERROR (sprintf "Unknown 'wms_format' : %s !",$params->{wms_format});
        return undef;
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
    $this->{URL} = $params->{wms_url};
    $this->{VERSION} = $params->{wms_version};
    $this->{FORMAT} = $params->{wms_format};
    $this->{LAYERS} = $params->{wms_layer};

    return TRUE;
}

####################################################################################################
#                               Group: Request methods                                             #
####################################################################################################


=begin nd
Function: getHarvestUrl
=cut
sub getHarvestUrl {
    my $this = shift;

    my $srs = shift;
    my $width = shift;
    my $height = shift;

    my $w = $width;
    if (defined $this->{max_width} && $this->{max_width} < $width) {$w = $this->{max_width};}
    my $h = $height;
    if (defined $this->{max_height} && $this->{max_height} < $height) {$h = $this->{max_height};}

    return sprintf "http://%s?LAYERS=%s&SERVICE=WMS&VERSION=%s&REQUEST=GetMap&FORMAT=%s&CRS=%s&WIDTH=%s&HEIGHT=%s&%s",
        $this->getURL(), $this->getLayers(), $this->getVersion(), $this->getFormat(), $srs, $w, $h, $this->getOptions();
}

=begin nd
Function: getBboxesList

Determine bboxes' list to harvest, according to slab size, harvesting's limits and coordinates system particluarities

Return:
    A string array, bboxes as string, empty array if failure
=cut
sub getBboxesList {
    my $this = shift;

    my $xmin = shift;
    my $ymin = shift;
    my $xmax = shift;
    my $ymax = shift;
    my $width = shift;
    my $height = shift;
    my $inversion = shift;
    
    my $imagePerWidth = 1;
    my $imagePerHeight = 1;
    
    # Ground size of an harvested image
    my $groundHeight = $xmax-$xmin;
    my $groundWidth = $ymax-$ymin;
    
    if (defined $this->{max_width} && $this->{max_width} < $width) {
        if ($width % $this->{max_width} != 0) {
            ERROR(sprintf "Max harvested width (%s) is not a divisor of the image's width (%s) in the request." , $this->{max_width}, $width);
            return ();
        }
        $imagePerWidth = int($width/$this->{max_width});
        $groundWidth /= $imagePerWidth;
    }
    
    if (defined $this->{max_height} && $this->{max_height} < $height) {
        if ($height % $this->{max_height} != 0) {
            ERROR(sprintf "Max harvested height (%s) is not a divisor of the image's height (%s) in the request." ,$this->{max_height}, $height);
            return ();
        }
        $imagePerHeight = int($height/$this->{max_height});
        $groundHeight /= $imagePerHeight;
    }
    
    my @bboxes;
    for (my $i = 0; $i < $imagePerHeight; $i++) {
        for (my $j = 0; $j < $imagePerWidth; $j++) {
            if ($inversion) {
                push(
                    @bboxes,
                    sprintf ("%s,%s,%s,%s",
                        $ymax-($i+1)*$groundHeight, $xmin+$j*$groundWidth,
                        $ymax-$i*$groundHeight, $xmin+($j+1)*$groundWidth
                    )
                )
            } else {
                push(
                    @bboxes,
                    sprintf ("%s,%s,%s,%s",
                        $xmin+$j*$groundWidth, $ymax-($i+1)*$groundHeight,
                        $xmin+($j+1)*$groundWidth, $ymax-$i*$groundHeight
                    )
                )
            }
        }
    }
    
    return ("$imagePerWidth $imagePerHeight", @bboxes);
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getURL
sub getURL {
    my $this = shift;
    return $this->{URL};
}

# Function: getVersion
sub getVersion {
    my $this = shift;
    return $this->{VERSION};
}

# Function: getFormat
sub getFormat {
    my $this = shift;
    return $this->{FORMAT};
}

# Function: getMinSize
sub getMinSize {
    my $this = shift;
    return $this->{min_size};
}

# Function: getHarvestExtension
sub getHarvestExtension {
    my $this = shift;
    # Extension des images moissonnées
    if ($this->{FORMAT} eq "image/png") {
        return "png";
    } elsif ($this->{FORMAT} eq "image/jpeg") {
        return "jpeg";
    } else {
        return "tif";
    }
}

# Function: getLayers
sub getLayers {
    my $this = shift;
    return $this->{LAYERS};
}

# Function: getOptions
sub getOptions {
    my $this = shift;
    return $this->{OPTIONS};
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
    my $this = shift ;
    
    my $export = "";
    
    $export .= "\nObject COMMON::Harvesting :\n";
    $export .= sprintf "\t URL : %s\n",$this->{URL};
    $export .= sprintf "\t VERSION : %s\n",$this->{VERSION};
    $export .= sprintf "\t FORMAT : %s\n",$this->{FORMAT};
    $export .= sprintf "\t LAYERS : %s\n",$this->{LAYERS};
    $export .= sprintf "\t OPTIONS : %s\n",$this->{OPTIONS};

    $export .= "\t Limits : \n";
    $export .= sprintf "\t\t- Size min : %s\n",$this->{min_size};
    $export .= sprintf "\t\t- Max width (in pixel) : %s\n",$this->{max_width};
    $export .= sprintf "\t\t- Max height (in pixel) : %s\n",$this->{max_height};
    
    return $export;
}

1;
__END__
