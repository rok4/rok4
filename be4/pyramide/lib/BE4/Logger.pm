package BE4::Logger;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Cwd;

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
# [ logger ]
#
#   path_log  =
#   file_log  =
#   level_log =

################################################################################
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    file     => "STDOUT",  # by default !
    level    => "WARN",    # by default !
    layout   => '[%M](%L): %m%n', # always by default !
    utf8     => 1,                # always by default !
    category => ""                # always rootlogger by default !
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  
  # fixme ...
  return {
          level    => $self->{level},
          file     => $self->{file},
          layout   => $self->{layout},
          utf8     => 1,
          category => $self->{category},
          };
}

################################################################################
# privates init.
sub _init {
    my $self   = shift;
    my %params = @_;

    TRACE;
    
    # init. params
    $self->{level} = $params{level}  if (exists($params{level}));
    $self->{file}  = $params{file}   if (exists($params{file}));
    $self->{layout}= $params{layout} if (exists($params{layout}));
    $self->{utf8}  = $params{utf8}   if (exists($params{utf8}));
    my $path       = $params{path}   if (exists($params{path}));
    
    my $file = $self->{file};
    if ((defined $file || $file ne "STDOUT") && (defined $path))
    {
        $self->{file} = File::Spec->catfile($path, $file);
        return FALSE if (! -f $self->{file});
    }
    
    # TODO no control ...
    
    return TRUE;
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
