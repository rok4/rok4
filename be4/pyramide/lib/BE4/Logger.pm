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
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################

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
=begin nd
Group: variable

variable: $self
    * file => "STDOUT",  # by default !
    * level => "WARN",# by default !
    * layout => '[%M](%L): %m%n', # always by default !
    * utf8 => 1, # always by default !
    * category => ""# always rootlogger by default !
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
    file     => "STDOUT",
    level    => "WARN",
    layout   => '[%M](%L): %m%n',
    utf8     => 1,
    category => ""
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  
  return $self;
}

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

=head1 NAME

BE4::Logger -

=head1 SYNOPSIS

    use BE4::Process;
  
    # Process object creation
    my $objProcess = BE4::Process->new({
        level => "INFO",
        file => "log.txt",
        layout => "[%M](%L): %m%n",
        utf8 => 1,
        path => "/home/IGN/logs/,
    });

=head1 SEE ALSO

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jean-philippe.bazonnais@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
