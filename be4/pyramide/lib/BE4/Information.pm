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

################################################################################
# constructor
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

################################################################################
# privates init.
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

################################################################################
# get/set
# fixme : without control...
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
