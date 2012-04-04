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

package BE4::DataSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use List::Util qw(min max);

use Data::Dumper;

# My module
use BE4::ImageSource;
use BE4::HarvestSource;
use BE4::PropertiesLoader;

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
my %SOURCE;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {

%SOURCE = (
    type     => ['image','harvest']
);

}
END {}

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    FILEPATH_DATACONF => undef, # path of data's configuration file
    type => undef, # (image or harvest)
    sources  => [],    # list of ImageSource or HarvestSource objects
    SRS => undef,
    bottomExtent => undef, # OGR::Geometry object, in the previous SRS
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  
  # load. class
  return undef if (! $self->_load());

  return $self;
}

################################################################################
# privates init.
sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);

    ALWAYS(sprintf "paramètres de DataSource : %s",Dumper($params)); #TEST#
    
    # init. params    
    $self->{FILEPATH_DATACONF} = $params->{filepath_conf} if (exists($params->{filepath_conf}));
    
    if (! -f $self->{FILEPATH_DATACONF}) {
        ERROR (sprintf "Data's configuration file ('%s') doesn't exist !",$self->{FILEPATH_DATACONF});
        return FALSE;
    }

    if (! exists($params->{type}) || ! defined ($params->{type})) {
        ERROR("key/value required to 'type' !");
        return FALSE ;
    }
    if (! $self->is_type($params->{type})) {
        ERROR("Invalid data's type !");
        return FALSE ;
    }
    $self->{type} = $params->{type};

    return TRUE;
}

################################################################################
# privates load.
sub _load {
    my $self   = shift;

    TRACE;

    my $propLoader = BE4::PropertiesLoader->new($self->{FILEPATH_DATACONF});

    if (! defined $propLoader) {
        ERROR("Can not load sources' properties !");
        return FALSE;
    }

    my $sourcesProperties = $propLoader->getAllProperties();

    if (! defined $sourcesProperties) {
        ERROR("All parameters properties of sources are empty !");
        return FALSE;
    }

    my @sources = ();

    foreach my $level (keys %{$sourcesProperties}) {
        if ($self->{type} eq "harvest") {
            my $harvestSource = BE4::HarvestSource->new(%{$sourcesProperties}->{$level}));
            push @sources, 
            ALWAYS(sprintf "harvestSource pour le niveau %s : %s",$level,Dumper(%{$sourcesProperties}->{$level})); #TEST#
        }
        elsif ($self->{type} eq "image") {

        }
    }

    return TRUE;
}

################################################################################
# tests
sub is_type {
    my $self = shift;
    my $type = shift;

    TRACE;

    return FALSE if (! defined $type);

    foreach (@{$SOURCE{type}}) {
        return TRUE if ($type eq $_);
    }
    ERROR (sprintf "Unknown 'type' of data (%s) !",$type);
    return FALSE;
}

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################


1;
__END__

=pod

=head1 NAME

  BE4::DataSource - Managing data sources

=head1 SYNOPSIS

  use BE4::DataSource;
  
  my $objImplData = BE4::DataSource->new(path_conf => $path,
                                         type => image);

=head1 DESCRIPTION


  
=head2 EXPORT

None by default.

=head1 LIMITATION & BUGS

* Support data source multiple !

* Does not implement the managing of metadata !

=head1 SEE ALSO

  eg BE4::HarvestSource
    eg BE4::Harvesting
  eg BE4::ImageSource
    eg BE4::GeoImage

=head1 AUTHOR

Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
