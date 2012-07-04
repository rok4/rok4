#!/usr/bin/env perl
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

use warnings;
use strict;

use POSIX qw(locale_h);

# Module
use Log::Log4perl qw(:easy);
use Getopt::Long;
use File::Basename;
use Pod::Usage;
use Geo::OSR;
use Cwd;

# My search module
use FindBin qw($Bin);
use lib "$Bin/../lib/perl5";

# My module
use BE4::TileMatrixSet;
use BE4::Layer;

# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

# version
my $VERSION = "0.0.1";

# Options
my $MYPYR   = undef;
my $MYLAYPATH = undef;
my $MYTMSPATH = undef;
my $MYSAMPLE = "lanczos_4";
my $MYSTYLE  = "normal";
my $MYOPAQUE = "true";

#
sub init {
    
    # init Getopt
    local $ENV{POSIXLY_CORRECT} = 1;
    
    Getopt::Long::config qw(
        default
        no_autoabbrev
        no_getopt_compat
        require_order
        bundling
        no_ignorecase
        permute
    );
    
    # init Options
    GetOptions(
        "help|h"        => sub { pod2usage( -sections => "NAME|DESCRIPTION|SYNOPSIS|OPTIONS", -exitval=> 0, -verbose => 99); },
        "version|v"     => sub { printf "%s version %s", basename($0), $VERSION; exit 0; },
        "usage"         => sub { pod2usage( -sections => "SYNOPSIS", -exitval=> 0, -verbose => 99); },
        #
        "pyr=s"  => \$MYPYR,
        "tmsdir=s"  => \$MYTMSPATH,
        "layerdir=s"  => \$MYLAYPATH,
        "resampling|r=s"     => \$MYSAMPLE,
        "style|s=s"          => \$MYSTYLE,
        "opaque!"            => \$MYOPAQUE,
    
    ) or pod2usage( -message => "Usage inapproprié", -verbose => 1);
    
    # pyramid : obligatoire !
    if (! defined $MYPYR) {
        ERROR("Option 'pyr' not defined !");
        return FALSE;
    }
    
    my $pyrFile = File::Spec->rel2abs($MYPYR);
    if (! -d dirname($pyrFile)) {
        ERROR(sprintf "Le répertoire du descripteur de pyramide n'existe pas ('%s') !", dirname($pyrFile));
        return FALSE;
    }
    if (! -f $pyrFile) {
        ERROR(sprintf "Le descripteur de pyramide n'existe pas ('%s') !", $pyrFile);
        return FALSE;
    }
    $MYPYR = $pyrFile;
    
    
    # tms path : obligatoire !
    if (! defined $MYTMSPATH) {
        ERROR("Option 'tmsdir' (directory where to find TMS) not defined !");
        return FALSE;
    }
    my $tmsDir = File::Spec->rel2abs($MYTMSPATH);
    if (! -d $tmsDir) {
        ERROR(sprintf "Le répertoire des TMS n'existe pas ('%s') !", $tmsDir);
        return FALSE;
    }
    $MYTMSPATH = $tmsDir;
    
    return TRUE;
}
#

printf STDOUT "[BEGIN]\n";
## LOGGER
Log::Log4perl->easy_init( { level  => "WARN",
                            file   => "STDOUT",
                            layout => '[%M](%L): %m%n'}
                        );

## INITOPTIONS (error -1)
if (! main::init()) {
    ERROR("Erreur d'initialisation !");
    exit -1;
}

## PYRAMID (error -2X)
ALWAYS("- Loading pyramid ...");

my @keyword;

# read xml pyramid
my $parser  = XML::LibXML->new();
my $xmltree =  eval { $parser->parse_file($MYPYR); };

if (! defined ($xmltree) || $@) {
    ERROR (sprintf "Can not read the XML file Pyramid : %s !", $@);
    exit -21;
}

my $root = $xmltree->getDocumentElement;

## TMS (error -3X)
my $tmsname = $root->findnodes('tileMatrixSet')->to_literal;
if ($tmsname eq '') {
    ERROR (sprintf "Can not determine parameter 'tileMatrixSet' in the XML file Pyramid !");
    exit -31;
}

my $tmsFilePath = File::Spec->catfile($MYTMSPATH,$tmsname.".tms");
my $objTMS  = BE4::TileMatrixSet->new($tmsFilePath);
if (! defined $objTMS) {
    ERROR (sprintf "Can not create object TileMatrixSet from this path : %s ",$tmsFilePath);
    exit -32;
}

push @keyword, $tmsname;

# NODATA
my $nodata = $root->findnodes('nodataValue')->to_literal;
if ($nodata ne '') {
    push @keyword, $nodata;
}

# PHOTOMETRIC
my $photometric = $root->findnodes('photometric')->to_literal;
if ($photometric ne '') {
    push @keyword, $photometric;
}

# INTERPOLATION    
my $interpolation = $root->findnodes('interpolation')->to_literal;
if ($interpolation ne '') {
    push @keyword, $interpolation;
}

# FORMAT
my $format = $root->findnodes('format')->to_literal;
if ($format ne '') {
    push @keyword, $format;
}

# SAMPLESPERPIXEL  
my $samplesperpixel = $root->findnodes('channels')->to_literal;
if ($samplesperpixel ne '') {
    push @keyword, "Samples per pixel: $samplesperpixel";
}

# load pyramid level to determine the top and bottom, and bbox
my @levels = $root->getElementsByTagName('level');

# global informations
my $level = $levels[0];
my $tilesPerWidth = $level->findvalue('tilesPerWidth');
if ($tilesPerWidth ne '') {
    push @keyword, "Tiles per width: $tilesPerWidth";
}
my $tilesPerHeight = $level->findvalue('tilesPerHeight');
if ($tilesPerHeight ne '') {
    push @keyword, "Tiles per height: $tilesPerHeight";
}
my $dirdepth = $level->findvalue('pathDepth');
if ($dirdepth ne '') {
    push @keyword, "Directory depth: $dirdepth";
}

my $bottomID = undef;
my $bottomOrder = undef;
my $topID = undef;
my $topOrder = undef;
my ($imin,$imax,$jmin,$jmax);

foreach my $v (@levels) {
    my $ID = $v->findvalue('tileMatrix');
    my $order = $objTMS->getTileMatrixOrder($ID);
    
    if (! defined $bottomOrder || $order < $bottomOrder) {
        $bottomOrder = $order;
        $bottomID = $ID;
        ($imin,$imax,$jmin,$jmax) = (
            $v->findvalue('TMSLimits/minTileCol'),
            $v->findvalue('TMSLimits/maxTileCol'),
            $v->findvalue('TMSLimits/minTileRow'),
            $v->findvalue('TMSLimits/maxTileRow')
        );
    }
    
    if (! defined $topOrder || $order > $topOrder) {
        $topOrder = $order;
        $topID = $ID;
    }
}

my $bottomTM = $objTMS->getTileMatrix($bottomID);

my $res = $bottomTM->getResolution();
my $X0 = $bottomTM->getTopLeftCornerX();
my $Y0 = $bottomTM->getTopLeftCornerY();
my $tileWidth = $bottomTM->getTileWidth();
my $tileHeight = $bottomTM->getTileHeight();

my $xmin = $X0 + $imin * $res * $tileWidth;
my $ymax = $Y0 - $jmin * $res * $tileHeight;
my $xmax = $X0 + ($imax+1) * $res * $tileWidth;
my $ymin = $Y0 - ($jmax+1) * $res * $tileHeight;

ALWAYS(sprintf "BBOX : xmin %s xmax %s ymin %s ymax %s\n", $xmin,$xmax,$ymin,$ymax);


## LAYER (error -4X)
ALWAYS("- Loading layer ...");

my $srs  = $objTMS->getSRS();
my $auth = (split(":", $srs))[0];

# TODO ajouter une liste par defaut
my @lstsrs;
push @lstsrs, $srs; # Toujour en 1er !!!
push @lstsrs, "CR:84";
push @lstsrs, "IGNF:WGS84G";
push @lstsrs, "EPSG:3857";
push @lstsrs, "EPSG:4258";

# TODO informatif...
my $minres=$bottomTM->getResolution();
my $maxres=$objTMS->getTileMatrix($topID)->getResolution();

my $srsini= new Geo::OSR::SpatialReference;
eval { $srsini->ImportFromProj4('+init='.$srs.' +wktext'); };

if ($@) {
  ERROR(sprintf "Erreur de projection : %s !", $@);
  exit -41;
}
    
my $srsfin= new Geo::OSR::SpatialReference;
eval { $srsfin->ImportFromProj4('+init=IGNF:WGS84G +wktext'); };
    
if ($@) {
  ERROR(sprintf "Erreur de projection : %s !", $@);
  exit -42;
}

my $ct = new Geo::OSR::CoordinateTransformation($srsini, $srsfin);

my $bg= $ct->TransformPoint($xmin,$ymin);
my $hd= $ct->TransformPoint($xmax,$ymax);

my $pyrName = File::Basename::basename($MYPYR);
$pyrName =~ s/\.(pyr|PYR)$//;

my $params = {
    title            => $pyrName,
    abstract         => "Couche utilisant le descripteur de pyramide $pyrName.pyr",
    keywordlist      => \@keyword,
    style            => $MYSTYLE,
    minres           => $minres,
    maxres           => $maxres,
    opaque           => $MYOPAQUE, 
    authority        => $auth,
    srslist          => \@lstsrs,
    resampling       => $MYSAMPLE,
    geo_bbox         => [$bg->[0],$bg->[1],$hd->[0],$hd->[1]],
    proj             => $srs,
    proj_bbox        => [$xmin,$ymin,$xmax,$ymax],
    pyramid          => $MYPYR,
};
    
my $objLayer = new BE4::Layer($params);

if (! defined $objLayer){
    ERROR("Erreur de configuration du layer !");
    exit -43;
}

my $strlayer = $objLayer->to_string();

if (! defined $strlayer) {
    ERROR("Erreur de construction du layer !");
    exit -44;
}


## WRITE IN FILE (error -5X)
ALWAYS("- Write layer file ...");

if (! defined $MYLAYPATH) {
  $MYLAYPATH = cwd();
}

my $filelayer = File::Spec->catfile($MYLAYPATH,$pyrName.".lay");

if (! open (FILE, ">", $filelayer)) {
    ERROR(sprintf "Erreur de creation du fichier layer (%s) : %s!", $!, $filelayer);
    exit -51;
}

printf FILE "%s", $strlayer;

close FILE;
  
printf STDOUT "[END]\n";
exit 0;
END {}

=pod

=head1 NAME

  create-layer - Outil orienté maintenance qui permet de construire un layer pour Rok4, sur
  la pyramide du fichier de configuration.

=head1 SYNOPSIS

  ORIENTÉ MAINTENANCE !
  perl create-layer.pl --properties=path
                      [--resampling="" --opaque --style=""  ]
  perl create-layer.pl --conf=path
                      [--r="" --opaque --s=""  ] 

=head1 DESCRIPTION

  ORIENTÉ MAINTENANCE !
  
  Pas de conf. d'environement..., donc mettre les parametres suivants dans la
  conf de la pyramide :
    - pyr_desc_path
    - tms_path
    - log_path
    - log_file
    
  Par defaut, la liste des SRS sont les suivantes :
  - la projection des données sources,
  - "CR:84",
  - "IGNF:WGS84G",
  - "epsg:3857",
  - "epsg:4258",

  Le nom du layer est le nom de la pyramide !

=head1 OPTIONS

=over

=item B<--help>

=item B<--usage>

=item B<--version>

=item B<--pyr=path>

Obligatoire. Chemin du descripteur de pyramide dont on veut le layer correspondant.

=item B<--tmsdir=path>

Obligatoire. Chemin du répertoire contenant le TMS utilisé par la pyramide.

=item B<--layerdir=path>

Optionnel. Chemin du répertoire dans lequel on souhaite écrire le fichier layer. Si non renseigné, le fichier est écrit dans le dossier courant.

=item B<--resampling|r="value">

Optionnel, par defaut resampling = "lanczos_4".

=item B<--style|s="value">

Optionnel, par defaut style = "normal".

=item B<--noopaque>

Optionnel, par defaut opaque = 1.

=back

=head1 DIAGNOSTICS

Ecriture du fichier "layer-<nom de la pyramide>.lay" à l'emplacement des logs
ou dans le repertoire courant !

=head1 REQUIRES

=over

=item * LIB EXTERNAL


=item * MODULES (CPAN)

    use POSIX qw(locale_h);
    use Getopt::Long;
    use Pod::Usage;
    use Log::Log4perl;
    use File::Basename;

=item * MODULES (owner)

    use BE4::PropertiesLoader;
    use BE4::Pyramid;
    use BE4::DataSource;
    use BE4::Layer;

=back

=head1 BUGS AND LIMITATIONS

=over

=item * FIXME

=item * TODO

=back

=head1 SEE ASLO

=head1 AUTHOR

Jean-Philippe Bazonnais, E<lt>Jean-Philippe.Bazonnais@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright 2011 by SIEL/PZGG/Jean-Philippe Bazonnais - Institut Géographique National

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut
