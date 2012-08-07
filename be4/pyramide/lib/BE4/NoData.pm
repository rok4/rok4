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
use constant CREATE_NODATA     => "createNodata";

################################################################################

BEGIN {}
INIT {}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    * pixel => undef, # Pixel object
    * value => undef, # FF per sample or -99999 by default !
    * nowhite => undef, # FALSE by default
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
        pixel           => undef, # Pixel object
        value           => undef, # FF per sample or -99999 by default !
        nowhite         => undef, # FALSE by default
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
    # All attributes have to be present in parameters and defined, except 'value' which could be undefined

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

    if (! exists  $params->{pixel} || ! defined  $params->{pixel}) {
        ERROR ("Parameter 'pixel' required !");
        return FALSE;
    }
    $self->{pixel} = $params->{pixel};

    if (! exists  $params->{value}) {
        ERROR ("Parameter 'value' required !");
        return FALSE;
    }

#   for nodata value, it has to be coherent with bitspersample/sampleformat :
#       - 32/float -> an integer in decimal format (-99999 for a DTM for example)
#       - 8/uint -> a uint in hexadecimal format (FF for example. Just first two are used)
    if (! exists $params->{value} || ! defined ($params->{value})) {
        if (int($self->{pixel}->{bitspersample}) == 32 && $self->{pixel}->{sampleformat} eq 'float') {
            WARN ("Parameter 'nodata value' has not been set. The default value is -99999");
            $params->{value} = '-99999';
        } elsif (int($self->{pixel}->{bitspersample}) == 8 && $self->{pixel}->{sampleformat} eq 'uint') {
            WARN ("Parameter 'nodata value' has not been set. The default value is FFFFFF");
            $params->{value} = 'FF'x($self->{pixel}->{samplesperpixel});
        } else {
            ERROR ("sampleformat/bitspersample not supported !");
            return FALSE;
        }
    } else {
        if (int($self->{pixel}->{bitspersample}) == 32 && $self->{pixel}->{sampleformat} eq 'float') {
            if (!($params->{value} =~ m/^[-+]?(\d)+$/)) {
                ERROR (sprintf "Incorrect parameter nodata for a float32 pixel's format (%s) !",$params->{value});
                return FALSE;
            }
        } elsif (int($self->{pixel}->{bitspersample}) == 8 && $self->{pixel}->{sampleformat} eq 'uint') {
            if (!($params->{value}=~m/^[A-Fa-f0-9]{2,}$/)) {
                ERROR (sprintf "Incorrect parameter nodata for this int8 pixel's format (%s) !",$params->{value});
                return FALSE;
            }
        } else {
            ERROR ("sampleformat/bitspersample not supported !");
            return FALSE;
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

sub getNoWhite {
    my $self = shift;
    return $self->{nowhite};
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
      compression - compression to apply : raw/none, png, pjg, lzw, zip.

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
        #create folders
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

#
=begin nd
   method: getNodataName

   Return the name of the nodata tile : nd.tif
=cut
sub getNodataFilename {
    my $self = shift;
    
    return "nd.tif";
}

1;
__END__

=head1 NAME

BE4::NoData - components of nodata

=head1 SYNOPSIS
    
    use BE4::NoData;

    # NoData object creation
    my $objNodata = BE4::NoData->new({
            pixel            => objPixel,
            color            => "FFFFFF",
            nowhite          => TRUE
    });

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item pixel

A Pixel object, the same as the cache one.

=item value

For unsigned 8-bits integer sample : integer between 0 and 255 in hexadecimal format, for each sample (FFFFFF for white). For 32-bits float sample : an signed integer (-99999).

=item nowhite

This boolean will be used in mergeNtiff. If it's TRUE, when images are stacked, white pixel are ignored. As this treatment is longer and often useless , default value is FALSE.

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
