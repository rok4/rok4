package BE4::Product;

# use strict;
use warnings;

use Log::Log4perl qw(:easy);

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
# Global
my %TILES;

################################################################################
# Preloaded methods go here.

#
# Group: Preloaded methods
#

#
# function: get/set in init()
#
#   get/set with field, BitsPerSample SampleFormat CompressionScheme Photometric SamplesPerPixel Interpolation
#

BEGIN {}
INIT {
  # TODO :
  # put in a configuration file !
  
  %TILES = (
    bitspersample     => [8,32],
    sampleformat      => ['uint','float'],
    compressionscheme => ['none','zip','jpeg','packbits','lzw','deflate'],
    photometric       => ['rgb','gray','mask'], # ie 'min_is_black'
    samplesperpixel   => [1,3],
    interpolation     => ['lanczos','ppv','linear','bicubique'],
  );
  
  # getter/setter except "" because of  !
  foreach my $i (qw(BitsPerSample SampleFormat CompressionScheme Photometric SamplesPerPixel Interpolation)) {
    my $field = $i;
    
    *{"get$field"} = sub {
      my $self = shift;
      TRACE ("$i:$self->{lc $field}");
      return $self->{lc $field};
    };
    
    *{"set$field"} = sub {
      my $self = shift;
      @_ or die "not enough arguments to set$field, stopped !";
      my $value = shift;
      TRACE ("$i:$value");
      my $fct = "is_$field";
      return 0 if (! $self->$fct($value));
      $self->{lc $field} = $value;
      return 1;
    };
  }
}
END {}

################################################################################
#  [ product ]
#  
#  bitspersample               = 8,32
#  sampleformat                = uint,float
#  compressionscheme           = none,jpeg,lzw,deflate,zip,packbits
#  photometric                 = rgb,gray,(mask?)
#  samplesperpixel             = 1,3
#  ; rowsstrip                 = 1
#  ; planarconfiguration       = 1
#  interpolation               = lanczos ppv linear bicubique

#
# Group: variable
#

#
# variable: $self
#
#    *    bitspersample           => undef, # ie 8
#    *    sampleformat            => undef, # ie uint
#    *    compressionscheme       => undef, # ie none
#    *    photometric             => undef, # ie gray
#    *    samplesperpixel         => undef, # ie 3
#    *    rowsstrip               => undef,   # NOT IMPLEMENTED !
#    *    planarconfiguration     => undef,   # NOT IMPLEMENTED !
#    *    interpolation           => undef,   # ie resample : bicubique
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
    bitspersample           => undef,
    sampleformat            => undef,
    compressionscheme       => undef,
    photometric             => undef,
    samplesperpixel         => undef,
    # rowsstrip               => undef, # NOT IMPLEMENTED !
    # planarconfiguration     => undef, # NOT IMPLEMENTED !
    interpolation           => undef,   # resample ?
    
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
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    return undef if (! $self->is_BitsPerSample($params->{bitspersample}));
    return undef if (! $self->is_SampleFormat($params->{sampleformat}));
    return undef if (! $self->is_CompressionScheme($params->{compressionscheme}));
    return undef if (! $self->is_Photometric($params->{photometric}));
    return undef if (! $self->is_SamplesPerPixel($params->{samplesperpixel}));
    return undef if (! $self->is_Interpolation($params->{interpolation}));

    # init. params    
    $self->{bitspersample}      =$params->{bitspersample};
    $self->{sampleformat}       =$params->{sampleformat};
    $self->{compressionscheme}  =$params->{compressionscheme};
    $self->{photometric}        =$params->{photometric};
    $self->{samplesperpixel}    =$params->{samplesperpixel};
    $self->{interpolation}      =$params->{interpolation};
    # $self->{rowsstrip}          =$params->{rowsstrip};
    # $self->{planarconfiguration}=$params->{planarconfiguration};
    
    return TRUE;
}

################################################################################
# privates check
sub is_BitsPerSample {
  my $self = shift;
  my $p = shift;
  
  TRACE;
  
  return FALSE if (! defined $p);
  
  foreach (@{$TILES{bitspersample}}) {
    return TRUE if ($p eq $_);
  }
  ERROR ("Can not define 'bitspersample' (unsupported)!");
  return FALSE;
}
sub is_SampleFormat {
  my $self = shift;
  my $p = shift;
  
  TRACE;
  
  return FALSE if (! defined $p);
  
  foreach (@{$TILES{sampleformat}}) {
    return TRUE if (lc $p eq lc $_);
  }
  ERROR ("Can not define 'sampleformat' (unsupported)!");
  return FALSE;
}
sub is_CompressionScheme {
  my $self = shift;
  my $p = shift;
  
  TRACE;
  
  return FALSE if (! defined $p);
  
  foreach (@{$TILES{compressionscheme}}) {
    return TRUE if (lc $p eq lc $_);
  }
  ERROR ("Can not define 'compressionscheme' (unsupported)!");
  return FALSE;
}
sub is_Photometric {
  my $self = shift;
  my $p = shift;
  
  TRACE;
  
  return FALSE if (! defined $p);
  
  foreach (@{$TILES{photometric}}) {
    return TRUE if (lc $p eq lc $_);
  }
  ERROR ("Can not define 'photometric' (unsupported)!");
  return FALSE;
}
sub is_SamplesPerPixel {
  my $self = shift;
  my $p = shift;
  
  TRACE;
  
  return FALSE if (! defined $p);
  
  foreach (@{$TILES{samplesperpixel}}) {
    return TRUE if ($p eq $_);
  }
  ERROR ("Can not define 'samplesperpixel' (unsupported)!");
  return FALSE;
}
sub is_Interpolation {
  my $self = shift;
  my $p = shift;
  
  TRACE;
  
  return FALSE if (! defined $p);
  
  foreach (@{$TILES{interpolation}}) {
    return TRUE if ($p eq $_);
  }
  ERROR ("Can not define 'interpolation' (unsupported)!");
  return FALSE;
}
################################################################################
# public
sub equal {
  my $self = shift;
  my $obj  = shift;
  
  if(
    $self->{bitspersample} ne $obj->getBitsPerSample() ||
    $self->{sampleformat} ne $obj->getSampleFormat() ||
    $self->{compressionscheme} ne $obj->getCompressionScheme() ||
    $self->{photometric} ne $obj->getPhotometric() ||
    $self->{samplesperpixel} ne $obj->getSamplesPerPixel() ||
    # $self->{rowsstrip} ne $obj->getRowsStrip() ||
    # $self->{planarconfiguration} ne $obj->getPlanarConfiguration() ||
    $self->{interpolation} ne $obj->getInterpolation()
    ) {
    return FALSE;
  }
  return TRUE;
}
1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

 BE4::Product - It's the tile format out (= tileImage Cache) of pyramid.

=head1 SYNOPSIS

  use BE4::Product;
  
  my $tile = {
    bitspersample    => 8,
    sampleformat     => "uint",
    compressionscheme=> "none",
    photometric      => "rgb",
    samplesperpixel  => 3,
    interpolation    => "bicubique",
  };

  my $objP = BE4::Product->new($tile);
  
  $objP->getBitPerSample();   # 8
  $objP->getInterpolation();  # bicubique
  $objP->setInterpolation("linear");  # linear
  $objP->setSampleFormat("decimal");  # Can not define 'sampleformat' !

=head1 DESCRIPTION

The parameter 'compressionscheme' is usefull for the following programs :
 - merge4tiff
 - tiff2gray

Notice : with tiffinfo (and gdalinfo)
  
  # - RAW Uint8 / rgb / 3 channel
  
  TIFF Directory at offset 0xe (14)
    Image Width: 1024 Image Length: 1024
    Tile Width: 256 Tile Length: 256
    Bits/Sample: 8
    Compression Scheme: None
    Photometric Interpretation: RGB color
    Samples/Pixel: 3
    Planar Configuration: single image plane
    
    (gdalinfo : INTERLEAVE=PIXEL
                Band 1 Block=256x256 Type=Byte, ColorInterp=Red
                Band 2 Block=256x256 Type=Byte, ColorInterp=Green
                Band 3 Block=256x256 Type=Byte, ColorInterp=Blue )
  
  # - JPEG Uint8 / rgb / 3 channel
  
  TIFF Directory at offset 0xe (14)
    Image Width: 1024 Image Length: 1024
    Tile Width: 256 Tile Length: 256
    Bits/Sample: 8
    Compression Scheme: JPEG
    Photometric Interpretation: YCbCr
    YCbCr Subsampling: 2, 2
    Samples/Pixel: 3
    Planar Configuration: single image plane
  
    (gdalinfo : SOURCE_COLOR_SPACE=YCbCr
                COMPRESSION=YCbCr JPEG
                INTERLEAVE=PIXEL
                Band 1 Block=256x256 Type=Byte, ColorInterp=Red
                Band 2 Block=256x256 Type=Byte, ColorInterp=Green
                Band 3 Block=256x256 Type=Byte, ColorInterp=Blue )
  
  
  # - PNG Uint8 / gray / 1 channel
  
  TIFF Directory at offset 0xe (14)
    Image Width: 1024 Image Length: 1024
    Tile Width: 256 Tile Length: 256
    Bits/Sample: 8
    Compression Scheme: AdobeDeflate
    Photometric Interpretation: min-is-black
    Samples/Pixel: 1
    Planar Configuration: single image plane
  
    (gdalinfo : Band 1 Block=256x256 Type=Byte, ColorInterp=Gray
                COMPRESSION=DEFLATE
                INTERLEAVE=BAND )
  
  # - PNG Float32 / gray / 1 channel
  
  TIFF Directory at offset 0x2e008 (188424)
    Image Width: 4096 Image Length: 4096
    Bits/Sample: 32
    Sample Format: IEEE floating point
    Compression Scheme: AdobeDeflate
    Photometric Interpretation: min-is-black
    Orientation: row 0 top, col 0 lhs
    Samples/Pixel: 1
    Rows/Strip: 1
    Planar Configuration: single image plane
  
  # - PNG Uint32 / gray / 1 channel
  
  TIFF Directory at offset 0x27008 (159752)
    Image Width: 4096 Image Length: 4096
    Resolution: 72, 72 pixels/inch
    Bits/Sample: 32
    Compression Scheme: AdobeDeflate
    Photometric Interpretation: min-is-black
    FillOrder: msb-to-lsb
    Orientation: row 0 top, col 0 lhs
    Samples/Pixel: 1
    Rows/Strip: 1
    Planar Configuration: single image plane
  
  # - PackBits Uint8 / gray / 1 channel
   
  TIFF Directory at offset 0x40008 (262152)
    Subfile Type: (0 = 0x0)
    Image Width: 4096 Image Length: 4096
    Resolution: 2, 2 pixels/inch
    Bits/Sample: 8
    Compression Scheme: PackBits
    Photometric Interpretation: min-is-black
    Orientation: row 0 top, col 0 lhs
    Samples/Pixel: 1
    Rows/Strip: 64
    Planar Configuration: single image plane


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
