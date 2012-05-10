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
use Math::BigFloat;
use File::Spec::Link;
use File::Basename;
use File::Spec;
use File::Path;
use Cwd;

use Log::Log4perl qw(:easy);
use XML::LibXML;

# My search module
use FindBin qw($Bin);
use lib "$Bin/../lib/perl5";

# My module
use BE4::TileMatrixSet;
use BE4::Pixel;
use BE4::Level;

# constantes
use constant TRUE  => 1;
use constant FALSE => 0;
use constant RESULT_TEST => "if [ \$? != 0 ] ; then echo \$0 : Erreur a la ligne \$(( \$LINENO - 1)) >&2 ; exit 1; fi\n";

# pas de bufferisation des sorties.
$|=1;

# version
# my $VERSION = "@VERSION_TEXT@";
my $VERSION = "0.1.0";

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

    logger        => undef,
    pyramid       => undef,
    bboxes        => undef,
    composition   => undef,
    process       => undef,

    sourceByLevel => {},
    sourcePyramids => {},

    currentscript => [],
    scripts => [],

    doneTiles => {},
);

my %CONF = (
    sections => ['pyramid','process','composition','logger','bboxes'],
    merge_method => ['replace','transparency'],
    compression => ['raw','jpg','png','lzw'],
);

my %SAMPLEFORMAT2CODE = (
    uint => "INT",
    float => "FLOAT"
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
    if (! main::_init()) {
        $message = "ERROR INITIALIZATION !";
        printf STDERR "%s\n", $message;
        exit 1;
    }

    # configuration
    ALWAYS("> Configuration");
    if (! main::_config()) {
        $message = "ERROR CONFIGURATION !";
        printf STDERR "%s\n", $message;
        exit 2;
    }

    # execution
    ALWAYS("> Validation");
    if (! main::_validate()) {
        $message = "ERROR VALIDATION !";
        printf STDERR "%s\n", $message;
        exit 3;
    }

    # execution
    ALWAYS("> Execution");
    if (! main::doIt()) {
        $message = "ERROR EXECUTION !";
        printf STDERR "%s\n", $message;
        exit 4;
    }

    # writting
    ALWAYS("> Ecriture des scripts");
    if (! main::saveScripts()) {
        $message = "ERROR EXECUTION !";
        printf STDERR "%s\n", $message;
        exit 4;
    }

#DEBUG(sprintf "PYRAMIDS : %s",Dumper($this{pyramid}));

    $message = "END";
    printf STDOUT "%s\n", $message;
}
################################################################################
# function: init
#   Check options and initialize the logger
#  
sub _init {

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

####################################################################################################
#                                     CONFIGURATION LOADER                                         #
####################################################################################################  

sub _config {
  
    ALWAYS(">>> Load Properties ...");

    my $fprop = $opts{properties};

    if (! defined $fprop) {
        ERROR ("The file path is required to read configuration !");
        return FALSE;
    }
    
    if (! -f $fprop) {
        ERROR (sprintf "Configuration file doesn't exist : %s",$fprop);
        return FALSE;
    }

    return FALSE if (! main::_read($fprop));
    return FALSE if (! main::checkParams($fprop));

    return TRUE;

}

# method: _read
#---------------------------------------------------------------------------------------------------
sub _read {
    my $filepath = shift;

    TRACE;

    if (! open CFGF, "<", $filepath ){
        ERROR(sprintf "Impossible d'ouvrir le fichier de configuration %s.",$filepath);
        return FALSE;
    }

    my $currentSection = undef;

    while( defined( my $l = <CFGF> ) ) {
        chomp $l;
        $l =~ s/\s+//g; # we remove all spaces
        $l =~ s/;\S*//; # we remove comments
        
        next if ($l eq '');

        if ($l =~ m/^\[(\w*)\]$/) {
            $l =~ s/[\[\]]//g;

            if (! main::is_ConfSection($l)) {
                ERROR ("Invalid section's name");
                return FALSE;
            }
            $currentSection = $l;
            next;
        }

        if (! defined $currentSection) {
            ERROR (sprintf "A property must always be in a section (%s)",$l);
            return FALSE;
        }

        my @prop = split(/=/,$l,-1);
        if (scalar @prop != 2 || $prop[0] eq '' || $prop[1] eq '') {
            ERROR (sprintf "A line is invalid (%s). Must be prop = val",$l);
            return FALSE;   
        }

        if ($currentSection ne 'composition') {
            if (exists $this{$currentSection}->{$prop[0]}) {
                ERROR (sprintf "A property is defined twice in the configuration : section %s, parmaeter %s",
                    $currentSection,$prop[0]);
                return FALSE; 
            }
            $this{$currentSection}->{$prop[0]} = $prop[1];
        } else {
            if (! main::readComposition($prop[0],$prop[1])) {
                ERROR (sprintf "Cannot read a composition !");
                return FALSE;
            }
        }
        
    }

    close CFGF;

    return TRUE;
}

# method: readComposition
#---------------------------------------------------------------------------------------------------
sub readComposition {
    my $prop = shift;
    my $val = shift;

    TRACE;

    my ($levelId,$bboxId) = split(/\./,$prop,-1);

    if ($levelId eq '' || $bboxId eq '') {
        ERROR (sprintf "Cannot define a level id and a bbox id (%s). Must be levelId.bboxId",$prop);
        return FALSE;
    }

    my @pyrs = split(/,/,$val,-1);

    foreach my $pyr (@pyrs) {
        if ($pyr eq '') {
            ERROR (sprintf "Invalid list of pyramids (%s). Must be /path/pyr1.pyr,/path/pyr2.pyr",$val);
            return FALSE;
        }
        if (! -f $pyr) {
            ERROR (sprintf "A referenced pyramid's file doesn't exist : %s",$pyr);
            return FALSE;
        }

        my $priority = 1;
        if (exists $this{sourceByLevel}->{$levelId}) {
            $this{sourceByLevel}->{$levelId} += 1;
            $priority = $this{sourceByLevel}->{$levelId};
        } else {
            $this{sourceByLevel}->{$levelId} = 1;
        }

        $this{composition}->{$levelId}->{$priority} = {
            bbox => $bboxId,
            pyr => $pyr,
        };

        if (! exists $this{sourcePyramids}->{$pyr}) {
            # we have a new source pyramid, but not yet information about
            $this{sourcePyramids}->{$pyr} = undef;
        }

    }

    return TRUE;

}

# method: is_ConfSection
#---------------------------------------------------------------------------------------------------
sub is_ConfSection {
    my $section = shift;

    TRACE;

    return FALSE if (! defined $section);

    foreach (@{$CONF{sections}}) {
        return TRUE if ($section eq $_);
    }
    ERROR (sprintf "Unknown 'section' (%s) !",$section);
    return FALSE;
}



# method: readComposition
#---------------------------------------------------------------------------------------------------
sub checkParams {

    ###################
    # check parameters

    my $pyramid      = $this{pyramid};        
    my $logger       = $this{logger};         
    my $composition  = $this{composition};    
    my $bboxes       = $this{bboxes};         
    my $process      = $this{process};    

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



####################################################################################################
#                                       VALIDATION METHODS                                         #
####################################################################################################  


# method: _validate
#---------------------------------------------------------------------------------------------------
sub _validate {


    ##################

    ALWAYS(">>> Validate merge pyramid ...");

    if (! main::validateMergedPyramid()) {
        ERROR ("Merged pyramid is not valid !");
        return FALSE;
    }

    ##################
    # load bounding boxes
    ALWAYS(">>> Validate bounding boxes ...");

    if (! main::validateBboxes()) {
        ERROR ("Some bboxes are not valid !");
        return FALSE;
    }

    DEBUG(sprintf "BBOXES : %s",Dumper($this{bboxes}));

    ##################

    ALWAYS(">>> Validate source pyramids ...");

    if (! main::validateSourcePyramids()) {
        ERROR ("Some source pyramids are not valid !");
        return FALSE;
    }

    DEBUG(sprintf "SOURCE PYRAMIDS : %s",Dumper($this{sourcePyramids}));

    ALWAYS(">>> Validate composition");
    if (! main::validateComposition()) {
        ERROR ("Cannot validate composition !");
        return FALSE;
    }

    DEBUG(sprintf "COMPOSITION : %s",Dumper($this{composition}));

    return TRUE;
    
}

# method: validateComposition
#       - we control the levelId (have to be present in the TMS)
#       - 
#---------------------------------------------------------------------------------------------------
sub validateComposition {

    TRACE();

    my $composition = $this{composition};
    my $bboxes = $this{bboxes};
    my $tms = $this{pyramid}->{tms};

    while( my ($levelId,$sources) = each(%$composition) ) {
        if (! defined $tms->getTileMatrix($levelId)) {
            ERROR (sprintf "A level id (%s) from the configuration file is not in the TMS !",$levelId);
            return FALSE;
        }

        while( my ($priority,$source) = each(%$sources) ) {
            if (! exists $bboxes->{$source->{bbox}}) {
                ERROR (sprintf "A bbox id (%s) from the composition is not define in the 'bboxes' section !",
                    $source->{bbox});
                return FALSE;
            }
            # we replace .pyr path by the data directory for this level
            if (! exists $this{sourcePyramids}->{$source->{pyr}}->{$levelId}) {
                ERROR (sprintf "The pyramid '%s' is used for the level %s but has not it !",
                    $source->{pyr},$levelId);
                return FALSE;
            }

            if (exists $this{sourcePyramids}->{$source->{pyr}}->{isCompatible}) {
                $source->{isCompatible} = TRUE;
            } else {
                $source->{format} = $this{sourcePyramids}->{$source->{pyr}}->{format};
                $source->{photometric} = $this{sourcePyramids}->{$source->{pyr}}->{photometric};
                $source->{samplesperpixel} = $this{sourcePyramids}->{$source->{pyr}}->{samplesperpixel};
            }

            $source->{pyr} = $this{sourcePyramids}->{$source->{pyr}}->{$levelId};
            my @bboxArray = main::calculateBbox($levelId,$source->{bbox});
            $source->{bbox} = undef;
            @{$source->{bbox}} = @bboxArray;
        }
    
    }

    return TRUE;
}

# method: calculateBbox
#  from a bbox ID and a level ID, calculate extrems index and return them
#---------------------------------------------------------------------------------------------------
sub calculateBbox {
    my $levelId = shift;
    my $bboxId = shift;

    TRACE();

    my @bboxArray = @{$this{bboxes}->{$bboxId}} ;

    my $tm  = $this{pyramid}->{tms}->getTileMatrix($levelId);

    my $Res = Math::BigFloat->new($tm->getResolution());
    my $imgGroundWidth = $tm->getTileWidth() * $this{pyramid}->{tilesPerWidth} * $Res;
    my $imgGroundHeight = $tm->getTileHeight() * $this{pyramid}->{tilesPerHeight} * $Res;

    my $iMin=int(($bboxArray[0] - $tm->getTopLeftCornerX()) / $imgGroundWidth);   
    my $iMax=int(($bboxArray[2] - $tm->getTopLeftCornerX()) / $imgGroundWidth);   
    my $jMin=int(($tm->getTopLeftCornerY() - $bboxArray[3]) / $imgGroundHeight); 
    my $jMax=int(($tm->getTopLeftCornerY() - $bboxArray[1]) / $imgGroundHeight);

    $bboxArray[0] = $iMin;
    $bboxArray[1] = $jMin;
    $bboxArray[2] = $iMax;
    $bboxArray[3] = $jMax;

    return @bboxArray;

    return TRUE;
}

# method: validateBboxes
#  for each bbox, we parse string to store values in array and we control consistency (min < max)
#---------------------------------------------------------------------------------------------------
sub validateBboxes {
    TRACE();

    my $bboxes = $this{bboxes};

    while( my ($bboxId,$bbox) = each(%$bboxes) ) {
        if ($bbox !~ m/([+-]?\d+(\.\d+)?),([+-]?\d+(\.\d+)?),([+-]?\d+(\.\d+)?),([+-]?\d+(\.\d+)?)/) {
            ERROR (sprintf "The bbox with id '%s' is not valid (%s).
                Must be 'xmin,ymin,xmax,ymax', to decimal format.",$bboxId,$bbox);
            return FALSE;
        }

        my @bboxArray = split(/,/,$bbox,-1);
        if (!($bboxArray[0] < $bboxArray[2] && $bboxArray[1] < $bboxArray[3])) {
            ERROR (sprintf "The bbox with id '%s' is not valid (%s). Max is not greater than min !",$bboxId,$bbox);
            return FALSE;
        }
        
        # we replace the string bbox by the array bbox
        $bboxes->{$bboxId} = undef;
        @{$bboxes->{$bboxId}} = @bboxArray;
    }

    return TRUE;
}

# method: validateMergedPyramid
#---------------------------------------------------------------------------------------------------
sub validateMergedPyramid {

    TRACE();

    ##################
    # load TMS

    if (!( exists $this{pyramid}->{tms_path} &&  exists $this{pyramid}->{tms_name})) {
        ERROR ("TMS path information are missing in the configuration file !");
        return FALSE;
    }
    my $objTMS = BE4::TileMatrixSet->new(File::Spec->catfile($this{pyramid}->{tms_path},$this{pyramid}->{tms_name}));

    if (! defined $objTMS) {
        ERROR (sprintf "Cannot load the TMS %s !",$this{pyramid}->{tms_name});
        return FALSE;
    } else {
        delete($this{pyramid}->{tms_path});
        delete($this{pyramid}->{tms_name});
    }

    $this{pyramid}->{tms} = $objTMS;

    if (! exists $this{pyramid}->{merge_method}) {
        ERROR ("'merge_method' must be given in the configuration file !");
        return FALSE;
    } elsif (! main::is_MergeMethod($this{pyramid}->{merge_method})) {
        ERROR ("Invalid 'merge_method' !");
        return FALSE;
    }

    if (! exists $this{pyramid}->{compression}) {
        ERROR ("'compression' must be given in the configuration file !");
        return FALSE;
    } elsif (! main::is_Compression($this{pyramid}->{compression})) {
        ERROR ("Invalid 'compression' !");
        return FALSE;
    }

    my $objPixel = BE4::Pixel->new({
        photometric => $this{pyramid}->{photometric},
        sampleformat => $this{pyramid}->{sampleformat},
        bitspersample => $this{pyramid}->{bitspersample},
        samplesperpixel => $this{pyramid}->{samplesperpixel}
    });
    if (! defined $objPixel) {
        ERROR (sprintf "Cannot create the Pixel object !");
        return FALSE;
    } else {

        # formatCode : TIFF_[COMPRESSION]_[SAMPLEFORMAT][BITSPERSAMPLE]
        $this{pyramid}->{format} = sprintf "TIFF_%s_%s%s",
            uc $this{pyramid}->{compression},
            $SAMPLEFORMAT2CODE{$this{pyramid}->{sampleformat}},
            $this{pyramid}->{bitspersample};

        delete($this{pyramid}->{photometric});
        delete($this{pyramid}->{sampleformat});
        delete($this{pyramid}->{bitspersample});
        delete($this{pyramid}->{samplesperpixel});
    }
    $this{pyramid}->{pixel} = $objPixel;

    return TRUE;

}

# method: is_MergeMethod
#---------------------------------------------------------------------------------------------------
sub is_MergeMethod {
    my $mergeMethod = shift;

    TRACE;

    return FALSE if (! defined $mergeMethod);

    foreach (@{$CONF{merge_method}}) {
        return TRUE if ($mergeMethod eq $_);
    }
    ERROR (sprintf "Unknown 'merge_method' (%s) !",$mergeMethod);
    return FALSE;
}

# method: is_Compression
#---------------------------------------------------------------------------------------------------
sub is_Compression {
    my $compression = shift;

    TRACE;

    return FALSE if (! defined $compression);

    foreach (@{$CONF{compression}}) {
        return TRUE if ($compression eq $_);
    }
    ERROR (sprintf "Unknown 'compression' (%s) !",$compression);
    return FALSE;
}

# method: validateSourcePyramids
#  for each pyramids, we control attributes : TMS, format... They must be the same for every one
#---------------------------------------------------------------------------------------------------
sub validateSourcePyramids {

    TRACE();

    my $tms;
    my $dirDepth;
    my $tilesPerWidth;
    my $tilesPerHeight;

    foreach my $pyr (keys %{$this{sourcePyramids}}) {
        my ($volume,$directories,$file) = File::Spec->splitpath($pyr);

        # read xml pyramid
        my $parser  = XML::LibXML->new();
        my $xmltree =  eval { $parser->parse_file($pyr); };

        if (! defined ($xmltree) || $@) {
            ERROR (sprintf "Can not read the XML file Pyramid : %s !", $@);
            return FALSE;
        }

        my $root = $xmltree->getDocumentElement;

        # FORMAT
        my $tagformat = $root->findnodes('format')->to_literal;
        if ($tagformat eq '') {
            ERROR (sprintf "Can not determine parameter 'format' in the XML file Pyramid (%s) !",
                $pyr);
            return FALSE;
        }
        # to remove when format 'TIFF_INT8' and 'TIFF_FLOAT32' will be remove
        if ($tagformat eq 'TIFF_INT8') {
            WARN("'TIFF_INT8' is a deprecated format, use 'TIFF_RAW_INT8' instead");
            $tagformat = 'TIFF_RAW_INT8';
        }
        if ($tagformat eq 'TIFF_FLOAT32') {
            WARN("'TIFF_FLOAT32' is a deprecated format, use 'TIFF_RAW_FLOAT32' instead");
            $tagformat = 'TIFF_RAW_FLOAT32';
        }
        
        # SAMPLES PER PIXEL
        my $tagchannels = $root->findnodes('channels')->to_literal;
        if ($tagchannels eq '') {
            ERROR (sprintf "Can not determine parameter 'channels' in the XML file Pyramid (%s) !",
                $pyr);
            return FALSE;
        }

        # TMS
        my $tagTMS = $root->findnodes('tileMatrixSet')->to_literal;
        if ($tagTMS eq '') {
            ERROR (sprintf "Can not determine parameter 'tileMatrixSet' in the XML file Pyramid (%s) !",
                $pyr);
            return FALSE;
        }
        if (defined $tms && $tagTMS ne $tms) {
            ERROR (sprintf "The TMS in the pyramid '%s' is different from %s (%s) !",
                $pyr,$tms,$tagTMS);
            return FALSE;
        }
        $tms = $tagTMS if (! defined $tms);
        if ($tms ne $this{pyramid}->{tms}->{name}) {
            ERROR (sprintf "The TMS in the pyramid '%s' is different from the TMS in the configuration %s (%s) !",
                $pyr,$this{pyramid}->{tms}->{name},$tms);
            return FALSE;
        }

        # PHOTOMETRIC
        my $tagphotometric = $root->findnodes('photometric')->to_literal;
        if ($tagphotometric eq '') {
            ERROR (sprintf "Can not determine parameter 'photometric' in the XML file Pyramid (%s) !",
                $pyr);
            return FALSE;
        }

        # LEVELS
        my @levels = $root->getElementsByTagName('level');
        
        # read tilesPerWidth and tilesPerHeight, using a level
        my $level = $levels[0];
        my $tagtilesPerWidth = $level->findvalue('tilesPerWidth');
        my $tagtilesPerHeight = $level->findvalue('tilesPerHeight');

        if (defined $tilesPerHeight && 
            ($tagtilesPerHeight ne $tilesPerHeight || $tagtilesPerWidth ne $tilesPerWidth)) {
            ERROR (sprintf "The tile size in the pyramid '%s' is different from %s,%s (%s,%s) !",
                $pyr,$tilesPerHeight,$tilesPerWidth,$tagtilesPerHeight,$tagtilesPerWidth);
            return FALSE;
        }
        $tilesPerHeight = $tagtilesPerHeight if (! defined $tilesPerHeight);
        $tilesPerWidth = $tagtilesPerWidth if (! defined $tilesPerWidth);

        # read dirDepth, using a level
        my $tagdepth = $level->findvalue('pathDepth');

        if (defined $dirDepth && $tagdepth ne $dirDepth) {
            ERROR (sprintf "The depth in the pyramid '%s' is different from %s (%s) !",$pyr,$dirDepth,$tagdepth);
            return FALSE;
        }
        $dirDepth = $tagdepth if (! defined $dirDepth);

        foreach my $v (@levels) {
            my $levelId = $v->findvalue('tileMatrix');
            my $baseDir = $v->findvalue('baseDir');
            $this{sourcePyramids}->{$pyr}->{$levelId} = File::Spec->rel2abs($baseDir,$volume.$directories);
        }

        if (main::is_Compatible($tagformat,$tagchannels)) {
            $this{sourcePyramids}->{$pyr}->{isCompatible} = TRUE;
        } else {
            $this{sourcePyramids}->{$pyr}->{format} = $tagformat;
            $this{sourcePyramids}->{$pyr}->{samplesperpixel} = $tagchannels;
            $this{sourcePyramids}->{$pyr}->{photometric} = $tagphotometric;
        }

    }

    # we save merged pyramid's attributes
    $this{pyramid}->{dirDepth} = $dirDepth;
    $this{pyramid}->{tilesPerWidth} = $tilesPerWidth;
    $this{pyramid}->{tilesPerHeight} = $tilesPerHeight;

    return TRUE;
}

# method: is_Compatible
#---------------------------------------------------------------------------------------------------
sub is_Compatible {
    my $format = shift;
    my $samplesperpixel = shift;

    TRACE;

    return ($format eq $this{pyramid}->{format} && $samplesperpixel eq $this{pyramid}->{pixel}->{samplesperpixel});
}

####################################################################################################
#                                        PROCESS METHODS                                           #
####################################################################################################  

# method: doIt
#---------------------------------------------------------------------------------------------------
sub doIt {

    TRACE();

    main::initScripts();

    my $composition = $this{composition};
    my $bboxes = $this{bboxes};
    my $tms = $this{pyramid}->{tms};

    while( my ($levelId,$sources) = each(%$composition) ) {
        INFO(sprintf "Level %s",$levelId);

        my $priority = 1;
        my $baseImage = sprintf "%s/%s/%s/%s",
                        $this{pyramid}->{pyr_data_path},$this{pyramid}->{pyr_name},
                        $this{pyramid}->{image_dir},$levelId;
        my $baseNodata = sprintf "%s/%s/%s/%s",
                        $this{pyramid}->{pyr_data_path},$this{pyramid}->{pyr_name},
                        $this{pyramid}->{nodata_dir},$levelId;
        my ($IMIN,$JMIN,$IMAX,$JMAX);

        while( exists $sources->{$priority}) {
            my $source = $sources->{$priority};
            INFO(sprintf "Priority %s : pyramid %s",$priority,$source->{pyr});
            my ($imin,$jmin,$imax,$jmax) = @{$source->{bbox}};

            for (my $i = $imin; $i <= $imax; $i++) {
                for (my $j = $jmin; $j <= $jmax; $j++) {
                    if (exists $this{doneTiles}->{$i."_".$j}) {
                        #Tile already exists (created by a treatment)
                        next;
                    }

                    my $imagePath = main::getCacheNameOfImage($i,$j);
                    my $sourceImage = $source->{pyr}.$imagePath;
                    if (! -f $sourceImage) {
                        # no data source
                        next;
                    }

                    my $finaleImage = $baseImage.$imagePath;
                    if (-f $finaleImage) {
                        #Tile already exists (it's a link)
                        next;
                    }

                    my @images;
                    if ($this{pyramid}->{merge_method} ne 'replace') {
                        @images = main::searchTile($levelId,$priority,$i,$j,$imagePath);
                    }

                    if (! main::treatTile($i,$j,$finaleImage,@images)) {
                        ERROR(sprintf "Cannot treat the image %s",$finaleImage);
                        return FALSE;                        
                    }
                    # we update extrems tiles
                    if (! defined $IMIN || $i < $IMIN) {$IMIN = $i;}
                    if (! defined $JMIN || $j < $JMIN) {$JMIN = $j;}
                    if (! defined $IMAX || $i > $IMAX) {$IMAX = $i;}
                    if (! defined $JMAX || $j > $JMAX) {$JMAX = $j;}
                }
            }
            $priority++;
        }

        my $levelOrder = $tms->getTileMatrixOrder($levelId);
        my $objLevel = BE4::Level->new({
            id                => $levelId,
            order             => $levelOrder,
            dir_image         => File::Spec->abs2rel($baseImage,$this{pyramid}->{pyr_desc_path}),
            dir_nodata        => File::Spec->abs2rel($baseNodata,$this{pyramid}->{pyr_desc_path}),
            dir_metadata      => undef,      # TODO !
            compress_metadata => undef,      # TODO !
            type_metadata     => undef,      # TODO !
            size              => [$this{pyramid}->{tilesPerWidth},$this{pyramid}->{tilesPerHeight}],
            dir_depth         => $this{pyramid}->{dirDepth},
            limit             => [$IMIN,$JMIN,$IMAX,$JMAX],
            is_in_pyramid     => 0
        });

        if (! defined $objLevel) {
            ERROR(sprintf "Can not create the pyramid Level object : '%s'", $levelId);
            return FALSE;
        }

        $this{pyramid}->{levels}->{$levelId} = $objLevel;

        delete $this{doneTiles};
    }

    return TRUE;

}

# method: searchTile
#  search a tile in source pyramids which have not the priority
#---------------------------------------------------------------------------------------------------
sub searchTile {
    my $levelId = shift;
    my $priority = shift;
    my $i = shift;
    my $j = shift;
    my $imagePath = shift;

    TRACE();

    my $sources = $this{composition}->{$levelId};
    my @others;

    while( exists $sources->{$priority}) {
        my $source = $sources->{$priority};
        $priority++;
        my ($imin,$jmin,$imax,$jmax) = @{$source->{bbox}};

        if ($i < $imin || $i > $imax || $j < $jmin || $j > $jmax) {
            next;
        }

        my $sourceImage = $source->{pyr}.$imagePath;
        if (-f $sourceImage) {
            if (exists $source->{isCompatible}) {
                push @others,{img => $sourceImage,isCompatible => TRUE};
            } else {
                push @others,{img => $sourceImage,format => $source->{format}};
            }
        }
    }  

    return @others;
  
}

# method: treatTile
#       - we have only one source image for this tile : we create a symbolic link to this source if compatibility
#       - we have more sources : according to the merge method, we write necessary commands in the script
#--------------------------------------------------------------------------------------------------------
sub treatTile {
    my $i = shift;
    my $j = shift;
    my $finaleImage = shift;
    my @images = @_;

    TRACE();

    if (scalar @images == 1 && exists $images[0]{isCompatible}) {
        INFO(sprintf "Je fais un lien : %s",$finaleImage); #TEST#
        if (! main::makeLink($finaleImage,$images[0]{img})) {
            ERROR(sprintf "Cannot create link between %s and %s",$finaleImage,$images[0]);
            return FALSE;
        }
        return TRUE
    }

    if (scalar @images == 1) {
        INFO(sprintf "Je transforme une image : %s",$finaleImage); #TEST#
        main::transformImage($finaleImage,$images[0]{img},($images[0]{format} =~ m/PNG/));
        $this{doneTiles}->{$i."_".$j} = TRUE;
        return TRUE
    }

    my $mergeMethod = $this{pyramid}->{merge_method};

    if ($mergeMethod eq 'transparency') {
        INFO(sprintf "Je superpose pour créer : %s",$finaleImage); #TEST#
        main::mergeWithTransparency($finaleImage,@images);
        $this{doneTiles}->{$i."_".$j} = TRUE;
        return TRUE;
    }

    return FALSE;
}

# method: initScripts
#--------------------------------------------------------------------------------------------------------
sub initScripts {

    for (my $i = 0; $i < $this{process}->{job_number}; $i++) {
        my $scriptId = sprintf "SCRIPT_%s",$i+1;

        my $header = sprintf ("# Variables d'environnement\n");
        $header   .= sprintf ("SCRIPT_ID=\"%s\"\n", $scriptId);
        $header   .= sprintf ("TMP_DIR=\"%s\"\n",
            File::Spec->catdir($this{process}->{path_temp},$this{pyramid}->{pyr_name},$scriptId));
        $header   .= sprintf ("PYR_DIR=\"%s\"\n", $this{pyramid}->{pyr_data_path});
        $header   .= "\n";

        $header .= "# creation du repertoire de travail\n";
        $header .= "if [ ! -d \"\${TMP_DIR}\" ] ; then mkdir -p \${TMP_DIR} ; fi\n\n";

        push @{$this{scripts}}, $header
    }

    $this{currentscript} = 0;

}

# method: saveScripts
#  ecrit tous les scripts dans les fichiers correspondant
#-------------------------------------------------------------------------------
sub saveScripts {

    TRACE;

    for (my $i = 0; $i < $this{process}->{job_number}; $i++) {
        my $scriptId = sprintf "SCRIPT_%s",$i+1;
        if (! defined $scriptId) {
            ERROR("No ScriptId to save the script ?");
            return FALSE;
        }

        if (! defined $this{scripts}[$i]) {
            ERROR("No code to pass into the script ?"); 
            return FALSE;
        }

        my $scriptName     = join('.',$scriptId,'sh');
        my $scriptFilePath = File::Spec->catfile($this{process}->{path_shell}, $scriptName);

        if (! -d dirname($this{process}->{path_shell})) {
            my $dir = dirname($this{process}->{path_shell});
            DEBUG (sprintf "Create the script directory'%s' !", $dir);
            eval { mkpath([$dir],0,0751); };
            if ($@) {
                ERROR(sprintf "Can not create the script directory '%s' : %s !", $dir , $@);
                return FALSE;
            }
        }

        if ( ! (open SCRIPT,">", $scriptFilePath)) {
            ERROR(sprintf "Can not save the script '%s' into directory '%s' !.", $scriptName, dirname($scriptFilePath));
            return FALSE;
        }


        printf SCRIPT "%s", $this{scripts}[$i];
        close SCRIPT;

    }

    return TRUE;
}

sub mergeWithTransparency {
    my $finaleImage = shift;
    my @images = @_;

    TRACE();

    my $code = '';

    # Pretreatment

    for (my $i = 0; $i < scalar @images; $i++) {
        if (exists $images[$i]{format} && $images[$i]{format} =~ m/PNG/) {
            $code .= sprintf "untile %s \${TMP_DIR}/\n%s",$images[$i]{img},RESULT_TEST;
            $code .= sprintf "montage -geometry 256x256 -tile 16x16 \${TMP_DIR}/*.png -depth 8 -background none -define tiff:rows-per-strip=4096 \${TMP_DIR}/PNG_untiled.tif\n%s",RESULT_TEST;
            $code .= sprintf "tiff2rgba -c none \${TMP_DIR}/PNG_untiled.tif \${TMP_DIR}/img%s.tif\n%s",$i,RESULT_TEST;
        } else {
            $code .= sprintf "tiff2rgba -c none %s \${TMP_DIR}/img%s.tif\n%s",$images[$i]{img},$i,RESULT_TEST;
        }
    }

    # Superposition
    $code .= sprintf "overlayNtiff -transparent 255,255,255 -opaque 255,255,255 -channels %s -input ",
        $this{pyramid}->{pixel}->{samplesperpixel};

    for (my $i = scalar @images - 1; $i >= 0; $i--) {
        $code .= sprintf "\${TMP_DIR}/img%s.tif ",$i;
    }

    $code .= " -output \${TMP_DIR}/result.tif \n";

    my $compression = $this{pyramid}->{compression};
    $compression = ($compression eq 'raw'?'none':$compression);
    $compression = ($compression eq 'jpg'?'jpeg':$compression);

    # Final location writting
    $code .= sprintf "if [ -r \"%s\" ] ; then rm -f %s ; fi\n", $finaleImage, $finaleImage;
    $code .= sprintf "if [ ! -d \"%s\" ] ; then mkdir -p %s ; fi\n", dirname($finaleImage), dirname($finaleImage);

    $code .= sprintf "tiff2tile \${TMP_DIR}/result.tif -c %s -p %s -t 256 256 -b %s -a uint -s %s  %s\n%s",
        $compression,
        $this{pyramid}->{pixel}->{photometric},
        $this{pyramid}->{pixel}->{bitspersample},
        $this{pyramid}->{pixel}->{samplesperpixel},
        $finaleImage,RESULT_TEST;

    # Cleaning
    $code .= " rm -f \${TMP_DIR}/* \n\n";

    $this{scripts}[$this{currentscript}] .= $code;

    $this{currentscript} = ($this{currentscript}+1)%($this{process}->{job_number});
    
    return TRUE;

}

sub makeLink {
    my $finaleImage = shift;
    my $baseImage = shift;

    TRACE();

    #create folders
    eval { mkpath(dirname($finaleImage),0,0751); };
    if ($@) {
        ERROR(sprintf "Can not create the cache directory '%s' : %s !",dirname($finaleImage), $@);
        return FALSE;
    }

    my $follow_relfile = undef;

    if (-f $baseImage && ! -l $baseImage) {
        $follow_relfile = File::Spec->abs2rel($baseImage,dirname($finaleImage));
    }
    elsif (-f $baseImage && -l $baseImage) {
        my $linked   = File::Spec::Link->linked($baseImage);
        my $realname = File::Spec::Link->full_resolve($linked);
        $follow_relfile = File::Spec->abs2rel($realname, dirname($finaleImage));
    } else {
        ERROR(sprintf "The tile '%s' is not a file or a link in '%s' !",basename($baseImage),dirname($baseImage));
        return FALSE;  
    }

    if (! defined $follow_relfile) {
        ERROR (sprintf "The link '%s' can not be resolved in '%s' ?",
                    basename($baseImage),
                    dirname($baseImage));
        return FALSE;
    }

    my $result = eval { symlink ($follow_relfile, $finaleImage); };
    if (! $result) {
        ERROR (sprintf "The tile '%s' can not be linked to '%s' (%s) ?",
                    $follow_relfile,$finaleImage,$!);
        return FALSE;
    }

    return TRUE;

}

sub transformImage {
    my $finaleImage = shift;
    my $baseImage = shift;
    my $isPNG = shift;

    TRACE();

    my $code = '';

    # Pretreatment

    if ($isPNG) {
        $code .= sprintf "untile %s \${TMP_DIR}/\n%s",$$baseImage,RESULT_TEST;
        $code .= sprintf "montage -geometry 256x256 -tile 16x16 \${TMP_DIR}/*.png -depth 8 -background none -define tiff:rows-per-strip=4096 \${TMP_DIR}/PNG_untiled.tif\n%s",RESULT_TEST;
        $code .= sprintf "tiff2rgba -c none \${TMP_DIR}/PNG_untiled.tif \${TMP_DIR}/img.tif\n%s",RESULT_TEST;
    } else {
        $code .= sprintf "tiff2rgba -c none %s \${TMP_DIR}/img.tif\n%s",$baseImage,RESULT_TEST;
    }

    my $compression = $this{pyramid}->{compression};
    $compression = ($compression eq 'raw'?'none':$compression);
    $compression = ($compression eq 'jpg'?'jpeg':$compression);

    # Final location writting
    $code .= sprintf "if [ -r \"%s\" ] ; then rm -f %s ; fi\n", $finaleImage, $finaleImage;
    $code .= sprintf "if [ ! -d \"%s\" ] ; then mkdir -p %s ; fi\n", dirname($finaleImage), dirname($finaleImage);

    $code .= sprintf "tiff2tile \${TMP_DIR}/result.tif -c %s -p %s -t 256 256 -b %s -a uint -s %s  %s\n%s",
        $compression,
        $this{pyramid}->{pixel}->{photometric},
        $this{pyramid}->{pixel}->{bitspersample},
        $this{pyramid}->{pixel}->{samplesperpixel},
        $finaleImage,RESULT_TEST;

    # Cleaning
    $code .= " rm -f \${TMP_DIR}/* \n\n";

    $this{scripts}[$this{currentscript}] .= $code;

    $this{currentscript} = ($this{currentscript}+1)%($this{process}->{job_number});
    
    return TRUE;

}

# method: getCacheNameOfImage
#  return the image path : /3E/42/01.tif
#---------------------------------------------------------------------------------------------------
sub getCacheNameOfImage {
    my $i     = shift; # X idx !
    my $j     = shift; # Y idx !

    my $ib36 = main::encodeIntToB36($i);
    my $jb36 = main::encodeIntToB36($j);

    my @icut = split (//, $ib36);
    my @jcut = split (//, $jb36);

    if (scalar(@icut) != scalar(@jcut)) {
        if (length ($ib36) > length ($jb36)) {
            $jb36 = "0"x(length ($ib36) - length ($jb36)).$jb36;
            @jcut = split (//, $jb36);
        } else {
            $ib36 = "0"x(length ($jb36) - length ($ib36)).$ib36;
            @icut = split (//, $ib36);
        }
    }

    my $padlength = $this{pyramid}->{dirDepth} + 1;
    my $size      = scalar(@icut);
    my $pos       = $size;
    my @l;

    for(my $i=0; $i<$padlength;$i++) {
        $pos--;
        push @l, $jcut[$pos];
        push @l, $icut[$pos];
        push @l, '/';
    }

    pop @l;

    if ($size>$padlength) {
        while ($pos) {
            $pos--;
            push @l, $jcut[$pos];
            push @l, $icut[$pos];
        }
    }

    my $imagePath = scalar reverse(@l);
    my $imagePathName = join('.', $imagePath, 'tif');

    return File::Spec->catfile('/'.$imagePathName); 
}

sub encodeIntToB36 {
    my $number= shift; # idx !

    my $padlength = $this{pyramid}->{dirDepth} + 1;

    my $b36 = "";
    $b36 = "000" if $number == 0;

    while ( $number ) {
        my $v = $number % 36;
        if($v <= 9) {
            $b36 .= $v;
        } else {
            $b36 .= chr(55 + $v); # Assume that 'A' is 65
        }
        $number = int $number / 36;
    }

    # fill with 0 !
    $b36 = "0"x($padlength - length $b36).reverse($b36);

    return $b36;
}

sub encodeB36toInt {
    my $b36  = shift; # idx in base 36 !

    my $padlength = $this{pyramid}->{dirDepth} + 1;

    my $number = 0;
    my $i = 0;
    foreach(split //, reverse uc $b36) {
        $_ = ord($_) - 55 unless /\d/; # Assume that 'A' is 65
        $number += $_ * (36 ** $i++);
    }

    # fill with 0 !
    return "0"x($padlength - length $number).$number;
    # return decode_base36($b36,$padlength);
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

[bboxes]
WLD = -20037508.3,-20037508.3,20037508.3,20037508.3
FXX = -518548.8,5107913.5,978393,6614964.6
REU = 6140645.1,-2433358.9,6224420.1,-2349936.0
GUF = -6137587.6,210200.4,-5667958.5,692618.0

[ process ]
path_shell  = /home/theo/TEST/BE4/SCRIPT
path_temp   = /home/theo/TEST/BE4/TMP
job_number  = 3

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

Théo Satabin, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright 2011 by SIEL/TLGE/Théo Satabin - Institut Géographique National

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut
