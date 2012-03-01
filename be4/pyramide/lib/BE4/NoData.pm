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
INIT {}
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
    bitspersample   => undef, # ie 8
    samplesperpixel => undef, # ie 3
    sampleformat    => undef, # ie uint
    photometric     => undef, # ie rgb
    color           => 'FFFFFF', # ie FFFFFF by default !
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
    
    my $args = $params;
    
    if (ref($args) ne "HASH") {
      ERROR ("Parameters in a HASH structure required !");
      return undef;
    }

    TRACE;
    
    return FALSE if (! defined $args);
    
    # init. params
    if (! exists  ($args->{imagesize})) {
      WARN ("Parameter 'imagesize' by default !");
    }
    if (! exists  ($args->{bitspersample})) {
      ERROR ("Parameter 'bitspersample' required !");
      return FALSE;
    }
    if (! exists  ($args->{samplesperpixel})) {
      ERROR ("Parameter 'samplesperpixel' required !");
      return FALSE;
    }
    if (! exists  ($args->{sampleformat})) {
      ERROR ("Parameter 'sampleformat' required !");
      return FALSE;
    }
    if (! exists  ($args->{photometric})) {
      ERROR ("Parameter 'photometric' required !");
      return FALSE;
    }
    if (! exists  ($args->{color})) {
      WARN ("Parameter 'color' by default !");
    }
    #
    if (! exists  ($args->{path_nodata})) {
      ERROR ("Parameter 'path' required !");
      return FALSE;
    }
    #
    if (! -d $args->{path_nodata}) {
      ERROR ("Directory doesn't exist !");
      return FALSE;
    }
    #
    $self->{path_nodata}    = $args->{path_nodata};
    $self->{imagesize}      = $args->{imagesize} if (exists  ($args->{imagesize}) && defined ($args->{imagesize}));
    $self->{bitspersample}  = $args->{bitspersample};
    $self->{samplesperpixel}= $args->{samplesperpixel};
    $self->{sampleformat}   = $args->{sampleformat};
    $self->{photometric}    = $args->{photometric};
#   for nodata value, it has to be coherent with bitspersample/sampleformat :
#       - 32/float -> an integer in decimal format (-99999 for a DTM for example)
#       - 8/uint -> a uint in hexadecimal format (FF for example. Just first two are used)
    if (exists  ($args->{color}) && defined ($args->{color})) {
        if (int($args->{bitspersample}) == 32 && $args->{sampleformat} eq 'float') {
            if (!($args->{color} =~ m/^[-+]?(\d)+$/)) {
                ERROR ("Incorrect parameter nodata for this bitspersample/sampleformat couple !");
                return FALSE;
            }
        } elsif (int($args->{bitspersample}) == 8 && $args->{sampleformat} eq 'uint') {
            if (!($args->{color}=~m/^[A-Fa-f0-9]{2,}$/)) {
                ERROR ("Incorrect parameter nodata for this bitspersample/sampleformat couple !");
                return FALSE;
            }
        } else {
            ERROR ("sampleformat/bitspersample not supported !");
            return FALSE;
        }
    
        $self->{color} =$args->{color};
    }
    
    return TRUE;
}
################################################################################
# get /set
sub getColor {
  my $self = shift;
  return $self->{color};
}
sub getFile {
  my $self = shift;
  
  my $imagefile = join(".", $self->getName(), "tif");
  
  return $imagefile;
}
sub getPath {
  my $self = shift;
  
  my $path = File::Spec->catdir($self->{path_nodata}, $self->{imagesize});
  
  return $path;
}
sub getName {
  my $self = shift;
  
  my $imagename = join("_",
                    "nodata",
                    $self->{sampleformat}.$self->{bitspersample},
                    $self->{photometric},
                    $self->{samplesperpixel},
                    $self->{color},
                  );
  
  return $imagename;
}
################################################################################
# public method
sub createImageNoData {
  my $self = shift;
  
  # TODO : create on runtime a tile image !
  return TRUE;
}
################################################################################
# static method
sub getImageNoData {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  my $args = shift;
  
  if (ref($args) ne "HASH") {
    ERROR ("Parameters in a HASH structure required !");
    return undef;
  }
  
  TRACE;
  
  my $imagename = undef;
  
  # optional !
  #if (! exists  ($args->{imagesize})) {
  #  ERROR ("Parameter 'imagesize' required !");
  #  return undef;
  #}
  if (! exists  ($args->{bitspersample})) {
    ERROR ("Parameter 'bitspersample' required !");
    return undef;
  }
  if (! exists  ($args->{samplesperpixel})) {
    ERROR ("Parameter 'samplesperpixel' required !");
    return undef;
  }
  if (! exists  ($args->{sampleformat})) {
    ERROR ("Parameter 'sampleformat' required !");
    return undef;
  }
  if (! exists  ($args->{photometric})) {
    ERROR ("Parameter 'photometric' required !");
    return undef;
  }
  if (! exists  ($args->{color})) {
    ERROR ("Parameter 'color' required !");
    return undef;
  }
  
  $imagename = join("_",
                    "nodata",
                    $args->{sampleformat}.$args->{bitspersample},
                    $args->{photometric},
                    $args->{samplesperpixel},
                    $args->{color},
                  );
  $imagename .= ".tif";
  
  return $imagename;
}
1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

=head1 SYNOPSIS

=head1 DESCRIPTION

=head2 EXPORT

None by default.

=head1 SEE ALSO

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
