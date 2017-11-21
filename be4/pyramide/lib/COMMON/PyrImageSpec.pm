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

Class: COMMON::PyrImageSpec

Store all image's components.

Using:
    (start code)
    use COMMON::PyrImageSpec;

    # PyrImageSpec object creation

    # Basic constructor
    my $objPIS = COMMON::PyrImageSpec->new({
        compression => "raw",
        sampleformat => "uint",
        bitspersample => 8,
        samplesperpixel => 3,
        photometric => "rgb",
        compressionoption => "none",
        interpolation => "bicubic",
        gamma  => 1
    });

    (end code)

Attributes:
    pixel - <COMMON::Pixel> - Contains pixel intrinsic components.
    compression - string - Data compression. Only PNG is a unofficial TIFF compression.
    compressionoption - string - Precise additionnal actions, to do before compression. Just "crop" is available, with JPEG compression. It's allowed to empty blocs which contain white pixel, to keep pure white, even with JPEG compression.
    interpolation - string - Image could be resampling. Resampling use a kind of interpolation.
    gamma - float - Positive, used by merge4tiff to make dark (between 0 and 1) or light (greater than 1) RGB images. 1 is a neutral value.
    formatCode - string - Used in the pyramid's descriptor. Format is : TIFF_<COMPRESSION>_<SAMPLEFORMAT><BITSPERSAMPLE> (TIFF_RAW_INT8).
=cut

################################################################################

package COMMON::PyrImageSpec;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

# My module
use COMMON::Pixel;
use COMMON::Array;
use COMMON::CheckUtils;

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

# Constant: COMPRESSIONOPTIONS
# Define allowed values for attributes compression options.
my @COMPRESSIONOPTIONS = ('none','crop');

# Constant: COMPRESSIONS
# Define allowed values for attributes compression
my @COMPRESSIONS = ('raw','jpg','png','lzw','zip','pkb');

# Constant: INTERPOLATIONS
# Define allowed values for attributes interpolation
my @INTERPOLATIONS = ('nn','bicubic','linear','lanczos');


# Constant: DEFAULT
# Define default values for attributes interpolation, compression, compressionoption and gamma.
my %DEFAULT = (
    interpolation => 'bicubic',
    compression => 'raw',
    compressionoption => 'none',
    gamma => 1
);

# Constant: SAMPLEFORMAT2CODE
# Convert the sample format parameter to the code element.
my %SAMPLEFORMAT2CODE = (
    uint => "INT",
    float => "FLOAT"
);

# Constant: CODES
# Define all available formats' codes, and parse them.
my %CODES = (
    TIFF_RAW_INT8 => ["raw", "uint", 8],
    TIFF_JPG_INT8 => ["jpg", "uint", 8],
    TIFF_PNG_INT8 => ["png", "uint", 8],
    TIFF_LZW_INT8 => ["lzw", "uint", 8],
    TIFF_ZIP_INT8 => ["zip", "uint", 8],
    TIFF_PKB_INT8 => ["pkb", "uint", 8],

    TIFF_RAW_FLOAT32 => ["raw", "float", 32],
    TIFF_LZW_FLOAT32 => ["lzw", "float", 32],
    TIFF_ZIP_FLOAT32 => ["zip", "float", 32],
    TIFF_PKB_FLOAT32 => ["pkb", "float", 32]
);

####################################################################################################
#                                   Group: Class methods                                           #
####################################################################################################

=begin nd
Function: decodeFormat

Extracts bits per sample, compression and sample format from a code (present in pyramid's descriptor).

Returns a list : (compression, sample format, bits per sample) : ("png", "uint", 8), (undef,undef,undef) if error.

Parameters (list):
    formatCode - Format code to decode
=cut
sub decodeFormat {
    my $formatCode = shift;
    
    if (! exists $CODES{$formatCode}) {
        ERROR(sprintf "Format code is not valid '%s' !", $formatCode);
        return (undef,undef,undef);
    }

    return ($CODES{$formatCode}[0], $CODES{$formatCode}[1], $CODES{$formatCode}[2]);

}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

PyrImageSpec constructor. Bless an instance. Possibilities :
    - We have an ancestor : all output format properties come from its pyramid desciptor. We have a 'formatCode' to extract compression, sampleformat and bitspersample
    - bitspersample or samplesperpixel or photometric or sampleformat is not provided by configuration. We try to use those from image source
    - bitspersample and samplesperpixel and photometric and sampleformat is provided by configuration : we use it, with potentially conversion

Gamma, interpolation and compressionoption own default value, but we have to provide it in configuration file if we want special value.

Parameters (list):
    params - string hash - Pyramid parameters, from configuration file or ancestor's pyramid descriptor
    pixelIn - <COMMON::Pixel> - Pixel caracteristic of input image data. Undefined if no image source or several format

See also:
    <_load>
=cut
sub new {
    my $class = shift;
    my $params = shift;
    my $pixelIn = shift;
    
    $class= ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        pixel    => undef,
        compression => undef,
        compressionoption => undef,
        interpolation => undef,
        gamma  => undef,
        formatCode  => undef
    };

    bless($this, $class);

    # init. class
    if (! $this->_load($params, $pixelIn)) {
        ERROR ("Can not create PyrImageSpec object !");
        return undef;
    }
  
    return $this;

}

=begin nd
Function: _load

Checks and stores informations.

Parameters (list):
    params - string hash - Pyramid parameters, from configuration file or ancestor's pyramid descriptor
    pixelIn - <COMMON::Pixel> - Pixel caracteristic of input image data. Undefined if no image source
=cut
sub _load {
    my $this   = shift;
    my $params = shift;
    my $pixelIn = shift;
    
    return FALSE if (! defined $params);

    if ( (! exists $params->{photometric} && ! defined $params->{photometric}) ||
         (! exists $params->{sampleformat} && ! defined $params->{sampleformat}) ||
         (! exists $params->{bitspersample} && ! defined $params->{bitspersample}) ||
         (! exists $params->{samplesperpixel} && ! defined $params->{samplesperpixel}) ) {

        # One pixel parameter is missing, we must have a pixelIn (information from image source)
        if (! defined $pixelIn) {
            ERROR(
                "One pixel parameter is missing (photometric, sampleformat, bitspersample ".
                "or samplesperpixel), we must have a pixelIn (information from image source)"
            );
            return FALSE;
        }

        INFO(
            "One pixel parameter is missing (photometric, sampleformat, bitspersample ".
            "or samplesperpixel), we pick all information from image source"
        );

        $params->{samplesperpixel} = $pixelIn->getSamplesPerPixel();
        $params->{sampleformat} = $pixelIn->getSampleFormat();
        $params->{photometric} = $pixelIn->getPhotometric();
        $params->{bitspersample} = $pixelIn->getBitsPerSample();
        
    }

    ### Pixel object
    my $objPixel = COMMON::Pixel->new($params);

    if (! defined $objPixel) {
        ERROR ("Can not create Pixel object !");
        return FALSE;
    }

    $this->{pixel} = $objPixel;
    
    ### Compression
    if (! exists $params->{compression} || ! defined $params->{compression}) {
        $params->{compression} = $DEFAULT{compression};
        DEBUG(sprintf "Default value for 'compression' : %s", $params->{compression});
    } else {
        if (! defined COMMON::Array::isInArray($params->{compression}, @COMPRESSIONS) ) {
            ERROR (sprintf "Unknown 'compression' : %s !",$params->{compression});
            return FALSE;
        }
    }
    $this->{compression} = $params->{compression};

    ### Compression option
    if (! exists $params->{compressionoption} || ! defined $params->{compressionoption}) {
        $params->{compressionoption} = $DEFAULT{compressionoption};
        DEBUG(sprintf "Default value for 'compressionoption' : %s", $params->{compressionoption});
    } else {
        if (! defined COMMON::Array::isInArray($params->{compressionoption}, @COMPRESSIONOPTIONS) ) {
            ERROR (sprintf "Unknown compressionoption option : %s !",$params->{compressionoption});
            return FALSE;
        }
    }
    $this->{compressionoption} = $params->{compressionoption};

    ### Interpolation
    if (! exists $params->{interpolation} || ! defined $params->{interpolation}) {
        $params->{interpolation} = $DEFAULT{interpolation};
        DEBUG(sprintf "Default value for 'interpolation' : %s", $params->{interpolation});
    } else {
        if (! defined COMMON::Array::isInArray($params->{interpolation}, @INTERPOLATIONS) ) {
            ERROR (sprintf "Unknown interpolation : '%s'",$params->{interpolation});
            return FALSE;
        }
    }
    $this->{interpolation} = $params->{interpolation};

    ### Gamma
    if (! exists $params->{gamma} || ! defined $params->{gamma}) {
        $params->{gamma} = $DEFAULT{gamma};
        DEBUG(sprintf "Default value for 'gamma' : %s", $params->{gamma});
    } else {
        if (! COMMON::CheckUtils::isNumber($params->{gamma}) || $params->{gamma} < 0) {
            ERROR ("gamma have to be a positive float");
            return FALSE;
        }
    }
    $this->{gamma} = $params->{gamma};

    ### Format code : TIFF_[COMPRESSION]_[SAMPLEFORMAT][BITSPERSAMPLE]
    $this->{formatCode} = sprintf "TIFF_%s_%s%s",
        uc $this->{compression},
        $SAMPLEFORMAT2CODE{$this->{pixel}->getSampleFormat()},
        $this->{pixel}->getBitsPerSample();

    if (! exists $CODES{$this->{formatCode}}) {
        ERROR(sprintf "Format code is not handled '%s' !", $this->{formatCode});
        return FALSE;
    }

    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getInterpolation
sub getInterpolation {
    my $this = shift;
    return $this->{interpolation};
}

# Function: getGamma
sub getGamma {
    my $this = shift;
    return $this->{gamma};
}

# Function: getCompression
sub getCompression {
    my $this = shift;
    return $this->{compression};
}

# Function: getCompressionOption
sub getCompressionOption {
    my $this = shift;
    return $this->{compressionoption};
}

# Function: getFormatCode
sub getFormatCode {
    my $this = shift;
    return $this->{formatCode};
}

# Function: getPixel
sub getPixel {
    my $this = shift;
    return $this->{pixel};
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all image's components. Useful for debug.

Example:
    (start code)
    Object COMMON::PyrImageSpec :
         Global information :
                - Compression : raw
                - Compression option : none
                - Interpolation : bicubic
                - Gamma : 1
                - Format code : TIFF_RAW_INT8
         Pixel components :
    Object COMMON::Pixel :
         Bits per sample : 8
         Photometric : rgb
         Sample format : uint
         Samples per pixel : 1
    (end code)
=cut
sub exportForDebug {
    my $this = shift ;
    
    my $export = "";
    
    $export .= "\nObject COMMON::PyrImageSpec :\n";
    $export .= "\t Global information : \n";
    $export .= sprintf "\t\t- Compression : %s\n", $this->{compression};
    $export .= sprintf "\t\t- Compression option : %s\n", $this->{compressionoption};
    $export .= sprintf "\t\t- Interpolation : %s\n", $this->{interpolation};
    $export .= sprintf "\t\t- Gamma : %s\n", $this->{gamma};
    $export .= sprintf "\t\t- Format code : %s\n", $this->{formatCode};
    
    $export .= sprintf "\t Pixel components : %s\n", $this->{pixel}->exportForDebug();
    
    return $export;
}

1;
__END__