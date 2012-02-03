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

package BE4::ImageDesc;

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
  my %args = @_;

  my $class= ref($this) || $this;
  my $self = {
    filePath => $args{filePath},
    xMin => $args{xMin},
    xMax => $args{xMax},
    yMin => $args{yMin},
    yMax => $args{yMax},
    xRes => $args{xRes},
    yRes => $args{yRes},
  };

  bless($self, $class);
  
  TRACE;
  
  return $self;
}

sub getBbox {
  my $self = shift;

  TRACE;
  
  my @bbox;

  # FIXME : format bbox (Upper Left, Lower Right) ?
  push @bbox, ($self->{xMin},$self->{yMax},$self->{xMax},$self->{yMin});
  
  return @bbox;
}
sub copy {
 my $this = shift;
 my $filePath = shift;

  my $class= ref($this) || $this;
  my $self = {
    filePath => $filePath,
    xMin => $this->{xMin},
    xMax => $this->{xMax},
    yMin => $this->{yMin},
    yMax => $this->{yMax},
    xRes => $this->{xRes},
    yRes => $this->{yRes},
  };

  bless($self, $class);
  
  TRACE;
  
  return $self;
}
sub getFilePath {
  my $self = shift;
  
  return $self->{filePath};
}
# to_string methode
# la sortie est formatée pour pouvoir être utilisée dans le fichier de conf de mergeNtif
#---------------------------------------------------------------------------------------------------
sub to_string {
  my $self = shift;
  
  TRACE;
  
  my $output = sprintf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
          $self->{filePath},
          $self->{xMin},
          $self->{yMax},
          $self->{xMax},
          $self->{yMin},
          $self->{xRes},
          $self->{yRes};
  #DEBUG ($output);
  return $output;

}
1;
__END__
