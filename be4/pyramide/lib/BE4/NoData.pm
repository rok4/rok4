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

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {

%HEX2DEC = (
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
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    path_nodata     => undef,
    #
    imagesize       => '4096', # ie 4096 px by default !
    pixel           => undef, # Pixel object
    color           => undef, # ie FFFFFF by default !
    nowhite         => FALSE, # false by default !
    # no interpolation for the tile image !
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


    if (! exists  $params->{path_nodata} || ! defined  $params->{path_nodata}) {
        ERROR ("Parameter 'path_nodata' required !");
        return FALSE;
    }
    if (! -d $params->{path_nodata}) {
        ERROR ("Directory doesn't exist !");
        return FALSE;
    }
    $self->{path_nodata} = $params->{path_nodata};


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
            my $valueDec = $self->hex2dec($params->{color});
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

        foreach $value (@nodata) {
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
################################################################################
# get /set
sub getColor {
    my $self = shift;
    return $self->{color};
}

################################################################################
# public method

sub hex2dec {
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

        if (! (exists $HEX2DEC{substr($hex,$i,1)} &&  exists $HEX2DEC{substr($hex,$i+1,1)}) ) {
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
  

        nowhite           => [TRUE,FALSE],

=head1 DESCRIPTION


=head2 EXPORT

None by default.

=head1 SEE ALSO

 BE4::Pixel

=head1 AUTHOR

Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
