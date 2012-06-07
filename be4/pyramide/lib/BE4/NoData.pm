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

package BE4::NoData;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

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
my %HEX2DEC;

################################################################################

BEGIN {}
INIT {

my %HEX2DEC = (
    0 => 0,
    1 => 1,
    2 => 2,
    3 => 3,
    4 => 4,
    5 => 5,
    6 => 6,
    7 => 7,
    8 => 8,
    9 => 9,
    A => 10,
    B => 11,
    C => 12,
    D => 13,
    E => 14,
    F => 15,
);

}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    * imagesize       => '4096',
    * pixel           => undef, # Pixel object
    * color           => undef, # 255 (uint) or -99999 (float) per sample by default
    * nowhite         => FALSE,
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    #
    imagesize       => '4096', # ie 4096 px by default !
    pixel           => undef, # Pixel object
    color           => undef,
    nowhite         => FALSE, # false by default !
    # no interpolation for the tile image !
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  
  return $self;
}


sub _init {
    my $self = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # init. params
    # All attributes have to be present in parameters and defined, except 'color' which could be undefined

    if (! exists  $params->{nowhite} || ! defined  $params->{nowhite}) {
        ERROR ("Parameter 'nowhite' required !");
        return FALSE;
    }
    if (lc $params->{nowhite} eq 'true') {
        $self->{nowhite} = TRUE;
    }
    elsif (lc $params->{nowhite} eq 'false') {
        $self->{nowhite} = FALSE;
    } else {
        ERROR (sprintf "Parameter 'nowhite' is not valid (%s). Possible values are true or false !",$params->{nowhite});
        return FALSE;
    }

    if (! exists  $params->{imagesize} || ! defined  $params->{imagesize}) {
        ERROR ("Parameter 'imagesize' required !");
        return FALSE;
    }
    $self->{imagesize} = $params->{imagesize};


    if (! exists  $params->{pixel} || ! defined  $params->{pixel}) {
        ERROR ("Parameter 'pixel' required !");
        return FALSE;
    }
    $self->{pixel} = $params->{pixel};

    if (! exists  $params->{color}) {
        ERROR ("Parameter 'color' required !");
        return FALSE;
    }

#   for nodata value, it has to be coherent with bitspersample/sampleformat :
#       - 32/float -> an integer in decimal format (-99999 for a DTM for example)
#       - 8/uint -> a uint in decimal format (255 for example)
    if (! defined ($params->{color})) {
        if (int($self->{pixel}->{bitspersample}) == 32 && $self->{pixel}->{sampleformat} eq 'float') {
            WARN ("Parameter 'nodata.color' has not been set. The default value is -99999");
            $params->{color} = '-99999';
            $params->{color} .= ',-99999'x($self->{pixel}->{samplesperpixel}-1);
        } elsif (int($self->{pixel}->{bitspersample}) == 8 && $self->{pixel}->{sampleformat} eq 'uint') {
            WARN ("Parameter 'nodata.color' has not been set. The default value is 255");
            $params->{color} = '255';
            $params->{color} .= ',255'x($self->{pixel}->{samplesperpixel}-1);
        } else {
            ERROR ("sampleformat/bitspersample not supported !");
            return FALSE;
        }
    } else {

        if (int($self->{pixel}->{bitspersample}) == 8 && $self->{pixel}->{sampleformat} eq 'uint' &&
             $params->{color} =~ m/^[0-9A-F]{2,}$/) {

            WARN (sprintf "Nodata value in hexadecimal format (%s) is deprecated, use decimal format instead !",
                $params->{color});
            # nodata is supplied in hexadecimal format, we convert it
            my $valueDec = $self->hexToDec($params->{color});
            if (! defined $valueDec) {
                ERROR (sprintf "Incorrect value for nodata in hexadecimal format '%s' ! Impossible to convert",
                    $params->{color});
                return FALSE;
            }
            WARN (sprintf "Nodata value in hexadecimal format have been converted : %s ",$valueDec);
            $params->{color} = $valueDec;
        }

        $params->{color} =~ s/ //;
        my @nodata = split(/,/,$params->{color},-1);
        if (scalar @nodata != int($self->{pixel}->{samplesperpixel})) {
            ERROR (sprintf "Incorrect parameter nodata (%s) : we need one value per sample (%s), seperated by ',' !",
                $params->{color},$self->{pixel}->{samplesperpixel});
            return FALSE;
        }

        foreach my $value (@nodata) {
            if (int($self->{pixel}->{bitspersample}) == 32 && $self->{pixel}->{sampleformat} eq 'float') {
                if ( $value !~ m/^[-+]?[0-9]+$/ ) {
                    ERROR (sprintf "Incorrect value for nodata for a float32 pixel's format (%s) !",$value);
                    return FALSE;
                }
            } elsif (int($self->{pixel}->{bitspersample}) == 8 && $self->{pixel}->{sampleformat} eq 'uint') {
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
    
    $self->{color} = $params->{color};
    
    return TRUE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getColor {
    my $self = shift;
    return $self->{color};
}

####################################################################################################
#                                       PUBLIC METHODS                                             #
####################################################################################################

# Group: public methods

=begin nd
method: hexToDec

From a color value in hexadecimal format (string), convert in decimal format (string). Different samples are seperated by comma. Input string must have an even length (one sample <=> 2character).

Example : hexToDec("7BFF0300") = "123,255,3,0"
=cut
sub hexToDec {
    my $self = shift;
    my $hex = shift;

    if (length($hex) % 2 != 0) {
        ERROR ("Length of an hexadecimal nodata must be even");
        return undef;
    }

    my $dec = "";

    my $i = 0;
    while ($i < length($hex)) {
        $dec .= "," if ($i > 1);

        if (! (exists $HEX2DEC{substr($hex,$i,1)} && exists $HEX2DEC{substr($hex,$i+1,1)}) ) {
            ERROR ("A character in not valid in the hexadecimal value of nodata");
            return undef;
        }
        my $b1 = $HEX2DEC{substr($hex,$i,1)};
        my $b0 = $HEX2DEC{substr($hex,$i+1,1)};
        $dec .= $b0 + 16*$b1;

        $i += 2;
    }

    return $dec;
}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

    BE4::NoData - components of nodata

=head1 SYNOPSIS
    
    use BE4::NoData;

    # NoData object creation
    my $objNodata = BE4::NoData->new({
            pixel            => objPixel,
            imagesize        => 4096, 
            color            => "0,255,123",
            nowhite          => TRUE
    });

=head1 DESCRIPTION

    A NoData object :

        * pixel (Pixel object)
        * imagesize
        * color
        * nowhite

    The boolean nowhite will be used in mergeNtiff. If it's TRUE, when images are stacked, white pixel are ignored.
    As this treatment is longer and often useless , default value is FALSE.

    The color is a string and contain on value per sample, in decimal format, seperated by comma. For 8 bits unsigned integer, value must be between 0 and 255. For 32 bits float, an integer is expected too, but can be negative.
    Example : 
        - "255,255,255" (white) for images whithout alpha sample
        - "-99999" for a DTM

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
