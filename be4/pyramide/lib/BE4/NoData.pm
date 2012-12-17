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
use File::Spec::Link;
use File::Basename;
use File::Spec;
use File::Path;

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
use constant CREATE_NODATA => "createNodata";

################################################################################
# Global
my %HEX2DEC;

################################################################################

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
=begin nd
Group: variable

variable: $self
    * pixel : BE4::Pixel
    * value - 255 (uint) or -99999 (float) per sample
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
        pixel           => undef,
        value           => undef,
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
    # Mandatory : pixel
    # Optionnal : value

    ### Pixel
    if (! exists  $params->{pixel} || ! defined  $params->{pixel}) {
        ERROR ("Parameter 'pixel' required !");
        return FALSE;
    }
    $self->{pixel} = $params->{pixel};

    ### Value
    # For nodata value, it has to be coherent with bitspersample/sampleformat :
    #       - 32/float -> an integer in decimal format (-99999 for a DTM for example)
    #       - 8/uint -> a uint in decimal format (255 for example)
    if (! exists $params->{value} || ! defined ($params->{value})) {
        if ($self->{pixel}->getBitsPerSample == 32 && $self->{pixel}->getSampleFormat eq 'float') {
            WARN ("Parameter 'nodata value' has not been set. The default value is -99999 per sample");
            $params->{value} .= '-99999' . ',-99999'x($self->getPixel->getSamplesPerPixel - 1);
        } elsif ($self->{pixel}->getBitsPerSample == 8 && $self->{pixel}->getSampleFormat eq 'uint') {
            WARN ("Parameter 'nodata value' has not been set. The default value is 255 per sample");
            $params->{value} = '255' . ',255'x($self->{pixel}->getSamplesPerPixel - 1);
        } else {
            ERROR ("sampleformat/bitspersample not supported !");
            return FALSE;
        }
    } else {
        $params->{value} =~ s/ //g;

        if ($self->{pixel}->getBitsPerSample == 8 &&
            $self->{pixel}->getSampleFormat eq 'uint' &&
            $params->{value} !~ m/^[0-9,]+$/) {

            WARN (sprintf "Nodata value in hexadecimal format (%s) is deprecated, use decimal format instead !",
                $params->{value});
            
            # nodata is supplied in hexadecimal format, we convert it
            my $valueDec = $self->hexToDec($params->{value});
            if (! defined $valueDec) {
                ERROR (sprintf "Incorrect value for nodata in hexadecimal format '%s' ! Unable to convert",
                    $params->{value});
                return FALSE;
            }
            WARN (sprintf "Nodata value in hexadecimal format have been converted : %s ",$valueDec);
            $params->{value} = $valueDec;
        }
        
        my @nodata = split(/,/,$params->{value},-1);
        if (scalar @nodata != $self->{pixel}->getSamplesPerPixel) {
            ERROR (sprintf "Incorrect parameter nodata (%s) : we need one value per sample (%s), seperated by ',' !",
                $params->{value},$self->{pixel}->getSamplesPerPixel);
            return FALSE;
        }

        foreach my $value (@nodata) {
            if ($self->{pixel}->getBitsPerSample == 32 && $self->{pixel}->getSampleFormat eq 'float') {
                if ( $value !~ m/^[-+]?[0-9]+$/ ) {
                    ERROR (sprintf "Incorrect value for nodata for a float32 pixel's format (%s) !",$value);
                    return FALSE;
                }
            } elsif ($self->{pixel}->getBitsPerSample == 8 && $self->{pixel}->getSampleFormat eq 'uint') {
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
    
    $self->{value} = $params->{value};

    return TRUE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getValue {
    my $self = shift;
    return $self->{value};
}

#
=begin nd
method: getNodataFilename

Returns:
    The name of the nodata tile : nd.tif
=cut
sub getNodataFilename {
    my $self = shift;
    
    return "nd.tif";
}

####################################################################################################
#                                           COMMAND                                                #
####################################################################################################

# Group: command

#
=begin nd
method: createNodata

Compose the command to create a nodata tile and execute it. The tile's name is given by the method getNodataName.

Parameters:
    nodataDirPath - complete absolute directory path, where to write the nodata tile ("/path/to/write/")
    width - width in pixel of the tile (256)
    height - height in pixel of the tile (256)
    compression - compression to apply : raw/none, png, jpg, lzw, zip, pkb.

Returns:
    TRUE if the nodata tile is succefully written, FALSE otherwise.
=cut
sub createNodata {
    my $self = shift;
    my $nodataDirPath = shift;
    my $width = shift;
    my $height = shift;
    my $compression = shift;
    
    TRACE();
    
    my $nodataFilePath = File::Spec->catfile($nodataDirPath,$self->getNodataFilename());
    
    my $cmd = sprintf ("%s -n %s",CREATE_NODATA, $self->{value});
    $cmd .= sprintf ( " -c %s", $compression);
    $cmd .= sprintf ( " -p %s", $self->{pixel}->getPhotometric);
    $cmd .= sprintf ( " -t %s %s",$width,$height);
    $cmd .= sprintf ( " -b %s", $self->{pixel}->getBitsPerSample);
    $cmd .= sprintf ( " -s %s", $self->{pixel}->getSamplesPerPixel);
    $cmd .= sprintf ( " -a %s", $self->{pixel}->getSampleFormat);
    $cmd .= sprintf ( " %s", $nodataFilePath);

    if (! -d $nodataDirPath) {
        # create folders
        eval { mkpath([$nodataDirPath]); };
        if ($@) {
            ERROR(sprintf "Can not create the nodata directory '%s' : %s !", $nodataDirPath , $@);
            return FALSE;
        }
    }
    
    if (! system($cmd) == 0) {
        ERROR (sprintf "The command to create a nodata tile is incorrect : '%s'",$cmd);
        return FALSE;
    }

    return TRUE; 
}

# Group: public methods

=begin nd
method: hexToDec

From a color value in hexadecimal format (string), convert in decimal format (string). Different samples are seperated by comma. Input string must have an even length (one sample <=> 2 character).

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

        if (! exists $HEX2DEC{substr($hex,$i,1)} ) {
            ERROR (sprintf "A character in not valid in the hexadecimal value of nodata : %s", substr($hex,$i,1));
            return undef;
        }
        
        if (! exists $HEX2DEC{substr($hex,$i+1,1)} ) {
            ERROR (sprintf "A character in not valid in the hexadecimal value of nodata : %s",substr($hex,$i,1));
            return undef;
        }
        
        my $b1 = $HEX2DEC{substr($hex,$i,1)};
        my $b0 = $HEX2DEC{substr($hex,$i+1,1)};
        $dec .= $b0 + 16*$b1;

        $i += 2;
    }

    return $dec;
}

####################################################################################################
#                                          EXPORT METHODS                                          #
####################################################################################################

# Group: export methods

sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject BE4::NoData :\n";
    $export .= sprintf "\t Value : %s\n", $self->{value};
    
    return $export;
}

1;
__END__

=head1 NAME

BE4::NoData - components of nodata

=head1 SYNOPSIS
    
    use BE4::NoData;

    # NoData object creation
    my $objNodata = BE4::NoData->new({
            pixel   => $objPixel,
            value   => "255,255,255"
    });


=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item pixel

A Pixel object, the same as the cache one.

=item value

The color is a string and contain on value per sample, in decimal format, seperated by comma. For 8 bits unsigned integer, value must be between 0 and 255. For 32 bits float, an integer is expected too, but can be negative.

Example : "255,255,255" (white) for images whithout alpha sample, "-99999" for a DTM.

=back

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-Pixel.html">BE4::Pixel</A></li>
</ul>

=end html

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
