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

Class: BE4::GeoImage

Describes a georeferenced image and enable to know its components.

Using:
    (start code)
    use BE4::GeoImage;

    # GeoImage object creation
    my $objGeoImage = BE4::GeoImage->new("/home/ign/DATA/XXXXX_YYYYY.tif");
    (end code)

Attributes:
    completePath - string - Complete path (/home/ign/DATA/XXXXX_YYYYY.tif)
    filename - string - Just the image name, with file extension (XXXXX_YYYYY.tif).
    filepath - string - The directory which contain the image (/home/ign/DATA)
    maskCompletePath - string - Complete path of associated mask, if exists (undef otherwise).
    imgSrc - ImageSource - Image source to whom the image belong
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

package BE4::GeoImage;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Geo::OSR;
use Geo::GDAL;

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

See also:
    <_init>
=cut
sub new {
    my $this = shift;
    my $completePath = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        completePath => undef,
        filename => undef,
        filepath => undef,
        maskCompletePath => undef,
        imgSrc => undef,
        xmin => undef,
        ymax => undef,
        xmax => undef,
        ymin => undef,
        xres => undef,
        yres => undef,
        xcenter => undef,
        ycenter => undef,
        height  => undef,
        width   => undef,
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init($completePath));

    return $self;
}

=begin nd
Function: _init

Checks and stores file's informations.

Search a potential associated data mask : A file with the same base name but the extension *.msk*.

Parameters (list):
    completePath - string - Complete path to the image file.
=cut
sub _init {
    my $self   = shift;
    my $completePath = shift;

    TRACE;
    
    return FALSE if (! defined $completePath);
    
    if (! -f $completePath) {
      ERROR ("File doesn't exist !");
      return FALSE;
    }
    
    # init. params    
    $self->{completePath} = $completePath;
    
    #
    $self->{filepath} = File::Basename::dirname($completePath);
    $self->{filename} = File::Basename::basename($completePath);
    
    my $maskPath = $completePath;
    $maskPath =~ s/\.[a-zA-Z]+$/\.msk/;
    
    if (-f $maskPath) {
        INFO(sprintf "We have a mask associated to the image '%s' :\n\t%s",$completePath,$maskPath);
        $self->{maskCompletePath} = $maskPath;
    }
    
    return TRUE;
}

####################################################################################################
#                                Group: computing methods                                          #
####################################################################################################

=begin nd
Function: computeInfo

Extracts and calculates all GeoImage attributes' values, using GDAL library (see <Details>).

Image parameters are checked (sample per pixel, bits per sample...) and returned by the function. <ImageSource> can verify if all images own same components and the compatibility with be4's configuration.

Returns a list : (bitspersample,photometric,sampleformat,samplesperpixel), an empty list if error.
=cut
sub computeInfo {
    my $self = shift;

    my $image = $self->{filename};

    DEBUG(sprintf "compute '%s'", $image);

    my $dataset;
    eval { $dataset= Geo::GDAL::Open($self->{completePath}, 'ReadOnly'); };
    if ($@) {
        ERROR (sprintf "Can not open image ('%s') : '%s' !", $image, $@);
        return ();
    }

    my $driver = $dataset->GetDriver();
    my $code   = $driver->{ShortName};
    # FIXME : type of driver ?
    if ($code !~ /(GTiff|GeoTIFF)/) {
        ERROR (sprintf "This driver '%s' is not implemented ('%s') !", $code, $image);
        return ();
    }

    my $i = 0;

    my $DataType       = undef;
    my $Band           = undef;
    my @Interpretation;

    foreach my $objBand ($dataset->Bands()) {

        # FIXME undefined !
        # TRACE (sprintf "NoDataValue         :%s", $objBand->GetNoDataValue());
        # TRACE (sprintf "NoDataValue         :%s", $objBand->NoDataValue());

        # ie Float32,  GrayIndex,          , , .
        # ie Byte,     (Red|Green|Blue)Band, , .
        # ie Byte,     GrayIndex,          , , .
        # ie UInt32,   GrayIndex,          , , .
        # Byte, UInt16, Int16, UInt32, Int32, Float32, Float64, CInt16, CInt32, CFloat32, or CFloat64
        # Undefined GrayIndex PaletteIndex RedBand GreenBand BlueBand AlphaBand HueBand SaturationBand LightnessBand CyanBand MagentaBand YellowBand BlackBand

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

    my $refgeo = $dataset->GetGeoTransform();
    if (! defined ($refgeo) || scalar (@$refgeo) != 6) {
        ERROR ("Can not found parameters of image ('$image') !");
        return ();
    }

    # forced formatting string !
    my ($xmin, $dx, $rx, $ymax, $ry, $ndy)= @$refgeo;

    # FIXME : precision ?
    $self->{xmin} = sprintf "%.12f", $xmin;
    $self->{xmax} = sprintf "%.12f", $xmin + $dx*$dataset->{RasterXSize};
    $self->{ymin} = sprintf "%.12f", $ymax + $ndy*$dataset->{RasterYSize};
    $self->{ymax} = sprintf "%.12f", $ymax;
    $self->{xres} = sprintf "%.12f", $dx;      # $rx null ?
    $self->{yres} = sprintf "%.12f", abs($ndy);# $ry null ?
    $self->{xcenter}   = sprintf "%.12f", $xmin + $dx*$dataset->{RasterXSize}/2.0;
    $self->{ycenter}   = sprintf "%.12f", $ymax + $ndy*$dataset->{RasterYSize}/2.0;
    $self->{height} = $dataset->{RasterYSize};
    $self->{width}  = $dataset->{RasterXSize};

    if (! (defined $bitspersample && defined $photometric && defined $sampleformat && defined $samplesperpixel)) {
        ERROR ("The format of this image ('$image') is not handled by be4 !");
        return ();
    }
    
    return ($bitspersample,$photometric,$sampleformat,$samplesperpixel);
    
}

=begin nd
Function: convertBBox

Not just convert corners, but 7 points on each side, to determine reprojected bbox. Use OSR library.

Parameters (list):
    ct - <Geo::OSR::CoordinateTransformation> - To convert bbox. Can be undefined (no reprojection).

Returns the onverted (according to the given CoordinateTransformation) image bbox as a double array [xMin, yMin, xMax, yMax], [0,0,0,0] if error.
=cut
sub convertBBox {
  my $self = shift;
  my $ct = shift;
  
  TRACE;
  
  my @BBox = [0,0,0,0];

  if (! defined($ct)){
    $BBox[0] = Math::BigFloat->new($self->getXmin());
    $BBox[1] = Math::BigFloat->new($self->getYmin());
    $BBox[2] = Math::BigFloat->new($self->getXmax());
    $BBox[3] = Math::BigFloat->new($self->getYmax());
    DEBUG (sprintf "BBox (xmin,ymin,xmax,ymax) to '%s' : %s - %s - %s - %s", $self->getName(),
        $BBox[0], $BBox[1], $BBox[2], $BBox[3]);
    return @BBox;
  }
  # TODO:
  # Dans le cas où le SRS de la pyramide n'est pas le SRS natif des données, il faut reprojeter la bbox.
  # 1. L'algo trivial consiste à reprojeter les coins et prendre une marge de 20%.
  #    C'est rapide et simple mais comment on justifie les 20% ?
  
  # 2. on fait une densification fixe (ex: 5 pts par coté) et on prend une marge beaucoup plus petite.
  #    Ca reste relativement simple, mais on ne sait toujours pas quoi prendre comme marge.
  
  # 3. on fait une densification itérative avec calcul d'un majorant de l'erreur et on arrête quand on 
  #    a une erreur de moins d'un pixel. Puis on prend une marge d'un pixel. Cette fois ça peut prendre 
  #    du temps et l'algo commence à être compliqué.

  # methode 2.  
  # my ($xmin,$ymin,$xmax,$ymax);
  my $step = 7;
  my $dx= ($self->getXmax() - $self->getXmin())/(1.0*$step);
  my $dy= ($self->getYmax() - $self->getYmin())/(1.0*$step);
  my @polygon= ();
  for my $i (@{[0..$step-1]}) {
    push @polygon, [$self->getXmin()+$i*$dx, $self->getYmin()];
  }
  for my $i (@{[0..$step-1]}) {
    push @polygon, [$self->getXmax(), $self->getYmin()+$i*$dy];
  }
  for my $i (@{[0..$step-1]}) {
    push @polygon, [$self->getXmax()-$i*$dx, $self->getYmax()];
  }
  for my $i (@{[0..$step-1]}) {
    push @polygon, [$self->getXmin(), $self->getYmax()-$i*$dy];
  }

  my ($xmin_reproj, $ymin_reproj, $xmax_reproj, $ymax_reproj);
  for my $i (@{[0..$#polygon]}) {
    # FIXME: il faut absoluement tester les erreurs ici:
    #        les transformations WGS84G vers PM ne sont pas possible au dela de 85.05°.

    my $p = 0;
    eval { $p= $ct->TransformPoint($polygon[$i][0],$polygon[$i][1]); };
    if ($@) {
        ERROR($@);
        ERROR(sprintf "Impossible to transform point (%s,%s). Probably limits are reached !",$polygon[$i][0],$polygon[$i][1]);
        return [0,0,0,0];
    }

    if ($i==0) {
      $xmin_reproj= $xmax_reproj= @{$p}[0];
      $ymin_reproj= $ymax_reproj= @{$p}[1];
    } else {
      $xmin_reproj= @{$p}[0] if @{$p}[0] < $xmin_reproj;
      $ymin_reproj= @{$p}[1] if @{$p}[1] < $ymin_reproj;
      $xmax_reproj= @{$p}[0] if @{$p}[0] > $xmax_reproj;
      $ymax_reproj= @{$p}[1] if @{$p}[1] > $ymax_reproj;
    }
  }

  my $margeX = ($xmax_reproj - $xmin_reproj) * 0.02; # FIXME: la taille de la marge est arbitraire!!
  my $margeY = ($ymax_reproj - $ymax_reproj) * 0.02; # FIXME: la taille de la marge est arbitraire!!

  $BBox[0] = Math::BigFloat->new($xmin_reproj - $margeX);
  $BBox[1] = Math::BigFloat->new($ymin_reproj - $margeY);
  $BBox[2] = Math::BigFloat->new($xmax_reproj + $margeX);
  $BBox[3] = Math::BigFloat->new($ymax_reproj + $margeY);
  
  DEBUG (sprintf "BBox (xmin,ymin,xmax,ymax) to '%s' (with proj) : %s ; %s ; %s ; %s", $self->getName(), $BBox[0], $BBox[1], $BBox[2], $BBox[3]);
  
  return @BBox;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

=begin nd
Function: getInfo

Returns all image's geo informations in a list.

Example:
    (start code)
    # GeoImage information getter
    my (
        $filename,
        $filepath,
        $xmin,
        $ymax,
        $xmax,
        $ymin,
        $xres,
        $yres,
        $xcenter,
        $ycenter,
        $height,
        $width
    ) = $objGeoImage->getInfo();
    (end code)
=cut
sub getInfo {
    my $self = shift;

    TRACE;

    return (
        $self->{filename},
        $self->{filepath},
        $self->{xmin},
        $self->{ymax},
        $self->{xmax},
        $self->{ymin},
        $self->{xres},
        $self->{yres},
        $self->{xcenter},
        $self->{ycenter},
        $self->{height},
        $self->{width},
    );
}

=begin nd
Function: getBBox

Return the image's bbox as a double array [xMin, yMin, xMax, yMax], source SRS.
=cut
sub getBBox {
  my $self = shift;

  TRACE;
  
  my @bbox;

  push @bbox, ($self->{xmin},$self->{ymin},$self->{xmax},$self->{ymax});
  
  return @bbox;
}

# Function: setImageSource
sub setImageSource {
    my $self = shift;
    my $imgSrc = shift;

    if (! defined ($imgSrc) || ref ($imgSrc) ne "BE4::ImageSource") {
        ERROR("We expect to a BE4::ImageSource object.");
    } else {
        $self->{imgSrc} = $imgSrc;
    }
}

# Function: getXmin
sub getXmin {
  my $self = shift;
  return $self->{xmin};
}

# Function: getYmin
sub getYmin {
  my $self = shift;
  return $self->{ymin};
}

# Function: getXmax
sub getXmax {
  my $self = shift;
  return $self->{xmax};
}

# Function: getYmax
sub getYmax {
  my $self = shift;
  return $self->{ymax};
}

# Function: getXres
sub getXres {
  my $self = shift;
  return $self->{xres};  
}

# Function: getYres
sub getYres {
  my $self = shift;
  return $self->{yres};  
}

# Function: getName
sub getName {
  my $self = shift;
  return $self->{filename}; 
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
    my $self = shift;
    my $useMasks = shift;
    $useMasks = TRUE if (! defined $useMasks);

    TRACE;

    my $output = sprintf "IMG %s", $self->{completePath};

    if (defined $self->{imgSrc}) {
        $output .= sprintf "\t%s", $self->{imgSrc}->getSRS();
    }

    $output .= sprintf "\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $self->{xmin}, $self->{ymax}, $self->{xmax}, $self->{ymin},
        $self->{xres}, $self->{yres};
        
    if ($useMasks && defined $self->{maskCompletePath}) {
        $output .= sprintf "MSK %s\n", $self->{maskCompletePath};
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
