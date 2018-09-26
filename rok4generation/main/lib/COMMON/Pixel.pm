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
File: Pixel.pm

Class: COMMON::Pixel

Store all pixel's intrinsic components.

Using:
    (start code)
    use COMMON::Pixel;

    my $objC = COMMON::Pixel->new({
        sampleformat => "uint",
        photometric => "rgb",
        samplesperpixel => 3,
        bitspersample => 8,
    });
    (end code)

Attributes:
    photometric - string - Samples' interpretation.
    sampleformat - string - Sample format, type.
    bitspersample - integer - Number of bits per sample (the same for all samples).
    samplesperpixel - integer - Number of channels.
=cut

################################################################################

package COMMON::Pixel;

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

# Constant: BITSPERSAMPLES
# Define allowed values for attributes bitspersample
my @BITSPERSAMPLES = (1,8,32);

# Constant: PHOTOMETRICS
# Define allowed values for attributes photometric
my @PHOTOMETRICS = ('rgb','gray','mask');

# Constant: SAMPLESPERPIXELS
# Define allowed values for attributes samplesperpixel
my @SAMPLESPERPIXELS = (1,2,3,4);

# Constant: SAMPLEFORMATS
# Define allowed values for attributes sampleformat
my @SAMPLEFORMATS = ('uint','float');

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Pixel constructor. Bless an instance. Check and store attributes values.

Parameters (hash):
    photometric - string - Samples' interpretation. Default value : "rgb".
    sampleformat - string - Sample format, type.
    bitspersample - integer - Number of bits per sample (the same for all samples).
    samplesperpixel - integer - Number of channels.
=cut
sub new {
    my $class = shift;
    my $params = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        photometric => undef,
        sampleformat => undef,
        bitspersample => undef,
        samplesperpixel => undef,
    };

    bless($this, $class);

    # All attributes have to be present in parameters and defined

    ### Sample format : REQUIRED
    if (! exists $params->{sampleformat} || ! defined $params->{sampleformat}) {
        ERROR ("'sampleformat' required !");
        return undef;
    } else {
        if (! defined COMMON::Array::isInArray($params->{sampleformat}, @SAMPLEFORMATS)) {
            ERROR (sprintf "Unknown 'sampleformat' : %s !",$params->{sampleformat});
            return undef;
        }
    }
    $this->{sampleformat} = $params->{sampleformat};

    ### Samples per pixel : REQUIRED
    if (! exists $params->{samplesperpixel} || ! defined $params->{samplesperpixel}) {
        ERROR ("'samplesperpixel' required !");
        return undef;
    } else {
        if (! defined COMMON::Array::isInArray($params->{samplesperpixel}, @SAMPLESPERPIXELS)) {
            ERROR (sprintf "Unknown 'samplesperpixel' : %s !",$params->{samplesperpixel});
            return undef;
        }
    }
    $this->{samplesperpixel} = int($params->{samplesperpixel});

    ### Photometric :  REQUIRED
    if (! exists $params->{photometric} || ! defined $params->{photometric}) {
        ERROR ("'photometric' required !");
        return undef;
    } else {
        if (! defined COMMON::Array::isInArray($params->{photometric}, @PHOTOMETRICS)) {
            ERROR (sprintf "Unknown 'photometric' : %s !",$params->{photometric});
            return undef;
        }
    }
    $this->{photometric} = $params->{photometric};

    ### Bits per sample : REQUIRED
    if (! exists $params->{bitspersample} || ! defined $params->{bitspersample}) {
        ERROR ("'bitspersample' required !");
        return undef;
    } else {
        if (! defined COMMON::Array::isInArray($params->{bitspersample}, @BITSPERSAMPLES)) {
            ERROR (sprintf "Unknown 'bitspersample' : %s !",$params->{bitspersample});
            return undef;
        }
    }
    $this->{bitspersample} = int($params->{bitspersample});

    # If image own one-bit sample, conversion will be done, so it's like a 8-bit image
    if ($this->{bitspersample} == 1) {
        INFO("We have a one-bit pixel, we memorize an 8-bit pixel because on fly conversion will be done");
        $this->{bitspersample} = 8;
    }

    return $this;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getPhotometric
sub getPhotometric {
    my $this = shift;
    return $this->{photometric};
}

# Function: getSampleFormat
sub getSampleFormat {
    my $this = shift;
    return $this->{sampleformat};
}

# Function: getBitsPerSample
sub getBitsPerSample {
    my $this = shift;
    return $this->{bitspersample};
}

# Function: getSamplesPerPixel
sub getSamplesPerPixel {
    my $this = shift;
    return $this->{samplesperpixel};
}

# Function: equals
sub equals {
    my $this = shift;
    my $other = shift;

    return (
        $this->{samplesperpixel} eq $other->getSamplesPerPixel() &&
        $this->{sampleformat} eq $other->getSampleFormat() &&
        $this->{photometric} eq $other->getPhotometric() &&
        $this->{bitspersample} eq $other->getBitsPerSample()
    );
}

=begin nd
Function: convertible

Tests if conversion is allowed between two pixel formats

Parameters (list):
    other - <COMMON::Pixel> - Destination pixel for conversion to test
=cut
sub convertible {
    my $this = shift;
    my $other = shift;

    if ($this->equals($other)) {
        return TRUE;
    }


    # La conversion se fait par la classe de la libimage PixelConverter, dont une instance est ajoutée à un FileImage pour convertir à la volée
    # Les tests de faisabilité ici doivent être identiques à ceux dans PixelConverter :
    # ----------------------------- PixelConverter constructor : C++ ----------------------------------
    # if (inSampleFormat == SampleFormat::FLOAT || outSampleFormat == SampleFormat::FLOAT) {
    #     LOGGER_WARN("PixelConverter doesn't handle float samples");
    #     return;
    # }
    # if (inSampleFormat != outSampleFormat) {
    #     LOGGER_WARN("PixelConverter doesn't handle different samples format");
    #     return;
    # }
    # if (inBitsPerSample != outBitsPerSample) {
    #     LOGGER_WARN("PixelConverter doesn't handle different number of bits per sample");
    #     return;
    # }

    # if (inSamplesPerPixel == outSamplesPerPixel) {
    #     LOGGER_WARN("PixelConverter have not to be used if number of samples per pixel is the same");
    #     return;
    # }

    # if (inBitsPerSample != 8) {
    #     LOGGER_WARN("PixelConverter only handle 8 bits sample");
    #     return;
    # }
    # -------------------------------------------------------------------------------------------------

    if ($this->getSampleFormat() eq "float" || $other->getSampleFormat() eq "float") {
        # aucune conversion pour des canaux flottant
        return FALSE;
    }

    if ($this->getSampleFormat() ne $other->getSampleFormat()) {
        return FALSE;
    }

    if ($this->getBitsPerSample() != $other->getBitsPerSample()) {
        return FALSE;
    }

    if ($this->getBitsPerSample() != 8) {
        return FALSE;
    }

    return TRUE;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all pixel's components. Useful for debug.

Example:
    (start code)
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
    
    $export .= "\nObject COMMON::Pixel :\n";
    $export .= sprintf "\t Bits per sample : %s\n", $this->{bitspersample};
    $export .= sprintf "\t Photometric : %s\n", $this->{photometric};
    $export .= sprintf "\t Sample format : %s\n", $this->{sampleformat};
    $export .= sprintf "\t Samples per pixel : %s\n", $this->{samplesperpixel};
    
    return $export;
}

1;
__END__
