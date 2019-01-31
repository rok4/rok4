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
File: GeoImage.pm

Class: COMMON::GeoImage

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
    xcenter - double - Center X coordinate.
    ycenter - double - Center Y coordinate.
    height - integer - Pixel height.
    width - integer - Pixel width.
    
=cut

################################################################################

package COMMON::GeoImage;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Geo::GDAL;
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
        xcenter => undef,
        ycenter => undef,
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

    return TRUE;
}

####################################################################################################
#                                Group: computing methods                                          #
####################################################################################################

=begin nd
Function: computeImageInfo

Extracts and calculates all image attributes' values, using GDAL library (see <Details>).

Image parameters are checked (sample per pixel, bits per sample...) and returned by the function. <ImageSource> can verify if all images own same components and the compatibility with be4's configuration.

Returns:
    a list : (bitspersample,photometric,sampleformat,samplesperpixel), an empty list if error.
=cut
sub computeImageInfo {
    my $this = shift;

    my $image = $this->{filename};

    DEBUG(sprintf "compute '%s'", $image);

    my $dataset;
    eval { $dataset= Geo::GDAL::Open($this->{completePath}, 'ReadOnly'); };
    if ($@) {
        ERROR (sprintf "Can not open image ('%s') : '%s' !", $image, $@);
        return ();
    }

    my $driver = $dataset->GetDriver();
    my $code   = $driver->{ShortName};
    # FIXME : type of driver ?
    DEBUG (sprintf "use driver '%s'.", $code);

    my $i = 0;

    my $DataType       = undef;
    my $Band           = undef;
    my @Interpretation;

    foreach my $objBand ($dataset->Bands()) {

        push @Interpretation, lc $objBand->ColorInterpretation();

        if (!defined $DataType) {
            $DataType = lc $objBand->DataType();
        } else {
            if (! (lc $objBand->DataType() eq $DataType)) {
                ERROR (sprintf "DataType is not the same (%s and %s) for all band in this image !", lc $objBand->DataType(), $DataType);
                return ();
            }
        }
        
        $i++;
    }

    $Band = $i;

    my $bitspersample = undef;
    my $photometric = undef;
    my $sampleformat = undef;
    my $samplesperpixel = undef;

    if ($DataType eq "byte") {
        $bitspersample = 8;
        $sampleformat  = "uint";
    }
    else {
        ($sampleformat, $bitspersample) = ($DataType =~ /(\w+)(\d{2})/);
    }

    if ($Band == 3) {
        foreach (@Interpretation) {
            last if ($_ !~ m/(red|green|blue)band/);
        }
        $photometric     = "rgb";
        $samplesperpixel = 3;
    }

    if ($Band == 4) {
        foreach (@Interpretation) {
            last if ($_ !~ m/(red|green|blue|alpha)band/);
        }
        $photometric     = "rgb";
        $samplesperpixel = 4;
    }

    if ($Band == 1) {
        if ($Interpretation[0] eq "grayindex") {
            $photometric     = "gray";
            $samplesperpixel = 1;
        }
        if ($Interpretation[0] eq "paletteindex") {
            $photometric     = "gray";
            $samplesperpixel = 1;
            $bitspersample = 1;
        }
    }

    DEBUG(sprintf "Format image : bps %s, photo %s, sf %s, spp %s", $bitspersample, $photometric, $sampleformat, $samplesperpixel);

    if (! (defined $bitspersample && defined $photometric && defined $sampleformat && defined $samplesperpixel)) {
        ERROR ("The format of this image ('$image') is not handled by be4 !");
        return ();
    }
    
    return ($bitspersample,$photometric,$sampleformat,$samplesperpixel);
    
}

=begin nd
Function: computeGeometryInfo

Extracts and calculates all geometric attributes' values, using GDAL library (see <Details>).

Returns:
    TRUE if success, FALSE if error.
=cut
sub computeGeometryInfo {
    my $this = shift;

    my $image = $this->{filename};

    DEBUG(sprintf "compute '%s'", $image);

    my $dataset;
    eval { $dataset= Geo::GDAL::Open($this->{completePath}, 'ReadOnly'); };
    if ($@) {
        ERROR (sprintf "Can not open image ('%s') : '%s' !", $image, $@);
        return FALSE;
    }

    my $driver = $dataset->GetDriver();
    my $code   = $driver->{ShortName};
    # FIXME : type of driver ?
    DEBUG (sprintf "use driver '%s'.", $code);

    my $refgeo = $dataset->GetGeoTransform();
    if (! defined ($refgeo) || scalar (@$refgeo) != 6) {
        ERROR ("Can not found geometric parameters of image ('$image') !");
        return FALSE;
    }

    # forced formatting string !
    my ($xmin, $dx, $rx, $ymax, $ry, $ndy)= @$refgeo;

    # FIXME : precision ?
    $this->{xmin} = sprintf "%.12f", $xmin;
    $this->{xmax} = sprintf "%.12f", $xmin + $dx*$dataset->{RasterXSize};
    $this->{ymin} = sprintf "%.12f", $ymax + $ndy*$dataset->{RasterYSize};
    $this->{ymax} = sprintf "%.12f", $ymax;
    $this->{xres} = sprintf "%.12f", $dx;      # $rx null ?
    $this->{yres} = sprintf "%.12f", abs($ndy);# $ry null ?
    $this->{xcenter}   = sprintf "%.12f", $xmin + $dx*$dataset->{RasterXSize}/2.0;
    $this->{ycenter}   = sprintf "%.12f", $ymax + $ndy*$dataset->{RasterYSize}/2.0;
    $this->{height} = $dataset->{RasterYSize};
    $this->{width}  = $dataset->{RasterXSize};
    
    return TRUE;
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
  
  my @bbox;

  push @bbox, ($this->{xmin},$this->{ymin},$this->{xmax},$this->{ymax});
  
  return @bbox;
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

Export a GeoImage object as a string. Output is formated to be used in <Commands::mergeNtiff> configuration.

Example:
|    IMG completePath xmin ymax xmax ymin xres yres
|    MSK maskCompletePath

Parameter:
    useMasks - boolean - Specify if we want to export mask (if present). TRUE by default.
=cut
sub exportForMntConf {
    my $this = shift;
    my $useMasks = shift;
    $useMasks = TRUE if (! defined $useMasks);

    my $output = sprintf "IMG %s\t%s", $this->{completePath}, $this->{srs};

    $output .= sprintf "\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $this->{xmin}, $this->{ymax}, $this->{xmax}, $this->{ymin},
        $this->{xres}, $this->{yres};
        
    if ($useMasks && defined $this->{maskCompletePath}) {
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

=begin nd

Group: Details

Use the binding Perl of Gdal

Sample with gdalinfo:
    (start code)
    GDAL 1.7.2, released 2010/04/23

    ~$ gdalinfo  image.png

    Driver: PNG/Portable Network Graphics
    Files: image.png
    Size is 316, 261
    Coordinate System is `'
    Metadata:
    Software=Shutter
    Image Structure Metadata:
    INTERLEAVE=PIXEL
    Corner Coordinates:
    Upper Left  (    0.0,    0.0)
    Lower Left  (    0.0,  261.0)
    Upper Right (  316.0,    0.0)
    Lower Right (  316.0,  261.0)
    Center      (  158.0,  130.5)
    Band 1 Block=316x1 Type=Byte, ColorInterp=Red
    Band 2 Block=316x1 Type=Byte, ColorInterp=Green
    Band 3 Block=316x1 Type=Byte, ColorInterp=Blue


    ~$ gdalinfo  image.tif
    (...)
    Metadata:
    TIFFTAG_IMAGEDESCRIPTION=
    TIFFTAG_SOFTWARE=Adobe Photoshop CS2 Windows
    TIFFTAG_DATETIME=2007:03:15 11:17:17
    TIFFTAG_XRESOLUTION=600
    TIFFTAG_YRESOLUTION=600
    TIFFTAG_RESOLUTIONUNIT=2 (pixels/inch)
    Image Structure Metadata:
    INTERLEAVE=PIXEL
    (...)
    Upper Left  (  440720.000, 3751320.000) (117d38'28.21"W, 33d54'8.47"N)
    Lower Left  (  440720.000, 3720600.000) (117d38'20.79"W, 33d37'31.04"N)
    Upper Right (  471440.000, 3751320.000) (117d18'32.07"W, 33d54'13.08"N)
    Lower Right (  471440.000, 3720600.000) (117d18'28.50"W, 33d37'35.61"N)
    Center      (  456080.000, 3735960.000) (117d28'27.39"W, 33d45'52.46"N)

    ~$ gdalinfo image.tif

    Driver: GTiff/GeoTIFF
    Files: image.tif
         image.tfw
    Size is 5000, 5000
    Coordinate System is `'
    Origin = (937500.000000000000000,6541000.000000000000000)
    Pixel Size = (0.100000000000000,-0.100000000000000)
    Metadata:
    TIFFTAG_XRESOLUTION=100
    TIFFTAG_YRESOLUTION=100
    TIFFTAG_RESOLUTIONUNIT=3 (pixels/cm)
    Image Structure Metadata:
    INTERLEAVE=PIXEL
    Corner Coordinates:
    Upper Left  (  937500.000, 6541000.000) 
    Lower Left  (  937500.000, 6540500.000) 
    Upper Right (  938000.000, 6541000.000) 
    Lower Right (  938000.000, 6540500.000) 
    Center      (  937750.000, 6540750.000) 
    Band 1 Block=5000x1 Type=Byte, ColorInterp=Red
    Band 2 Block=5000x1 Type=Byte, ColorInterp=Green
    Band 3 Block=5000x1 Type=Byte, ColorInterp=Blue
    (end code)

=cut
