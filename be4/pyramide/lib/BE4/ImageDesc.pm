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
    * filePath => $args{filePath},
    * xMin => $args{xMin},
    * xMax => $args{xMax},
    * yMin => $args{yMin},
    * yMax => $args{yMax},
    * xRes => $args{xRes},
    * yRes => $args{yRes},
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

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

  push @bbox, ($self->{xMin},$self->{yMin},$self->{xMax},$self->{yMax});
  
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

####################################################################################################
#                                           EXPORT                                                 #
####################################################################################################

# Group: export

=begin nd
   method: to_string

   Output is formated to can be used in mergeNtiff configuration.
   path  xmin  ymax  xmax  ymin  resx  resy
   
   Example :
   /home/ign/DATA/BDORTHO/XX_YY.tif   5.44921875   43.330078125   5.537109375   43.2421875   0.000021457672119140625   0.000021457672119140625
=cut
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

  return $output;

}

1;
__END__

=head1 NAME

    BE4::ImageDesc - Describe a georeferenced image (more general than GeoImage)

=head1 SYNOPSIS

    use BE4::ImaegDesc;
  
    # ImageDesc object creation
    my $objImgDesc = BE4::ImageDesc->new({
        filePath => "/home/ign/DATA/BDORTHO/XX_YY.tif",
        xMin => 5.44921875,
        xMax => 5.537109375,
        yMin => 43.2421875,
        yMax => 43.330078125,
        xRes => 0.000021457672119140625,
        yRes => 0.000021457672119140625,
    });
    
    # To write in mergeNtiff configuration
    my $stringDesc = $objImgDesc->to_string();
    # $stringDesc = "/home/ign/DATA/BDORTHO/XX_YY.tif   5.44921875   43.330078125   5.537109375   43.2421875   0.000021457672119140625   0.000021457672119140625"
    
=head1 DESCRIPTION

    A ImageDesc object

        * filePath : complete file path
        * xMin
        * xMax
        * yMin
        * yMax
        * xRes
        * yRes

=head1 AUTHOR

    Bazonnais Jean Philippe, E<lt>jpbazonnais<gt>

=head1 COPYRIGHT AND LICENSE

    Copyright (C) 2011 by Bazonnais Jean Philippe

    This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut