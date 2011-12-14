#!/usr/bin/env perl

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

use BE4::PropertiesLoader;
use BE4::Pyramid;
use BE4::DataSource;
use BE4::Layer;

# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

# version
my $VERSION = "0.0.1";

# Options
my $MYCONF   = undef;
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
            "properties=s"  => \$MYCONF,
            "resampling=s"  => \$MYSAMPLE,
            "style=s"       => \$MYSTYLE,
            "opaque!"       => \$MYOPAQUE,
            
    ) or pod2usage( -message => "Usage inapproprié", -verbose => 1);
  
  # properties : obligatoire !
  if (! defined $MYCONF) {
    ERROR("Option 'properties' not defined !");
    return FALSE;
  }
  
  my $fproperties = File::Spec->rel2abs($MYCONF);
  
  if (! -d dirname($fproperties)) {
    ERROR(sprintf "Le répertoire n'existe pas ('%s') !", dirname($fproperties));
    return FALSE;
  }
  
  if (! -f $fproperties) {
    ERROR(sprintf "Le fichier n'existe pas ('%s') !", basename($fproperties));
    return FALSE;
  }
  
  $MYCONF = $fproperties;
  
  # properties : facultative !
  
  return TRUE;
}
#

printf STDOUT "[BEGIN]\n";
## LOGGER
Log::Log4perl->easy_init( { level  => "WARN",
                            file   => "STDOUT",
                            layout => '[%M](%L): %m%n'}
                        );

## INITOPTIONS
if (! main::init()) {
    ERROR("Erreur d'initialisation !");
    exit -1;
}

## PROPERTIES
ALWAYS("- Loading properties ...");

my $objprop = BE4::PropertiesLoader->new($MYCONF);

if (! defined $objprop) {
    ERROR("Erreur sur la lecture du fichier de configuration !");
    exit -2;
}

my $params_logger   = $objprop->getPropertiesBySection("logger"); 
my $params_process  = $objprop->getPropertiesBySection("process"); 
my $params_pyramid  = $objprop->getPropertiesBySection("pyramid");
my $params_data     = $objprop->getPropertiesBySection("datasource");
my $params_nodata   = $objprop->getPropertiesBySection("nodata");
my $params_tile     = $objprop->getPropertiesBySection("tile");
my $params_tms      = $objprop->getPropertiesBySection("tilematrixset");

$params_pyramid = { map %$_, grep ref $_ eq 'HASH', ($params_tms,         $params_pyramid) };
$params_pyramid = { map %$_, grep ref $_ eq 'HASH', ($params_nodata,      $params_pyramid) };
$params_pyramid = { map %$_, grep ref $_ eq 'HASH', ($params_tile,        $params_pyramid) };
$params_pyramid = { map %$_, grep ref $_ eq 'HASH', ($params_tms,         $params_pyramid) };

## PYRAMID
ALWAYS("- Loading pyramid ...");

my $objPyramid  = BE4::Pyramid->new($params_pyramid);

if(! defined $objPyramid) {
    ERROR("Erreur de configuration de la pyramide !");
    exit -3;
}

## DATASOURCE
ALWAYS("- Loading datasource ...");

my $objDataSrc = BE4::DataSource->new($params_data);

if(! defined $objDataSrc) {
    ERROR("Erreur de configuration des données sources !");
    exit -4;
}

if (! $objDataSrc->computeImageSource()) {
    ERROR("Erreur de calcul sur données sources !");
    exit -44;
}

my ($xmin,$ymax,$xmax,$ymin) = $objDataSrc->computeBbox();
INFO(sprintf "BBOX : %s %s %s %s\n", $xmin,$ymax,$xmax,$ymin);

## LAYER
ALWAYS("- Loading layer ...");

my $srs  = $objPyramid->getTileMatrixSet()->getSRS();
my $auth = (split(":", $srs))[0];

# TODO ajouter une liste par defaut
my @lstsrs;
push @lstsrs, $srs; # Toujour en 1er !!!
push @lstsrs, "CR:84";
push @lstsrs, "IGNF:WGS84G";
push @lstsrs, "epsg:3857";
push @lstsrs, "epsg:4258";

# TODO informatif...
my $minres=$objPyramid->getTileMatrixSet()->getFirstTileMatrix()->getResolution();
my $maxres=$objPyramid->getTileMatrixSet()->getLastTileMatrix()->getResolution();

# TODO informatif...
my @keyword;
push @keyword, $objPyramid->getFormat()->getCompression();
push @keyword, $objPyramid->getPyrName();
push @keyword, $objPyramid->getTmsName();
push @keyword, $srs;

if ($srs ne $objDataSrc->getSRS()) {
  push @keyword, "REPROJECTION";
}

my $srsini= new Geo::OSR::SpatialReference;
eval { $srsini->ImportFromProj4('+init='.$srs.' +wktext'); };

if ($@) {
  ERROR(sprintf "Erreur de projection : %s !", $@);
  exit -51;
}
    
my $srsfin= new Geo::OSR::SpatialReference;
eval { $srsfin->ImportFromProj4('+init=IGNF:WGS84G +wktext'); };
    
if ($@) {
  ERROR(sprintf "Erreur de projection : %s !", $@);
  exit -52;
}

my $ct = new Geo::OSR::CoordinateTransformation($srsini, $srsfin);

my $bg= $ct->TransformPoint($xmin,$ymin);
my $hd= $ct->TransformPoint($xmax,$ymax);

my $params = {
            title            => $objPyramid->getPyrName(),
            abstract         => undef,      # TODO
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
            pyramid          => File::Spec->catfile($objPyramid->getPyrDescPath(),
                                                    $objPyramid->getPyrFile()),
    };
    
my $objLayer = new BE4::Layer($params);

if (! defined $objLayer){
    ERROR("Erreur de configuration du layer !");
    exit -5;
}

my $strlayer = $objLayer->to_string();

if (! defined $strlayer) {
    ERROR("Erreur de construction du layer !");
    exit -55;
}

my $pathlayer = undef;
if (defined $params_logger->{log_path}) {
  $pathlayer = $params_logger->{log_path};
}
else {
  $pathlayer = cwd();
}

my $filelayer = File::Spec->catfile($pathlayer,
                                   join("_","layer",$objPyramid->getPyrName()));

if (! open (FILE, ">", $filelayer.".lay")) {
    ERROR(sprintf "Erreur de creation du fichier layer (%s) !", $!);
    exit -6;
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

=item B<--properties=path>

=item B<--resampling="value">

Optionnel, par defaut resampling = "lanczos_4".

=item B<--style="value">

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
