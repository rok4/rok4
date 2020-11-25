# Copyright © (2011) Institut national de l'information
#                    géographique et forestière 
# 
# Géoportail SAV <contact.geoservices@ign.fr>
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
File: GeoImage.pm

Class: COMMON::GeoImage

(see ROK4GENERATION/libperlauto/COMMON_GeoImage.png)

Describes a georeferenced image and enable to know its components.

Using:
    (start code)
    use COMMON::GeoImage;

    # GeoImage object creation
    my $objGeoImage = COMMON::GeoImage->new("/home/ign/DATA/XXXXX_YYYYY.tif");
    (end code)

Attributes:
    completePath - string - Complete path (/home/ign/DATA/XXXXX_YYYYY.tif)
    filename - string - Just the image name, with file extension (XXXXX_YYYYY.tif).
    filepath - string - The directory which contain the image (/home/ign/DATA)
    maskCompletePath - string - Complete path of associated mask, if exists (undef otherwise).
    srs - string - Projection of image
    xmin - double - Bottom left corner X coordinate.
    ymin - double - Bottom left corner Y coordinate.
    xmax - double - Top right corner X coordinate.
    ymax - double - Top right corner Y coordinate.
    xres - double - X wise resolution (in SRS unity).
    yres - double - Y wise resolution (in SRS unity).
    height - integer - Pixel height.
    width - integer - Pixel width.
    
=cut

################################################################################

package COMMON::GeoImage;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

use BE4::Shell;

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

GeoImage constructor. Bless an instance.

Parameters (list):
    completePath - string - Complete path to the image file.
    srs - string - Projection of georeferenced image

See also:
    <_init>
=cut
sub new {
    my $class = shift;
    my $completePath = shift;
    my $srs = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        completePath => undef,
        filename => undef,
        filepath => undef,
        maskCompletePath => undef,
        srs => undef,
        xmin => undef,
        ymax => undef,
        xmax => undef,
        ymin => undef,
        xres => undef,
        yres => undef,
        height  => undef,
        width   => undef
    };

    bless($this, $class);

    # init. class
    return undef if (! $this->_init($completePath, $srs));

    return $this;
}

=begin nd
Function: _init

Checks and stores file's informations.

Search a potential associated data mask : A file with the same base name but the extension *.msk*.

Parameters (list):
    completePath - string - Complete path to the image file.
    srs - string - Projection of georeferenced image
=cut
sub _init {
    my $this   = shift;
    my $completePath = shift;
    my $srs = shift;

    return FALSE if (! defined $completePath);
    return FALSE if (! defined $srs);

    $this->{srs} = $srs;
    
    if (! -f $completePath) {
        ERROR ("File doesn't exist !");
        return FALSE;
    }
    
    # init. params    
    $this->{completePath} = $completePath;
        
    my $maskPath = $completePath;
    $maskPath =~ s/\.[a-zA-Z0-9]+$/\.msk/;
    
    if (-f $maskPath) {
        INFO(sprintf "We have a mask associated to the image '%s' :\t%s",$completePath,$maskPath);
        $this->{maskCompletePath} = $maskPath;
    }
    
    #
    $this->{filepath} = File::Basename::dirname($completePath);
    $this->{filename} = File::Basename::basename($completePath);

    my $geo = COMMON::ProxyGDAL::getGeoreferencement($completePath);
    if (! defined $geo) {
        ERROR ("Cannot extract georeferencement from $completePath");
        return FALSE;        
    }

    $this->{xmin} = $geo->{bbox}->[0];
    $this->{ymin} = $geo->{bbox}->[1];
    $this->{xmax} = $geo->{bbox}->[2];
    $this->{ymax} = $geo->{bbox}->[3];

    $this->{xres} = $geo->{resolutions}->[0];
    $this->{yres} = $geo->{resolutions}->[1];

    $this->{width} = $geo->{dimensions}->[0];
    $this->{height} = $geo->{dimensions}->[1];

    return TRUE;
}

####################################################################################################
#                                Group: computing methods                                          #
####################################################################################################

=begin nd
Function: getImageInfo

Get image's pixel informations, using GDAL library (see <COMMON::ProxyGDAL::getPixel>).

Returns:
    a <COMMON::Pixel> object, undefined if failure
=cut
sub getImageInfo {
    my $this = shift;

    return COMMON::ProxyGDAL::getPixel($this->{completePath});    
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

=begin nd
Function: getBBox

Return the image's bbox as a double array [xMin, yMin, xMax, yMax], source SRS.
=cut
sub getBBox {
  my $this = shift;
  return ($this->{xmin},$this->{ymin},$this->{xmax},$this->{ymax});
}

# Function: setImagePath
sub setImagePath {
    my $this = shift;
    my $imagePath = shift;

    $this->{completePath} = $imagePath;
    $this->{filepath} = File::Basename::dirname($imagePath);
    $this->{filename} = File::Basename::basename($imagePath);
}


# Function: getXmin
sub getXmin {
  my $this = shift;
  return $this->{xmin};
}

# Function: getYmin
sub getYmin {
  my $this = shift;
  return $this->{ymin};
}

# Function: getXmax
sub getXmax {
  my $this = shift;
  return $this->{xmax};
}

# Function: getYmax
sub getYmax {
  my $this = shift;
  return $this->{ymax};
}

# Function: getXres
sub getXres {
  my $this = shift;
  return $this->{xres};  
}

# Function: getYres
sub getYres {
  my $this = shift;
  return $this->{yres};  
}

# Function: getName
sub getName {
  my $this = shift;
  return $this->{filename}; 
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForMntConf

Export a GeoImage object as a string. Output is formated to be used in <BE4::Node::mergeNtiff> configuration.

Example:
|    IMG completePath xmin ymax xmax ymin xres yres
|    MSK maskCompletePath
=cut
sub exportForMntConf {
    my $this = shift;

    my $output = sprintf "IMG %s\t%s", $this->{completePath}, $this->{srs};

    $output .= sprintf "\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $this->{xmin}, $this->{ymax}, $this->{xmax}, $this->{ymin},
        $this->{xres}, $this->{yres};
        
    if ($BE4::Shell::USEMASK && defined $this->{maskCompletePath}) {
        $output .= sprintf "MSK %s\n", $this->{maskCompletePath};
    }

    return $output;
}

=begin nd
Function: exportForDebug

Returns all informations about the georeferenced image. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= sprintf "\nObject BE4::GeoImage :\n";
    $export .= sprintf "\t Image path : %s\n",$self->{completePath};
    $export .= sprintf "\t Mask path : %s\n",$self->{maskCompletePath} if (defined $self->{maskCompletePath});

    $export .= "\t Dimensions (in pixel) :\n";
    $export .= sprintf "\t\t- width : %s\n",$self->{width};
    $export .= sprintf "\t\t- height : %s\n",$self->{height};
    
    $export .= "\t Resolution (in SRS unity) :\n";
    $export .= sprintf "\t\t- x : %s\n",$self->{xres};
    $export .= sprintf "\t\t- y : %s\n",$self->{yres};
    
    $export .= sprintf "\t Bbox (in SRS '%s' unity) :\n",$self->{imgSrc}->getSRS();
    $export .= sprintf "\t\t- xmin : %s\n",$self->{bbox}[0];
    $export .= sprintf "\t\t- ymin : %s\n",$self->{bbox}[1];
    $export .= sprintf "\t\t- xmax : %s\n",$self->{bbox}[2];
    $export .= sprintf "\t\t- ymax : %s\n",$self->{bbox}[3];
    
    return $export;
}

1;
__END__
