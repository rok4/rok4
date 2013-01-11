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

Class: BE4::Pixel

Store all pixel's intrinsic components.

Using:
    (start code)
    use BE4::Pixel;

    my $objC = BE4::Pixel->new({
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

package BE4::Pixel;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
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

# Variable: IMAGESPEC
# Define allowed values for attributes bitspersample, sampleformat, photometric and samplesperpixel.
my %PIXEL;

# Variable: DEFAULT
# Define default values for attribute photometric.
my %DEFAULT;

################################################################################

BEGIN {}
INIT {
    %PIXEL = (
        bitspersample     => [8,32],
        sampleformat      => ['uint','float'],
        photometric       => ['rgb','gray','mask'],
        samplesperpixel   => [1,3,4],
    );

    %DEFAULT = (
        photometric => 'rgb',
    );
}
END {}

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
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        photometric => undef,
        sampleformat => undef,
        bitspersample => undef,
        samplesperpixel => undef,
    };

    bless($self, $class);

    TRACE;

    # All attributes have to be present in parameters and defined

    ### Sample format : REQUIRED
    if (! exists $params->{sampleformat} || ! defined $params->{sampleformat}) {
        ERROR ("'sampleformat' required !");
        return undef;
    } else {
        if (! $self->is_SamplesPerPixel($params->{sampleformat})) {
            ERROR (sprintf "Unknown 'sampleformat' : %s !",$params->{sampleformat});
            return undef;
        }
    }
    $self->{sampleformat} = $params->{sampleformat};

    ### Samples per pixel : REQUIRED
    if (! exists $params->{samplesperpixel} || ! defined $params->{samplesperpixel}) {
        ERROR ("'samplesperpixel' required !");
        return undef;
    } else {
        if (! $self->is_SamplesPerPixel($params->{samplesperpixel})) {
            ERROR (sprintf "Unknown 'samplesperpixel' : %s !",$params->{samplesperpixel});
            return undef;
        }
    }
    $self->{samplesperpixel} = $params->{samplesperpixel};

    ### Photometric
    if (! exists $params->{photometric} || ! defined $params->{photometric}) {
        $params->{photometric} = $DEFAULT{photometric};
        INFO(sprintf "Default value for 'photometric' : %s", $params->{photometric});
    } else {
        if (! $self->is_Photometric($params->{photometric})) {
            ERROR (sprintf "Unknown 'photometric' : %s !",$params->{photometric});
            return undef;
        }
    }
    $self->{photometric} = $params->{photometric};

    ### Bits per sample : REQUIRED
    if (! exists $params->{bitspersample} || ! defined $params->{bitspersample}) {
        ERROR ("'bitspersample' required !");
        return undef;
    } else {
        if (! $self->is_BitsPerSample($params->{bitspersample})) {
            ERROR (sprintf "Unknown 'bitspersample' : %s !",$params->{bitspersample});
            return undef;
        }
    }
    $self->{bitspersample} = $params->{bitspersample};

    return $self;
}

####################################################################################################
#                             Group: Attributes' testers                                           #
####################################################################################################

=begin nd
Function: is_SampleFormat

Tests if sample format value is allowed.

Parameters (list):
    sampleformat - string - Sample format value to test
=cut
sub is_SampleFormat {
    my $self = shift;
    my $sampleformat = shift;

    TRACE;

    return FALSE if (! defined $sampleformat);

    foreach (@{$PIXEL{sampleformat}}) {
        return TRUE if ($sampleformat eq $_);
    }
    return FALSE;
}

=begin nd
Function: is_BitsPerSample

Tests if bits per sample value is allowed.

Parameters (list):
    bitspersample - string - Bits per sample value to test
=cut
sub is_BitsPerSample {
    my $self = shift;
    my $bitspersample = shift;

    TRACE;

    return FALSE if (! defined $bitspersample);

    foreach (@{$PIXEL{bitspersample}}) {
        if ($bitspersample eq $_) {
            return TRUE;
        }
    }
    return FALSE;
}

=begin nd
Function: is_Photometric

Tests if photometric value is allowed.

Parameters (list):
    photometric - string - Photometric value to test
=cut
sub is_Photometric {
    my $self = shift;
    my $photometric = shift;

    TRACE;

    return FALSE if (! defined $photometric);

    foreach (@{$PIXEL{photometric}}) {
        return TRUE if ($photometric eq $_);
    }
    return FALSE;
}

=begin nd
Function: is_SamplesPerPixel

Tests if samples per pixel value is allowed.

Parameters (list):
    samplesperpixel - string - Samples per pixel value to test
=cut
sub is_SamplesPerPixel {
    my $self = shift;
    my $samplesperpixel = shift;

    TRACE;

    return FALSE if (! defined $samplesperpixel);

    foreach (@{$PIXEL{samplesperpixel}}) {
        return TRUE if ($samplesperpixel eq $_);
    }
    return FALSE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getPhotometric
sub getPhotometric {
    my $self = shift;
    return $self->{photometric};
}

# Function: getSampleFormat
sub getSampleFormat {
    my $self = shift;
    return $self->{sampleformat};
}

# Function: getBitsPerSample
sub getBitsPerSample {
    my $self = shift;
    return $self->{bitspersample};
}

# Function: getSamplesPerPixel
sub getSamplesPerPixel {
    my $self = shift;
    return $self->{samplesperpixel};
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all pixel's components. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject BE4::Pixel :\n";
    $export .= sprintf "\t Bits per sample : %s\n", $self->{bitspersample};
    $export .= sprintf "\t Photometric : %s\n", $self->{photometric};
    $export .= sprintf "\t Sample format : %s\n", $self->{sampleformat};
    $export .= sprintf "\t Samples per pixel : %s\n", $self->{samplesperpixel};
    
    return $export;
}

1;
__END__
