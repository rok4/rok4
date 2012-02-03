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

package BE4::Pyramid;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use XML::LibXML;

use File::Spec::Link;
use File::Basename;
use File::Spec;
use File::Path;

use Data::Dumper;

# My module
use BE4::Product;
use BE4::TileMatrixSet;
use BE4::Format;
use BE4::Level;
use BE4::NoData;

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
use constant CREATE_NODATA     => "createNodata";

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

################################################################################
# properties :
#
#    [ pyramid ]
#
#    pyr_desc_path      =
#    pyr_desc_path_old  =
#    pyr_data_path      =
#    pyr_data_path_old  =
#    ; pyr_schema_path = 
#    ; pyr_schema_name =
#    
#    pyr_level_bottom =
#    pyr_level_top    =
#    pyr_name_old     =
#    pyr_name_new     =
#
#    ; eg section [ tilematrixset ]
#    tms_name     =  
#    tms_path     =
#    tms_level_min=
#    tms_level_max=
#    ; tms_schema_path = 
#    ; tms_schema_name =
#
#    image_width  = 
#    image_height =
#
#    compression  =
#    gamma        =
#
#    ; eg section [ tile ]
#    bitspersample       = 
#    sampleformat        = 
#    photometric         = 
#    samplesperpixel     =
#    interpolation       = 
#    
#    dir_depth    =
#    dir_image    = IMAGE
#    dir_nodata    = NODATA
#    dir_metadata = METADATA
#
#    ; eg section [ nodata ]
#    path_nodata =
#    imagesize   =
#    color       =

################################################################################
# Global template Pyr

my $STRPYRTMPLT   = <<"TPYR";
<?xml version='1.0' encoding='US-ASCII'?>
<Pyramid>
    <tileMatrixSet>__TMSNAME__</tileMatrixSet>
    <format>__FORMATIMG__</format>
    <channels>__CHANNEL__</channels>
    <nodataValue>__NODATAVALUE__</nodataValue>
    <interpolation>__INTERPOLATION__</interpolation>
    <photometric>__PHOTOMETRIC__</photometric>
<!-- __LEVELS__ -->
</Pyramid>
TPYR

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
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    #
    # WARNING !
    #
    # 2 options possible with parameters :
    #   - a new pyramid configuration
    #   - a existing pyramid configuration
    #
    # > in a HASH entry only (no ref) !
    #
    #
    # the choice is on the parameter 'pyr_name_old'
    #   1) if param is null, it's a new pyramid only !
    #   2) if param is not null, it's an existing pyramid !
    #
    
    # IN
    #  it's all possible parameters !
    pyramid   => { 
                    pyr_name_new      => undef, # string name
                    pyr_name_old      => undef, # string name
                    pyr_desc_path     => undef, # path
                    pyr_desc_path_old => undef, # path
                    pyr_data_path     => undef, # path
                    pyr_data_path_old => undef, # path
                    pyr_level_bottom  => undef, # number
                    pyr_level_top     => undef, # number
                    #
                    tms_name     => undef, # string name
                    tms_path     => undef, # path
                    tms_level_min=> undef, # number
                    tms_level_max=> undef, # number
                    #
                    compression  => undef, # string value ie raw by default !
                    gamma        => undef, # number ie 1 by default !
                    #
                    dir_depth    => undef, # number
                    dir_image    => undef, # dir name
                    dir_nodata    => undef, # dir name
                    dir_metadata => undef, # dir name
                    image_width  => undef, # number
                    image_height => undef, # number
                    #
                    bitspersample           => undef,# number
                    sampleformat            => undef,# number
                    photometric             => undef,# string value ie rgb by default !
                    samplesperpixel         => undef,# number
                    interpolation           => undef,# string value ie bicubic by default !
                    #
                    path_nodata     => undef, # path
                    imagesize       => undef, # number ie 4096 px by default !
                    color           => undef, # string value ie FFFFFF by default !
                 },
    # OUT
    tile       => undef,   # it's an object !
    tms        => undef,   # it's an object !
    nodata     => undef,   # it's an object !
    format     => undef,   # it's an object !
    level      => [],      # it's a table of object level !
    cache_tile => [],      # ie tile image to link  !
    cache_dir  => [],      # ie dir to search !
    #
    dataLimits => {      # data's limits, in the pyramid's SRS
                    xmin => undef,
                    ymin => undef,
                    xmax => undef,
                    ymax => undef,
                }, 
    isnewpyramid => 1,     # new pyramid by default !
    
   };

  bless($self, $class);
  
  TRACE;
  
  # init. parameters 
  return undef if (! $self->_init(@_));
  
  # init. :
  # a new pyramid or from existing pyramid !
  return undef if (! $self->_load());
  
  return $self;
}

################################################################################
# privates init.

# TODO
#  - no test for path and type (string, number, ...) !

sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;

    if (! defined $params ) {
        ERROR ("paramters argument required (null) !");
        return FALSE;
    }
    
    # init. params .
    $self->{isnewpyramid} = 0 if (defined $params->{pyr_name_old});
    
    my $pyr = $self->{pyramid};
    
    if ($self->{isnewpyramid}) {
        # To a new pyramid, you must have to this parameters !
        #
        # you can choice this option by default !
        if (! exists($params->{compression})) {
            WARN ("Optional parameter 'compression' is not set. The default value is raw");
            $params->{compression} = 'raw';
        }
        $pyr->{compression}  = $params->{compression};
        #
        $pyr->{image_width}  = $params->{image_width}  || ( ERROR ("The parameter 'image_width' is required!") && return FALSE );
        $pyr->{image_height} = $params->{image_height} || ( ERROR ("The parameter 'image_height' is required!") && return FALSE );
        #
        $pyr->{tms_name}     = $params->{tms_name} || ( ERROR ("The parameter 'tms_name' is required!") && return FALSE );
        #
        $pyr->{bitspersample}    = $params->{bitspersample}     || ( ERROR ("The parameter 'bitspersample' is required!") && return FALSE );
        $pyr->{sampleformat}     = $params->{sampleformat}      || ( ERROR ("The parameter 'sampleformat' is required!") && return FALSE );
        $pyr->{samplesperpixel}  = $params->{samplesperpixel}   || ( ERROR ("The parameter 'samplesperpixel' is required!") && return FALSE );
       
    }
    else {
        # To an existing pyramid, you must have to this parameters !
        #
        $pyr->{pyr_name_old} = $params->{pyr_name_old} || ( ERROR ("The parameter 'pyr_name_old' is required!") && return FALSE );
   
        #
        # this option can be determined !
        if (exists($params->{tms_name})) {
          WARN ("The parameter 'tms_name' must not be set if pyr_name_old is set too ! Il will be ignore.");
        }
        $params->{tms_name} = undef;
        #
        if (exists($params->{compression})) {
          WARN ("Parameter 'compression' must not be set if pyr_name_old is set too ! Il will be ignore.");
        }
        $params->{compression} = undef;
    }
    #
    # All parameters are mandatory (or initializate by default) whatever the pyramid !
    # 
    $pyr->{pyr_name_new}  = $params->{pyr_name_new}  || ( ERROR ("Parameter 'pyr_name_new' is required!") && return FALSE );
    $pyr->{pyr_desc_path} = $params->{pyr_desc_path} || ( ERROR ("Parameter 'pyr_desc_path' is required!") && return FALSE );
    $pyr->{pyr_data_path} = $params->{pyr_data_path} || ( ERROR ("Parameter 'pyr_data_path' is required!") && return FALSE );
    #
    $pyr->{tms_path}     = $params->{tms_path}     || ( ERROR ("Parameter 'tms_path' is required!") && return FALSE );
    #
    $pyr->{dir_depth}    = $params->{dir_depth}    || ( ERROR ("Parameter 'dir_depth' is required!") && return FALSE );
    #
    $pyr->{path_nodata}  = $params->{path_nodata}  || ( ERROR ("Parameter to 'path_nodata' is required!") && return FALSE );
    #
    
    # this option are optional !
    #
    if (exists($params->{dir_metadata})) {
        WARN ("Parameter 'dir_metadata' is not implemented yet! It will be ignore");
    }
    $params->{dir_metadata} = undef;
    #
    if (! exists($params->{pyr_level_bottom})) {
        WARN ("Parameter 'pyr_level_bottom' has not been set. The default value is undef, then the min level will be calculated with source images resolution");
        $params->{pyr_level_bottom} = undef;
    }
    $pyr->{pyr_level_bottom} = $params->{pyr_level_bottom};
    #
    if (! exists($params->{pyr_level_top})) {
        WARN ("Parameter 'pyr_level_top' has not been set. The defaut value is the top of the TMS' !");
        $params->{pyr_level_top} = undef;
    }
    $pyr->{pyr_level_top} = $params->{pyr_level_top};
    
    # 
    # you can choice this option with value by default !
    #
    # NV: FIXME: il ne doit pas etre possible de definir une taille differente de celle des dalles du cache.
    #            il ne faut donc pas que ce soit un parametre.
    if (! exists($params->{imagesize})) {
        WARN ("Parameter 'nodata.imagesize' has not been set. The default value is 4096 px");
        $params->{imagesize} = '4096';
    }
    $pyr->{imagesize} = $params->{imagesize};
    # 
    # NV: FIXME: the default value should depend on the sample type.
    # NV: FIXME: color should be named nodata_color.
    if (! exists($params->{color})) {
        WARN ("Parameter 'nodata.color' has not been set. The default value is 'FFFFFF'");
        $params->{color} = 'FFFFFF';
    }
    $pyr->{color} = $params->{color};
    #
    if (! exists($params->{interpolation})) {
        WARN ("Parameter 'interpolation' has not been set. The default value is 'bicubic'");
        $params->{interpolation} = 'bicubic';
    }
    # to remove when interpolation 'bicubique' will be remove
    if ($params->{interpolation} eq 'bicubique') {
        WARN("'bicubique' is a deprecated interpolation name, use 'bicubic' instead");
        $params->{interpolation} = 'bicubic';
    }
    $pyr->{interpolation} = $params->{interpolation};
    #
    if (! exists($params->{photometric})) {
        WARN ("Parameter 'photometric' has not been set. The default value is 'rgb'");
        $params->{photometric} = 'rgb';
    }
    $pyr->{photometric} = $params->{photometric};
    #
    if (! exists($params->{gamma})) {
        WARN ("Parameter 'gamma' has not been set. The default value is 1 (no effect)");
        $params->{gamma} = 1;
    }
    $pyr->{gamma} = $params->{gamma};
    #
    
    if (! exists($params->{dir_image})) {
        WARN ("Parameter 'dir_image' has not been set. The default value is 'IMAGE'");
        $params->{dir_image} = 'IMAGE';
    }
    $pyr->{dir_image} = $params->{dir_image};
    #
    if (! exists($params->{dir_nodata})) {
        WARN ("Parameter 'dir_nodata' has not been set. The default value is 'NODATA'");
        $params->{dir_nodata} = 'NODATA';
    }
    $pyr->{dir_nodata} = $params->{dir_nodata};
    #
    if (! exists($params->{pyr_desc_path_old})) {
        WARN ("Parameter 'pyr_desc_path_old' has not been set. The default value is the same as 'pyr_desc_path'");
        $params->{pyr_desc_path_old} = $params->{pyr_desc_path};
    }
    $pyr->{pyr_desc_path_old} = $params->{pyr_desc_path_old};
    #
    if (! exists($params->{pyr_data_path_old})) {
        WARN ("Parameter 'pyr_data_path_old' has not been set. The default value is the same as 'pyr_data_path'.");
        $params->{pyr_data_path_old} = $params->{pyr_data_path};
    }
    $pyr->{pyr_data_path_old} = $params->{pyr_data_path_old};
    #
    # TODO path !
    if (! -d $pyr->{path_nodata}) {}
    if (! -d $pyr->{pyr_desc_path}) {}
    if (! -d $pyr->{pyr_desc_path_old}) {}
    if (! -d $pyr->{tms_path}) {}
    if (! -d $pyr->{pyr_data_path}) {}
    if (! -d $pyr->{pyr_data_path_old}) {}
    
    return TRUE;
}

sub _load {
  my $self = shift;

  TRACE;
  
  if ($self->{isnewpyramid}) {
    
    # It's a new pyramid !
    
    # create Tile !
    my $objTile = BE4::Product->new({
        bitspersample    => $self->{pyramid}->{bitspersample},
        sampleformat     => $self->{pyramid}->{sampleformat},
        photometric      => $self->{pyramid}->{photometric},
        samplesperpixel  => $self->{pyramid}->{samplesperpixel},
        interpolation    => $self->{pyramid}->{interpolation},
    });
    
    if (! defined $objTile) {
      ERROR ("Can not load tile !");
      return FALSE;
    }
    
    $self->{tile} = $objTile;
    DEBUG (sprintf "TILE = %s", Dumper($objTile));
    
    # create Format !
    my $objFormat = BE4::Format->new($self->{pyramid}->{compression},$self->{pyramid}->{sampleformat},$self->{pyramid}->{bitspersample});
    
    if (! defined $objFormat) {
      ERROR ("Can not load format !");
      return FALSE;
    }
    
    $self->{format} = $objFormat;
    DEBUG (sprintf "FORMAT = %s", Dumper($objFormat));
    
    # create TileMatrixSet !
    my $objTMS = BE4::TileMatrixSet->new(File::Spec->catfile($self->{pyramid}->{tms_path},
                                                             $self->{pyramid}->{tms_name}),
                                                             $self->{pyramid}->{tms_level_min},
                                                             $self->{pyramid}->{tms_level_max});

    
    if (! defined $objTMS) {
      ERROR ("Can not load TMS !");
      return FALSE;
    }
    
    # save tms' extrema if doesn't exist !
    if (! defined ($self->{pyramid}->{tms_level_min})) {
        $self->{pyramid}->{tms_level_min} = $objTMS->{levelmin};
    }
    if (! defined ($self->{pyramid}->{tms_level_max})) {
        $self->{pyramid}->{tms_level_max} = $objTMS->{levelmax};
    }
    
    $self->{tms} = $objTMS;
    DEBUG (sprintf "TMS = %s", Dumper($objTMS));
    
    # init. method has checked all parameters,
    # so we can only create all level...
    
    return FALSE if (! $self->_fillToPyramid());
  }
  else 
  {
    # a new pyramid from existing pyramid !
    #
    # init. process hasn't checked all parameters,
    # so, we must read file pyramid to initialyze them...
    
    return FALSE if (! $self->_fillFromPyramid());
  }

    # create NoData !
    my $objNodata = BE4::NoData->new({
            path_nodata      => $self->{pyramid}->{path_nodata},
            bitspersample    => $self->getTile()->getBitsPerSample(),
            sampleformat     => $self->getTile()->getSampleFormat(),
            photometric      => $self->getTile()->getPhotometric(),
            samplesperpixel  => $self->getTile()->getSamplesPerPixel(),
            imagesize        => $self->{pyramid}->{imagesize}, 
            color            => $self->{pyramid}->{color}
    });

    if (! defined $objNodata) {
    ERROR ("Can not load NoData !");
    return FALSE;
    }

    $self->{nodata} = $objNodata;
    DEBUG (sprintf "NODATA = %s", Dumper($objNodata));

    return TRUE;
  
}

# method: createNodata
#  create command to create a nodata tile with same parameters as images.
#---------------------------------------------------------------------------------------------------
sub createNodata {
    my $self = shift;
    my $nodataFilePath = shift;
    
    my $sizex = int($self->{pyramid}->{imagesize}) / int($self->{pyramid}->{image_width});
    my $sizey = int($self->{pyramid}->{imagesize}) / int($self->{pyramid}->{image_height});
    
    my $cmd = sprintf ("%s -n %s",CREATE_NODATA, $self->{pyramid}->{color});
    $cmd .= sprintf ( " -c %s", $self->{pyramid}->{compression});
    $cmd .= sprintf ( " -p %s", $self->{pyramid}->{photometric});
    $cmd .= sprintf ( " -t %s %s", $sizex, $sizey);
    $cmd .= sprintf ( " -b %s", $self->{pyramid}->{bitspersample});
    $cmd .= sprintf ( " -s %s", $self->{pyramid}->{samplesperpixel});
    $cmd .= sprintf ( " -a %s", $self->{pyramid}->{sampleformat});
    $cmd .= sprintf ( " %s", $nodataFilePath);
    return $cmd;
    
}


sub _fillToPyramid { 
  my $self  = shift;

  TRACE;

  # get tms object
  my $objTMS = $self->getTileMatrixSet();
  
  if (! defined $objTMS) {
    ERROR("Object TMS not defined !");
    return FALSE;
  }
  
  # load all level
  my $i = ($objTMS->getFirstTileMatrix())->getID();
  while(defined (my $objTm = $objTMS->getNextTileMatrix($i))) {
    
    my $tileperwidth     = $self->getTilePerWidth(); 
    my $tileperheight    = $self->getTilePerHeight();
    
    # base dir image
    my $baseimage = File::Spec->catdir($self->getPyrDataPath(),  # all directories structure of pyramid ! 
                                  $self->getPyrName(),
                                  $self->getDirImage(),
                                  $objTm->getID()               # FIXME : level = id !
                                  );
                                  
    # base dir nodata
    my $basenodata = File::Spec->catdir($self->getPyrDataPath(),  # all directories structure of pyramid ! 
                                  $self->getPyrName(),
                                  $self->getDirNodata(),
                                  $objTm->getID()               # FIXME : level = id !
                                  );
                                  
    # TODO : metadata
    #   compression, type ...
    my $basemetadata = File::Spec->catdir($self->getPyrDataPath(),  # all directories structure of pyramid ! 
                                  $self->getPyrName(),
                                  $self->getDirMetadata(),
                                  $objTm->getID()                  # FIXME : level = id !
                                  );
    
    # FIXME :
    #   compute tms limit in row/col from TMS ?
    
    # params to level
    my $params = {
            id                => $objTm->getID(),
            dir_image         => File::Spec->abs2rel($baseimage, $self->getPyrDescPath()), # FIXME rel with the pyr path !
            compress_image    => $self->getFormat()->getCode(), # ie TIFF_RAW_INT8 !
            dir_nodata        => File::Spec->abs2rel($basenodata, $self->getPyrDescPath()), # FIXME rel with the pyr path !
            dir_metadata      => undef,           # TODO,
            compress_metadata => undef,           # TODO  : raw  => TIFF_RAW_INT8,
            type_metadata     => "INT32_DB_LZW",  # FIXME : type => INT32_DB_LZW, 
            bitspersample     => $self->getTile()->getBitsPerSample(),
            samplesperpixel   => $self->getTile()->getSamplesPerPixel(),
            size              => [ $tileperwidth, $tileperheight],
            dir_depth         => $self->getDirDepth(),
            limit             => [undef, undef, undef, undef] # FIXME : can be computed or fix ?
    };
    my $objLevel = BE4::Level->new($params);
    
    if(! defined  $objLevel) {
      ERROR (sprintf "Can not create the level '%s' !", $objTm->getID());
      return FALSE;
    }
    push @{$self->{level}}, $objLevel;
    # push dir to create : just directories for nodata. Directories for image will be created during script execution
    # push @{$self->{cache_dir}}, $basenodata; #absolute path
    # push @{$self->{cache_dir}}, $baseimage, $basenodata; #absolute path
    # push @{$self->{cache_dir}}, File::Spec->abs2rel($baseimage, $self->getPyrDataPath());
    $i++;
  }
  
  if (! scalar (@{$self->{level}})) {
    ERROR ("No level loaded !");
    return FALSE;
  }
  
  return TRUE;
}
sub _fillFromPyramid {
  my $self  = shift;
  
  TRACE;

  my $filepyramid =  File::Spec->catfile($self->getPyrDescPathOld(),
                                         $self->getPyrFileOld());
  
  if (! $self->readConfPyramid($filepyramid)) {
    ERROR (sprintf "Can not read the XML file Pyramid : %s !", $filepyramid);
    return FALSE;
  }
  
  
  my $cachepyramid = File::Spec->catdir($self->getPyrDataPathOld(),
                                        $self->getPyrNameOld());
  
  if (! $self->readCachePyramid($cachepyramid)) {
    ERROR (sprintf "Can not read the Directory Cache Pyramid : %s !", $cachepyramid);
    return FALSE;
  }
  
  return TRUE;
}
################################################################################
# public method serialization
#  Manipulate the Configuration File Pyramid /* in/out */

sub writeConfPyramid {
    my $self    = shift;
    my $filepyramid = shift; # Can be null !

    TRACE;

    # to write TM limits in the pyramid descriptor
    if (! $self->calculateTMLimits) {
      ERROR ("Can not calculate TM limits !");
      return FALSE;
    }
    
    # parsing template
    my $parser = XML::LibXML->new();

    my $doctpl = eval { $parser->parse_string($STRPYRTMPLT); };
    if (!defined($doctpl) || $@) {
        ERROR(sprintf "Can not parse template file of pyramid : %s !", $@);
        return FALSE;
    }
    my $strpyrtmplt = $doctpl->toString(0);
  
    #
    my $tmsname = $self->getTmsName();
    $strpyrtmplt =~ s/__TMSNAME__/$tmsname/;
    #
    my $formatimg = $self->getFormat()->getCode(); # ie TIFF_RAW_INT8 !
    $strpyrtmplt  =~ s/__FORMATIMG__/$formatimg/;
    #  
    my $channel = $self->getTile()->getSamplesPerPixel();
    $strpyrtmplt =~ s/__CHANNEL__/$channel/;
    #  
    my $nodata = $self->{pyramid}->{color};
    $strpyrtmplt =~ s/__NODATAVALUE__/$nodata/;
    #  
    my $interpolation = $self->{pyramid}->{interpolation};
    $strpyrtmplt =~ s/__INTERPOLATION__/$interpolation/;
    #  
    my $photometric = $self->{pyramid}->{photometric};
    $strpyrtmplt =~ s/__PHOTOMETRIC__/$photometric/;

    my @levels = $self->getLevels();
    foreach my $objLevel (@levels){
        if ($objLevel->{id} >= $self->{pyramid}->{pyr_level_top} && $objLevel->{id} <= $self->{pyramid}->{pyr_level_bottom}) {
        
            # image
            $strpyrtmplt =~ s/<!-- __LEVELS__ -->\n/$STRLEVELTMPLT/;
            
            my $id       = $objLevel->{id};
            $strpyrtmplt =~ s/__ID__/$id/;
        
            my $dirimg   = $objLevel->{dir_image};
            $strpyrtmplt =~ s/__DIRIMG__/$dirimg/;
            
            my $dirnd   = $objLevel->{dir_nodata};
            my $pathnd = $dirnd."/nd.tiff";
            $strpyrtmplt =~ s/__NODATAPATH__/$pathnd/;
            
            my $tilew    = $objLevel->{size}->[0];
            $strpyrtmplt =~ s/__TILEW__/$tilew/;
            my $tileh    = $objLevel->{size}->[1];
            $strpyrtmplt =~ s/__TILEH__/$tileh/;
            
            my $depth    =  $objLevel->{dir_depth};
            $strpyrtmplt =~ s/__DEPTH__/$depth/;
            
            my $minrow   =  $objLevel->{limit}->[0];
            $strpyrtmplt =~ s/__MINROW__/$minrow/;
            my $maxrow   =  $objLevel->{limit}->[1];
            $strpyrtmplt =~ s/__MAXROW__/$maxrow/;
            my $mincol   =  $objLevel->{limit}->[2];
            $strpyrtmplt =~ s/__MINCOL__/$mincol/;
            my $maxcol   =  $objLevel->{limit}->[3];
            $strpyrtmplt =~ s/__MAXCOL__/$maxcol/;
        
            # metadata
            if (defined $objLevel->{dir_metadata}) {

                $strpyrtmplt =~ s/<!-- __MTD__ -->/$STRLEVELTMPLTMORE/;

                my $dirmtd   = $objLevel->{dir_metadata};
                $strpyrtmplt =~ s/__DIRMTD__/$dirmtd/;

                my $formatmtd = $objLevel->{compress_metadata};
                $strpyrtmplt  =~ s/__FORMATMTD__/$formatmtd/;
            }
            $strpyrtmplt =~ s/<!-- __MTD__ -->\n//;
            
        }
    }
    #
    $strpyrtmplt =~ s/<!-- __LEVELS__ -->\n//;
    $strpyrtmplt =~ s/^$//g;
    $strpyrtmplt =~ s/^\n$//g;
    #

    # TODO check the new template !
  
    if (! defined $filepyramid) {
        $filepyramid = File::Spec->catfile($self->getPyrDescPath(),$self->getPyrFile());
    }

    if (-f $filepyramid) {
        ERROR(sprintf "File Pyramid ('%s') exist, can not overwrite it ! ", $filepyramid);
        return FALSE;
    }
    #
    my $PYRAMID;

    if (! open $PYRAMID, ">", $filepyramid) {
        ERROR("");
        return FALSE;
    }
    #
    printf $PYRAMID "%s", $strpyrtmplt;
    #
    close $PYRAMID;

    return TRUE;
}

sub readConfPyramid {
    my $self    = shift;
    my $filepyramid = shift; # Can be null !

    TRACE;

    if (! defined $filepyramid) {
        $filepyramid = File::Spec->catfile($self->getPyrDescPathOld(),
                                       $self->getPyrFileOld());
    }

    if (! -f $filepyramid) {
        ERROR (sprintf "Can not find the XML file Pyramid : %s !", $filepyramid);
        return FALSE;
    }

    # read xml pyramid
    my $parser  = XML::LibXML->new();
    my $xmltree =  eval { $parser->parse_file($filepyramid); };

    if (! defined ($xmltree) || $@) {
        ERROR (sprintf "Can not read the XML file Pyramid : %s !", $@);
        return FALSE;
    }

    my $root   = $xmltree->getDocumentElement;

    # read tag value of tileMatrixSet, format and channel

    my $tagtmsname = $root->findnodes('tileMatrixSet')->to_literal;

    if ($tagtmsname eq '') {
        ERROR (sprintf "Can not determine parameter 'tileMatrixSet' in the XML file Pyramid !");
        return FALSE;
    }

    my $tagformat = $root->findnodes('format')->to_literal;

    if ($tagtmsname eq '') {
        ERROR (sprintf "Can not determine parameter 'format' in the XML file Pyramid !");
        return FALSE;
    }
    
    my $tagnodata = $root->findnodes('nodataValue')->to_literal;

    if ($tagnodata eq '') {
        WARN (sprintf "Can not determine parameter 'nodata' in the XML file Pyramid ! Value from parameters kept");
    } else {
        INFO (sprintf "Nodata value ('%s') in the XML file Pyramid is used",$tagnodata);
        $self->{pyramid}->{color} = $tagnodata;
    }
    
    my $tagphotometric = $root->findnodes('photometric')->to_literal;

    if ($tagphotometric eq '') {
        WARN (sprintf "Can not determine parameter 'photometric' in the XML file Pyramid ! Value from parameters kept");
    } else {
        INFO (sprintf "Photometric value ('%s') in the XML file Pyramid is used",$tagphotometric);
        $self->{pyramid}->{photometric} = $tagphotometric;
    }
    
    my $taginterpolation = $root->findnodes('interpolation')->to_literal;

    if ($taginterpolation eq '') {
        WARN (sprintf "Can not determine parameter 'interpolation' in the XML file Pyramid ! Value from parameters kept");
    } else {
        INFO (sprintf "Interpolation value ('%s') in the XML file Pyramid is used",$taginterpolation);
        $self->{pyramid}->{interpolation} = $taginterpolation;
    }
    
    # to remove when interpolation 'bicubique' will be remove
    if ($taginterpolation eq 'bicubique') {
        WARN("'bicubique' is a deprecated interpolation name, use 'bicubic' instead");
        $taginterpolation = 'bicubic';
    }

#   to remove when format 'TIFF_INT8' and 'TIFF_FLOAT32' will be remove
    if ($tagformat eq 'TIFF_INT8') {
        WARN("'TIFF_INT8' is a deprecated format, use 'TIFF_RAW_INT8' instead");
        $tagformat = 'TIFF_RAW_INT8';
    }
    if ($tagformat eq 'TIFF_FLOAT32') {
        WARN("'TIFF_FLOAT32' is a deprecated format, use 'TIFF_RAW_FLOAT32' instead");
        $tagformat = 'TIFF_RAW_FLOAT32';
    }
  
    my $tagsamplesperpixel = $root->findnodes('channels')->to_literal;

    if ($tagsamplesperpixel eq '') {
    ERROR (sprintf "Can not determine parameter 'channels' in the XML file Pyramid !");
    return FALSE;
    }
    

    # create a object tileMatrixSet

    my $tmsname = $self->getTmsName();
    if (! defined $tmsname) {
    WARN ("Null parameter for the name of TMS, so extracting from file pyramid !");
    $tmsname = $tagtmsname;
    }

    if ($tmsname ne $tagtmsname) {
    WARN ("Selecting the name of TMS in the file of the pyramid !");
    $tmsname = $tagtmsname;
    }

    my $tmsfile = join(".", $tmsname, "tms"); 
    my $objTMS  = BE4::TileMatrixSet->new(File::Spec->catfile($self->getTmsPath(), $tmsfile),
                                         $self->{pyramid}->{tms_level_min},
                                         $self->{pyramid}->{tms_level_max});

    if (! defined $objTMS) {
    ERROR ("Can not create object TileMatrixSet !");
    return FALSE;
    }

    # save it if doesn't exist !
    if (! defined ($self->getTileMatrixSet())) {
    $self->{tms} = $objTMS;
    }
    
    # save tms' extrema if doesn't exist !
    if (! defined ($self->{pyramid}->{tms_level_min})) {
        $self->{pyramid}->{tms_level_min} = $objTMS->{levelmin};
    }
    if (! defined ($self->{pyramid}->{tms_level_max})) {
        $self->{pyramid}->{tms_level_max} = $objTMS->{levelmax};
    }

    # fill parameters if not... !
    $self->{pyramid}->{tms_name} = $self->getTileMatrixSet()->getFile();
    $self->{pyramid}->{tms_path} = $self->getTileMatrixSet()->getPath();
    $self->{pyramid}->{samplesperpixel} = $tagsamplesperpixel;

    # create tile and format objects

    # ie TIFF, compression, sampleformat, bitspersample !
    # return compression = raw, jpg or png !
    my ($formatimg, $compression, $sampleformat, $bitspersample) = BE4::Format->decodeFormat($tagformat);
    
    # create format
    my $objFormat = BE4::Format->new($compression, $sampleformat, $bitspersample);

    if (! defined $objFormat) {
        ERROR ("Can not create the Format object !");
        return FALSE;
    }

    # save it if doesn't exist !
    if (! defined ($self->getFormat())) {
        $self->{format} = $objFormat;
    }
    
    # check format
    if ($self->getFormat()->getCode() ne $tagformat) {
        ERROR (sprintf "The mode compression is different between configuration ('%s') and pyramid file ('%s') !",
                    $self->getFormat()->getCode(),
                    $tagformat);
        return FALSE;
    }
    
    $self->{pyramid}->{sampleformat} = $sampleformat;
    $self->{pyramid}->{bitspersample} = $bitspersample;
    $self->{pyramid}->{compression} = $compression;
    
    # create tile
    my $tile = {
        bitspersample    => $bitspersample,
        sampleformat     => $sampleformat,
        photometric      => $self->getPhotometric(),
        samplesperpixel  => $tagsamplesperpixel,
        interpolation    => $self->getInterpolation(),
    };

    my $objTile = BE4::Product->new($tile);

    if (! defined $objTile) {
    ERROR ("Can not create the Tile format !");
    return FALSE;
    }

    # save it if doesn't exist !
    if (! defined ($self->getTile())) {
    $self->{tile} = $objTile;
    }

    # check format
    if ($self->getFormat()->getCode() ne $tagformat) {
        ERROR (sprintf "The mode compression is different between configuration ('%s') and pyramid file ('%s') !",
                    $self->getFormat()->getCode(),
                    $tagformat);
        return FALSE;
    }

    # load pyramid level
    my @levels = $root->getElementsByTagName('level');
    
    # read image directory name in the old pyramid, using a level
    my $level = $levels[0];
    my @directories = File::Spec->splitdir($level->findvalue('baseDir'));
    # <baseDir> : rel_datapath_from_desc/dir_image/level
    #                                       -2      -1
    my $dir_image = $directories[scalar(@directories)-2];
    $self->{pyramid}->{dir_image} = $dir_image;
    
    # read nodata directory name in the old pyramid, using a level
    @directories = File::Spec->splitdir($level->findvalue('nodata/filePath'));
    # <filePath> : rel_datapath_from_desc/dir_nodata/level/nd.tiff
    #                                        -3       -2     -1
    my $dir_nodata = $directories[scalar(@directories)-3];
    $self->{pyramid}->{dir_nodata} = $dir_nodata;

    foreach my $v (@levels) {

        my $tagtm       = $v->findvalue('tileMatrix');
        my @tagsize     =  (
                             $v->findvalue('tilesPerWidth'),
                             $v->findvalue('tilesPerHeight')
                           );
        my $tagdirdepth = $v->findvalue('pathDepth');
        my @taglimit    = (
                            $v->findvalue('TMSLimits/minTileRow'),
                            $v->findvalue('TMSLimits/maxTileRow'),
                            $v->findvalue('TMSLimits/minTileCol'),
                            $v->findvalue('TMSLimits/maxTileCol')
                          );
        #
        my $baseimage = File::Spec->catdir($self->getPyrDataPath(),  # all directories structure of pyramid ! 
                                           $self->getPyrName(),
                                           $self->getDirImage(),
                                           $tagtm
                                           );
        #
        my $basenodata = File::Spec->catdir($self->getPyrDataPath(),
                                           $self->getPyrName(),
                                           $self->getDirNodata(),
                                           $tagtm
                                           );
        #
        my $objLevel = BE4::Level->new(
            {
                id                => $tagtm,
                dir_image         => File::Spec->abs2rel($baseimage, $self->getPyrDescPath()),
                compress_image    => $tagformat, 
                dir_nodata        => File::Spec->abs2rel($basenodata, $self->getPyrDescPath()),
                dir_metadata      => undef,      # TODO !
                compress_metadata => undef,      # TODO !
                type_metadata     => undef,      # TODO !
                bitspersample     => $bitspersample,
                samplesperpixel   => $tagsamplesperpixel,
                size              => [$tagsize[0],$tagsize[1]],
                dir_depth         => $tagdirdepth,
                limit             => [$taglimit[0],$taglimit[1],$taglimit[2],$taglimit[3]],
            });
            

        if (! defined $objLevel) {
            WARN(sprintf "Can not load the pyramid level : '%s'", $tagtm);
            next;
        }

        push @{$self->{level}}, $objLevel;

        # fill parameters if not ... !
        $self->{pyramid}->{image_width}  = $tagsize[0];
        $self->{pyramid}->{image_height} = $tagsize[1];
    }

    #
    if (scalar @{$self->{level}} != scalar @levels) {
        WARN (sprintf "Be careful, the level pyramid in not complete (%s != %s) !",
            scalar @{$self->{level}},
            scalar @levels);
    }
    #
    if (! scalar @{$self->{level}}) {
        ERROR ("List of Level Pyramid is empty !");
        return FALSE;
    }

    return TRUE;
}

#############################################################################
# public method serialization (on disk)
#  Manipulate the Directory Structure Cache (DSC) /* in/out */

sub writeCachePyramid {
    my $self = shift;

    TRACE;

    #
    # Params useful to create a cache directory empty or not 
    #
    # pyr_data_path     : path of new pyramid (abs)
    # pyr_data_path_old : path of old pyramid (abs)
    # pyr_name_new      : new pyramid name
    # pyr_name_old      : old pyramid name
    # dir_image     : IMAGE
    # dir_nodata     : NODATA
    # cache_dir     : old or new directories (rel from new pyramid)
    # cache_tile    : old tiles (rel from new pyramid)

    my $oldcachepyramid = File::Spec->catdir($self->getPyrDataPathOld(),$self->getPyrNameOld());
    my $newcachepyramid = File::Spec->catdir($self->getPyrDataPath(),$self->getPyrName());


    $newcachepyramid =~ s/\//\\\//g;
    $oldcachepyramid =~ s/\//\\\//g;
    
    DEBUG(sprintf "%s to %s !",$oldcachepyramid , $newcachepyramid);
    my $dirimage      = $self->getDirImage();
    my $dirnodata     = $self->getDirNodata();
    my $dirmetadata   = undef; # TODO ?

    # substring function 
    my $substring;
    $substring = sub {
        my $expr = shift;
        $_       = $expr;

        my $regex = "s/".$oldcachepyramid."/".$newcachepyramid."/";

        eval ($regex);
        if ($@) {
          ERROR(sprintf "REGEXE", $regex, $@);
          return FALSE;
        }

        return $_;
    };

    # create new cache directory
    my @newdirs;
    my @olddirs = @{$self->{cache_dir}};

    if ($self->isNewPyramid()) {
        @newdirs = @olddirs;
    } else {
        @newdirs = map ({ &$substring($_) } @olddirs); # list cache modified !
    }

    if (! scalar @newdirs) {
    ERROR("Listing of new cache directory is empty !");
    return FALSE;
    }

    foreach my $absdir (@newdirs) {
        #create folders
        eval { mkpath([$absdir],0,0751); };
        if ($@) {
            ERROR(sprintf "Can not create the cache directory '%s' : %s !", $absdir , $@);
            return FALSE;
        }
    }
  
    # search and create link for only new cache tile
    if (! $self->isNewPyramid()) {

        my @oldtiles = @{$self->{cache_tile}};
        my @newtiles = map ({ &$substring($_) } @{$self->{cache_tile}}); # list cache modified !
        my $ntile    = scalar(@oldtiles)-1;

        if (! scalar @oldtiles) {
            WARN("Listing of old cache tile is empty !");
            # return FALSE;
        }

        if (! scalar @newtiles) {
            WARN("Listing of new cache tile is empty !");
            # return FALSE;
        }

        foreach my $i (0..$ntile) {

            my $new_absfile = $newtiles[$i];
            my $old_absfile = $oldtiles[$i];

            if (! -d dirname($new_absfile)) {
                ERROR(sprintf "The directory cache '%s' doesn't exist !",dirname($new_absfile));
                return FALSE;
            }

            if (! -e $old_absfile) {
                ERROR(sprintf "The tile '%s' doesn't exist in '%s' !",
                            basename($old_absfile),
                            dirname($old_absfile));
                return FALSE;  
            }

            my $follow_relfile = undef;

            if (-f $old_absfile && ! -l $old_absfile) {
                $follow_relfile = File::Spec->abs2rel($old_absfile,dirname($new_absfile));
            }

            elsif (-f $old_absfile && -l $old_absfile) {
                my $linked   = File::Spec::Link->linked($old_absfile);
                my $realname = File::Spec::Link->full_resolve($linked);
                $follow_relfile = File::Spec->abs2rel($realname, dirname($new_absfile));
            }

            else {
                ERROR(sprintf "The tile '%s' is not a file or a link in '%s' !",
                            basename($old_absfile),
                            dirname($old_absfile));
                return FALSE;  
            }

            if (! defined $follow_relfile) {
                ERROR (sprintf "The link '%s' can not be resolved in '%s' ?",
                            basename($old_absfile),
                            dirname($old_absfile));
                return FALSE;
            }

            my $result = eval { symlink ($follow_relfile, $new_absfile); };
            if (! $result) {
                ERROR (sprintf "The tile '%s' can not be linked to '%s' (%s) ?",
                            $follow_relfile,
                            $new_absfile,
                            $!);
                return FALSE;
            }
        }
    }

    # we need to create nodata tiles, for each level. If a symbolic link already exists, we move on
    my @levels = $self->getLevels();
    foreach my $objLevel (@levels){
        
        if ($objLevel->{id} >= $self->{pyramid}->{pyr_level_top} 
            && $objLevel->{id} <= $self->{pyramid}->{pyr_level_bottom}) {
            # we have to create the nodata tile
            my $nodataFilePath = File::Spec->rel2abs($objLevel->{dir_nodata}, $self->getPyrDescPath());
            $nodataFilePath = File::Spec->catfile($nodataFilePath,"nd.tiff");
            
            if (! -e $nodataFilePath) {
                
                my $nodatadir = dirname($nodataFilePath);

                if (! -e $nodatadir) {
                    #create folders
                    eval { mkpath([$nodatadir],0,0751); };
                    if ($@) {
                        ERROR(sprintf "Can not create the nodata directory '%s' : %s !", $nodatadir , $@);
                        return FALSE;
                    }
                }

                my $createNodataCommand = $self->createNodata($nodataFilePath);
                
                if (! system($createNodataCommand) == 0) {
                    ERROR (sprintf "Impossible to create the nodata tile for the level %i !\n
                                    The command is incorrect : '%s'",
                                    $objLevel->getID(),
                                    $createNodataCommand);
                    return FALSE;
                }
            }
        }        
    }

    return TRUE;
  
}
sub readCachePyramid {
  my $self     = shift;
  my $cachedir = shift; # old cache directory by default !
  
  TRACE;
  
  # Node IMAGE
  my $dir = File::Spec->catdir($cachedir);
  my $searchitem = $self->FindCacheNode($dir);
  
  DEBUG(Dumper($searchitem));
  
  # Info, cache file of old cache !
  if (! scalar @{$searchitem->{cachetile}}) {
    WARN("No tiles found in directory cache ?");
  }
  # Info, cache dir of old cache !
  if (! scalar @{$searchitem->{cachedir}}){
    ERROR("No directory found in directory cache ?");
    return FALSE;
  }
  
  my @tiles = sort(@{$searchitem->{cachetile}});
  my @dirs  = sort(@{$searchitem->{cachedir}});
  
  $self->{cache_dir} = \@dirs;
  $self->{cache_tile}= \@tiles;
  
  return TRUE;
}

sub FindCacheNode {
  my $self      = shift;
  my $directory = shift;

  TRACE();
  
  my $search = {
    cachedir  => [],
    cachetile => [],
  };
  
  my $pyr_datapath = $self->getPyrDataPath();

  if (! opendir (DIR, $directory)) {
    ERROR("Can not open directory cache ?");
    return undef;
  }

  my $newsearch;
  
  foreach my $entry (readdir DIR) {
    
    next if ($entry =~ m/^\.{1,2}$/);
    
    if ( -d File::Spec->catdir($directory, $entry)) {
      TRACE(sprintf "DIR:%s\n",$entry);
      #push @{$search->{cachedir}}, File::Spec->abs2rel(File::Spec->catdir($directory, $entry), $pyr_datapath);
      push @{$search->{cachedir}}, File::Spec->catdir($directory, $entry);
      
      # recursif
      $newsearch = $self->FindCacheNode(File::Spec->catdir($directory, $entry));
      push @{$search->{cachetile}}, $_  foreach(@{$newsearch->{cachetile}});
      push @{$search->{cachedir}},  $_  foreach(@{$newsearch->{cachedir}});
    }
    
    elsif( -f File::Spec->catfile($directory, $entry) &&
         ! -l File::Spec->catfile($directory, $entry)) {
      TRACE(sprintf "FIL:%s\n",$entry);
      #push @{$search->{cachetile}}, File::Spec->abs2rel(File::Spec->catfile($directory, $entry), $pyr_datapath);
      push @{$search->{cachetile}}, File::Spec->catfile($directory, $entry);
    }
    
    elsif (  -f File::Spec->catfile($directory, $entry) &&
             -l File::Spec->catfile($directory, $entry)) {
      TRACE(sprintf "LIK:%s\n",$entry);
      #push @{$search->{cachetile}}, File::Spec->abs2rel(File::Spec->catfile($directory, $entry), $pyr_datapath);
      push @{$search->{cachetile}}, File::Spec->catfile($directory, $entry);
    }
    
    else {
        TRACE(sprintf "???:%s\n",$entry);
    }
    
  }
  
  return $search;
  
}
################################################################################
# get/set
#  return the params values
sub getPyrFile {
  my $self = shift;

  my $file = $self->{pyramid}->{pyr_name_new};

  return undef if (! defined $file);
  
  if ($file !~ m/\.(pyr|PYR)$/) {
    $file = join('.', $file, "pyr");
  }
  return $file;
}


sub getPyrFileOld {
  my $self = shift;

  my $file = $self->{pyramid}->{pyr_name_old};
  return undef if (! defined $file);
  if ($file !~ m/\.(pyr|PYR)$/) {
    $file = join('.', $file, "pyr");
  }
  return $file;

}
sub getPyrNameOld {
  my $self = shift;
  
  my $name = $self->{pyramid}->{pyr_name_old};
  return undef if (! defined $name);
  $name =~ s/\.(pyr|PYR)$//;
  return $name;
}
sub getPyrDescPath {
  my $self = shift;

  return $self->{pyramid}->{pyr_desc_path};
}
sub getPyrDescPathOld {
  my $self = shift;

  return $self->{pyramid}->{pyr_desc_path_old};
}
sub getPyrDataPath {
  my $self = shift;
  
  return $self->{pyramid}->{pyr_data_path};
}
sub getPyrDataPathOld {
  my $self = shift;
  
  return $self->{pyramid}->{pyr_data_path_old};
}
sub getPyrName {
  my $self = shift;
  
  my $name = $self->{pyramid}->{pyr_name_new};
  return undef if (! defined $name);
  $name =~ s/\.(pyr|PYR)$//;
  return $name;
}
sub getPyrLevelBottom {
  my $self = shift;
  
  return $self->{pyramid}->{pyr_level_bottom};
}
sub getPyrLevelTop {
  my $self = shift;
  
  return $self->{pyramid}->{pyr_level_top};
}
# 
sub getTmsName {
  my $self   = shift;
  
  my $name = $self->{pyramid}->{tms_name};
  return undef if (! defined $name);
  $name =~ s/\.(tms|TMS)$//;
  return $name;
}

sub getTmsFile {
  my $self   = shift;
  
  my $file = $self->{pyramid}->{tms_name};
  return undef if (! defined $file);
  if ($file =! m/\.(tms|TMS)$/) {
    $file = join('.', $file, "tms");
  }
  return $file;

}
sub getTmsPath {
  my $self   = shift;
  
  return $self->{pyramid}->{tms_path};
}
# 
sub getDirImage {
  my $self = shift;
  
  return $self->{pyramid}->{dir_image};
}
sub getDirNodata {
  my $self = shift;
  
  return $self->{pyramid}->{dir_nodata};
}
sub getDirMetadata {
  my $self = shift;
  
  return $self->{pyramid}->{dir_metadata};
}
sub getDirDepth {
  my $self = shift;
  
  return $self->{pyramid}->{dir_depth};
}
#
sub getInterpolation {
  my $self = shift;
  
  return $self->{pyramid}->{interpolation};
}
sub getPhotometric {
  my $self = shift;
  
  return $self->{pyramid}->{photometric};
}
sub getGamma {
  my $self = shift;
  
  return $self->{pyramid}->{gamma};
}
#
################################################################################
# get/set
#  return the objects values

sub getNoData {
  my $self = shift;
  return $self->{nodata};
}
#  
sub getFormat {
  my $self = shift;
  return $self->{format}; # ie objet Format
}
# 
sub getTile {
   my $self = shift;
   return $self->{tile};
}
sub setTile {
  my $self = shift;
  my $tile = shift;
  
  $self->{tile} = $tile;
}
# 
sub getTileMatrixSet {
  my $self = shift;
  
  return $self->{tms};
}
sub setTileMatrixSet {
  my $self = shift;
  my $tms  = shift;
  
  $self->{tms} = $tms;
}
#
sub updateLimits {
    my $self = shift;
    my ($xMin, $yMin, $xMax, $yMax) = @_;
  
    if (! defined $self->{dataLimits}->{xmin} || $xMin < $self->{dataLimits}->{xmin}) {$self->{dataLimits}->{xmin} = $xMin;}
    if (! defined $self->{dataLimits}->{xmax} || $xMax > $self->{dataLimits}->{xmax}) {$self->{dataLimits}->{xmax} = $xMax;}
    if (! defined $self->{dataLimits}->{ymin} || $yMin < $self->{dataLimits}->{ymin}) {$self->{dataLimits}->{ymin} = $yMin;}
    if (! defined $self->{dataLimits}->{ymax} || $yMax > $self->{dataLimits}->{ymax}) {$self->{dataLimits}->{ymax} = $yMax;}
}

sub calculateTMLimits {
    my $self = shift;
    
    if (! defined $self->{dataLimits}->{xmin} || ! defined $self->{dataLimits}->{xmax} || 
        ! defined $self->{dataLimits}->{ymin} || ! defined $self->{dataLimits}->{ymax})
    {
        ERROR("Can not calculate TM limits, limit coordinates are undefined !");
        return FALSE;
    }
    
    my @levels = $self->getLevels();
    foreach my $objLevel (@levels){
        # we need resolution for this level
        my $TM = $self->{tms}->getTileMatrix($objLevel->getID());
        
        my $resolution = Math::BigFloat->new($TM->getResolution());
        my $width = $resolution*$TM->getTileWidth();
        my $height = $resolution*$TM->getTileHeight();
        
        my $iMin=int(($self->{dataLimits}->{xmin} - $TM->getTopLeftCornerX()) / $width);   
        my $iMax=int(($self->{dataLimits}->{xmax} - $TM->getTopLeftCornerX()) / $width);   
        my $jMin=int(($TM->getTopLeftCornerY() - $self->{dataLimits}->{ymax}) / $height); 
        my $jMax=int(($TM->getTopLeftCornerY() - $self->{dataLimits}->{ymin}) / $height);
        
        # we store this values, taking account of the old values.
        
        if (! defined $objLevel->{limit}->[0] || $jMin < $objLevel->{limit}->[0]) {$objLevel->{limit}->[0] = $jMin;}
        if (! defined $objLevel->{limit}->[1] || $jMax > $objLevel->{limit}->[1]) {$objLevel->{limit}->[1] = $jMax;}
        if (! defined $objLevel->{limit}->[2] || $iMin < $objLevel->{limit}->[2]) {$objLevel->{limit}->[2] = $iMin;}
        if (! defined $objLevel->{limit}->[3] || $iMax > $objLevel->{limit}->[3]) {$objLevel->{limit}->[3] = $iMax;}
        
    }
    
    return TRUE;
}

################################################################################
# get/set
#  return the list of objects values

sub getLevels {
  my $self = shift;
  return @{$self->{level}};
}
sub getFirstLevel {
  my $self = shift;
  
  my $levelid = 0;

  TRACE;
  
  # fixme : variable POSIX to put correctly !
  foreach my $k (sort {$a->getID() <=> $b->getID()} ($self->getLevels())) {
    $levelid = $k->getID();
    last;
  }
  
  return $levelid;
}

sub getLastLevel {
  my $self = shift;
  
  my $levelid = 0;
  
  TRACE;
  
  # fixme : variable POSIX to put correctly !
  foreach my $k (sort {$a->getID() <=> $b->getID()} ($self->getLevels())) {
    $levelid = $k->getID();
  }
  
  return $levelid;
}
################################################################################
# privates method (low level)
#  Manipulate the Directory Structure Cache (DSC)

sub _IDXtoX {
  my $self  = shift;
  my $level = shift;
  my $idx   = shift; # x index !
  
  #Res  : 2 m  (determined by level)
  #Xmin : 933888.00
  #Ymax : 6537216.00
  #Size : 8192 m (imagesize = 4096*4096)
  #Level: 3
  #Index X = 933888/8192 = 114
  #Index Y = (16777216-6537216)/8192 = 1250
  
  my $tm  = $self->getTileMatrixSet()->getTileMatrix($level);
  
  my $xo  = $tm->getTopLeftCornerX();
  my $rx  = $tm->getResolution();
  my $sx  = $self->getCacheImageWith();
  
  my $x = ($idx * $rx * $sx) + $xo ;

  return $x;    
}
sub _IDXtoY {
  my $self  = shift;
  my $level = shift;
  my $idx   = shift; # y index !
  
  my $tm  = $self->getTileMatrixSet()->getTileMatrix($level);
  
  my $yo  = $tm->getTopLeftCornerY();
  my $ry  = $tm->getResolution();
  my $sy  = $self->getCacheImageHeight();
  
  my $y = $yo - ($idx * $ry * $sy);
  
  return $y;
}
sub _XtoIDX {
  my $self  = shift;
  my $level = shift;
  my $x     = shift; # x meters !
  
  my $idx = undef;
  #Res  : 2 m  (determined by level)
  #Xmin : 933888.00
  #Ymax : 6537216.00
  #Size : 8192 m (imagesize = 4096*4096)
  #Level: 3
  #Index X = 933888/8192 = 114
  #Index Y = (16777216-6537216)/8192 = 1250
  
  my $tm  = $self->getTileMatrixSet()->getTileMatrix($level);
  
  my $xo  = $tm->getTopLeftCornerX();
  my $rx  = $tm->getResolution();
  my $sx  = $self->getCacheImageWith();
  
  $idx = int(($x - $xo) / ($rx * $sx)) ;

  return $idx;
}
sub _YtoIDX {
  my $self  = shift;
  my $level = shift;
  my $y     = shift; # y meters !
  
  my $idx = undef;
  
  my $tm  = $self->getTileMatrixSet()->getTileMatrix($level);
  
  my $yo  = $tm->getTopLeftCornerY();
  my $ry  = $tm->getResolution();
  my $sy  = $self->getCacheImageHeight();
  
  $idx = int(($yo - $y) / ($ry * $sy)) ;
  
  return $idx;
}
sub _encodeIDXtoB36 {
  my $self  = shift;
  my $number= shift; # idx !
  
  my $padlength = $self->getDirDepth() + 1;
  
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
  # my $b36       = encode_base36($number);
  
  # fill with 0 !
  $b36 = "0"x($padlength - length $b36).reverse($b36);

  DEBUG ($b36);

  return $b36;
}
sub _encodeB36toIDX {
  my $self = shift;
  my $b36  = shift; # idx in base 36 !
  
  my $padlength = $self->getDirDepth() + 1;
  
  my $number = 0;
  my $i = 0;
  foreach(split //, reverse uc $b36) {
    $_ = ord($_) - 55 unless /\d/; # Assume that 'A' is 65
    $number += $_ * (36 ** $i++);
  }
  
  INFO ("0"x($padlength - length $number).$number);
  
  # fill with 0 !
  return "0"x($padlength - length $number).$number;
  # return decode_base36($b36,$padlength);
}

################################################################################
# public method
#  TileImage from Cache (TIC)

sub getImageSize {
  my $self = shift;
  # size of cache image in pixel !
  return $self->{pyramid}->{imagesize} ;
}
sub getCacheImageSize {
  my $self = shift;
  # size of cache image in pixel !
  return ($self->getCacheImageWith(), $self->getCacheImageHeight());
}
sub getCacheImageWith {
  my $self = shift;
  # size of cache image in pixel !
  return $self->getTilePerWidth() * $self->getTileMatrixSet()->getTileWidth();
}
sub getCacheImageHeight {
  my $self = shift;
  # size of cache image in pixel !
  return $self->getTilePerHeight() * $self->getTileMatrixSet()->getTileHeight();
}
#
sub getTilePerWidth {
  my $self = shift;

  return $self->{pyramid}->{image_width};
}
sub getTilePerHeight {
  my $self = shift;
  
  return $self->{pyramid}->{image_height};
}

# retourne le chemin du fichier de la dalle à partir de la racine de l'arbo de
# la pyramide.
# ex: IMAGES/3e/42/01.tif
# ex: METADATA/3e/42/01.tif
sub getCacheNameOfImage {
  my $self  = shift;
  my $level = shift;
  my $x     = shift; # X idx !
  my $y     = shift; # Y idx !
  my $type  = shift;
  
  my $typeDir;
  if ($type eq "data"){
    $typeDir=$self->getDirImage();
  } elsif ($type eq "metadata"){
    $typeDir=$self->getDirMetadata();;
  }
  
  #my $xb36 = $self->_encodeIDXtoB36($self->_getIDXX($level, $x));
  #my $yb36 = $self->_encodeIDXtoB36($self->_getIDXY($level,$y));
  
  my $xb36 = $self->_encodeIDXtoB36($x);
  my $yb36 = $self->_encodeIDXtoB36($y);

  my @xcut = split (//, $xb36);
  my @ycut = split (//, $yb36);
  
  if (scalar(@xcut) != scalar(@ycut)) {
    DEBUG(sprintf "xb36 ('%s') and yb36 ('%s') are not the same size !", $xb36, $yb36);
    
    $yb36 = "0"x(length ($xb36) - length ($yb36)).$yb36 if (length ($xb36) > length ($yb36));
    $xb36 = "0"x(length ($yb36) - length ($xb36)).$xb36 if (length ($yb36) > length ($xb36));
    
    DEBUG(sprintf " > xb36 = '%s' and yb36 = '%s' ", $xb36, $yb36);
  }
  
  my $padlength = $self->getDirDepth() + 1;
  my $size      = scalar(@xcut);
  my $pos       = $size;
  my @l;
  
  for(my $i=0; $i<$padlength;$i++) {
    $pos--;
    push @l, $ycut[$pos];
    push @l, $xcut[$pos];
    push @l, '/';
  }
  
  pop @l;
  
  if ($size>$padlength) {
    while ($pos) {
        $pos--;
        push @l, $ycut[$pos];
        push @l, $xcut[$pos];
    }
  }
  
  my $imagePath     = scalar reverse(@l);
  my $imagePathName = join('.', $imagePath, 'tif');
  
  return File::Spec->catfile($typeDir, $level, $imagePathName); 
}

# retourne le chemin absolu du fichier de la dalle en paramètre.
# ex: /mnt/data/PYRAMIDS/ORTHO/IMAGES/34/31/0a.tif
sub getCachePathOfImage {
  my $self  = shift;
  my $level = shift;
  my $x     = shift;
  my $y     = shift;
  my $type  = shift;
  
  my $imageName = $self->getCacheNameOfImage($level, $x, $y, $type);
  
  return File::Spec->catfile($self->getPyrDataPath(), $self->getPyrName(), $imageName); 
}

# ref alias to getCacheNameOfImage !
sub getCacheImageName {
  my $self  = shift;
  my $level = shift;
  my $x     = shift;
  my $y     = shift;
  my $type  = shift;

  return $self->getCacheNameOfImage($level, $x, $y, $type);
  
}
# ref alias to getCachePathOfImage !
sub getCacheImagePath {
  my $self  = shift;
  my $level = shift;
  my $x     = shift;
  my $y     = shift;
  my $type  = shift;

  return  $self->getCachePathOfImage($level, $x, $y, $type);
}
################################################################################
# public method
#  
sub isNewPyramid {
  my $self = shift;
  return $self->{isnewpyramid};
}
################################################################################
# public method
#  TileImage of Work (TIW)

# No manipulation of TIW by Class Pyramid !

################################################################################
# public method
#  Manipulate the Level Pyramid

sub getBottomLevel {
  my $self = shift;
  
  my $level = $self->getPyrLevelBottom();
  
  return undef if (! defined $level);
  
  foreach my $l ($self->getLevels()) {
    next if ($l->getID() != $level);
    return $level;
  }
  # level not found !
  return undef;
}
sub getTopLevel {
  my $self = shift;
  
  my $level = $self->getPyrLevelTop();
  
  return undef if (! defined $level);
  
  foreach my $l ($self->getLevels()) {
    next if ($l->getID() != $level);
    return $level;
  }
  # level not found !
  return undef;
}

################################################################################
# public method
#  Viewer Pyramid

sub to_string {}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

=head1 SYNOPSIS

 use BE4::Pyramid;
 
 # 1. a pyramid configured from an existing another
 
 my $params_options  = {
    #
    pyr_name_old => "SCAN_RAW_TESTOLD.pyr",
    pyr_name_new => "SCAN_RAW_TESTNEW.pyr",
    pyr_desc_path => "./t/data/pyramid/",
    pyr_data_path => "./t/data/pyramid/ORTHO",
    #
    tms_path     => "./t/data/tms/",
    #
    dir_depth    => "2",  
    dir_image    => "IMAGE",
    dir_nodata    => "NODATA",
    dir_metadata => "METADATA",
    #
    path_nodata   => "./t/data/nodata/",
    imagesize     => "1024",
    color         => "FFFFFF,
    #
    interpolation => "bicubic",
    photometric   => "rgb",
 };

 my $objP = BE4::Pyramid->new($params_options);
 
 $objP->writeConfPyramid();           # in ./t/data/pyramid/SCAN_RAW_TESTNEW.pyr !
 $objP->writeConfPyramid("./t/data/pyramid/TEST.pyr"); 
 
 $objP->writeCachePyramid();  # in 'pyr_data_path' determined by pyramid ! 
 $objP->writeCachePyramid("./t/data/pyramid/test/"); # in another path !
 
 # 2. a new pyramid
 
 my $params_options = {
    #
    pyr_name_new => "SCAN_RAW_TESTNEW.pyr",
    pyr_desc_path => "./t/data/pyramid/",
    pyr_data_path => "./t/data/pyramid/",
    # 
    tms_name     => "LAMB93_50cm_TEST.tms",
    tms_path     => "./t/data/tms/",
    #
    compression  => "raw",
    #
    dir_depth    => "2",
    dir_image    => "IMAGE",
    dir_nodata    => "NODATA",
    dir_metadata => "METADATA",
    #
    image_width  => "16", 
    image_height => "16",
    # 
    path_nodata   => "./t/data/nodata/",
    imagesize     => "1024",
    color         => "FFFFFF",
    #
    bitspersample       => "8", 
    sampleformat        => "uint", 
    photometric         => "rgb", 
    samplesperpixel     => "3",
    interpolation       => "bicubic",
 };

 my $objP = BE4::Pyramid->new($params_options);

 $objP->writeConfPyramid();  # in ./t/data/pyramid/SCAN_RAW_TESTNEW.pyr !
 $objP->writeCachePyramid(); # in ./t/data/pyramid/


=head1 DESCRIPTION

=over

=item * create a new pyramid

To create a new pyramid, you must fill all parameters following :

    pyr_name_new   =
    pyr_desc_path  =
    pyr_data_path  =
    #
    compression   => by default, it's 'raw' !
    #
    image_width   = 
    image_height  =
    #
    dir_depth     =  
    dir_image     = 
    dir_metadata  = 
    # 
    tms_name      =
    tms_path      = 
    # 
    path_nodata   =
    imagesize     => by default, it's '4096' !
    color         => by default, it's 'FFFFFF' !
    # 
    bitspersample       = 
    sampleformat        = 
    photometric         => by default, it's 'rgb' !
    samplesperpixel     =
    interpolation       => by default, it's 'bicubic' !

The pyramid file and the directory structure can be create.

=item * create a new pyramid from an existing pyramid

To create a new pyramid, you must fill all parameters following :

    pyr_name_old  =
    pyr_name_new  =
    pyr_desc_path  =
    pyr_data_path  =
    #
    dir_depth    =  
    dir_image    = 
    #
    tms_path      = 
    # 
    path_nodata   =
    imagesize     => by default, it's '4096' !
    color         => by default, it's 'FFFFFF' !
    # 
    interpolation => by default, it's 'rgb' !
    photometric   => by default, it's 'bicubic' !

All paramaters are filled by loading the old configuration pyramid.
So, object 'BE4::Product' and 'BE4::TileMatrixSet' are created, and the other
parameters are filled...

The pyramid file can be create. The Directory structure of the old pyramid can be
duplicate to the new target directory.

=item * create a file configuration of pyramid

For an new pyramid, all level of the tms file are saved into.
For an existing pyramid, all level of the existing pyramid are only duplicated and
it's the tms value name of the existing pyramid that's considered valid!

=item * create a directory structure

For an new pyramid, the directory structure is empty, only the level directory
are written on disk !
ie :
 ROOTDIR/
  |__PYRAMID_NAME/
        |__IMAGE/
            |__ ID_LEVEL0/
            |__ ID_LEVEL1/
            |__ ID_LEVEL2/

But for an existing pyramid, the directory structure is duplicated to the new
pyramid with all file linked !
ie :
 ROOTDIR/
  |__PYRAMID_NAME/
        |__IMAGE/
            |__ ID_LEVEL0/
                |__ 00/
                    |__ 7F/
                    |__ 7G/
                        |__ CV.tif 
                        |__ ...
            |__ ID_LEVEL1/
            |__ ID_LEVEL2/
                |__ ...
                
    with
     ls -l CV.tif
     CV.tif -> ../../../../../PYRAMID_NAME_OLD/IMAGE/ID_LEVEL0/7G/CV.tif

So be careful when you create a new tile in a directory structure of pyramid,
you must test if the linker exist ! If not, you can destroy the old tile !

=back

=head2 EXPORT

None by default.

=head1 DIRECTORY STRUCTURE

=over

=item * Directory structure :
  
  ${ROOTDIR}/
    |_____ ${PYRAMID_NAME}/
           (ie ortho_raw_dept75)
                |_____ ${IMAGE}/
                            |__ ${ID_LEVEL0}/
                                |__ DEPTH(BASE36)
                                        |__X(BASE36)/
                                            |__ Y(BASE36).tif (it's a link !)
                                            |__ ...
                            |__ ${ID_LEVEL1}/
                            |__ ${ID_LEVELN}/
                |_____ ${NODATA}/
                            |__ ${ID_LEVEL0}/
                            |__ ${ID_LEVELN}/
                |_____ ${METADATA}/
                |_____ ${PYRAMID_FILE}
                        (ie ortho_raw_dept75.xml)

  with the variables following :
    ROOTDIR
    PYRAMID_NAME
    ID_LEVEL(0)  
    IMAGE
    NODATA
    METADATA
    PYRAMID_FILE

=item * Rule Image/Directory Naming :
  
  Res  : 2 m  (determined by level)
  Xmin : 933888.00
  Ymax : 6537216.00
  Size : 8192 m (imagesize = 4096*4096)
  Level: 3
  Index X = 933888/8192 = 114
  Index Y = (16777216-6537216)/8192 = 1250
  Index X base 36 = 36
  Index Y base 36 = QY
  Index X base 36 (write with 3 number) = 036
  Index Y base 36 (write with 3 number) = 0QY
  The directory structure and the image name was defined :
    /$ROOTDIR/$PYRAMID_NAME/IMAGE/3/00/3Q/6Y.tif.

=back

=head1 SAMPLE

=over

=item * Sample Pyramid file (.pyr) :

  [SCAN_RAW_TEST.pyr]
  
  <?xml version='1.0' encoding='US-ASCII'?>
  <Pyramid>
    <tileMatrixSet>LAMB93_50cm_TEST</tileMatrixSet>
    <level>
        <tileMatrix>18</tileMatrix>
        <baseDir>../config/pyramids/SCAN_RAW_TEST/512</baseDir>
        <format>TIFF_RAW_INT8</format>
        <metadata type='INT32_DB_LZW'>
            <baseDir>../config/pyramids/SCAN_RAW_TEST/512</baseDir>
            <format>TIFF_INT8</format>
        </metadata>
        <channels>3</channels>
        <tilesPerWidth>4</tilesPerWidth>
        <tilesPerHeight>4</tilesPerHeight>
        <pathDepth>2</pathDepth>
        <TMSLimits>
            <minTileRow>1</minTileRow>
            <maxTileRow>1000000</maxTileRow>
            <minTileCol>1</minTileCol>
            <maxTileCol>1000000</maxTileCol>
        </TMSLimits>
    </level>
    <level>
        <tileMatrix>17</tileMatrix>
        <baseDir>../config/pyramids/SCAN_RAW_TEST/1024</baseDir>
        <format>TIFF_RAW_INT8</format>
        <metadata type='INT32_DB_LZW'>
            <baseDir>../config/pyramids/SCAN_RAW_TEST/1024</baseDir>
            <format>TIFF_INT8</format>
        </metadata>
        <channels>3</channels>
        <tilesPerWidth>4</tilesPerWidth>
        <tilesPerHeight>4</tilesPerHeight>
        <pathDepth>2</pathDepth>
        <TMSLimits>
            <minTileRow>1</minTileRow>
            <maxTileRow>1000000</maxTileRow>
            <minTileCol>1</minTileCol>
            <maxTileCol>1000000</maxTileCol>
        </TMSLimits>
    </level>
  </Pyramid>

=item * Sample TMS file (.tms) :

  eg SEE ASLO

=item * Sample LAYER file (.lay) :

  eg SEE ASLO

=back

=head1 LIMITATIONS AND BUGS

 File name of pyramid must be with extension : pyr or PYR !
 All levels must be continuous and unique !

=head1 SEE ALSO

 eg package module following :
 
 BE4::Layer
 BE4::TileMatrixSet

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
