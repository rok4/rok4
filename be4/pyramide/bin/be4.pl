#!/usr/bin/env perl

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
use BE4::Pyramid;
use BE4::DataSource;
use BE4::Process;

# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

# pas de bufferisation des sorties.
$|=1;

# version
my $VERSION = "develop 0.3.2";

#
# Title: be4
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
    "environment" => undef, # file environment be4 params (optional) !
    
    # Only for maintenance !
    "test" => 0,
);

#
# variable: parameters
#   all parameters by sections
#        - logger        
#        - datasource    
#        - harvesting    
#        - pyramid      
#        - tilematrixset
#        - nodata       
#        - tile         
#        - process
#
my %this =
(
    params     => {
        logger        => undef, # can be null !
        datasource    => undef, # can be null if no data to process !
        harvesting    => undef, # can be null if no harvesting to do !
        pyramid       => undef, # ALWAYS !!!
        tilematrixset => undef, # can be in pyramid params !
        nodata        => undef, # can be in pyramid params !
        tile          => undef, # can be in pyramid params !
        process       => undef, # can be null if no process to do !
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
  printf("BE4: version [%s]\n",$VERSION);
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
            "environment|env=s" => \$opts{environment},
            #
            "test"          => \$opts{test},
            
    ) or pod2usage( -message => "Usage inapproprié", -verbose => 1);
  
  
  # logger by default at runtime
  Log::Log4perl->easy_init( {
                             level => $WARN,
                             layout => '[%M](%L): %m%n'}
                           );
  
  # check Options
  
  # env : optional !
  if (defined $opts{environment} && $opts{environment} ne "") {
    my $fenvironment = File::Spec->rel2abs($opts{environment});
    
    if (! -d dirname($fenvironment)) {
      ERROR(sprintf "File path doesn't exist ('%s') !", dirname($fenvironment));
      return FALSE;
    }
    
    if (! -f $fenvironment ) {
      ERROR(sprintf "File name doesn't exist ('%s') !", basename($fenvironment));
      return FALSE;
    }
    #
    $opts{environment} = $fenvironment;
  }
  
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
  
  ALWAYS(">>> Load Properties Environment ...");
  
  ###################
  # load environement be4
  my $fenv = undef;
  
  # env user !
  $fenv = $opts{environment} if (defined $opts{environment} && $opts{environment} ne "");
  
  # if env user is null, we search in env sys !
  if (! defined $opts{environment}) {
    
    # if variable environment is fixed !
    if(exists ($ENV{BE4_ENV}) &&
       defined($ENV{BE4_ENV}) ) {
      
      INFO("Use environment variable 'BE4_ENV' to determined the global configuration !");
      $fenv = File::Spec->catfile($ENV{BE4_ENV}, "conf", "be4.properties");
    }
    # if not, by default, we take the file env in the install directory !
    else {
      
      INFO("Use path runtime to determined the configuration !");
      $fenv = File::Spec->catfile($Bin, "..", "conf", "be4.properties");
    }
    
  }
  
  # FIXME :
  #   This option may be optional ?
  if (! -f $fenv) {
    ERROR(sprintf "The environment properties '%s' doesn't exist into directory '%s' !", basename($fenv), dirname($fenv));
    return FALSE;
  }
  
  my $objEnv = BE4::PropertiesLoader->new($fenv);
  
  if (! defined $objEnv) {
    ERROR("Can not load environment properties !");
    return FALSE;
  }
  
  my $refEnv = $objEnv->getAllProperties();
  
  if (! defined $refEnv) {
    ERROR("All parameters environment are empty !");
    return FALSE;
  }
  
  ALWAYS(">>> Load Properties ...");
  
  ###################
  # load specific properties 
  my $fprop = $opts{properties};
  
  my $objProp = BE4::PropertiesLoader->new($fprop);
  
  if (! defined $objProp) {
    ERROR("Can not load specific properties !");
    return FALSE;
  }
  
  my $refProp = $objProp->getAllProperties();
  
  if (! defined $refProp) {
    ERROR("All parameters properties are empty !");
    return FALSE;
  }
  
  # merge params/env
  #sub merge_hashref {
  #  return { map %$_, grep ref $_ eq 'HASH', @_ }
  #}
  
  my $hashref;
  foreach (keys %{$this{params}}) {
    my $href = { map %$_, grep ref $_ eq 'HASH', ($this{params}->{$_}, $refEnv->{$_}, $refProp->{$_}) };
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
  
  my $pyramid     = $this{params}->{pyramid};       # 
  my $logger      = $this{params}->{logger};        # 
  my $tms         = $this{params}->{tilematrixset}; #  
  my $tile        = $this{params}->{tile};          # 
  my $nodata      = $this{params}->{nodata};        # 
  my $datasource  = $this{params}->{datasource};    # 
  my $harvesting  = $this{params}->{harvesting};    # 
  my $process     = $this{params}->{process};       # 
  
  # pyramid
  if (! defined $pyramid) {
    ERROR ("Parameters Pyramid can not be null !");
    return FALSE;
  }
  
  # logger
  if (defined $logger) {
    
    my @args;
    
    my $layout= '[%C][%M](%L): %m%n';
    my $level = $logger->{log_level};
    my $out   = sprintf (">>%s", File::Spec->catfile($logger->{log_path}, $logger->{log_file}))
                    if (! IsEmpty($logger->{log_path}) && ! IsEmpty($logger->{log_file}));
    
    $out   = "STDOUT" if (! defined $out);
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
    
    if ($out ne "STDOUT") {
        # add the param logger to the STDOUT
        push @args, {
                        file   => "STDOUT",
                        level  => $level,
                        layout => $layout,
                    },
    }
    Log::Log4perl->easy_init(@args); 
  }
  
  # 
  $pyramid = { map %$_, grep ref $_ eq 'HASH', ($tms,         $pyramid) };
  $pyramid = { map %$_, grep ref $_ eq 'HASH', ($tile,        $pyramid) };
  $pyramid = { map %$_, grep ref $_ eq 'HASH', ($nodata,      $pyramid) };
  
  #
  if (! defined $datasource) {}
  #
  if (! defined $harvesting) {}
  #
  if (! defined $process) {}
  
  # save
  $this{params}->{pyramid} = $pyramid;
  $this{params}->{logger}  = $logger;
  
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
    
    ###################
    # objects to implemented
    
    my $objPyramid     = undef;
    my $objData        = undef;
    my $objProcess     = undef;
  
    ##################
    # create a pyramid
    
    ALWAYS(">>> Load a Pyramid ...");
    
    $objPyramid = BE4::Pyramid->new($params->{pyramid});
    
    if (! defined $objPyramid) {
      ERROR ("Can not load Pyramid !");
      return FALSE;
    }
  
    # we cannot write the pyramid descriptor and cache now. We need data's limits and top/bottom levels 
    # which are calculated in the Process creation
    
    ###################
    # load data source
    
    if (main::HasDataSource()) {
    
        ALWAYS(">>> Load Data Source ...");

        $objData = BE4::DataSource->new($params->{datasource});
        $objData->{nodataColor} = $objPyramid->{pyramid}->{color};
        ALWAYS (sprintf "nodata = %s", $objPyramid->{pyramid}->{color});#TEST#
        
        if (! defined $objData) {
          ERROR ("Can not load the data source !");
          return FALSE;
        }
        
        # load an image descriptor of data source
        ALWAYS(">>> Compute Data Source image ...");
        
        if (! $objData->hasImages() ||
            ! $objData->computeImageSource()) {
          ERROR ("Can not compute the list of data image !");
          return FALSE;
        }
        
        # TODO : Metadata
        #        not implemented !
        
        DEBUG (sprintf "DATASOURCE (dump) = %s", Dumper($objData));
    }
  
    #######################
    # create process script
    
    return TRUE if ! main::HasDataSource();
    
    ALWAYS(">>> Load Process ...");
  
    $objProcess = BE4::Process->new(
                      $params->{process},
                      $params->{harvesting},
                      $objPyramid,
                      $objData
                    );
  
    if (! defined $objProcess) {
      ERROR ("Can not prepare process !");
      return FALSE;
    }
    
    ##################
    # write the pyramid cache
    
    ALWAYS(">>> Write Cache Pyramid ...");

    if (! $objPyramid->writeCachePyramid()) {
      ERROR ("Can not write Pyramid Cache !");
      return FALSE;
    }
    
    ##################
    # write the pyramid descriptor

    ALWAYS(">>> Write Configuration Pyramid ...");

    if (! $objPyramid->writeConfPyramid()) {
      ERROR ("Can not write Pyramid File !");
      return FALSE;
    }
  
    DEBUG (sprintf "PYRAMID (dump) = %s", Dumper($objPyramid));
  
    ##################
    # compute process
    
    ALWAYS(">>> Compute Process ...");
    
    if (! $objProcess->computeWholeTree()) {
      ERROR ("Can not compute process !");
      return FALSE;
    }
      
    DEBUG (sprintf "PROCESS (dump) = %s", Dumper($objProcess));
    

    #######################
    # execute process script
    
    return TRUE if (! $opts{test});
    
    if (! $objProcess->processScript()) {
      ERROR ("Can not execute process !");
      return FALSE;
    }
    
    return TRUE;
}

#
# Group: get/set
#
#   None

#
# Group: functions 
#

sub HasDataSource {
  
  TRACE();
  
  my $datasource = $this{params}->{datasource};
  return ! IsEmpty($datasource->{path_image});
}
sub IsEmpty {

  my $value = shift;
  
  return FALSE if (ref($value) eq "HASH");
  return TRUE  if (! defined $value);
  return TRUE  if ($value eq "");
  return FALSE;
}
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

  be4 - create a pyramid of tile in tiff, jpeg or png format from data image (ortho, scan, ...)
  or from an harvesting WMS (eg with Rok4).

=head1 SYNOPSIS

  perl be4.pl --conf=path [ --env=path ]
  All parameters of the --env config file can be overided by --conf config file parameter

=head1 DESCRIPTION

Use case possible :

    * 1.  create an empty pyramid in native mode (raw) or in compression mode (jpg or png)
    * 2.  create a new pyramid with data (in native mode)
    * 3.  create a pyramid from an existing with data source (in native mode)
    * 3'. create a pyramid from an existing empty with data source (in native mode)
    * 4.  create a pyramid from an existing in native mode without data source (in compression mode) 
    * 5.  create a pyramid from an existing in native mode with data source (in compression mode)

=head1 FILE CONFIGURATION

* ALL PARAMS POSSIBLE :
 
  logger        => log_path, log_file, log_level
  datasource    => path_image, [ path_metadata ], srs
  harvesting    => wms_url, wms_version, wms_request, wms_format, wms_layer
  pyramid       =>  compression
                    image_width, image_height,
                    tms_name, tms_path
                    dir_depth, dir_root, dir_image, dir_metadata,
                    pyr_name_old, pyr_name_new, pyr_path
                    bitspersample,sampleformat,compressionscheme,photometric,samplesperpixel,interpolation
                    path_nodata, imagesize, color
  [ tilematrixset => tms_name,tms_path ]
  [ nodata        => path_nodata, imagesize, color ]
  [ tile          => bitspersample,sampleformat,compressionscheme,photometric,samplesperpixel,interpolation ]
  [ compression   => type ]
  process       => job_number, path_temp, path_shell, [ percentexpansion, percentprojection ]

* SAMPLE OF FILE CONFIGURATION OF PYRAMID :

cf. directory ./conf/Samples/ for more sample of pyramid configuration.

A file configuration can be composed of sections and parameters following :

 [ logger ]
 log_path  =
 log_file  =
 log_level =

 [ harvesting ]
 wms_layer   =
 wms_url     =
 wms_version =
 wms_request =
 wms_format  =

 [ datasource ]
 path_image    = 
 ; path_metadata =    # NOT IMPLEMENTED ! 
 srs           =

 [ pyramid ]
 pyr_name_old  =
 pyr_name_new  =
 pyr_path      =
 ; pyr_schema_path =   # NOT IMPLEMENTED ! 
 ; pyr_schema_name =   # NOT IMPLEMENTED ! 
 compression  =
 image_width  = 
 image_height =
 dir_depth    = 
 dir_root     = 
 dir_image    = 
 dir_metadata = 

 [ tilematrixset ]
 tms_name      =
 tms_path      = 
 ; tms_schema_path =    # NOT IMPLEMENTED ! 
 ; tms_schema_name =    # NOT IMPLEMENTED ! 
 
 [ nodata ]
 path_nodata   =
 imagesize     =
 color         =

 [ tile ]
 bitspersample       = 
 sampleformat        = 
 compressionscheme   = 
 photometric         = 
 samplesperpixel     =
 interpolation       =
 
 [ process ]
 job_number        =
 path_temp         =
 path_shell        =
 ; percentexpansion  =     # NOT IMPLEMENTED ! 
 ; percentprojection =     # NOT IMPLEMENTED ! 

But you can combine the following sections for more readibility :

 [ pyramid ]
 
 # ie section tile
 bitspersample       = 
 sampleformat        = 
 compressionscheme   = 
 photometric         = 
 samplesperpixel     =
 interpolation       =
 
 # ie section tilematrixset
 tms_name      =
 tms_path      =
 
 # ie section nodata
 path_nodata   =
 imagesize     =
 color         =
 
 # 
 pyr_name_old  =
 pyr_name_new  =
 pyr_desc_path =
 pyr_data_path =
 compression   =
 image_width   = 
 image_height  =
 dir_depth     =  
 dir_image     = 
 dir_metadata  =
 
 [ process ]
 ...
 [ datasource ]
 ...
 [ logger ]
 ...
 [ harvesting ]
 ...
 
Few sections can be optional, depending on the selected use cases :

 [ datasource ]
 [ harvesting ]
 [ process ]
 [ logger ]

And few parameters must be declared, whatever the use cases selected :

 [ pyramid ]
    pyr_desc_path
    pyr_data_path
    tms_path
    pyr_name_new
    compression
    dir_depth
    dir_image
    dir_metadata
    path_nodata

These parameters can be null because of parameters by default :

 [ logger ]
   path      - null
   level     - WARN
   file      - STDOUT
 
 [ nodata ] or [ pyramid ]
   color     - FFFFFF
   imagesize - 4096
 
 [ tile ] or [ pyramid ]
   interpolation   - bicubic
   photometric     - rgb
 
 [ pyramid ]
   compression - raw

=head1 OPTIONS

=over

=item B<--help>

=item B<--usage>

=item B<--version>

=item B<--conf=path>

Path to file configuration of the pyramid.
This option is manadatory !

=item B<--env=path>

Path to file environment of all pyramid, it's the common configuration.
This option is optional !
By default, the file configuration of install is used.

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
    use BE4::Pyramid;
    use BE4::DataSource;
    use BE4::Process;

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
