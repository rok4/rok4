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

use Geo::OSR;

use File::Spec::Link;
use File::Basename;
use File::Spec;
use File::Path;

use Data::Dumper;

# My module
use BE4::TileMatrixSet;
use BE4::Level;
use BE4::NoData;
use BE4::PyrImageSpec;
use BE4::Pixel;

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
#    ; tms_schema_path = 
#    ; tms_schema_name =
#
#    image_width  = 
#    image_height =
#
#    ; eg section [ pyrImageSpec ]
#    compression  =
#    compressionoption  =
#    gamma        =
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

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;
    my $params = shift;
    my $datasource = shift;

    my $class= ref($this) || $this;
    my $self = {
        # NOTE
        # 2 options possible with parameters :
        #   - a new pyramid configuration
        #   - a existing pyramid configuration
        # > in a HASH entry only (no ref) !
        # the choice is on the parameter 'pyr_name_old'
        #   1) if param is null, it's a new pyramid only !
        #   2) if param is not null, it's an existing pyramid !

        new_pyramid => { 
            name      => undef, # string name
            desc_path     => undef, # path
            data_path     => undef, # path
        },
        isnewpyramid => 1,     # new pyramid by default !
        old_pyramid => { 
            name      => undef, # string name
            desc_path     => undef, # path
            data_path     => undef, # path
        },

        pyr_level_bottom  => undef, # number
        pyr_level_top     => undef, # number
        #
        dir_depth    => undef, # number
        dir_image    => undef, # dir name
        dir_nodata   => undef, # dir name
        dir_metadata => undef, # dir name
        image_width  => undef, # number
        image_height => undef, # number
        #
        imagesize    => undef, # number ie 4096 px by default !

        # OUT
        pyrImgSpec => undef,   # it's an object !
        datasource => undef,   # it's an object !
        tms        => undef,   # it's an object !
        nodata     => undef,   # it's an object !
        levels     => {},      # it's a hash of object level !
        cache_tile => [],      # ie tile image to link  !
        cache_dir  => [],      # ie dir to search !
        #
        dataLimits => {      # data's limits, in the pyramid's SRS
            xmin => undef,
            ymin => undef,
            xmax => undef,
            ymax => undef,
        }, 
    };

    bless($self, $class);

    TRACE;

    # init. parameters
    return undef if (! $self->_init($params,$datasource));

    # a new pyramid or from existing pyramid !
    return undef if (! $self->_load($params));

    return $self;
}

################################################################################
# privates init.
# on détecte les paramètres manquant,on remplit certains attribut de l'objet Pyramid, on met les valeurs par défaut
# les objets attribut sont créés dans _load.

# TODO
#  - no test for path and type (string, number, ...) !

sub _init {
    my $self   = shift;
    my $params = shift;
    my $datasource = shift;

    TRACE;

    if (! defined $params ) {
        ERROR ("Parameters argument required (null) !");
        return FALSE;
    }

    if (! defined $datasource ) {
        ERROR ("Datasource required (null) !");
        return FALSE;
    }
    $self->{datasource} = $datasource;
    
    # init. params .
    $self->{isnewpyramid} = 0 if (defined $params->{pyr_name_old});
    
    if ($self->{isnewpyramid}) {
        # To a new pyramid, you must have to this parameters !
        #
        # you can choice this option by default !
        if (! exists($params->{compression})) {
            WARN ("Optional parameter 'compression' is not set. The default value is 'raw'");
            $params->{compression} = 'raw';
        }
        #
        $self->{image_width} = $params->{image_width}
            || ( ERROR ("The parameter 'image_width' is required!") && return FALSE );
        $self->{image_height} = $params->{image_height}
            || ( ERROR ("The parameter 'image_height' is required!") && return FALSE );
        #
        exists $params->{tms_name} || ( ERROR ("The parameter 'tms_name' is required!") && return FALSE );
        #
        exists $params->{bitspersample}     || ( ERROR ("The parameter 'bitspersample' is required!") && return FALSE );
        exists $params->{sampleformat}      || ( ERROR ("The parameter 'sampleformat' is required!") && return FALSE );
        exists $params->{samplesperpixel}   || ( ERROR ("The parameter 'samplesperpixel' is required!") && return FALSE );
       
    }
    else {
        # To an existing pyramid, you must have to this parameters !
        #
        $self->{old_pyramid}->{name} = $params->{pyr_name_old}
            || ( ERROR ("The parameter 'pyr_name_old' is required!") && return FALSE );
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
    $self->{new_pyramid}->{name} = $params->{pyr_name_new}
        || ( ERROR ("Parameter 'pyr_name_new' is required!") && return FALSE );
    $self->{new_pyramid}->{desc_path} = $params->{pyr_desc_path}
        || ( ERROR ("Parameter 'pyr_desc_path' is required!") && return FALSE );
    $self->{new_pyramid}->{data_path} = $params->{pyr_data_path}
        || ( ERROR ("Parameter 'pyr_data_path' is required!") && return FALSE );
    #
    exists $params->{tms_path}
        || ( ERROR ("Parameter 'tms_path' is required!") && return FALSE );
    #
    $self->{dir_depth} = $params->{dir_depth}
        || ( ERROR ("Parameter 'dir_depth' is required!") && return FALSE );
    #
    exists $params->{path_nodata}
        || ( ERROR ("Parameter to 'path_nodata' is required!") && return FALSE );
    #
    
    # this option are optional !
    #
    if (exists($params->{dir_metadata})) {
        WARN ("Parameter 'dir_metadata' is not implemented yet! It will be ignore");
    }
    $params->{dir_metadata} = undef;
    #
    if (! exists($params->{compressionoption})) {
        WARN ("Optional parameter 'compressionoption' is not set. The default value is 'none'");
        $params->{compressionoption} = 'none';
    }
    #
    if (! exists($params->{pyr_level_bottom})) {
        WARN ("Parameter 'pyr_level_bottom' has not been set. The default value is undef, then the min level will be calculated with source images resolution");
        $params->{pyr_level_bottom} = undef;
    }
    $self->{pyr_level_bottom} = $params->{pyr_level_bottom};
    #
    if (! exists($params->{pyr_level_top})) {
        WARN ("Parameter 'pyr_level_top' has not been set. The defaut value is the top of the TMS' !");
        $params->{pyr_level_top} = undef;
    }
    $self->{pyr_level_top} = $params->{pyr_level_top};
    
    # 
    # you can choice this option with value by default !
    #
    # NV: FIXME: il ne doit pas etre possible de definir une taille differente de celle des dalles du cache.
    #            il ne faut donc pas que ce soit un parametre.
    if (! exists($params->{imagesize})) {
        WARN ("Parameter 'nodata.imagesize' has not been set. The default value is 4096 px");
        $params->{imagesize} = '4096';
    }
    $self->{imagesize} = $params->{imagesize};
    #
    if (! exists($params->{nowhite})) {
        $params->{nowhite} = 'false';
    }
    if (! exists($params->{color})) {
        WARN ("Parameter 'color' (for nodata) has not been set. The default value will be used (consistent with the pixel's format");
        $params->{color} = undef;
    }
    #
    if (! exists($params->{interpolation})) {
        WARN ("Parameter 'interpolation' has not been set. The default value is 'bicubic'");
        $params->{interpolation} = 'bicubic';
    }
    # to remove when interpolation 'bicubique' will be remove
    if ($params->{interpolation} eq 'bicubique') {
        WARN("'bicubique' is a deprecated interpolation value, use 'bicubic' instead");
        $params->{interpolation} = 'bicubic';
    }
    #
    if (! exists($params->{photometric})) {
        WARN ("Parameter 'photometric' has not been set. The default value is 'rgb'");
        $params->{photometric} = 'rgb';
    }
    #
    if (! exists($params->{gamma})) {
        WARN ("Parameter 'gamma' has not been set. The default value is 1 (no effect)");
        $params->{gamma} = 1;
    }
    #
    if (! exists($params->{dir_image})) {
        WARN ("Parameter 'dir_image' has not been set. The default value is 'IMAGE'");
        $params->{dir_image} = 'IMAGE';
    }
    $self->{dir_image} = $params->{dir_image};
    #
    if (! exists($params->{dir_nodata})) {
        WARN ("Parameter 'dir_nodata' has not been set. The default value is 'NODATA'");
        $params->{dir_nodata} = 'NODATA';
    }
    $self->{dir_nodata} = $params->{dir_nodata};
    #
    if (! exists($params->{pyr_desc_path_old})) {
        WARN ("Parameter 'pyr_desc_path_old' has not been set. The default value is the same as 'pyr_desc_path'");
        $params->{pyr_desc_path_old} = $params->{pyr_desc_path};
    }
    $self->{old_pyramid}->{desc_path} = $params->{pyr_desc_path_old};
    #
    if (! exists($params->{pyr_data_path_old})) {
        WARN ("Parameter 'pyr_data_path_old' has not been set. The default value is the same as 'pyr_data_path'.");
        $params->{pyr_data_path_old} = $params->{pyr_data_path};
    }
    $self->{old_pyramid}->{data_path} = $params->{pyr_data_path_old};
    #
    # TODO path !
    if (! -d $params->{path_nodata}) {}
    if (! -d $self->{new_pyramid}->{desc_path}) {}
    if (! -d $self->{old_pyramid}->{desc_path}) {}
    if (! -d $params->{tms_path}) {}
    if (! -d $self->{new_pyramid}->{data_path}) {}
    if (! -d $self->{old_pyramid}->{data_path}) {}
    
    return TRUE;
}

sub _load {
    my $self = shift;
    my $params = shift;

    TRACE;

    if ($self->{isnewpyramid}) {
        # It's a new pyramid !
        return FALSE if (! $self->_fillToPyramid($params));
    } else {
        # A new pyramid from existing pyramid !
        #
        # init. process hasn't checked all parameters,
        # so, we must read file pyramid to initialyze them...
        return FALSE if (! $self->_fillFromPyramid($params));
    }

    # create NoData !
    my $objNodata = BE4::NoData->new({
            path_nodata      => $params->{path_nodata},
            pixel            => $self->getPixel(),
            imagesize        => $self->getImageSize(), 
            color            => $params->{color},
            nowhite          => $params->{nowhite}
    });

    if (! defined $objNodata) {
        ERROR ("Can not load NoData !");
        return FALSE;
    }

    $self->{nodata} = $objNodata;
    DEBUG (sprintf "NODATA = %s", Dumper($objNodata));

    return TRUE;
  
}

####################################################################################################
#                                       FOR A NEW PYRAMID                                          #
####################################################################################################

# method: _fillToPyramid
# Fill pyramid's attributes from configuration. Used to create a new pyramid.
#---------------------------------------------------------------------------------------------------

sub _fillToPyramid { 
    my $self  = shift;
    my $params = shift;

    # create PyrImageSpec !
    my $pyrImgSpec = BE4::PyrImageSpec->new({
        bitspersample => $params->{bitspersample},
        sampleformat => $params->{sampleformat},
        photometric => $params->{photometric},
        samplesperpixel => $params->{samplesperpixel},
        interpolation => $params->{interpolation},
        compression => $params->{compression},
        compressionoption => $params->{compressionoption},
        gamma => $params->{gamma},
    });
    
    if (! defined $pyrImgSpec) {
        ERROR ("Can not load specification of pyramid's images !");
        return FALSE;
    }
    
    $self->{pyrImgSpec} = $pyrImgSpec;
    DEBUG (sprintf "PyrImageSpec = %s", Dumper($pyrImgSpec));
    
    # create TileMatrixSet !
    my $objTMS = BE4::TileMatrixSet->new(File::Spec->catfile($params->{tms_path},$params->{tms_name}));
    
    if (! defined $objTMS) {
      ERROR ("Can not load TMS !");
      return FALSE;
    }
    
    $self->{tms} = $objTMS;
    DEBUG (sprintf "TMS = %s", Dumper($objTMS));
    
    # identify top and bottom levels
    if (! $self->calculateExtremLevels()) {
        ERROR(sprintf "Impossible to calculate top and bottom levels");
        return FALSE;
    }

    # we create all levels between the top and the bottom levels
    if (! $self->createLevels()) {
        ERROR(sprintf "Cannot create levels !");
        return FALSE;
    }

    return TRUE;
}

####################################################################################################
#                                      FROM AN OLD PYRAMID                                         #
####################################################################################################

# method: _fillFromPyramid
# Fill pyramid's attributes from a pyramid's descriptor. Used to create a pyramid from an older.
#---------------------------------------------------------------------------------------------------
sub _fillFromPyramid {
    my $self  = shift;
    my $params = shift;

    TRACE;

    my $filepyramid = File::Spec->catfile($self->getPyrDescPathOld(),$self->getPyrFileOld());
    if (! $self->readConfPyramid($filepyramid,$params)) {
        ERROR (sprintf "Can not read the XML file Pyramid : %s !", $filepyramid);
        return FALSE;
    }

    my $cachepyramid = File::Spec->catdir($self->getPyrDataPathOld(),$self->getPyrNameOld());

    if (! $self->readCachePyramid($cachepyramid)) {
        ERROR (sprintf "Can not read the Directory Cache Pyramid : %s !", $cachepyramid);
        return FALSE;
    }

    # we create all levels between the top and the bottom levels
    if (! $self->createLevels()) {
        ERROR(sprintf "Cannot create levels !");
        return FALSE;
    }

    return TRUE;
}

# method: readConfPyramid
# Read a pyramid's descriptor (XML file, '.pyr').
#---------------------------------------------------------------------------------------------------
sub readConfPyramid {
    my $self   = shift;
    my $filepyramid = shift;
    my $params = shift;

    TRACE;

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

    my $root = $xmltree->getDocumentElement;

    # read tag value of nodata value, photometric and interpolation (not obligatory)

    # NODATA
    my $tagnodata = $root->findnodes('nodataValue')->to_literal;
    if ($tagnodata eq '') {
        WARN (sprintf "Can not determine parameter 'nodata' in the XML file Pyramid ! Value from parameters kept");
        $params->{color} = undef;
    } else {
        INFO (sprintf "Nodata value ('%s') in the XML file Pyramid is used",$tagnodata);
        $params->{color} = $tagnodata;
    }
    
    # PHOTOMETRIC
    my $tagphotometric = $root->findnodes('photometric')->to_literal;
    if ($tagphotometric eq '') {
        WARN (sprintf "Can not determine parameter 'photometric' in the XML file Pyramid ! Value from parameters kept");
    } else {
        INFO (sprintf "Photometric value ('%s') in the XML file Pyramid is used",$tagphotometric);
        $params->{photometric} = $tagphotometric;
    }

    # INTERPOLATION    
    my $taginterpolation = $root->findnodes('interpolation')->to_literal;
    if ($taginterpolation eq '') {
        WARN (sprintf "Can not determine parameter 'interpolation' in the XML file Pyramid ! Value from parameters kept");
    } else {
        INFO (sprintf "Interpolation value ('%s') in the XML file Pyramid is used",$taginterpolation);
        # to remove when interpolation 'bicubique' will be remove
        if ($taginterpolation eq 'bicubique') {
            WARN("'bicubique' is a deprecated interpolation name, use 'bicubic' instead");
            $taginterpolation = 'bicubic';
        }
        $params->{interpolation} = $taginterpolation;
    }

    # read tag value of tileMatrixSet, format and channel

    # TMS
    my $tagtmsname = $root->findnodes('tileMatrixSet')->to_literal;
    if ($tagtmsname eq '') {
        ERROR (sprintf "Can not determine parameter 'tileMatrixSet' in the XML file Pyramid !");
        return FALSE;
    }
    my $tmsname = $params->{tms_name};
    if (! defined $tmsname) {
        WARN ("Null parameter for the name of TMS, so extracting from file pyramid !");
        $tmsname = $tagtmsname;
    }
    if ($tmsname ne $tagtmsname) {
        WARN ("Selecting the name of TMS in the file of the pyramid !");
        $tmsname = $tagtmsname;
    }
    # create a object tileMatrixSet
    my $tmsfile = join(".", $tmsname, "tms"); 
    my $objTMS  = BE4::TileMatrixSet->new(File::Spec->catfile($params->{tms_path}, $tmsfile));
    if (! defined $objTMS) {
        ERROR (sprintf "Can not create object TileMatrixSet from this path : %s ",
            File::Spec->catfile($params->{tms_path}, $tmsfile));
        return FALSE;
    }
    # save it
    $self->{tms} = $objTMS;

    # FORMAT
    my $tagformat = $root->findnodes('format')->to_literal;
    if ($tagformat eq '') {
        ERROR (sprintf "Can not determine parameter 'format' in the XML file Pyramid !");
        return FALSE;
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

    # SAMPLESPERPIXEL  
    my $tagsamplesperpixel = $root->findnodes('channels')->to_literal;
    if ($tagsamplesperpixel eq '') {
        ERROR (sprintf "Can not determine parameter 'channels' in the XML file Pyramid !");
        return FALSE;
    }
    $params->{samplesperpixel} = $tagsamplesperpixel;
    

    # identify bottom and top levels
    if (! $self->calculateExtremLevels()) {
        ERROR(sprintf "Impossible to calculate top and bottom levels");
        return FALSE;
    }

    # create PyrImageSpec object !
    my $pyrImgSpec = BE4::PyrImageSpec->new({
        formatCode => $tagformat,
        photometric => $params->{photometric},
        samplesperpixel => $params->{samplesperpixel},
        interpolation => $params->{interpolation},
        compressionoption => $params->{compressionoption},
        gamma => $params->{gamma}
    });
    if (! defined $pyrImgSpec) {
        ERROR ("Can not load specification of pyramid's images !");
        return FALSE;
    }
    $self->{pyrImgSpec} = $pyrImgSpec;
    DEBUG (sprintf "PyrImageSpec = %s", Dumper($pyrImgSpec));


    # load pyramid level
    my @levels = $root->getElementsByTagName('level');
    
    # read image directory name in the old pyramid, using a level
    my $level = $levels[0];
    my @directories = File::Spec->splitdir($level->findvalue('baseDir'));
    # <baseDir> : rel_datapath_from_desc/dir_image/level
    #                                       -2      -1
    my $dir_image = $directories[scalar(@directories)-2];
    $self->{dir_image} = $dir_image;
    
    # read nodata directory name in the old pyramid, using a level
    @directories = File::Spec->splitdir($level->findvalue('nodata/filePath'));
    # <filePath> : rel_datapath_from_desc/dir_nodata/level/nd.tiff
    #                                        -3       -2     -1
    my $dir_nodata = $directories[scalar(@directories)-3];
    $self->{dir_nodata} = $dir_nodata;

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
        my $levelOrder = $self->getLevelOrder($tagtm);
        my $objLevel = BE4::Level->new(
            {
                id                => $tagtm,
                order             => $levelOrder,
                dir_image         => File::Spec->abs2rel($baseimage, $self->getPyrDescPath()),
                dir_nodata        => File::Spec->abs2rel($basenodata, $self->getPyrDescPath()),
                dir_metadata      => undef,      # TODO !
                compress_metadata => undef,      # TODO !
                type_metadata     => undef,      # TODO !
                size              => [$tagsize[0],$tagsize[1]],
                dir_depth         => $tagdirdepth,
                limit             => [$taglimit[0],$taglimit[1],$taglimit[2],$taglimit[3]],
                is_in_pyramid  => 0
            });
            

        if (! defined $objLevel) {
            WARN(sprintf "Can not load the pyramid level : '%s'", $tagtm);
            next;
        }

        $self->{levels}->{$tagtm} = $objLevel;

        # fill parameters if not ... !
        $self->{image_width}  = $tagsize[0];
        $self->{image_height} = $tagsize[1];
    }

    #
    if (scalar keys %{$self->{levels}} != scalar @levels) {
        WARN (sprintf "Be careful, the level pyramid in not complete (%s != %s) !",
            scalar keys %{$self->{levels}},
            scalar @levels);
    }
    #
    if (! scalar %{$self->{levels}}) {
        ERROR ("List of Level Pyramid is empty !");
        return FALSE;
    }

    return TRUE;
}

# method: readCachePyramid
# Browse pyramid's directories to list files already existing.
#---------------------------------------------------------------------------------------------------
sub readCachePyramid {
  my $self     = shift;
  my $cachedir = shift; # old cache directory by default !
  
  TRACE;
  
  # Node IMAGE
  my $dir = File::Spec->catdir($cachedir);
  # Find cache tiles ou directories
  my $searchitem = $self->findCacheNode($dir);
  
  if (! defined $searchitem) {
    ERROR("An error on reading the cache structure !");
    return FALSE;
  }

  DEBUG(Dumper($searchitem));
  
  # Error, broken link !
  if (scalar @{$searchitem->{cachebroken}}) {
    ERROR("Some links are broken in directory cache !");
    return FALSE;
  }
  
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

# method: findCacheNode
#---------------------------------------------------------------------------------------------------
sub findCacheNode {
  my $self      = shift;
  my $directory = shift;

  TRACE();
  
  my $search = {
    cachedir    => [],
    cachetile   => [],
    cachebroken => [],
  };
  
  my $pyr_datapath = $self->getPyrDataPath();

  if (! opendir (DIR, $directory)) {
    ERROR("Can not open directory cache (%s) ?",$directory);
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
      $newsearch = $self->findCacheNode(File::Spec->catdir($directory, $entry));
      
      push @{$search->{cachetile}},    $_  foreach(@{$newsearch->{cachetile}});
      push @{$search->{cachedir}},     $_  foreach(@{$newsearch->{cachedir}});
      push @{$search->{cachebroken}},  $_  foreach(@{$newsearch->{cachebroken}});
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
        # FIXME : on fait le choix de mettre en erreur le traitement dès le premier lien cassé
        # ou liste exaustive des liens cassés ?
        WARN(sprintf "This tile '%s' may be a broken link in %s !\n",$entry, $directory);
        push @{$search->{cachebroken}}, File::Spec->catfile($directory, $entry);
    }
    
  }
  
  return $search;
  
}

####################################################################################################
#                              FUNCTIONS FOR LEVELS AND LIMITS                                     #
####################################################################################################

# Group: levels methods

# method: calculateExtremLevels
#  Identify top and bottom level if they are not defined in parameters.
#---------------------------------------------------------------------------------------------------
sub calculateExtremLevels {
    my $self = shift;

    # tilematrix list sort by resolution
    my @tmList = $self->getTileMatrixSet()->getTileMatrixByArray();

    # on fait un hash pour retrouver l'ordre d'un niveau a partir de son id.
    my $levelIdx;
    for (my $i=0; $i < scalar @tmList; $i++){
        $levelIdx->{$tmList[$i]->getID()} = $i;
    }

    # initialisation de la transfo de coord du srs des données initiales vers
    # le srs de la pyramide. Si les srs sont identiques on laisse undef.
    my $ct = undef;  
    if ($self->getTileMatrixSet()->getSRS() ne $self->getDataSource()->getSRS()){
        my $srsini= new Geo::OSR::SpatialReference;
        eval { $srsini->ImportFromProj4('+init='.$self->getDataSource()->getSRS().' +wktext'); };
        if ($@) {
            eval { $srsini->ImportFromProj4('+init='.lc($self->getDataSource()->getSRS()).' +wktext'); };
            if ($@) {
                ERROR($@);
                ERROR(sprintf "Impossible to initialize the initial spatial coordinate system (%s) !",$self->getDataSource()->getSRS());
                return FALSE;
            }
        }
        my $srsfin= new Geo::OSR::SpatialReference;
        eval { $srsfin->ImportFromProj4('+init='.$self->getTileMatrixSet()->getSRS().' +wktext'); };
        if ($@) {
            eval { $srsfin->ImportFromProj4('+init='.lc($self->getTileMatrixSet()->getSRS()).' +wktext'); };
            if ($@) {
                ERROR($@);
                ERROR(sprintf "Impossible to initialize the destination spatial coordinate system (%s) !",$self->getTileMatrixSet()->getSRS());
                return FALSE;
            }
        }
        $ct = new Geo::OSR::CoordinateTransformation($srsini, $srsfin);
    }

    # Intitialisation du topLevel:
    #  - En priorité celui fourni en paramètre
    #  - Par defaut, c'est le plus haut niveau du TMS, 
    my $toplevel = $self->getTopLevel();

    if (defined $toplevel) {
        if (! exists $levelIdx->{$toplevel}) {
            ERROR(sprintf "The top level defined in configuration ('%s') does not exist in the TMS !",$toplevel);
            return FALSE;
        }
    } else {
        $self->setTopLevel($self->getTileMatrixSet()->{leveltop});
    }

    # Intitialisation du bottomLevel:
    #  - En priorité celui fourni en paramètre
    #  - Par defaut, le niveau de base du calcul est le premier niveau dont la résolution
    #  (réduite de 5%) est meilleure que celle des données sources.
    #  S'il n'y a pas de niveau dont la résolution est meilleure, on prend le niveau
    #  le plus bas de la pyramide.
    my $bottomlevel = $self->getBottomLevel();
    
    if (defined $bottomlevel) {
        if (! exists $levelIdx->{$bottomlevel}) {
            ERROR(sprintf "The bottom level defined in configuration ('%s') does not exist in the TMS !",$bottomlevel);
            return FALSE;
        }
    } else {
        my $projSrcRes = $self->computeSrcRes($ct);
        if ($projSrcRes < 0) {
            ERROR("La resolution reprojetee est negative");
            return FALSE;
        }

        $bottomlevel = $tmList[0]->getID(); 
        foreach my $tm (@tmList){
            next if ($tm->getResolution() * 0.95  > $projSrcRes);
            $bottomlevel = $tm->getID();
        }

        $self->setBottomLevel($bottomlevel);
    }

    my $topOrder = $self->getLevelOrder($self->getTopLevel());
    my $bottomOrder = $self->getLevelOrder($self->getBottomLevel());

    if ($topOrder < $bottomOrder) {
        ERROR(sprintf "Top (%s) and bottom (%s) levels are not coherent : top resolution is better than bottom resolution !",
            $self->getTopLevel(),$self->getBottomLevel());
        return FALSE;
    }

    if ($topOrder == $bottomOrder) {
        ALWAYS(sprintf "Top and bottom levels are identical (%s) : just one level will be generated",$self->getBottomLevel());
    }

    return TRUE;
    
}

# method: createLevels
#  Create all objects Level between the top and the bottom levels for the new pyramid.
#  If there are an old pyramid, levels already exist. We don't create twice the same level.
#---------------------------------------------------------------------------------------------------
sub createLevels {
    my $self = shift;

    my $objTMS = $self->getTileMatrixSet();

    if (! defined $objTMS) {
        ERROR("Object TMS not defined !");
        return FALSE;
    }

    my $topOrder = $self->getLevelOrder($self->getTopLevel());
    my $bottomOrder = $self->getLevelOrder($self->getBottomLevel());

    # load all level
    for (my $i = $bottomOrder; $i<=$topOrder; $i++) {

        my $tmID = $objTMS->getTileMatrixID($i);
        if (! defined $tmID) {
            ERROR(sprintf "Cannot identify ID for the order %s !",$i);
            return FALSE;
        }

        if (exists $self->{levels}->{$tmID}) {
            $self->{levels}->{$tmID}->{is_in_pyramid} = 2;
            next;
        }

        my $tileperwidth     = $self->getTilePerWidth();
        my $tileperheight    = $self->getTilePerHeight();

        # base dir image
        my $baseimage = File::Spec->catdir($self->getPyrDataPath(),  # all directories structure of pyramid ! 
            $self->getPyrName(),
            $self->getDirImage(),
            $tmID
        );

        # base dir nodata
        my $basenodata = File::Spec->catdir($self->getPyrDataPath(),  # all directories structure of pyramid ! 
            $self->getPyrName(),
            $self->getDirNodata(),
            $tmID
        );

        # TODO : metadata
        #   compression, type ...
        my $basemetadata = File::Spec->catdir($self->getPyrDataPath(),  # all directories structure of pyramid ! 
            $self->getPyrName(),
            $self->getDirMetadata(),
            $tmID
        );

        # FIXME :
        #   compute tms limit in row/col from TMS ?

        # params to level
        my $params = {
            id                => $tmID,
            order             => $i,
            dir_image         => File::Spec->abs2rel($baseimage, $self->getPyrDescPath()), # FIXME rel with the pyr path !
            dir_nodata        => File::Spec->abs2rel($basenodata, $self->getPyrDescPath()), # FIXME rel with the pyr path !
            dir_metadata      => undef,           # TODO,
            compress_metadata => undef,           # TODO  : raw  => TIFF_RAW_INT8,
            type_metadata     => "INT32_DB_LZW",  # FIXME : type => INT32_DB_LZW,
            size              => [$tileperwidth, $tileperheight],
            dir_depth         => $self->getDirDepth(),
            limit             => [undef, undef, undef, undef], # computed
            is_in_pyramid     => 1
        };
        my $objLevel = BE4::Level->new($params);

        if(! defined  $objLevel) {
            ERROR (sprintf "Can not create the level '%s' !", $tmID);
            return FALSE;
        }

        $self->{levels}->{$tmID} = $objLevel;
        # push dir to create : directories for nodata and images.
        push @{$self->{cache_dir}}, $baseimage, $basenodata; #absolute path
    }

    if (! scalar (%{$self->{levels}})) {
        ERROR ("No level loaded !");
        return FALSE;
    }

    return TRUE;
}


# method: computeSrcRes
#  Return best resolution from input data.
#  If SRS are different (between input data and final pyramid), it need a reprojection.
#------------------------------------------------------------------------------
sub computeSrcRes(){
    my $self = shift;
    my $ct = shift;

    TRACE();

    my $srcRes = $self->getDataSource()->getResolution();
    if (!defined($ct)){
        return $srcRes;
    }
    my @imgs = $self->getDataSource()->getImages();
    my $res = 50000000.0;  # un pixel plus gros que la Terre en m ou en deg.
    foreach my $img (@imgs){
        # FIXME: il faut absoluement tester les erreurs ici:
        #        les transformations WGS84G (PlanetObserver) vers PM ne sont pas possible au delà de 85.05°.

        my $p1 = 0;
        eval { $p1 = $ct->TransformPoint($img->getXmin(),$img->getYmin()); };
        if ($@) {
            ERROR($@);
            ERROR(sprintf "Impossible to transform point (%s,%s). Probably limits are reached !",$img->getXmin(),$img->getYmin());
            return -1;
        }

        my $p2 = 0;
        eval { $p2 = $ct->TransformPoint($img->getXmax(),$img->getYmax()); };
        if ($@) {
            ERROR($@);
            ERROR(sprintf "Impossible to transform point (%s,%s). Probably limits are reached !",$img->getXmax(),$img->getYmax());
            return -1;
        }

        # JPB : FIXME attention au erreur d'arrondi avec les divisions 
        my $xRes = $srcRes * (@{$p2}[0]-@{$p1}[0]) / ($img->getXmax()-$img->getXmin());
        my $yRes = $srcRes * (@{$p2}[1]-@{$p1}[1]) / ($img->getYmax()-$img->getYmin());

        $res=$xRes if $xRes < $res;
        $res=$yRes if $yRes < $res;
    }

    return $res;
}

# method: updateLimits
#  Compare old corners' coordinates with the news and update values.
#---------------------------------------------------------------------------------------------------------------
sub updateLimits {
    my $self = shift;
    my ($xMin, $yMin, $xMax, $yMax) = @_;
  
    if (! defined $self->{dataLimits}->{xmin} || $xMin < $self->{dataLimits}->{xmin}) {$self->{dataLimits}->{xmin} = $xMin;}
    if (! defined $self->{dataLimits}->{xmax} || $xMax > $self->{dataLimits}->{xmax}) {$self->{dataLimits}->{xmax} = $xMax;}
    if (! defined $self->{dataLimits}->{ymin} || $yMin < $self->{dataLimits}->{ymin}) {$self->{dataLimits}->{ymin} = $yMin;}
    if (! defined $self->{dataLimits}->{ymax} || $yMax > $self->{dataLimits}->{ymax}) {$self->{dataLimits}->{ymax} = $yMax;}
}

# method: calculateTMLimits
#  Calculate tile limits for each level of the pyramid. It use the resolution and corners' coordinates. If values
#  already exists, we take account of.
#---------------------------------------------------------------------------------------------------------------
sub calculateTMLimits {
    my $self = shift;
    
    if (! defined $self->{dataLimits}->{xmin} || ! defined $self->{dataLimits}->{xmax} || 
        ! defined $self->{dataLimits}->{ymin} || ! defined $self->{dataLimits}->{ymax})
    {
        ERROR("Can not calculate TM limits, limit coordinates are not defined !");
        return FALSE;
    }
    
    my %levels = $self->getLevels();
    foreach my $objLevel (values %levels){
        if ($objLevel->{is_in_pyramid} == 0) {
            # This level is just present in the old pyramid. Limits are not update
            next;
        }

        # we need resolution for this level
        my $TM = $self->getTileMatrixSet()->getTileMatrix($objLevel->getID());
        
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

####################################################################################################
#                              FUNCTIONS FOR WRITING PYRAMID'S ELEMENTS                            #
####################################################################################################

# Group: writting methods

# method: createNodata
#  Create command to create a nodata tile with same parameters as images.
#---------------------------------------------------------------------------------------------------
sub createNodata {
    my $self = shift;
    my $nodataFilePath = shift;
    
    my $sizex = int($self->getImageSize()) / int($self->getTilePerWidth());
    my $sizey = int($self->getImageSize()) / int($self->getTilePerHeight());
    my $compression = $self->getCompression();
  
    # cas particulier de la commande createNodata :
    $compression = ($compression eq 'raw'?'none':$compression);
    
    my $cmd = sprintf ("%s -n %s",CREATE_NODATA, $self->getNodataColor());
    $cmd .= sprintf ( " -c %s", $compression);
    $cmd .= sprintf ( " -p %s", $self->getPhotometric());
    $cmd .= sprintf ( " -t %s %s", $sizex, $sizey);
    $cmd .= sprintf ( " -b %s", $self->getBitsPerSample());
    $cmd .= sprintf ( " -s %s", $self->getSamplesPerPixel());
    $cmd .= sprintf ( " -a %s", $self->getSampleFormat());
    $cmd .= sprintf ( " %s", $nodataFilePath);

    return $cmd;
    
}



# method: writeConfPyramid
#  Manipulate the Configuration File Pyramid
#---------------------------------------------------------------------------------------------------
sub writeConfPyramid {
    my $self    = shift;
    my $filepyramid = shift; # Can be null !

    TRACE;

    # to write TM limits in the pyramid descriptor
    if (! $self->calculateTMLimits()) {
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
    my $formatimg = $self->getCode(); # ie TIFF_RAW_INT8 !
    $strpyrtmplt  =~ s/__FORMATIMG__/$formatimg/;
    #  
    my $channel = $self->getSamplesPerPixel();
    $strpyrtmplt =~ s/__CHANNEL__/$channel/;
    #  
    my $nodata = $self->getNodataColor();
    $strpyrtmplt =~ s/__NODATAVALUE__/$nodata/;
    #  
    my $interpolation = $self->getInterpolation();
    $strpyrtmplt =~ s/__INTERPOLATION__/$interpolation/;
    #  
    my $photometric = $self->getPhotometric;
    $strpyrtmplt =~ s/__PHOTOMETRIC__/$photometric/;

    my %levels = $self->getLevels();

    my $topLevelOrder = $self->getLevelOrder($self->getTopLevel());
    my $bottomLevelOrder = $self->getLevelOrder($self->getBottomLevel());

    ERROR(sprintf "Order top %s bottom %s ", $topLevelOrder,$bottomLevelOrder);

    for (my $i = $topLevelOrder; $i >= $bottomLevelOrder; $i--) {
        # we write levels in pyramid's descriptor from the top to the bottom
        my $ID = $self->getLevelID($i);
        ERROR(sprintf "Order %s ID %s ", $i,$ID);
        my $levelXML = $self->{levels}->{$ID}->getLevelToXML();
        $strpyrtmplt =~ s/<!-- __LEVELS__ -->\n/$levelXML/;
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



# method: writeCachePyramid
#  Manipulate the Directory Structure Cache (DSC) 
#---------------------------------------------------------------------------------------------------
sub writeCachePyramid {
    my $self = shift;

    TRACE;

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

    # Now, @newdir contains :
    #  - for a new pyramid : directories {IMAGE} and {NODATA}/{level}
    #  - for an update pyramid : every directory which contains an image (data or nodata) in the old pyramid
    # @newdir cannot be empty.

    if (! scalar @newdirs) {
        ERROR("Listing of new cache directory is empty !");
        return FALSE;
    }

    foreach my $absdir (@newdirs) {
        #create folders
        eval { mkpath([$absdir]); };
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

    # we need to create nodata tiles, for each level between pyr_level_bottom and pyr_level_top.
    # If a symbolic link already exists, we move on

    my %levels = $self->getLevels();

    my $topLevelOrder = $self->getLevelOrder($self->getTopLevel());
    my $bottomLevelOrder = $self->getLevelOrder($self->getBottomLevel());

    foreach my $objLevel (values %levels){

        my $levelOrder = $self->getLevelOrder($objLevel->{id});
        
        if (! $objLevel->{is_in_pyramid}) {next;}

        # we have to create the nodata tile
        my $nodataFilePath = File::Spec->rel2abs($objLevel->{dir_nodata}, $self->getPyrDescPath());
        $nodataFilePath = File::Spec->catfile($nodataFilePath,"nd.tiff");
        
        if (! -e $nodataFilePath) {
            
            my $nodatadir = dirname($nodataFilePath);

            if (! -e $nodatadir) {
                #create folders
                eval { mkpath([$nodatadir]); };
                if ($@) {
                    ERROR(sprintf "Can not create the nodata directory '%s' : %s !", $nodatadir , $@);
                    return FALSE;
                }
            }

            my $createNodataCommand = $self->createNodata($nodataFilePath);
            
            if (! system($createNodataCommand) == 0) {
                ERROR (sprintf "Impossible to create the nodata tile for the level %i !\nThe command is incorrect : '%s'",
                                $objLevel->getID(),
                                $createNodataCommand);
                return FALSE;
            }
        }
   
    }

    return TRUE;
  
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

# New pyramid
sub getPyrFile {
    my $self = shift;
    my $file = $self->{new_pyramid}->{name};
    return undef if (! defined $file);
    if ($file !~ m/\.(pyr|PYR)$/) {
        $file = join('.', $file, "pyr");
    }
    return $file;
}
sub getPyrName {
    my $self = shift;
    my $name = $self->{new_pyramid}->{name};
    return undef if (! defined $name);
    $name =~ s/\.(pyr|PYR)$//;
    return $name;
}
sub getPyrDescPath {
  my $self = shift;
  return $self->{new_pyramid}->{desc_path};
}
sub getPyrDataPath {
  my $self = shift;
  return $self->{new_pyramid}->{data_path};
}

# Old pyramid

sub isNewPyramid {
    my $self = shift;
    return $self->{isnewpyramid};
}

sub getPyrFileOld {
    my $self = shift;
    my $file = $self->{old_pyramid}->{name};
    return undef if (! defined $file);
    if ($file !~ m/\.(pyr|PYR)$/) {
        $file = join('.', $file, "pyr");
    }
    return $file;
}

sub getPyrNameOld {
    my $self = shift;
    my $name = $self->{old_pyramid}->{name};
    return undef if (! defined $name);
    $name =~ s/\.(pyr|PYR)$//;
    return $name;
}
sub getPyrDescPathOld {
  my $self = shift;
  return $self->{old_pyramid}->{desc_path};
}
sub getPyrDataPathOld {
  my $self = shift;
  return $self->{old_pyramid}->{data_path};
}

# TMS
sub getTmsName {
    my $self   = shift;
    return $self->{tms}->{name};
}

sub getTmsFile {
    my $self   = shift;
    return $self->{tms}->{filename};
}
sub getTmsPath {
    my $self   = shift;
    return $self->{tms}->{filepath};
}
sub getTileMatrixSet {
    my $self = shift;
    return $self->{tms};
}
sub setTileMatrixSet {
    my $self = shift;
    my $tms  = shift;
    $self->{tms} = $tms;
}

# Directories
sub getDirImage {
    my $self = shift;
    return $self->{dir_image};
}
sub getDirNodata {
    my $self = shift;
    return $self->{dir_nodata};
}
sub getDirMetadata {
    my $self = shift;
    return $self->{dir_metadata};
}
sub getDirDepth {
    my $self = shift;
    return $self->{dir_depth};
}

# Pyramid's images' specifications
sub getInterpolation {
    my $self = shift;
    return $self->{pyrImgSpec}->{interpolation};
}
sub getGamma {
    my $self = shift;
    return $self->{pyrImgSpec}->{gamma};
}
sub getCompression {
    my $self = shift;
    return $self->{pyrImgSpec}->{compression};
}
sub getCompressionOption {
    my $self = shift;
    return $self->{pyrImgSpec}->{compressionoption};
}
sub getPixel {
    my $self = shift;
    return $self->{pyrImgSpec}->{pixel};
}
sub getSamplesPerPixel {
    my $self = shift;
    return $self->{pyrImgSpec}->{pixel}->{samplesperpixel};
}
sub getPhotometric {
    my $self = shift;
    return $self->{pyrImgSpec}->{pixel}->{photometric};
}
sub getBitsPerSample {
    my $self = shift;
    return $self->{pyrImgSpec}->{pixel}->{bitspersample};
}
sub getSampleFormat {
    my $self = shift;
    return $self->{pyrImgSpec}->{pixel}->{sampleformat};
}
sub getCode {
    my $self = shift;
    return $self->{pyrImgSpec}->{formatCode};
}

# Nodata
sub getNodata {
    my $self = shift;
    return $self->{nodata};
}
sub getNodataColor {
    my $self = shift;
    return $self->{nodata}->{color};
}

# Datasource
sub getDataSource {
    my $self = shift;
    return $self->{datasource};
}

# Levels
sub getLevels {
    my $self = shift;
    return %{$self->{levels}};
}
# method: getLevelOrder
#  Return the tile matrix order from the ID :  
#   - 0 (bottom level, smallest resolution)
#   - NumberOfTM-1 (top level, biggest resolution).
#---------------------------------------------------------------------------------
sub getLevelOrder {
    my $self = shift;
    my $ID = shift;
    return $self->getTileMatrixSet()->getTileMatrixOrder($ID);
}
# method: getLevelID
#  return the tile matrix ID from the order.
#---------------------------------------------------------------------------------
sub getLevelID {
    my $self = shift;
    my $order = shift;
    return $self->getTileMatrixSet()->getTileMatrixID($order);
}

sub getBottomLevel {
    my $self = shift;
    return $self->{pyr_level_bottom};
}

sub getTopLevel {
    my $self = shift;
    return $self->{pyr_level_top};
}

sub setBottomLevel {
    my $self = shift;
    my $BottomLevel = shift;
    $self->{pyr_level_bottom} = $BottomLevel;
}

sub setTopLevel {
    my $self = shift;
    my $TopLevel = shift;
    $self->{pyr_level_top} = $TopLevel;
}

# Image's size
sub getImageSize {
    my $self = shift;
    # size of cache image in pixel !
    return $self->{imagesize} ;
}
sub getCacheImageSize {
    my $self = shift;
    # size of cache image in pixel !
    return ($self->getCacheImageWidth(), $self->getCacheImageHeight());
}
sub getCacheImageWidth {
    my $self = shift;
    # size of cache image in pixel !
    return $self->getTilePerWidth() * $self->getTileMatrixSet()->getTileWidth();
}
sub getCacheImageHeight {
    my $self = shift;
    # size of cache image in pixel !
    return $self->getTilePerHeight() * $self->getTileMatrixSet()->getTileHeight();
}
sub getTilePerWidth {
    my $self = shift;
    return $self->{image_width};
}
sub getTilePerHeight {
    my $self = shift;
    return $self->{image_height};
}

# method: getCacheNameOfImage
# Return the relative filepath of the tile from the pyramid's root.
# Parameters : level, tile's coordinates (i,j), type of data (image, metadata).
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

# method : getCachePathOfImage
# Return the absolute filepath of the tile.
# Parameters : level, tile's coordinates (i,j), type of data (image, metadata).
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



####################################################################################################
#                                   COORDINATES MANIPULATION                                       #
####################################################################################################

# Group: coordinates manipulation

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

sub to_string {}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

=head1 SYNOPSIS

 use BE4::Pyramid;
 my $objPyramid  = BE4::Pyramid->new($params_options,$objDataSrc);
 
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
    color         => "FFFFFF, ----> present in the file .pyr
    #
    interpolation => "bicubic", ----> present in the file .pyr
    photometric   => "rgb", ----> present in the file .pyr
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
So, object 'BE4::TileMatrixSet' are created, and the other
parameters are filled...

The pyramid file can be create. The Directory structure of the old pyramid can be
duplicate to the new target directory.

=item * create a file configuration of pyramid

For an new pyramid, all level of the tms file are saved into.
For an existing pyramid, all level of the existing pyramid are only duplicated and
it's the tms value name of the existing pyramid that's considered valid!

=item * create a directory structure

For an new pyramid, the directory structure is empty, only the level directory for images and directory and
 tile for nodata
are written on disk !
ie :
 ROOTDIR/
  |__PYRAMID_NAME/
        |__IMAGE/
            |__ ID_LEVEL0/
            |__ ID_LEVEL1/
            |__ ID_LEVEL2/
        |__NODATA/
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
                                            |__ Y(BASE36).tif (it can be a link !)
                                            |__ ...
                            |__ ${ID_LEVEL1}/
                            |__ ${ID_LEVELN}/
                |_____ ${NODATA}/
                            |__ ${ID_LEVEL0}/
                                |__nd.tiff (it can be a link)
                            .
                            .
                            .
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
    <tileMatrixSet>LAMB93_10cm</tileMatrixSet>
    <format>TIFF_RAW_INT8</format>
    <channels>3</channels>
    <nodataValue>FFFFFF</nodataValue>
    <interpolation>bicubic</interpolation>
    <photometric>rgb</photometric>
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
