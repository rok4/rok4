# Copyright © (2011) Institut national de l'information
#                    géographique et forestière 
# 
# Géoportail SAV <contact.geoservices@ign.fr>
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
File: NoData.pm

Class: COMMON::NoData

(see ROK4GENERATION/libperlauto/COMMON_NoData.png)

Define Nodata informations and tools.

Using:
    (start code)
    use COMMON::NoData;

    # NoData object creation
    my $objNodata = COMMON::NoData->new({
        pixel   => $objPixel,
        value   => "255,255,255"
    });
    (end code)

Attributes:
    pixel - <COMMON::Pixel> - Components of a nodata pixel.

    value - string - Contains one integer value per sample, in decimal format, separated by comma. For 8 bits unsigned integer, value must be between 0 and 255. For 32 bits float, an integer is expected too, but can be negative.
    Example : "255,255,255" (white) for images whithout alpha sample, "-99999" for a DTM.
=cut

################################################################################

package COMMON::NoData;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Spec;
use File::Path;

use COMMON::Pixel;

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

NoData constructor. Bless an instance.

Parameters (hash):
    pixel - <Pixel> - Nodata pixel
    value - string - Optionnal, value (color) to use when no input data
=cut
sub new {
    my $class = shift;
    my $params = shift;
    
    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        pixel           => undef,
        value           => undef,
    };
    
    bless($this, $class);
    
    
    # init. class
    return undef if (! $this->_init($params));
    
    return $this;
}

=begin nd
Function: _init

Check and store nodata attributes values. Define the default value if not supplied.
    - 255 per unsigned 8-bit integer sample
    - -99999 per 32-bit float sample

Parameters (hash):
    pixel - <Pixel> - Nodata pixel
    value - string - Optionnal, value (color) to use when no input data
=cut
sub _init {
    my $this = shift;
    my $params = shift;

    
    return FALSE if (! defined $params);
    
    # init. params
    # Mandatory : pixel
    # Optionnal : value

    ### Pixel
    if (! exists  $params->{pixel} || ! defined  $params->{pixel}) {
        ERROR ("Parameter 'pixel' required !");
        return FALSE;
    }
    $this->{pixel} = $params->{pixel};

    ### Value
    # For nodata value, it has to be coherent with bitspersample/sampleformat :
    #       - 32/float -> an integer in decimal format (-99999 for a DTM for example)
    #       - 8/uint -> a uint in decimal format (255 for example)
    if (! exists $params->{value} || ! defined ($params->{value})) {
        if ($this->{pixel}->getBitsPerSample == 32 && $this->{pixel}->getSampleFormat eq 'float') {
            WARN ("Parameter 'nodata value' has not been set. The default value is -99999 per sample");
            $params->{value} .= '-99999' . ',-99999'x($this->{pixel}->getSamplesPerPixel - 1);
        } elsif ($this->{pixel}->getBitsPerSample == 8 && $this->{pixel}->getSampleFormat eq 'uint') {
            WARN ("Parameter 'nodata value' has not been set. The default value is 255 per sample");
            $params->{value} = '255' . ',255'x($this->{pixel}->getSamplesPerPixel - 1);
        } else {
            ERROR ("sampleformat/bitspersample not supported !");
            return FALSE;
        }
    } else {
        $params->{value} =~ s/ //g;
        
        my @nodata = split(/,/, $params->{value}, -1);
        if (scalar @nodata != $this->{pixel}->getSamplesPerPixel() ) {
            ERROR (sprintf "Incorrect parameter nodata (%s) : we need one value per sample (%s), separated by ',' !",
                $params->{value},$this->{pixel}->getSamplesPerPixel);
            return FALSE;
        }

        foreach my $value (@nodata) {
            if ($this->{pixel}->getBitsPerSample == 32 && $this->{pixel}->getSampleFormat eq 'float') {
                if ( $value !~ m/^[-+]?[0-9]+$/ ) {
                    ERROR (sprintf "Incorrect value for nodata for a float32 pixel's format (%s) !",$value);
                    return FALSE;
                }
            } elsif ($this->{pixel}->getBitsPerSample == 8 && $this->{pixel}->getSampleFormat eq 'uint') {
                if ( $value !~ m/^[0-9]+$/ ) {
                    ERROR (sprintf "Incorrect value for nodata for a uint8 pixel's in decimal format '%s' !",$value);
                    return FALSE;
                }
                if ( $value > 255 ) {
                    ERROR (sprintf "Incorrect value for nodata for a uint8 pixel's format %s : greater than 255 !",$value);
                    return FALSE;
                }
            } else {
                ERROR ("sampleformat/bitspersample not supported !");
                return FALSE;
            }
        }
    }
    
    $this->{value} = $params->{value};

    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getValue
sub getValue {
    my $this = shift;
    return $this->{value};
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

Returns all nodata's informations. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $this = shift ;
    
    my $export = "";
    
    $export .= "\nObject COMMON::NoData :\n";
    $export .= sprintf "\t Value : %s\n", $this->{value};
    
    return $export;
}

1;
__END__
