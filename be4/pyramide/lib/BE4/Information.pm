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

package BE4::Information;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

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

################################################################################
=begin nd
Group: variable

variable: $self
    * PRODUCT => undef,
    * DATE    => undef,
    * ZONE    => undef,
    * COMMENT => undef,
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    PRODUCT => undef,
    DATE    => undef,
    ZONE    => undef,
    COMMENT => undef,
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  
  return $self;
}

sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # init. params    
    $self->{PRODUCT}=$params->{product};
    $self->{DATE}   =$params->{date};
    $self->{ZONE}   =$params->{zone};
    $self->{COMMENT}=$params->{comment};
    
    return TRUE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

# FIXME : without control...
sub product {
  my $self = shift;
  if (@_) { $self->{PRODUCT} = shift }
  return $self->{PRODUCT};
}
sub date {
  my $self = shift;
  if (@_) { $self->{DATE} = shift }
  return $self->{DATE};
}
sub zone {
  my $self = shift;
  if (@_) { $self->{ZONE} = shift }
  return $self->{ZONE};
}
sub comment {
  my $self = shift;
  if (@_) { $self->{COMMENT} = shift }
  return $self->{COMMENT};
}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

    BE4::Information - global information about a product

=head1 SYNOPSIS

    use BE4::Information;
  
    # Information object creation
    my $objInfo = BE4::Information->new({
        PRODUCT => "BDORTHO",
        DATE    => 2012,
        ZONE    => "FXX",
        COMMENT => "Prise de vue aérienne",
    });

=head1 DESCRIPTION

    A Information object

        * PRODUCT
        * DATE
        * ZONE
        * COMMENT
        
=head2 EXPORT

    None by default.

=head1 SEE ALSO

=head1 AUTHOR

    Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

    Copyright (C) 2011 by Bazonnais Jean Philippe

    This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself,    either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
