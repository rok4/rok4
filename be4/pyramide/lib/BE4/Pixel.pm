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
# Global
my %PIXEL;

################################################################################

BEGIN {}
INIT {

%PIXEL = (
    bitspersample     => [8,32],
    sampleformat      => ['uint','float'],
    photometric       => ['rgb','gray','mask'],
    samplesperpixel   => [1,3,4],
);

}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    * photometric
    * sampleformat
    * bitspersample
    * samplesperpixel
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        photometric => undef,
        sampleformat => undef,
        bitspersample => undef,
        samplesperpixel => undef,
    };

    bless($self, $class);

    TRACE;

    # All attributes have to be present in parameters and defined

    my $sampleformat = $params->{sampleformat};
    if ( ! defined $sampleformat || ! $self->is_SampleFormat($sampleformat)) {
        ERROR (sprintf "Unknown 'sampleformat' (%s) !",$sampleformat);
        return undef;
    }
    $self->{sampleformat} = $sampleformat;

    my $samplesperpixel = $params->{samplesperpixel};
    if (! defined $samplesperpixel || ! $self->is_SamplesPerPixel($samplesperpixel)) {
        ERROR (sprintf "Unknown 'samplesperpixel' (%s) !",$samplesperpixel);
        return undef;
    }
    $self->{samplesperpixel} = int($samplesperpixel);

    my $photometric = $params->{photometric};
    if (! defined $photometric || ! $self->is_Photometric($photometric)) {
        ERROR (sprintf "Unknown 'photometric' (%s) !",$photometric);
        return undef;
    }
    $self->{photometric} = $photometric;

    my $bitspersample = $params->{bitspersample};
    if (! defined $bitspersample || ! $self->is_BitsPerSample($bitspersample)) {
        ERROR (sprintf "Unknown 'bitspersample' (%s) !",$bitspersample);
        return undef;
    }
    $self->{bitspersample} = int($bitspersample);


    return $self;
}

####################################################################################################
#                                     ATTRIBUTE TESTS                                              #
####################################################################################################

# Group: attribute tests

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
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getPhotometric {
    my $self = shift;
    return $self->{photometric};
}

sub getSampleFormat {
    my $self = shift;
    return $self->{sampleformat};
}

sub getBitsPerSample {
    my $self = shift;
    return $self->{bitspersample};
}

sub getSamplesPerPixel {
    my $self = shift;
    return $self->{samplesperpixel};
}


1;
__END__

=head1 NAME

BE4::Pixel - components of a pixel in output images

=head1 SYNOPSIS

    use BE4::Pixel;
  
    my $objC = BE4::Pixel->new({
        sampleformat => "uint",
        photometric => "rgb",
        samplesperpixel => 3,
        bitspersample => 8,
    });

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item photometric

Possible values : rgb, gray

=item sampleformat

Possible values : uint, float

=item bitspersample

Possible values : 8, 32

=item samplesperpixel

Possible values : 1, 3, 4

=back

=head1 SEE ALSO

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
