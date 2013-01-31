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
File: PyrImageSpec.pm

Class: BE4::PyrImageSpec

Store all image's components.

Using:
    (start code)
    use BE4::PyrImageSpec;

    # PyrImageSpec object creation

    # Basic constructor
    my $objPIS = BE4::PyrImageSpec->new({
        compression => "raw",
        sampleformat => "uint",
        bitspersample => 8,
        samplesperpixel => 3,
        photometric => "rgb",
        compressionoption => "none",
        interpolation => "bicubic",
        gamma  => 1
    });

    # From a code
    my $objPIS = BE4::PyrImageSpec->new({
        formatCode => "TIFF_RAW_INT8",
        samplesperpixel => 3,
        photometric => "rgb",
        compressionoption => "none",
        interpolation => "bicubic",
        gamma  => 1
    });
    (end code)

Attributes:
    pixel - <Pixel> - Contains pixel intrinsic components.
    compression - string - Data compression. Only PNG is a unofficial TIFF compression.
    compressionoption - string - Precise additionnal actions, to do before compression. Just "crop" is available, with JPEG compression. It's allowed to empty blocs which contain white pixel, to keep pure white, even with JPEG compression.
    interpolation - string - Image could be resampling. Resampling use a kind of interpolation.
    gamma - float - Positive, used by merge4tiff to make dark (between 0 and 1) or light (greater than 1) RGB images. 1 is a neutral value.
    formatCode - string - Used in the pyramid's descriptor. Format is : TIFF_<COMPRESSION>_<SAMPLEFORMAT><BITSPERSAMPLE> (TIFF_RAW_INT8).
=cut

################################################################################

package BE4::PyrImageSpec;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

# My module
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

# Constant: IMAGESPEC
# Define allowed values for attributes interpolation, compression and compressionoption.
my %IMAGESPEC;

# Constant: DEFAULT
# Define default values for attributes interpolation, compression, compressionoption and gamma.
my %DEFAULT;

my %CODE2SAMPLEFORMAT;
my %SAMPLEFORMAT2CODE;

################################################################################

BEGIN {}
INIT {
    %IMAGESPEC = (
        interpolation => ['nn','bicubic','linear','lanczos'],
        compression => ['raw','jpg','png','lzw','zip','pkb'],
        compressionoption => ['none','crop']
    );

    %DEFAULT = (
        interpolation => 'bicubic',
        compression => 'raw',
        compressionoption => 'none',
        gamma => 1
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

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

PyrImageSpec constructor. Bless an instance.

Parameters (hash):
    formatCode - string - Format code, present in the pyramid's descriptor. If formatCode is provided, it has priority and overwrite other parameters.

        OR

    compression - string - Image's compression
    sampleformat - string - Image's sample format
    bitspersample - integer - Image's bits per sample

    samplesperpixel - integer - Image's samples per pixel
    photometric - string - Image's photometric
    compressionoption - string - Image's compression option
    interpolation - string - Image's interpolation
    gamma - float - Merge gamma

See also:
    <_init>
=cut
sub new {
    my $this = shift;
    my $params = shift;
    
    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
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

=begin nd
Function: _init

Checks and stores informations.

Parameters (hash):
    formatCode - string - Format code, present in the pyramid's descriptor. If formatCode is provided, it has priority and overwrite other parameters.
        
        OR
        
    compression - string - Image's compression
    sampleformat - string - Image's sample format
    bitspersample - integer - Image's bits per sample
    
    samplesperpixel - integer - Image's samples per pixel
    photometric - string - Image's photometric
    compressionoption - string - Image's compression option
    interpolation - string - Image's interpolation
    gamma - float - Merge gamma
=cut
sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);

    if (exists $params->{formatCode} && defined $params->{formatCode}) {
        (my $formatimg, $params->{compression}, $params->{sampleformat}, $params->{bitspersample})
            = $self->decodeFormat($params->{formatCode});
        if (! defined $formatimg) {
            ERROR (sprintf "Can not decode formatCode '%s' !",$params->{formatCode});
            return FALSE;
        }
    }

    ### Pixel object
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
    
    ### Compression
    if (! exists $params->{compression} || ! defined $params->{compression}) {
        $params->{compression} = $DEFAULT{compression};
        INFO(sprintf "Default value for 'compression' : %s", $params->{compression});
    } elsif ($params->{compression} eq 'floatraw') {
        # to remove when compression type 'floatraw' will be remove
        WARN("'floatraw' is a deprecated compression type, use 'raw' instead");
        $params->{compression} = 'raw';
    } else {
        if (! $self->isCompression($params->{compression})) {
            ERROR (sprintf "Unknown 'compression' : %s !",$params->{compression});
            return FALSE;
        }
    }
    $self->{compression} = $params->{compression};

    ### Compression option
    if (! exists $params->{compressionoption} || ! defined $params->{compressionoption}) {
        $params->{compressionoption} = $DEFAULT{compressionoption};
        INFO(sprintf "Default value for 'compressionoption' : %s", $params->{compressionoption});
    } else {
        if (! $self->isCompressionOption($params->{compressionoption})) {
            ERROR (sprintf "Unknown compression option : %s !",$params->{compressionoption});
            return FALSE;
        }
    }
    $self->{compressionoption} = $params->{compressionoption};

    ### Interpolation
    if (! exists $params->{interpolation} || ! defined $params->{interpolation}) {
        $params->{interpolation} = $DEFAULT{interpolation};
        INFO(sprintf "Default value for 'interpolation' : %s", $params->{interpolation});
    } elsif ($params->{interpolation} eq 'bicubique') {
        # to remove when interpolation 'bicubique' will be remove
        WARN("'bicubique' is a deprecated interpolation value, use 'bicubic' instead");
        $params->{interpolation} = 'bicubic';
    } else {
        if (! $self->isInterpolation($params->{interpolation})) {
        ERROR (sprintf "Unknown interpolation : '%s'",$params->{interpolation});
        return FALSE;
        }
    }
    $self->{interpolation} = $params->{interpolation};

    ### Gamma
    if (! exists $params->{gamma} || ! defined $params->{gamma}) {
        $params->{gamma} = $DEFAULT{gamma};
        INFO(sprintf "Default value for 'gamma' : %s", $params->{gamma});
    } else {
        if ($params->{gamma} !~ /^-?\d+\.?\d*$/) {
            ERROR ("'gamma' is not a number !");
            return FALSE;
        }
        
        if ($params->{gamma} < 0) {
            WARN ("Given value for gamma is negative : 0 is used !");
            $params->{gamma} = 0;
        }
    }
    $self->{gamma} = $params->{gamma};

    ### Format code : TIFF_[COMPRESSION]_[SAMPLEFORMAT][BITSPERSAMPLE]
    $self->{formatCode} = sprintf "TIFF_%s_%s%s",
        uc $self->{compression}, $SAMPLEFORMAT2CODE{$self->{pixel}->{sampleformat}}, $self->{pixel}->{bitspersample};

    return TRUE;
}

####################################################################################################
#                             Group: Attributes' testers                                           #
####################################################################################################

=begin nd
Function: isCompression

Tests if compression value is allowed.

Parameters (list):
    compression - string - Compression value to test
=cut
sub isCompression {
    my $self = shift;
    my $compression = shift;

    TRACE;

    return FALSE if (! defined $compression);

    foreach (@{$IMAGESPEC{compression}}) {
        return TRUE if ($compression eq $_);
    }
    return FALSE;
}

=begin nd
Function: isCompressionOption

Tests if compression option value is allowed, and consistent with the compression.

Parameters (list):
    compressionoption - string - Compression option value to test
=cut
sub isCompressionOption {
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
        return FALSE;
    }
    # NOTE
    # Compression have to be already define in the pixel objet
    if ($compressionoption eq 'crop' && $self->{compression} ne 'jpg') {
        ERROR (sprintf "Crop option is just allowed for jpeg compression, not for compression '%s' !",
            $self->{compression});
        return FALSE;
    }

    return TRUE;
}

=begin nd
Function: isInterpolation

Tests if interpolation value is allowed.

Parameters (list):
    interpolation - string - Interpolation value to test
=cut
sub isInterpolation {
    my $self = shift;
    my $interpolation = shift;

    TRACE;

    return FALSE if (! defined $interpolation);

    foreach (@{$IMAGESPEC{interpolation}}) {
        return TRUE if ($interpolation eq $_);
    }
    return FALSE;
}

####################################################################################################
#                                   Group: Code manager                                            #
####################################################################################################

=begin nd
method: decodeFormat

Extracts bits per sample, compression and sample format from a code (present in pyramid's descriptor). Returns a string array : [image format,compression,sample format,bits per sample] ( ["TIFF","png","uint",8] ), *undef* if error.

Parameters (list):
    formatCode - TIFF_INT8 and TIFF_FLOAT32 are deprecated, but handled (warnings) .
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

    $value[2] =~ m/([A-Z]+)([0-9]+)/;

    # Contrôle de la valeur sampleFormat extraite
    my $sampleformatCode = $1;

    if (! exists $CODE2SAMPLEFORMAT{$sampleformatCode}) {
        ERROR(sprintf "Extracted sampleFormat is not valid '%s' !", $sampleformatCode);
        return undef;
    }
    my $sampleformat = $CODE2SAMPLEFORMAT{$sampleformatCode};

    # Contrôle de la valeur compression extraite
    if (! $self->isCompression(lc $value[1])) {
        ERROR(sprintf "Extracted compression is not valid '%s' !", $value[1]);
        return undef;
    }

    my $bitspersample = $2;
    
    return ($value[0], lc $value[1], $sampleformat, $bitspersample);
    
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getInterpolation
sub getInterpolation {
    my $self = shift;
    return $self->{interpolation};
}

# Function: getGamma
sub getGamma {
    my $self = shift;
    return $self->{gamma};
}

# Function: getCompression
sub getCompression {
    my $self = shift;
    return $self->{compression};
}

# Function: getCompressionOption
sub getCompressionOption {
    my $self = shift;
    return $self->{compressionoption};
}

# Function: getFormatCode
sub getFormatCode {
    my $self = shift;
    return $self->{formatCode};
}

# Function: getPixel
sub getPixel {
    my $self = shift;
    return $self->{pixel};
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all image's components. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject BE4::PyrImageSpec :\n";
    $export .= "\t Global information : \n";
    $export .= sprintf "\t\t- Compression : %s\n", $self->{compression};
    $export .= sprintf "\t\t- Compression option : %s\n", $self->{compressionoption};
    $export .= sprintf "\t\t- Interpolation : %s\n", $self->{interpolation};
    $export .= sprintf "\t\t- Gamma : %s\n", $self->{gamma};
    $export .= sprintf "\t\t- Format code : %s\n", $self->{formatCode};
    
    $export .= sprintf "\t Pixel components : %s\n", $self->{pixel}->exportForDebug;
    
    return $export;
}

1;
__END__
