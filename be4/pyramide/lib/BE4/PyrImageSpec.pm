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

package BE4::PyrImageSpec;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

use BE4::Pixel;

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
# Global
my %IMAGESPEC;
my %CODE2SAMPLEFORMAT;
my %SAMPLEFORMAT2CODE;

################################################################################

BEGIN {}
INIT {

%IMAGESPEC = (
    interpolation => ['nn','bicubic','linear','lanczos'],
    compression => ['raw','jpg','png','lzw','zip'],
    compressionoption => ['none','crop']
);

%CODE2SAMPLEFORMAT = (
    INT => "uint",
    FLOAT => "float"
);

%SAMPLEFORMAT2CODE = (
    uint => "INT",
    float => "FLOAT"
);

}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    * pixel    => undef, (Pixel object)
    * compression => undef,
    * compressionoption => undef,
    * interpolation => undef,
    * gamma  => undef, 
    * formatCode  => undef
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;
    my $params = shift;
    
    my $class= ref($this) || $this;
    my $self = {
        pixel    => undef,
        compression => undef,
        compressionoption => undef,
        interpolation => undef,
        gamma  => undef,
        formatCode  => undef
    };

    bless($self, $class);

    TRACE;
  
    # init. class
    if (! $self->_init($params)) {
        ERROR ("Can not create PyrImageSpec object !");
        return undef;
    }
  
    return $self;

}


sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);

    if (exists($params->{formatCode})) {
        (my $formatimg, $params->{compression}, $params->{sampleformat}, $params->{bitspersample})
            = $self->decodeFormat($params->{formatCode});
        if (! defined $formatimg) {
            ERROR (sprintf "Can not decode formatCode '%s' !",$params->{formatCode});
            return FALSE;
        }
    }

    # Pixel object creation
    my $objPixel = BE4::Pixel->new({
        photometric => $params->{photometric},
        sampleformat => $params->{sampleformat},
        bitspersample => $params->{bitspersample},
        samplesperpixel => $params->{samplesperpixel}
    });

    if (! defined $objPixel) {
        ERROR ("Can not create Pixel object !");
        return FALSE;
    }

    $self->{pixel} = $objPixel;
    
    # Other attributes
    # All attributes have to be present in parameters and defined

    # Compression parameters
    # to remove when compression type 'floatraw' will be remove
    if (exists($params->{compression}) && $params->{compression} eq 'floatraw') {
        WARN("'floatraw' is a deprecated compression type, use 'raw' instead");
        $params->{compression} = 'raw';
    }
    if (! exists $params->{compression} || ! defined $params->{compression}) {
        ERROR ("'compression' is required !");
        return FALSE;
    }
    if (! $self->is_Compression($params->{compression})) {
        ERROR ("'compression' is not valid !");
        return FALSE;
    }
    $self->{compression} = $params->{compression};

    if (! exists $params->{compressionoption} || ! defined $params->{compressionoption}) {
        ERROR ("'compressionoption' is required !");
        return FALSE;
    }
    if (! $self->is_CompressionOption($params->{compressionoption})) {
        ERROR ("'compressionoption' is not valid !");
        return FALSE;
    }
    $self->{compressionoption} = $params->{compressionoption};

    # Interpolation parameter

    if (! exists $params->{interpolation} || ! defined $params->{interpolation}) {
        ERROR ("'interpolation' is required !");
        return FALSE;
    }
    if (! $self->is_Interpolation($params->{interpolation})) {
        ERROR ("'interpolation' is not valid !");
        return FALSE;
    }
    $self->{interpolation} = $params->{interpolation};

    # Gamma parameter
    if (! exists $params->{gamma} || ! defined $params->{interpolation}) {
        ERROR ("'gamma' is undefined !");
        return FALSE;
    }
    if ($params->{gamma} < 0) {
        WARN ("Given value for gamma is negative : 0 is used !");
        $params->{gamma} = 0;
    }
    if ($params->{gamma} > 1) {
        WARN ("Given value for gamma is greater than 1 : 1 is used !");
        $params->{gamma} = 1;
    }
    $self->{gamma} = $params->{gamma};

    # formatCode : TIFF_[COMPRESSION]_[SAMPLEFORMAT][BITSPERSAMPLE]
    $self->{formatCode} = sprintf "TIFF_%s_%s%s",
        uc $self->{compression},
        $SAMPLEFORMAT2CODE{$self->{pixel}->{sampleformat}},
        $self->{pixel}->{bitspersample};

    return TRUE;
}

####################################################################################################
#                                            TESTS                                                 #
####################################################################################################

# Group: tests

sub is_Compression {
    my $self = shift;
    my $compression = shift;

    TRACE;

    return FALSE if (! defined $compression);

    foreach (@{$IMAGESPEC{compression}}) {
        return TRUE if ($compression eq $_);
    }
    ERROR (sprintf "Unknown 'compression' (%s) !",$compression);
    return FALSE;
}

sub is_CompressionOption {
    my $self = shift;
    my $compressionoption = shift;

    TRACE;

    my $bool = FALSE;

    return FALSE if (! defined $compressionoption);

    foreach (@{$IMAGESPEC{compressionoption}}) {
        if ($compressionoption eq $_) {
            $bool = TRUE;
            last;
        }
    }
    if (! $bool) {
        ERROR (sprintf "Unknown 'compressionoption' (%s) !",$compressionoption);
        return FALSE;
    }
    # NOTE
    # Compression have to be already define in the pixel objet
    if ($compressionoption eq 'crop' && $self->{compression} ne 'jpg') {
        ERROR (sprintf "Crop option is just allowed for jpeg compression, not for compression '%s' !",
            $self->{pixel}->{compression});
        return FALSE;
    }

    return TRUE;
}

sub is_Interpolation {
    my $self = shift;
    my $interpolation = shift;

    TRACE;

    return FALSE if (! defined $interpolation);

    foreach (@{$IMAGESPEC{interpolation}}) {
        return TRUE if ($interpolation eq $_);
    }
    ERROR (sprintf "Unknown 'interpolation' (%s) !",$interpolation);
    return FALSE;
}

####################################################################################################
#                                       CODE MANAGER                                               #
####################################################################################################

# Group: code manager

=begin nd
method: decodeFormat
Extract the sample format, bits per sample and compression from the format code

Codes handled by rok4 are :
    - TIFF_INT8 (deprecated, use TIFF_RAW_INT8 instead)
    - TIFF_RAW_INT8
    - TIFF_JPG_INT8
    - TIFF_LZW_INT8
    - TIFF_PNG_INT8
    - TIFF_ZIP_INT8

    - TIFF_FLOAT32 (deprecated, use TIFF_RAW_FLOAT32 instead)
    - TIFF_RAW_FLOAT32
    - TIFF_LZW_FLOAT32
=cut
sub decodeFormat {
    my $self = shift;
    my $formatCode = shift;
    
#   to remove when format 'TIFF_INT8' and 'TIFF_FLOAT32' will be remove
    if ($formatCode eq 'TIFF_INT8') {
        WARN("'TIFF_INT8' is a deprecated format, use 'TIFF_RAW_INT8' instead");
        $formatCode = 'TIFF_RAW_INT8';
    }
    if ($formatCode eq 'TIFF_FLOAT32') {
        WARN("'TIFF_FLOAT32' is a deprecated format, use 'TIFF_RAW_FLOAT32' instead");
        $formatCode = 'TIFF_RAW_FLOAT32';
    }

    $self->{formatCode} = $formatCode;
    
    my @value = split(/_/, $formatCode);
    if (scalar @value != 3) {
        ERROR(sprintf "Format code is not valid '%s' !", $formatCode);
        return undef;
    }
  
    # FIXME : cette regex ne fonctionne pas toujours ?!
    # $value[2] =~ m/(\w+)(\d+)/; 
    $value[2] =~ m/([A-Z]+)([0-9]+)/;

    # Contrôle de la valeur sampleFormat extraite
    my $sampleformatCode = $1;
    my $sampleformat = '';

    foreach (keys %CODE2SAMPLEFORMAT) {
        if ($sampleformatCode eq $_) {
            $sampleformat = $CODE2SAMPLEFORMAT{$_};
        }
    }
    if ($sampleformat eq '') {
        ERROR(sprintf "Extracted sampleFormat is not valid '%s' !", $sampleformatCode);
        return undef;
    }

    # Contrôle de la valeur compression extraite
    if (! $self->is_Compression(lc $value[1])) {
        ERROR(sprintf "Extracted compression is not valid '%s' !", $value[1]);
        return undef;
    }

    my $bitspersample = $2;
    
    return (lc $value[0], lc $value[1], $sampleformat, $bitspersample);
  
    # ie 'tiff', 'raw', 'uint' , '8'
    # ie 'tiff', 'png', 'uint' , '8'
    # ie 'tiff', 'jpg', 'uint' , '8'
    # ie 'tiff', 'lzw', 'uint' , '8'
    # ie 'tiff', 'raw', 'float', '32'    
    # ie 'tiff', 'lzw', 'float', '32'
    
}


1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

    BE4::PyrImageSpec - components of pyramid's images

=head1 SYNOPSIS
  
    use BE4::PyrImageSpec;

    # create a PyrImageSpec object

    my $pyrImgSpec = BE4::PyrImageSpec->new({
        bitspersample => 32,
        sampleformat => float,
        photometric => gray,
        samplesperpixel => 1,
        interpolation => nn,
        compression => lzw,
        compressionoption => none,
        gamma => 1.0,
    });

        OR

    my $pyrImgSpec = BE4::PyrImageSpec->new({
        formatCode => TIFF_LZW_FLOAT32,
        photometric => gray,
        samplesperpixel => 1,
        interpolation => nn,
        compressionoption => none,
        gamma => 1.0,
    });

=head1 DESCRIPTION

    A PyrImageSpec object :

        * pixel (Pixel object)
        * compression
        * compressionoption
        * interpolation
        * gamma
        * formatCode

    Possible values :

        interpolation => ['nn','bicubic','linear','lanczos'],
        compression => ['raw','jpg','png','lzw','zip'],
        compressionoption => ['none','crop']

 
    'compression' is use for the program 'tiff2tile'.
    'formatCode' is written in the pyramid file, and it is useful for 'Rok4' !

=head2 EXPORT

    None by default.

=head1 SEE ALSO

    BE4::Pixel

=head1 AUTHOR

    Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

    Copyright (C) 2011 by Satabin Théo

    This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself,
    either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut