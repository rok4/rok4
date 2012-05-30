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
use Cwd;

# My search module
use FindBin qw($Bin);
use lib "$Bin/../lib/perl5";

# My module
use BE4::PropertiesLoader;
use BE4::Pyramid;
use BE4::DataSource;
use BE4::Tree;

# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

# version
my $VERSION = "0.0.1";

# Options
my $MYCONF = undef;

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
            
    ) or pod2usage( -message => "Usage inapproprié", -verbose => 1);
  
  # properties : mandatory !
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

## PYRAMID
ALWAYS("- Loading pyramid ...");

my $objPyramid  = BE4::Pyramid->new($params_pyramid, $objDataSrc);

if(! defined $objPyramid) {
    ERROR("Erreur de configuration de la pyramide !");
    exit -3;
}

## TREE
ALWAYS("- Loading tree ...");

my $objTree = BE4::Tree->new(
                            $objDataSrc,
                            $objPyramid,
                            $params_process->{job_number}
                            );
if (! defined $objTree) {
    ERROR("Erreur de configuration de l'arbre QTree !");
    exit -5;
}

my $pathexport = undef;
if (defined $params_logger->{log_path}) {
  $pathexport = $params_logger->{log_path};
}
else {
  $pathexport = cwd();
}

my $myexport = File::Spec->catfile($pathexport,
                                   join("-","export-tree",$objPyramid->getPyrName()));

if (! $objTree->exportTree($myexport.".txt")) {
    ERROR("Erreur sur l'export de l'arbre QTree !");
    exit -55;
}

printf STDOUT "[END]\n";
exit 0;
END {}

=pod

=head1 NAME

  export-tree - Outil orienté maintenance qui permet de connaitre les coordonnées des dalles du cache
  de la pyramide ainsi que les données sources utilisées pour la construction de ces dalles.

=head1 SYNOPSIS

  ORIENTÉ MAINTENANCE !
  perl export-tree.pl --properties=path
                    [ --log-level=WARN ] NON IMPLEMENTÉ !

=head1 DESCRIPTION

  ORIENTÉ MAINTENANCE !
  
  Pas de conf. d'environement..., donc mettre les parametres suivants dans la
  conf de la pyramide :
    - pyr_desc_path
    - tms_path
    - log_path
    - log_file

=head1 OPTIONS

=over

=item B<--help>

=item B<--usage>

=item B<--version>

=item B<--properties=path>

=item B<--log-level=(INFO|WARN|...)>

WARN par defaut !
NON IMPLEMENTÉ !

=back

=head1 DIAGNOSTICS

Ecriture du fichier "export-tree-<nom de la pyramide>.txt" à l'emplacement des logs
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
    use BE4::Tree;s

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
