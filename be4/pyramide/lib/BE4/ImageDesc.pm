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
