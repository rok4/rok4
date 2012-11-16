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

################################################################################
=begin nd
Group: variable

variable: $self
    * completePath
    * filename
    * filepath
    * maskCompletePath
    * xmin
    * ymax
    * xmax
    * ymin
    * xres
    * yres
    * xcenter
    * ycenter
    * heigh
    * width
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
  my $self = {
    completePath => undef,
    filename => undef,
    filepath => undef,
    maskCompletePath => undef,
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
  return undef if (! $self->_init(@_));
  
  return $self;
}


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
    $self->{completePath} = $param;
    
    #
    $self->{filepath} = File::Basename::dirname($param);
    $self->{filename} = File::Basename::basename($param);
    
    my $maskPath = $param;
    $maskPath =~ s/\.(tif|TIF|tiff|TIFF)$/\.msk/;
    
    if (-f $maskPath) {
        INFO(sprintf "We have a mask associated to the image '%s' :\n\t%s",$param,$maskPath);
        $self->{maskCompletePath} = $maskPath;
    }
    
    return TRUE;
}

####################################################################################################
#                                       COMPUTING METHODS                                          #
####################################################################################################

# Group: computing methods

#
=begin nd
method: computeInfo

Image parameters are checked (sample per pixel, bits per sample...) and return by the function. ImageSource can verify if all images own same components and the compatibility with be4's configuration.
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

    DEBUG(sprintf "format image : bps %s, photo %s, sf %s,  spp %s",
    $bitspersample, $photometric, $sampleformat, $samplesperpixel);

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

#
=begin nd
method: convertBBox

Not just convert corners, but 7 points on each side, to determine reprojected bbox.

Parameter:
    ct - a Geo::OSR::CoordinateTransformation object, to convert bbox .

Returns:
    Converted (according to the given CoordinateTransformation) image bbox as an array [xMin, yMin, xMax, yMax], [0,0,0,0] if error.
=cut
sub convertBBox {
  my $self = shift;
  my $ct = shift;
  
  TRACE;
  
  my @BBox = [0,0,0,0];

  if (!defined($ct)){
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
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

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
sub getBBox {
  my $self = shift;

  TRACE;
  
  my @bbox;

  push @bbox, ($self->{xmin},$self->{ymin},$self->{xmax},$self->{ymax});
  
  return @bbox;
}

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

####################################################################################################
#                                         EXPORT METHODS                                           #
####################################################################################################

# Group: export methods

#
=begin nd
method: exportForMntConf

Export a GeoImage object as a string : filepath+filename xmin ymax xmax ymin xres yres.

Output is formated to be used in mergeNtiff configuration
=cut
sub exportForMntConf {
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

    return $output;
}

sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= sprintf "\nObject BE4::GeoImage :\n";
    $export .= sprintf "\t Image path : %s\n",$self->{completePath};

    $export .= "\t Dimensions (in pixel) :\n";
    $export .= sprintf "\t\t- width : %s\n",$self->{width};
    $export .= sprintf "\t\t- height : %s\n",$self->{height};
    
    $export .= "\t Resolution (in SRS unity) :\n";
    $export .= sprintf "\t\t- x : %s\n",$self->{xres};
    $export .= sprintf "\t\t- y : %s\n",$self->{yres};
    
    $export .= "\t Bbox (in SRS unity) :\n";
    $export .= sprintf "\t\t- xmin : %s\n",$self->{bbox}[0];
    $export .= sprintf "\t\t- ymin : %s\n",$self->{bbox}[1];
    $export .= sprintf "\t\t- xmax : %s\n",$self->{bbox}[2];
    $export .= sprintf "\t\t- ymax : %s\n",$self->{bbox}[3];
    
    return $export;
}

1;
__END__

=head1 NAME

BE4::GeoImage - Describe a georeferenced image and enable to know its components.

=head1 SYNOPSIS

    use BE4::GeoImage;
    
    # GeoImage object creation
    my $objGeoImage = BE4::GeoImage->new("/home/ign/DATA/XXXXX_YYYYY.tif");

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

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item completePath

Complete path (/home/ign/DATA/XXXXX_YYYYY.tif)

=item filename

Just the image name, with file extension (XXXXX_YYYYY.tif).

=item filepath

The directory which contain the image (/home/ign/DATA)

=item maskCompletePath

Complete path of associated mask, if exists (undef otherwise).

=item xmin, ymin, xmax, ymax

=item xres, yres

=item xcenter, ycenter

=item height, width

=back

=head2 INFORMATIONS

=over 4

=item Constraint on the input formats of images : format tiff

=item Use the binding Perl of Gdal

=item Sample with gdalinfo

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

=back

=head1 SEE ALSO

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHOR

Satabin Théo, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
