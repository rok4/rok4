package BE4::ImageSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

use Geo::GDAL;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# version
our $VERSION = '0.0.1';

################################################################################
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

#
# Group: variable
#

#
# variable: $self
#
#    *    PATHFILENAME => undef,
#    *    filename => undef,
#    *    filepath => undef,
#    *    xmin => undef,
#    *    ymax => undef,
#    *    xmax => undef,
#    *    ymin => undef,
#    *    xres => undef,
#    *    yres => undef,
#    *    pixelsize => undef,
#    *    xcenter => undef,
#    *    ycenter => undef,
#    *    height  => undef,
#    *    width   => undef,
#

#
# Group: constructor
#

################################################################################
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    PATHFILENAME => undef,
    #
    filename => undef,
    filepath => undef,
    xmin => undef,
    ymax => undef,
    xmax => undef,
    ymin => undef,
    xres => undef,
    yres => undef,
    pixelsize => undef,
    xcenter => undef,
    ycenter => undef,
    height  => undef,
    width   => undef,
    
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  
  return $self;
}

################################################################################
# privates init.
sub _init {
    my $self   = shift;
    my $param = shift;

    TRACE;
    
    return FALSE if (! defined $param);
    
    if (! -f $param) {
      ERROR ("File doesn't exist !");
      return FALSE;
    }
    
    # init. params    
    $self->{PATHFILENAME}=$param;
    
    #
    $self->{filepath} = File::Basename::dirname($param);
    $self->{filename} = File::Basename::basename($param);
    
    return TRUE;
}
################################################################################
# public
sub computeInfo {
  my $self = shift;
  
  my $image = $self->{filename};
  
  INFO(sprintf "compute '%s'", $image);
  
  my $dataset;
  eval { $dataset= Geo::GDAL::Open($self->{PATHFILENAME}, 'ReadOnly'); };
  if ($@) {
    ERROR (sprintf "Can not open image ('%s') : '%s' !", $image, $@);
    return FALSE;
  }
  
  my $driver = $dataset->GetDriver();
  my $code   = $driver->{ShortName};
  # FIXME : type of driver ?
  if ($code !~ /(GTiff|GeoTIFF)/) {
    ERROR (sprintf "This driver '%s' is not implemented ('%s') !", $code, $image);
    return FALSE;
  }
  
  # NV : Dans la suite j'ai commente la recuperation des infos dont on a pas encore
  #      besoin et qui semble poser des problèmes (version de gdalinfo?)
  
#  my $i = 1;
#
#  my $DataType       = undef;
#  my $Band           = undef;
#  my @Interpretation;
#
#  foreach my $objBand ($dataset->Bands()) {
#    
#    # FIXME undefined !
#    # TRACE (sprintf "NoDataValue         :%s", $objBand->GetNoDataValue());
#    # TRACE (sprintf "NoDataValue         :%s", $objBand->NoDataValue());
#    
#    # ie Float32,  GrayIndex,          , , .
#    # ie Byte,     (Red|Green|Blue)Band, , .
#    # ie Byte,     GrayIndex,          , , .
#    # ie UInt32,   GrayIndex,          , , .
#    # Byte, UInt16, Int16, UInt32, Int32, Float32, Float64, CInt16, CInt32, CFloat32, or CFloat64
#    # Undefined GrayIndex PaletteIndex RedBand GreenBand BlueBand AlphaBand HueBand SaturationBand LightnessBand CyanBand MagentaBand YellowBand BlackBand
#    
#    $DataType = lc $objBand->DataType();
#    push @Interpretation, lc $objBand->ColorInterpretation();
#    $Band = $i;
#  }
#
#  my $bitspersample  =undef;
#  my $photometric    =undef;
#  my $sampleformat   =undef;
#  my $samplesperpixel=undef;
#
#  if ($DataType eq "byte") {
#    $bitspersample = 8;
#    $sampleformat  = "unit";
#    
#  }
#  else {
#    ($sampleformat, $bitspersample) = ($DataType =~ /(\w+)(\d{2})/);
#  }
#
#  if ($Band == 3) {
#    foreach (@Interpretation) {
#      last if ($_ !~ m/(red|green|blue)band/);
#    }
#    $photometric     = "rgb";
#    $samplesperpixel = 3;
#  }
#
#  if ($Band == 1 && $Interpretation[0] eq "grayindex") {
#    $photometric     = "gray";
#    $samplesperpixel = 1;
#  }
#
#  DEBUG(sprintf "format image : bps %s, photo %s, sf %s,  spp %s",
#        $bitspersample, $photometric, $sampleformat, $samplesperpixel);
  
  my $refgeo = $dataset->GetGeoTransform();
  if (! defined ($refgeo) || scalar (@$refgeo) != 6) {
    ERROR ("Can not found parameters of image ('$image') !");
    return FALSE;
  }
  
  # forced formatting string !
  my ($xmin, $dx, $rx, $ymax, $ry, $ndy)= @$refgeo;

  # FIXME : precision ?
  $self->{xmin} = sprintf "%.8f", $xmin;
  $self->{xmax} = sprintf "%.8f", $xmin + $dx*$dataset->{RasterXSize};
  $self->{ymin} = sprintf "%.8f", $ymax + $ndy*$dataset->{RasterYSize};
  $self->{ymax} = sprintf "%.8f", $ymax;
  $self->{xres} = sprintf "%.8f", $dx;      # $rx null ?
  $self->{yres} = sprintf "%.8f", abs($ndy);# $ry null ?
  $self->{xcenter}   = sprintf "%.8f", $xmin + $dx*$dataset->{RasterXSize}/2.0;
  $self->{ycenter}   = sprintf "%.8f", $ymax + $ndy*$dataset->{RasterYSize}/2.0;
  $self->{pixelsize} = sprintf "%.8f", $dx;
  $self->{height} = $dataset->{RasterYSize};
  $self->{width}  = $dataset->{RasterXSize};
  
  #DEBUG(sprintf "box:[%s %s %s %s] res:[%s %s] c:[%s %s] p[%s] size:[%s %s]\n",
  #      $self->{xmin},
  #      $self->{xmax},
  #      $self->{ymin},
  #      $self->{ymax},
  #      $self->{xres},
  #      $self->{yres},
  #      $self->{xcenter},
  #      $self->{ycenter},
  #      $self->{pixelsize},
  #      $self->{height},
  #      $self->{width});
  
  return TRUE;
}

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
    $self->{pixelsize},
    $self->{xcenter},
    $self->{ycenter},
    $self->{height},
    $self->{width},
  );
  
}
sub getBbox {
  my $self = shift;

  TRACE;
  
  my @bbox;

  # FIXME : format bbox (Upper Left, Lower Right) ?
  push @bbox, ($self->{xmin},$self->{ymax},$self->{xmax},$self->{ymin});
  
  return @bbox;
}
################################################################################
# get / set
sub getXmin {
  my $self = shift;
  return $self->{xmin};
}
sub getYmin {
  my $self = shift;
  return $self->{ymin};
}
sub getXmax {
  my $self = shift;
  return $self->{xmax};
}
sub getYmax {
  my $self = shift;
  return $self->{ymax};
}
sub getXres {
  my $self = shift;
  return $self->{xres};  
}
sub getYres {
  my $self = shift;
  return $self->{yres};  
}
sub getName {
  my $self = shift;
  return $self->{filename}; 
}
################################################################################
# to_string methode
sub to_string {
  my $self = shift;
  
  TRACE;
  
  my $output = sprintf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
          File::Spec->catfile($self->{filepath}, $self->{filename}),
          $self->{xmin},
          $self->{ymax},
          $self->{xmax},
          $self->{ymin},
          $self->{xres},
          $self->{yres},;
  #DEBUG ($output);
  return $output;
}
1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

  BE4::ImageSource - analyzes the features of the image.

=head1 SYNOPSIS

  use BE4::ImageSource;
  
  my $objImg = BE4::ImageSource->new($filepath);
  if (! $objImg->computeInfo()) { # ERROR ! }
  
  my (
    $filename,
    $filepath,
    $xmin,
    $ymax,
    $xmax,
    $ymin,
    $xres,
    $yres,
    $pixelsize,
    $xcenter,
    $ycenter,
    $height,
    $width
    ) = $objImg->getInfo();

=head1 DESCRIPTION

* Constraint on the input formats of images

  format tiff ...

* Use the binding Perl of Gdal

* Sample with gdalinfo
  
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

=head2 EXPORT

None by default.

=head1 SEE ALSO

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
