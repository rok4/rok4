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
# version
our $VERSION = '0.0.1';

################################################################################
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
# Global
my %PIXEL;

################################################################################
# Preloaded methods go here.
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
# Group: variable
#

#
# variable: $self
#
#    * photometric
#    * sampleformat
#    * bitspersample
#    * samplesperpixel
#

################################################################################
# Group: constructor
#
sub new {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    my $self = {
        photometric => undef, # ie raw
        sampleformat => undef, # ie uint
        bitspersample => undef, # ie 8
        samplesperpixel => undef, # ie 3
    };

    bless($self, $class);

    TRACE;

    # All attributes have to be present in parameters and defined

    my $sampleformat = $params->{sampleformat};
    if ( ! defined $sampleformat || ! $self->is_SampleFormat($sampleformat)) {
        ERROR ("'sampleformat' is undefined or not valid !");
        return undef;
    }
    $self->{sampleformat} = $sampleformat;

    my $samplesperpixel = $params->{samplesperpixel};
    if (! defined $samplesperpixel || ! $self->is_SamplesPerPixel($samplesperpixel)) {
        ERROR ("'samplesperpixel' is undefined or not valid !");
        return undef;
    }
    $self->{samplesperpixel} = $samplesperpixel;

    my $photometric = $params->{photometric};
    if (! defined $photometric || ! $self->is_Photometric($photometric)) {
        ERROR ("'photometric' is undefined or not valid !");
        return undef;
    }
    $self->{photometric} = $photometric;

    my $bitspersample = $params->{bitspersample};
    if (! defined $bitspersample || ! $self->is_BitsPerSample($bitspersample)) {
        ERROR ("'bitspersample' is undefined or not valid !");
        return undef;
    }
    $self->{bitspersample} = $bitspersample;


    return $self;
}

################################################################################
# Group: public methods
#

sub is_SampleFormat {
    my $self = shift;
    my $sampleformat = shift;

    TRACE;

    return FALSE if (! defined $sampleformat);

    foreach (@{$PIXEL{sampleformat}}) {
        return TRUE if ($sampleformat eq $_);
    }
    ERROR (sprintf "Unknown 'sampleformat' (%s) !",$sampleformat);
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
    ERROR (sprintf "Unknown 'bitspersample' (%s) !",$bitspersample);
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
    ERROR (sprintf "Unknown 'photometric' (%s) !",$photometric);
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
    ERROR (sprintf "Unknown 'samplesperpixel' (%s) !",$samplesperpixel);
    return FALSE;
}

################################################################################
# Group: get
#
sub getPhotometric {
    my $self = shift;
    return $self->{photometric};
}

sub getSampleFormat {
    my $self = shift;
    return $self->{sampleformat};
}

sub getBitsperSample {
    my $self = shift;
    return $self->{bitspersample};
}

sub getSamplesperPixels {
    my $self = shift;
    return $self->{samplesperpixel};
}


1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

 BE4::Pixel - components of a pixel in output images

=head1 SYNOPSIS
  
  my $objC = BE4::Pixel->new("rgb","uint",8);
  


  # mode static 
  my @info = BE4::Format->decodeFormat("TIFF_RAW_INT8");  #  ie 'tiff', 'raw', 'uint' , '8' !

=head1 DESCRIPTION

  Format {
      compression       => raw/jpg/png/lzw (floatraw is deprecated, use raw instead)
      sampleformat      => uint/float
      bitspersample     => 8/32
      code              => TIFF_RAW_INT8 for example
  }
 
  'compression' is use for the program 'tiff2tile'.
  'code' is written in the pyramid file, and it is useful for 'Rok4' !

=head2 EXPORT

None by default.

=head1 SEE ALSO

=head1 AUTHOR

Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
