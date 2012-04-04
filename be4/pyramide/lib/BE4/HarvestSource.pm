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

package BE4::HarvestSource;

# use strict;
use warnings;

use Log::Log4perl qw(:easy);

use List::Util qw(min max);

# My module
use BE4::Harvesting;

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
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

# Group: constructor
#

################################################################################
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    baseLevel => undef, # bottom level using this harvesting
    harvesting => undef, # Harvesting object
    size_image => [4096,4096], # images size which will be harvested
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
    my $self   = shift;
    my $baseLevel = shift;
    my $harvestParams = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    ALWAYS(sprintf "harvestSource parameters : %s",Dumper($params)); #TEST#

    # parameters mandatoy !
    if (! defined $baseLevel) {
        ERROR("key/value required to 'baseLevel' !");
        return FALSE ;
    }

    if (! exists($params->{size_image}) || ! defined ($params->{size_image})) {
        ERROR("key/value required to 'size_image' !");
        return FALSE ;
    }

    my $objHarvest = BE4::Harvesting->new({
        wms_layer => $params->{wms_layer},
        wms_url => $params->{wms_url},
        wms_version => $params->{wms_version},
        wms_request => $params->{wms_request},
        wms_format => $params->{wms_format}
    });
    if (! defined $objHarvest) {
        ERROR("Cannot create Harvesting object !");
        return FALSE ;
    }
    
    # init. params    
    $self->{baseLevel} = $params->{baseLevel};
    $self->{harvesting} = $objHarvest;
    $self->{size_image} = $params->{size_image};

    return TRUE;
}


1;
__END__

=pod

=head1 NAME

  BE4::HarvestSource

=head1 SYNOPSIS

  use BE4::HarvestSource;
  
  my $objHarvestSource = BE4::DataSource->new(baseLevel => 19,
                                         harvesting => $objHarvesting,
                                         size_image => [2048,2048]);

=head1 DESCRIPTION

  
=head2 EXPORT

None by default.

=head1 LIMITATION & BUGS


=head1 SEE ALSO

  eg BE4::DataSource
  eg BE4::ImageSource

=head1 AUTHOR

Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
