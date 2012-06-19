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
# version
my $VERSION = "0.0.1";

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
# Global template Pyramid

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
# sample: 
# 
#        <tileMatrix>0</tileMatrix>
#        <baseDir>MNT_MAYOTTE_REDUCT/IMAGE/0</baseDir>
#        <tilesPerWidth>16</tilesPerWidth>
#        <tilesPerHeight>16</tilesPerHeight>
#        <pathDepth>2</pathDepth>
#        <nodata>
#            <filePath>MNT_MAYOTTE_REDUCT/NODATA/0/nd.tif</filePath>
#        </nodata>
#        <TMSLimits>
#            <minTileRow>1</minTileRow>
#            <maxTileRow>1000000</maxTileRow>
#            <minTileCol>1</minTileCol>
#            <maxTileCol>1000000</maxTileCol>
#        </TMSLimits>
# 
################################################################################

#
# Group: variable
#

#
# variable: $self
#
#    *          id                => undef,
#    *          order             => undef,
#    *          dir_image         => undef,
#    *          dir_nodata        => undef,
#    *          dir_metadata      => undef,  # NOT IMPLEMENTED !
#    *          compress_metadata => undef,  # NOT IMPLEMENTED !
#    *          type_metadata     => undef,  # NOT IMPLEMENTED !
#    *          size              => [],    # number w/h !
#    *          dir_depth         => 0,     # ie 2
#    *          limit             => []     # dim bbox [jmin,jmax,imin,imax] !!!
#    *          is_in_pyramid     => undef,
#

#
# Group: constructor
#

################################################################################
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
        id                => undef,
        order             => undef,
        is_in_pyramid     => undef, # 0 : just in the old pyramid; 1 : just in the new pyramid; 2 : in two pyamids
        dir_image         => undef,
        dir_nodata        => undef,
        dir_metadata      => undef,  # NOT IMPLEMENTED !
        compress_metadata => undef,  # NOT IMPLEMENTED !
        type_metadata     => undef,  # NOT IMPLEMENTED !
        size              => [],    # number w/h !
        dir_depth         => 0,     # ie 2
        limit             => [undef,undef,undef,undef]     # dim bbox [jmin,jmax,imin,imax] !!!
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

################################################################################
# privates init.
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
      ERROR("list empty to 'size' !");
      return FALSE;
    }
    if (! $params->{dir_depth}){
      ERROR("value not informed to 'dir_depth' !");
      return FALSE;
    }
    if (! scalar (@{$params->{limit}})){
      ERROR("list empty to 'limit' !");
      return FALSE;
    }
    if (! exists($params->{is_in_pyramid})) {
        $params->{is_in_pyramid} = 1;
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
    $self->{is_in_pyramid}  = $params->{is_in_pyramid};
    
    return TRUE;
}

################################################################################
# get
sub getID {
    my $self = shift;
    return $self->{id};
}

sub getLevelToXML {
    my $self = shift;

    my $levelXML = $STRLEVELTMPLT;

    my $id       = $self->{id};
    $levelXML =~ s/__ID__/$id/;

    my $dirimg   = $self->{dir_image};
    $levelXML =~ s/__DIRIMG__/$dirimg/;

    my $pathnd = $self->{dir_nodata}."/nd.tiff";
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


1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

  BE4::Level -

=head1 SYNOPSIS

  use BE4::Level;
  
  my $params = {
            id                => 1024,
            order             => 12,
            dir_image         => "./t/data/pyramid/SCAN_RAW_TEST/1024/",
            dir_nodata        => undef,
            dir_metadata      => undef,
            compress_metadata => undef,
            type_metadata     => undef,
            size              => [ 4, 4],
            dir_depth         => 2,
            limit             => [1, 1000000, 1, 1000000] 
            is_in_pyramid     => 1
    };
    
  my $objLevel = BE4::Level->new($params);

=head1 DESCRIPTION

=head2 EXPORT

None by default.

=head1 SAMPLE

* Sample Pyramid file (.pyr) :

  [MNT_MAYOTTE_REDUCT.pyr]
  
<?xml version="1.0" encoding="US-ASCII"?>
<Pyramid>
    <tileMatrixSet>RGM04UTM38S_10cm</tileMatrixSet>
    <format>TIFF_LZW_FLOAT32</format>
    <channels>1</channels>
    <level>
        <tileMatrix>0</tileMatrix>
        <baseDir>MNT_MAYOTTE_REDUCT/IMAGE/0</baseDir>
        <tilesPerWidth>16</tilesPerWidth>
        <tilesPerHeight>16</tilesPerHeight>
        <pathDepth>2</pathDepth>
        <nodata>
            <filePath>MNT_MAYOTTE_REDUCT/NODATA/0/nd.tif</filePath>
        </nodata>
        <TMSLimits>
            <minTileRow>1</minTileRow>
            <maxTileRow>1000000</maxTileRow>
            <minTileCol>1</minTileCol>
            <maxTileCol>1000000</maxTileCol>
        </TMSLimits>
    </level>
    <level>
        <tileMatrix>1</tileMatrix>
        <baseDir>MNT_MAYOTTE_REDUCT/IMAGE/1</baseDir>
        <tilesPerWidth>16</tilesPerWidth>
        <tilesPerHeight>16</tilesPerHeight>
        <pathDepth>2</pathDepth>
        <nodata>
            <filePath>MNT_MAYOTTE_REDUCT/NODATA/1/nd.tif</filePath>
        </nodata>
        <TMSLimits>
            <minTileRow>1</minTileRow>
            <maxTileRow>1000000</maxTileRow>
            <minTileCol>1</minTileCol>
            <maxTileCol>1000000</maxTileCol>
        </TMSLimits>
    </level>
</Pyramid>

=head1 LIMITATIONS AND BUGS

 No test on the type value !
 Metadata not implemented !

=head1 SEE ALSO

=head1 AUTHORS

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
