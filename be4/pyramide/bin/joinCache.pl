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

use Getopt::Long;
use Pod::Usage;

use Data::Dumper;

use File::Basename;
use File::Spec;
use File::Path;
use Cwd;

use Log::Log4perl qw(:easy);

# My search module
use FindBin qw($Bin);
use lib "$Bin/../lib/perl5";

# My module
use BE4::PropertiesLoader;

# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

# pas de bufferisation des sorties.
$|=1;

# version
# my $VERSION = "@VERSION_TEXT@";
my $VERSION = "0.0.1";

#
# Title: joinCache
#
# (see uml-global.png)
#

################################################################################

#
# Group: variables
#

#
# variable: options
#   command options
#       - properties
#       - environment
#
my %opts =
(
    "version"    => 0,
    "help"       => 0,
    "usage"      => 0,
    
    # Configuration
    "properties"  => undef, # file properties params (mandatory) !
);

#
# variable: parameters
#   all parameters by sections
#        - logger
#        - pyramid      
#        - bboxes
#        - composition
#        - process
#
my %this =
(
    params     => {
        logger        => undef,
        pyramid       => undef,
        bboxes        => undef,
        composition   => undef,
        process       => undef,
    },
);

#
# Group: proc
#

################################################################################
# function: main
#   main program
#
sub main {
  printf("JoinCache : version [%s]\n",$VERSION);
  # message
  my $message = undef;

  $message = "BEGIN";
  printf STDOUT "%s\n", $message;
  
  # initialization
  ALWAYS("> Initialization");
  if (! main::init()) {
    $message = "ERROR INITIALIZATION !";
    printf STDERR "%s\n", $message;
    exit 1;
  }
  
  # configuration
  ALWAYS("> Configuration");
  if (! main::config()) {
    $message = "ERROR CONFIGURATION !";
    printf STDERR "%s\n", $message;
    exit 2;
  }
  
  # execution
  ALWAYS("> Execution");
  if (! main::doIt()) {
    $message = "ERROR EXECUTION !";
    printf STDERR "%s\n", $message;
    exit 3;
  }
  
  $message = "END";
  printf STDOUT "%s\n", $message;
}
################################################################################
# function: init
#   Check options and initialize the logger
#  
sub init {

    ALWAYS(">>> Check Configuration ...");

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
        "properties|conf=s"  => \$opts{properties},

    ) or pod2usage( -message => "Usage inapproprié", -verbose => 1);


    # logger by default at runtime
    Log::Log4perl->easy_init( {
        level => $WARN,
        layout => '[%M](%L): %m%n'}
    );

    # check Options

    # properties : mandatory !
    if (! defined $opts{properties}) {
        ERROR("Option 'properties' not defined !");
        return FALSE;
    }

    my $fproperties = File::Spec->rel2abs($opts{properties});

    if (! -d dirname($fproperties)) {
        ERROR(sprintf "File path doesn't exist ('%s') !", dirname($fproperties));
        return FALSE;
    }

    if (! -f $fproperties) {
        ERROR(sprintf "File name doesn't exist ('%s') !", basename($fproperties));
        return FALSE;
    }

    $opts{properties} = $fproperties;

    return TRUE;
}
################################################################################
# function: config
#   Loading file properties
#
sub config {
  
  ALWAYS(">>> Load Properties ...");

  my $fprop = $opts{properties};
  
  my $objProp = BE4::PropertiesLoader->new($fprop);
  
  if (! defined $objProp) {
    ERROR("Can not load properties !");
    return FALSE;
  }
  
  my $refProp = $objProp->getAllProperties();
  
  if (! defined $refProp) {
    ERROR("All parameters properties are empty !");
    return FALSE;
  }
  
  my $hashref;
  foreach (keys %{$this{params}}) {
    my $href = { map %$_, grep ref $_ eq 'HASH', ($this{params}->{$_}, $refProp->{$_}) };
    $hashref->{$_} = $href;
  }

  if (! defined $hashref) {
    ERROR("Can not merge all parameters of properties !");
    return FALSE;
  }
  
  # save params properties
  $this{params} = $hashref;
  
  if (! main::checkParams() ) {
    ERROR("Can not check parameters of properties !");
    return FALSE;
  }
  
  return TRUE;
}
################################################################################
# function: chechParams
#   Check parameters of properties
#
sub checkParams {

    ###################
    # check parameters

    my $pyramid      = $this{params}->{pyramid};        
    my $logger       = $this{params}->{logger};         
    my $composition  = $this{params}->{composition};    
    my $bboxes       = $this{params}->{bboxes};         
    my $process      = $this{params}->{process};    

    # pyramid
    if (! defined $pyramid) {
        ERROR ("Section [pyramid] can not be null !");
        return FALSE;
    }

    # composition
    if (! defined $composition) {
        ERROR ("Section [composition] can not be null !");
        return FALSE;
    }

    # bboxes
    if (! defined $bboxes) {
        ERROR ("Section [bboxes] can not be null !");
        return FALSE;
    }

    # process
    if (! defined $process) {
        ERROR ("Section [process] can not be null !");
        return FALSE;
    }

    # logger
    if (defined $logger) {
        my @args;

        my $layout= '[%C][%M](%L): %m%n';
        my $level = $logger->{log_level};

        my $out   = "STDOUT";
        $level = "WARN"   if (! defined $level);

        if ($level =~ /(ALL|DEBUG)/) {
            $layout = '[%C][%M](%L): %m%n';
        }

        # add the param logger by default (user settings !)
        push @args, {
            file   => $out,
            level  => $level,
            layout => $layout,
        };

        Log::Log4perl->easy_init(@args); 
    }

    return TRUE;
}


################################################################################
# function: doIt
#   Processing
#
sub doIt {

    ###################
    # link to parameters
    my $params = $this{params};
    
    ##################
    # load pyramid
    ALWAYS(">>> Load pyramid's attributes ...");

    if (! main::loadPyramid($params->{pyramid})) {
        ERROR ("Can not load bboxes from the configuration file !");
        return FALSE;
    }
    
    ##################
    # load bounding boxes
    ALWAYS(">>> Load bounding boxes ...");

    if (! main::loadBboxes($params->{pyramid})) {
        ERROR ("Can not load bboxes from the configuration file !");
        return FALSE;
    }
    
}

################################################################################
# function: loadPyramid
#   
#
sub loadPyramid {
    TRACE();

    my $pyramid = $this{params}->{pyramid};
    ALWAYS(sprintf " Pyramid parameters : %s", Dumper($pyramid)); #TEST#
}

################################################################################
# function: loadBboxes
#   
#
sub loadBboxes {
    TRACE();

    my $bboxes = $this{params}->{bboxes};
    ALWAYS(sprintf " Bboxes : %s", Dumper($bboxes)); #TEST#
}

################################################################################
#
# autoload
#

BEGIN {}
INIT {}

main;
exit 0;

END {}

=pod

=head1 NAME

  joinCache : create a pyramid from several, thanks to n triplets
     a triplet = a level (ID in the TMS) + an extent (a bbox) + a pyramid (file .pyr)

=head1 SYNOPSIS

  perl joinCache.pl --conf=path

=head1 DESCRIPTION

All used pyramids must have identical parameters :
    - used TMS
    - pixel's attributes : sample format, bits per sample, samples per pixel, photometric
    - compression

Resulting pyramid will have same attributes

Bounding boxes' SRS have to be the TMS' one

=head1 FILE CONFIGURATION

* ALL PARAMS POSSIBLE :
 
  logger        => log_level
  pyramid       =>  pyr_name, pyr_desc_path, pyr_data_path
                    tms_name, tms_path
                    image_dir, nodata_dir, metadata_dir
  [ tilematrixset => tms_name,tms_path ]
  [ nodata        => path_nodata, imagesize, color ]
  [ tile          => bitspersample,sampleformat,compressionscheme,photometric,samplesperpixel,interpolation ]
  [ compression   => type ]
  process       => job_number, path_temp, path_shell, [ percentexpansion, percentprojection ]

* SAMPLE OF FILE CONFIGURATION OF PYRAMID :

[logger]
log_level      = INFO

[pyramid]
pyr_name       = PYR_TEST
pyr_desc_path  = /home/theo/TEST/BE4/PYRAMIDS
pyr_data_path  = /home/theo/TEST/BE4/PYRAMIDS
tms_path       = /home/theo/TEST/BE4/TMS
tms_name       = PM.tms
image_dir      = IMAGE
metadata_dir   = METADATA
nodata_dir     = NODATA

[bbox]
WLD = -20037508.3,-20037508.3,20037508.3,20037508.3
FXX = -518548.8,5107913.5,978393,6614964.6
REU = 6140645.1,-2433358.9,6224420.1,-2349936.0
GUF = -6137587.6,210200.4,-5667958.5,692618.0

[composition]

merge_method = replace   

0.WLD = /FILER/DESC_PATH/MONDE12F.pyr

1.WLD = /FILER/DESC_PATH/MONDE12F.pyr

2.FXX = /FILER/DESC_PATH/SCAN25.pyr
2.REU = /FILER/DESC_PATH/SCAN25.pyr
2.GUF = /FILER/DESC_PATH/SCANREG.pyr

3.FXX = /FILER/DESC_PATH/ROUTE.pyr; /FILER/DESC_PATH/BATIMENT.pyr 
3.REU = /FILER/DESC_PATH/SCAN25.pyr
3.GUF = /FILER/DESC_PATH/SCANREG.pyr

=head1 OPTIONS

=over

=item B<--help>

=item B<--usage>

=item B<--version>

=item B<--conf=path>

Path to file configuration of the pyramid.
This option is manadatory !

=back

=head1 DIAGNOSTICS

=head1 REQUIRES

=over

=item * LIB EXTERNAL

Use of binding gdal (Geo::GDAL)

=item * MODULES (CPAN)

    use POSIX qw(locale_h);
    use sigtrap qw(die normal-signals);
    use Getopt::Long;
    use Pod::Usage;
    use Log::Log4perl qw(get_logger);
    use Cwd qw(abs_path cwd chdir);
    use File::Spec;

=item * MODULES (owner)

    use BE4::PropertiesLoader;

=item * deployment and installation

cf. INSTALL and README file

=back

=head1 BUGS AND LIMITATIONS

cf. FIXME and TODO file

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
