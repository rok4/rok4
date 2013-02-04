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

package BE4::Level;

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
# Global
my $STRLEVELTMPLT = <<"TLEVEL";
    <level>
        <tileMatrix>__ID__</tileMatrix>
        <baseDir>__DIRIMG__</baseDir>
<!-- __MTD__ -->
        <tilesPerWidth>__TILEW__</tilesPerWidth>
        <tilesPerHeight>__TILEH__</tilesPerHeight>
        <pathDepth>__DEPTH__</pathDepth>
        <nodata>
            <filePath>__NODATAPATH__</filePath>
        </nodata>
        <TMSLimits>
            <minTileRow>__MINROW__</minTileRow>
            <maxTileRow>__MAXROW__</maxTileRow>
            <minTileCol>__MINCOL__</minTileCol>
            <maxTileCol>__MAXCOL__</maxTileCol>
        </TMSLimits>
    </level>
<!-- __LEVELS__ -->
TLEVEL

my $STRLEVELTMPLTMORE = <<"TMTD";
            <metadata type='INT32_DB_LZW'>
                <baseDir>__DIRMTD__</baseDir>
                <format>__FORMATMTD__</format>
            </metadata>
TMTD

################################################################################

BEGIN {}
INIT {}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    * id : string
    * order : integer
    * dir_image
    * dir_nodata
    * dir_metadata - NOT IMPLEMENTED
    * compress_metadata - NOT IMPLEMENTED
    * type_metadata - NOT IMPLEMENTED
    * size - [width, height]
    * dir_depth : integer
    * limit - [rowMin,rowMax,colMin,colMax]
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
        id                => undef,
        order             => undef,
        dir_image         => undef,
        dir_nodata        => undef,
        dir_metadata      => undef,
        compress_metadata => undef,
        type_metadata     => undef,
        size              => [],
        dir_depth         => 0,
        limit             => [undef,undef,undef,undef]
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  if (! $self->_init(@_)) {
    ERROR ("One parameter is missing !");
    return undef;
  }
  
  return $self;
}

sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # init. params
    
    # parameters mandatoy !
    if (! exists($params->{id})) {
      ERROR ("key/value required to 'id' !");
      return FALSE;
    }
    if (! exists($params->{order})) {
      ERROR ("key/value required to 'order' !");
      return FALSE;
    }
    if (! exists($params->{dir_image})) {
      ERROR ("key/value required to 'dir_image' !");
      return FALSE;
    }
    if (! exists($params->{dir_nodata})) {
      ERROR ("key/value required to 'dir_nodata' !");
      return FALSE;
    }
    if (! exists($params->{size})) {
      ERROR ("key/value required to 'size' !");
      return FALSE;
    }
    if (! exists($params->{dir_depth})) {
      ERROR ("key/value required to 'dir_depth' !");
      return FALSE;
    }
    if (! exists($params->{limit})) {
      ERROR ("key/value required to 'limit' !");
      return FALSE;
    }

    # check type ref
    if (! scalar ($params->{size})){
      ERROR("List empty to 'size' !");
      return FALSE;
    }
    if (! $params->{dir_depth}){
      ERROR("Value not informed to 'dir_depth' !");
      return FALSE;
    }
    if (! scalar (@{$params->{limit}})){
      ERROR("List empty to 'limit' !");
      return FALSE;
    }
    
    # parameters optional !
    # TODO : metadata 
    
    $self->{id}             = $params->{id};
    $self->{order}          = $params->{order};
    $self->{dir_image}      = $params->{dir_image};
    $self->{dir_nodata}     = $params->{dir_nodata};
    $self->{size}           = $params->{size};
    $self->{dir_depth}      = $params->{dir_depth};
    $self->{limit}          = $params->{limit};
    
    return TRUE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getID {
    my $self = shift;
    return $self->{id};
}

sub getOrder {
    my $self = shift;
    return $self->{order};
}

sub getDirImage {
    my $self = shift;
    return $self->{dir_image};
}

sub getDirNodata {
    my $self = shift;
    return $self->{dir_nodata};
}

sub getLimits {
    my $self = shift;
    return ($self->{limit}[0],$self->{limit}[1],$self->{limit}[2],$self->{limit}[3]);
}

#
=begin nd
method: updateExtremTiles

Compare old extrems rows/columns with the news and update values.

Parameter:
    jMin, jMax, iMin, iMax - tiles indices to compare with current extrems
=cut
sub updateExtremTiles {
    my $self = shift;
    my ($jMin,$jMax,$iMin,$iMax) = @_;

    TRACE();
    
    if (! defined $self->{limit}[0] || $jMin < $self->{limit}[0]) {$self->{limit}[0] = $jMin;}
    if (! defined $self->{limit}[1] || $jMax > $self->{limit}[1]) {$self->{limit}[1] = $jMax;}
    if (! defined $self->{limit}[2] || $iMin < $self->{limit}[2]) {$self->{limit}[2] = $iMin;}
    if (! defined $self->{limit}[3] || $iMax > $self->{limit}[3]) {$self->{limit}[3] = $iMax;}
}

####################################################################################################
#                                          EXPORT METHODS                                          #
####################################################################################################

# Group: export methods

#
=begin nd
method: exportToXML

Insert Level's attributes in the XML template, write in the pyramid's descriptor.

Returns:
   A string in XML format.
=cut
sub exportToXML {
    my $self = shift;

    my $levelXML = $STRLEVELTMPLT;

    my $id       = $self->{id};
    $levelXML =~ s/__ID__/$id/;

    my $dirimg   = $self->{dir_image};
    $levelXML =~ s/__DIRIMG__/$dirimg/;

    my $pathnd = $self->{dir_nodata}."/nd.tif";
    $levelXML =~ s/__NODATAPATH__/$pathnd/;

    my $tilew    = $self->{size}[0];
    $levelXML =~ s/__TILEW__/$tilew/;
    my $tileh    = $self->{size}[1];
    $levelXML =~ s/__TILEH__/$tileh/;

    my $depth    =  $self->{dir_depth};
    $levelXML =~ s/__DEPTH__/$depth/;

    my $minrow   =  $self->{limit}[0];
    $levelXML =~ s/__MINROW__/$minrow/;
    my $maxrow   =  $self->{limit}[1];
    $levelXML =~ s/__MAXROW__/$maxrow/;
    my $mincol   =  $self->{limit}[2];
    $levelXML =~ s/__MINCOL__/$mincol/;
    my $maxcol   =  $self->{limit}[3];
    $levelXML =~ s/__MAXCOL__/$maxcol/;

    # metadata
    if (defined $self->{dir_metadata}) {
        $levelXML =~ s/<!-- __MTD__ -->/$STRLEVELTMPLTMORE/;

        my $dirmtd   = $self->{dir_metadata};
        $levelXML =~ s/__DIRMTD__/$dirmtd/;

        my $formatmtd = $self->{compress_metadata};
        $levelXML  =~ s/__FORMATMTD__/$formatmtd/;
    } else {
        $levelXML =~ s/<!-- __MTD__ -->\n//;
    }

    return $levelXML;
}

sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject BE4::Level :\n";
    $export .= sprintf "\t ID (string) : %s, and order (integer) : %s\n", $self->{id}, $self->{order};

    $export .= sprintf "\t Directories (depth = %s): \n",$self->{dir_depth};
    $export .= sprintf "\t\t- Images : %s\n",$self->{dir_image};
    $export .= sprintf "\t\t- Nodata : %s\n",$self->{dir_nodata};
    $export .= sprintf "\t\t- Metadata : %s\n",$self->{dir_metadata};
    
    $export .= "\t Tile limits : \n";
    $export .= sprintf "\t\t- Column min : %s\n",$self->{limits}[2];
    $export .= sprintf "\t\t- Column max : %s\n",$self->{limits}[3];
    $export .= sprintf "\t\t- Row min : %s\n",$self->{limits}[0];
    $export .= sprintf "\t\t- Row max : %s\n",$self->{limits}[1];
    
    $export .= "\t Tiles per image : \n";
    $export .= sprintf "\t\t- widthwise : %s\n",$self->{size}[0];
    $export .= sprintf "\t\t- heightwise : %s\n",$self->{size}[1];
    
    return $export;
}

1;
__END__

=head1 NAME

BE4::Level - Describe a level in a pyramid.

=head1 SYNOPSIS

    use BE4::Level;
    
    my $params = {
        id                => level_5,
        order             => 12,
        dir_image         => "./BDORTHO/IMAGE/level_5/",
        dir_nodata        => "./BDORTHO/NODATA/level_5/",
        dir_metadata      => undef,
        compress_metadata => undef,
        type_metadata     => undef,
        size              => [16, 16],
        dir_depth         => 2,
        limit             => [365,368,1026,1035]
    };
    
    my $objLevel = BE4::Level->new($params);

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item id, order

ID (in TMS) and order (integer) of the level.

=item dir_image

Relative images' directory path for this level, from the pyramid's descriptor.

=item dir_nodata

Relative nodata's directory path for this level, from the pyramid's descriptor..

=item dir_metadata, compress_metadata, type_metadata

Relative metadata's directory path for this level, from the pyramid's descriptor.. NOT IMPLEMENTED.

=item size

Number of tile in one image for this level, widthwise and heightwise (often 16x16).

=item dir_depth

Image's depth from the level directory. depth = 2 => /.../LevelID/SUB1/SUB2/IMG.tif

=item limit

Extrems tiles indices of data in this level : [jmin,jmax,imin,imax].

=back

=head1 SAMPLE

    <level>
        <tileMatrix>level_5</tileMatrix>
        <baseDir>./BDORTHO/IMAGE/level_5/</baseDir>
        <tilesPerWidth>16</tilesPerWidth>
        <tilesPerHeight>16</tilesPerHeight>
        <pathDepth>2</pathDepth>
        <nodata>
            <filePath>./BDORTHO/NODATA/level_5/nd.tif</filePath>
        </nodata>
        <TMSLimits>
            <minTileRow>365</minTileRow>
            <maxTileRow>368</maxTileRow>
            <minTileCol>1026</minTileCol>
            <maxTileCol>1035</maxTileCol>
        </TMSLimits>
    </level>

=head1 LIMITATIONS AND BUGS

Metadata not implemented.

=head1 SEE ALSO

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHORS

Bazonnais Jean Philippe, E<lt>jean-philippe@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
