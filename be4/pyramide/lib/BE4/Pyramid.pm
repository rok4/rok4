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
use Tie::File;

use Data::Dumper;

use BE4::TileMatrixSet;
use BE4::Level;
use BE4::NoData;
use BE4::PyrImageSpec;
use BE4::Pixel;
use BE4::Forest;
use BE4::Base36;

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

################################################################################

BEGIN {}
INIT {}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    * new_pyramid => { 
        name      => undef, # string name
        desc_path     => undef, # path
        data_path     => undef, # path
        content_path  => undef,
    },
    * old_pyramid => { 
        name      => undef, # string name
        desc_path     => undef, # path
        data_path     => undef, # path
        content_path  => undef,
    },
    * dir_depth    => undef, # number
    * dir_image    => undef, # dir name
    * dir_nodata   => undef, # dir name
    * dir_metadata => undef, # dir name
    * image_width  => undef, # number
    * image_height => undef, # number
    * pyrImgSpec => undef,   # it's an PyrImageSpec object
    * tms => undef,   # TileMatrixSet object
    * nodata => undef,   # Nodata object
    * levels => {},      # it's a hash of Level objects
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;
    my $params = shift;
    my $path_temp = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
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
            name          => undef, # string name
            desc_path     => undef, # path
            data_path     => undef, # path
            content_path  => undef,
        },
        old_pyramid => { 
            name          => undef, # string name
            desc_path     => undef, # path
            data_path     => undef, # path
            content_path  => undef,
        },
        #
        dir_depth    => undef, # number
        dir_image    => undef, # dir name
        dir_nodata   => undef, # dir name
        dir_metadata => undef, # dir name
        image_width  => undef, # number
        image_height => undef, # number

        # OUT
        pyrImgSpec => undef,   # it's an PyrImageSpec object
        tms        => undef,   # TileMatrixSet object
        nodata     => undef,   # Nodata object
        levels     => {},      # it's a hash of Level objects
    };

    bless($self, $class);

    TRACE;

    # init. parameters
    return undef if (! $self->_init($params));

    # a new pyramid or from existing pyramid !
    return undef if (! $self->_load($params,$path_temp));

    return $self;
}

#
=begin nd
method: _init

We detect missing parameters and define default values.

Parameters:
    params - All parameters abour pyramid's format.

See Also:
    <new>, <_load>
=cut
sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;

    if (! defined $params ) {
        ERROR ("Parameters argument required (null) !");
        return FALSE;
    }
    
    # Always mandatory :
    #   - pyr_name_new, pyr_desc_path, pyr_data_path
    #   - tms_path
    if (! exists $params->{pyr_name_new} || ! defined $params->{pyr_name_new}) {
        ERROR ("The parameter 'pyr_name_new' is required!");
        return FALSE;
    }
    $params->{pyr_name_new} =~ s/\.(pyr|PYR)$//;
    $self->{new_pyramid}->{name} = $params->{pyr_name_new};
    
    if (! exists $params->{pyr_desc_path} || ! defined $params->{pyr_desc_path}) {
        ERROR ("The parameter 'pyr_desc_path' is required!");
        return FALSE;
    }
    $self->{new_pyramid}->{desc_path} = $params->{pyr_desc_path};
    
    if (! exists $params->{pyr_data_path} || ! defined $params->{pyr_data_path}) {
        ERROR ("The parameter 'pyr_data_path' is required!");
        return FALSE;
    }
    $self->{new_pyramid}->{data_path} = $params->{pyr_data_path};
    
    if (! exists $params->{tms_path} || ! defined $params->{tms_path}) {
        ERROR ("The parameter 'tms_path' is required!");
        return FALSE;
    }
    $self->{tms_path} = $params->{tms_path};
    
    
    # Different treatment for a new or an update pyramid
    if (! exists $params->{pyr_name_old} || ! defined $params->{pyr_name_old}) {
        # For a new pyramid, are mandatory :
        #   - image_width, image_height
        #   - tms_name
        #   - bitspersample, sampleformat, samplesperpixel
        my @mandatory_parameters = ("image_width","image_height","tms_name","bitspersample","sampleformat","samplesperpixel","dir_depth");
        #
        foreach my $parameter (@mandatory_parameters) {
            if (! exists $params->{$parameter}) {
                ERROR ("The parameter '$parameter' is required for a new pyramid");
                return FALSE,
            }
            $self->{$parameter} = $params->{$parameter};
        }
        
        # Optionnal :
        #   - compression
        if (! exists($params->{compression})) {
            WARN ("Optional parameter 'compression' is not set. The default value is 'raw'");
            $params->{compression} = 'raw';
        }
        
    }
    else {
        $params->{pyr_name_old} =~ s/\.(pyr|PYR)$//;
        $self->{old_pyramid}->{name} = $params->{pyr_name_old};
        #
        if (! exists($params->{pyr_desc_path_old})) {
            WARN ("Parameter 'pyr_desc_path_old' has not been set, 'pyr_desc_path' is used.");
            $params->{pyr_desc_path_old} = $params->{pyr_desc_path};
        }
        $self->{old_pyramid}->{desc_path} = $params->{pyr_desc_path_old};
        #
        if (! exists($params->{pyr_data_path_old})) {
            WARN ("Parameter 'pyr_data_path_old' has not been set, 'pyr_data_path' is used.");
            $params->{pyr_data_path_old} = $params->{pyr_data_path};
        }
        $self->{old_pyramid}->{data_path} = $params->{pyr_data_path_old};
        
        # This parameter will be read in the ancestor's descriptor
        my @extracted_parameters = ("compression","tms_name","bitspersample","sampleformat","samplesperpixel","bitspersample");
        #
        foreach my $parameter (@extracted_parameters) {
            if (! exists $params->{$parameter}) {
                INFO ("The parameter '$parameter' will be extracted from the ancestor descriptor.");
            }
        }
    }

    #
    if (! exists($params->{compressionoption})) {
        INFO ("Optional parameter 'compressionoption' is not set. The default value is 'none'");
        $params->{compressionoption} = 'none';
    }    
    #
    if (! exists($params->{nowhite})) {
        $params->{nowhite} = 'false';
    }
    if (! exists($params->{color})) {
        WARN ("Parameter 'color' (for nodata) has not been set. The default value will be used (consistent with the pixel's format).");
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
    
    # Directories names : data, nodata and metadata
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
    if (! exists($params->{dir_metadata})) {
        WARN ("Parameter 'dir_metadata' has not been set. The default value is 'METADATA'");
        $params->{dir_nodata} = 'METADATA';
    }
    $self->{dir_metadata} = $params->{dir_metadata};
    
    return TRUE;
}

#
=begin nd
method: _load

We fill pyramid's attributes. 2 cases:
* a new pyramid : all informations must be present in configuration, _fillToPyramid is called.
* updating from an old pyramid : informations are collected in the old pyramid's descriptor, _fillFromPyramid is called.

Informations are checked, using perl classes like NoData, Level, PyrImageSpec...

Parameters:
    params - All parameters about a pyramid's format (new or update).

See Also:
    <new>, <_init>, <_fillToPyramid>, <_fillFromPyramid>
=cut
sub _load {
    my $self = shift;
    my $params = shift;
    my $path_temp = shift;

    TRACE;

    if ($self->isNewPyramid) {
        # It's a new pyramid !
        return FALSE if (! $self->_fillToPyramid($params));
    } else {
        # A new pyramid from existing pyramid !
        #
        # init. process hasn't checked all parameters,
        # so, we must read file pyramid to initialyze them...
        return FALSE if (! $self->_fillFromPyramid($params,$path_temp));
    }

    # create NoData !
    my $objNodata = BE4::NoData->new({
        pixel            => $self->getPixel(),
        value            => $params->{color},
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

#
=begin nd
method: _fillToPyramid

We generate a new pyramid (no ancestor). All information are in parameters:
* image specifications
* used TMS

Parameters:
    params - All parameters for a new pyramid.

=cut
sub _fillToPyramid { 
    my $self  = shift;
    my $params = shift;

    TRACE();
    
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

    return TRUE;
}

####################################################################################################
#                                      FROM AN OLD PYRAMID                                         #
####################################################################################################

#
=begin nd
method: _fillFromPyramid

We want to update an old pyramid with new data. We have to collect attributes' value in old pyramid descriptor and old cache. They have priority to parameters. If the old cache doesn't have a list, we create temporary one.

Parameters:
    params - Parameters for update a pyramid : old descriptor path, and old cache path. Others values will be used just in case informations are missing in the old pyramid.
    path_temp - Directory path, where to write the temporary old cache list, if not exist.

See Also:
    <readConfPyramid>, <readCachePyramid>
=cut
sub _fillFromPyramid {
    my $self  = shift;
    my $params = shift;
    my $path_temp = shift;

    TRACE;

    # Old pyramid's descriptor reading
    my $filepyramid = $self->getOldDescriptorFile();
    if (! $self->readConfPyramid($filepyramid,$params)) {
        ERROR (sprintf "Can not read the XML file Pyramid : %s !", $filepyramid);
        return FALSE;
    }

    # Old pyramid's cache list test : if it doesn't exist, we create a temporary one.
    my $listpyramid = $self->getOldListFile();
    if (! -f $listpyramid) {
        my $cachepyramid = $self->getOldDataDir();
        
        if (! defined $path_temp) {
            ERROR("'path_temp' must be defined to write the file list if it doesn't exist.");
            return FALSE;
        }
        $listpyramid = File::Spec->catfile($path_temp,$self->getNewName(),$self->getOldName().".list");
        $self->{old_pyramid}->{content_path} = $listpyramid;
        
        WARN(sprintf "Cache list file does not exist. We browse the old cache to create it (%s).",$listpyramid);
        
        if (! $self->readCachePyramid($cachepyramid,$listpyramid)) {
            ERROR (sprintf "Can not read the Directory Cache Pyramid : %s !", $cachepyramid);
            return FALSE;
        }
    }
    

    return TRUE;
}

#
=begin nd
method: readConfPyramid

Parse an XML file, a pyramid's descriptor (file.pyr) to pick up informations. We identify levels which are present in the old pyramid (not necessaraly the same in the new pyramid).

Parameters:
    filepyramid - Complete absolute descriptor path.
    params - Used just in case informations are missing in the old pyramid.
=cut
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
    # TODO : to remove when format 'TIFF_INT8' and 'TIFF_FLOAT32' will be remove
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
    my $dir_image = $directories[-2];
    $self->{dir_image} = $dir_image;
    
    # read nodata directory name in the old pyramid, using a level
    @directories = File::Spec->splitdir($level->findvalue('nodata/filePath'));
    # <filePath> : rel_datapath_from_desc/dir_nodata/level/nd.tif
    #                                        -3       -2     -1
    my $dir_nodata = $directories[-3];
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
        my $baseimage = File::Spec->catdir($self->getNewDataDir(),
                                           $self->getDirImage(),
                                           $tagtm );
        #
        my $basenodata = File::Spec->catdir($self->getNewDataDir(),
                                           $self->getDirNodata(),
                                           $tagtm );
        #
        my $levelOrder = $self->getOrderfromID($tagtm);
        my $objLevel = BE4::Level->new({
            id                => $tagtm,
            order             => $levelOrder,
            dir_image         => File::Spec->abs2rel($baseimage, $self->{new_pyramid}->{desc_path}),
            dir_nodata        => File::Spec->abs2rel($basenodata, $self->{new_pyramid}->{desc_path}),
            dir_metadata      => undef,      # TODO !
            compress_metadata => undef,      # TODO !
            type_metadata     => undef,      # TODO !
            size              => [$tagsize[0],$tagsize[1]],
            dir_depth         => $tagdirdepth,
            limit             => [$taglimit[0],$taglimit[1],$taglimit[2],$taglimit[3]],
        });
            

        if (! defined $objLevel) {
            WARN(sprintf "Can not load the pyramid level : '%s'", $tagtm);
            next;
        }

        $self->{levels}->{$tagtm} = $objLevel;

        # same for each level
        $self->{dir_depth}  = $tagdirdepth;
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

#
=begin nd
method: readCachePyramid

Browse old cache. We store images (data and nodata) in a file and broken symbolic links in an array.

Parameters:
    cachedir - Root directory to browse.
    listpyramid - File path, where to write files' list.
    
See Also:
    <findImages> 
=cut
sub readCachePyramid {
    my $self     = shift;
    my $cachedir = shift; # old cache directory by default !
    my $listpyramid = shift;
    
    TRACE("Reading cache of pyramid...");
  
    if (-f $listpyramid) {
        WARN(sprintf "Cache list ('%s') exists in temporary directory, overwrite it !", $listpyramid);
    }
    
    if (! -d dirname($listpyramid)) {
        eval { mkpath([dirname($listpyramid)]); };
        if ($@) {
            ERROR(sprintf "Can not create the old cache list directory '%s' : %s !",dirname($listpyramid), $@);
            return FALSE;
        }
    }
    
    # We list:
    #   - old cache files (write in the file $LIST)
    #   - old caches' roots (store in %cacheRoots)
    #   - old cache broken links (store in @brokenlinks)
    
    my $LIST;

    if (! open $LIST, ">", $listpyramid) {
        ERROR(sprintf "Cannot open (to write) old cache list file : %s",$listpyramid);
        return FALSE;
    }

    my $dir = File::Spec->catdir($cachedir);
    my @brokenlinks;
    my %cacheRoots;
    
    if (! $self->findImages($dir, $LIST, \@brokenlinks, \%cacheRoots)) {
        ERROR("An error on searching into the cache structure !");
        return FALSE;
    }
    
    close $LIST;
    
    # Have we broken links ?
    if (scalar @brokenlinks) {
        ERROR("Some links are broken in directory cache !");
        return FALSE;
    }

    
    # We write at the top of the list file, caches' roots, using Tie library
    
    my @list;
    if (! tie @list, 'Tie::File', $listpyramid) {
        ERROR(sprintf "Cannot write the header of old cache list file : %s",$listpyramid);
        return FALSE;
    }
    
    unshift @list,"#";
    
    while( my ($root,$rootID) = each(%cacheRoots) ) {
        unshift @list,(sprintf "%s=%s",$rootID,$root);
    }
    
    untie @list;
  
    return TRUE;
}

#
=begin nd
method: findImages

Recursive method to browse a file tree structure. Store directories, images (data and nodata) and broken symbolic links.

Parameters:
    directory - Root directory to browse.
    LIST - stream to the file, in which we write old cache list (target files, no link).
    brokenlinks - array reference, filled with broken links.
    cacheroots - hash reference, filled with different pyramids' roots and the corresponding identifiant.
=cut
sub findImages {
    my $self      = shift;
    my $directory = shift;
    my $LIST = shift;
    my $brokenlinks = shift;
    my $cacheroots = shift;
    
    TRACE(sprintf "Searching node in %s\n", $directory);
    
    my $pyr_datapath = $self->getNewDataDir();
    
    if (! opendir (DIR, $directory)) {
        ERROR("Can not open directory cache (%s) ?",$directory);
        return FALSE;
    }
    
    foreach my $entry (readdir DIR) {
        
        next if ($entry =~ m/^\.{1,2}$/);
        
        my $pathentry = File::Spec->catfile($directory, $entry);
        
        my $realName;
        
        if ( -d $pathentry) {
            TRACE(sprintf "DIR:%s\n",$pathentry);
            # recursif
            if (! $self->findImages($pathentry, $LIST, $brokenlinks, $cacheroots)) {
                ERROR("Can not search in directory cache (%s) ?",$pathentry);
                return FALSE;
            }
            next;
        }
        
        elsif( -f $pathentry && ! -l $pathentry) {
            TRACE(sprintf "%s\n",$pathentry);
            # It's the real file, not a link
            $realName = $pathentry;
        }
        
        elsif (  -f $pathentry && -l $pathentry) {
            TRACE(sprintf "%s\n",$pathentry);
            # It's a link
            
            my $linked   = File::Spec::Link->linked($pathentry);
            $realName = File::Spec::Link->full_resolve($linked);
            
            if (! defined $realName) {
                # FIXME : on fait le choix de mettre en erreur le traitement dès le premier lien cassé
                # ou liste exaustive des liens cassés ?
                WARN(sprintf "This tile '%s' may be a broken link in %s !\n",$entry, $directory);
                push @$brokenlinks,$entry;
                return TRUE;
            }         
        }
        
        # We extract from the old tile path, the cache name (without the old cache root path)
        my @directories = File::Spec->splitdir($realName);
        # $realName : abs_datapath/dir_image/level/XY/XY/XY.tif
        #                             -5      -4  -3 -2   -1
        #                     => -(3 + dir_depth)
        #    OR
        # $realName : abs_datapath/dir_nodata/level/nd.tif
        #                              -3      -2     -1
        #                           => - 3
        my $deb = -3;
            
        $deb -= $self->{dir_depth} if ($directories[-3] ne $self->{dir_nodata});
        
        my @indexName = ($deb..-1);
        my @indexRoot = (0..@directories+$deb-1);
        
        my $name = File::Spec->catdir(@directories[@indexName]);
        my $root = File::Spec->catdir(@directories[@indexRoot]);
        
        my $rootID;
        if (exists $cacheroots->{$root}) {
            $rootID = $cacheroots->{$root};
        } else {
            $rootID = scalar (keys %$cacheroots);
            $cacheroots->{$root} = $rootID;
        }

        printf $LIST "%s\n", File::Spec->catdir($rootID,$name);;
    }
    
    return TRUE;
}

####################################################################################################
#                              FUNCTIONS FOR LEVELS AND LIMITS                                     #
####################################################################################################

#
=begin nd
method: updateLevels

Determine top and bottom for the new pyramid and create Level objects.

Parameters:
    DSL - a DataSourceLoader, to determine extrem levels.
    topID - optionnal, from the 'pyramid' section in the configuration file
=cut
sub updateLevels {
    my $self = shift;
    my $DSL = shift;
    my $topID = shift;
    
    # update datasources top/bottom levels !
    my ($bottomOrder,$topOrder) = $DSL->updateDataSources($self->getTileMatrixSet, $topID);
    if ($bottomOrder == -1) {
        ERROR("Cannot determine top and bottom levels, from data sources.");
        return FALSE;
    }
    
    INFO (sprintf "Bottom level order : %s, top level order : %s", $bottomOrder, $topOrder);

    if (! $self->createLevels($bottomOrder,$topOrder)) {
        ERROR("Cannot create Level objects for the new pyramid.");
        return FALSE;
    }
    
    return TRUE
}

#
=begin nd
method: createLevels

Create all objects Level between the global top and the bottom levels (from data sources) for the new pyramid.

If there are an old pyramid, some levels already exist. We don't create twice the same level.

Parameters:
    bottomOrder, topOrder - global extrem levels' orders.
=cut
sub createLevels {
    my $self = shift;
    my $bottomOrder = shift;
    my $topOrder = shift;

    TRACE();
    
    my $objTMS = $self->getTileMatrixSet;
    if (! defined $objTMS) {
        ERROR("We need a TMS to create levels.");
        return FALSE;
    }
    
    # Create all level between the bottom and the top
    for (my $order = $bottomOrder; $order <= $topOrder; $order++) {

        my $ID = $self->getIDfromOrder($order);
        if (! defined $ID) {
            ERROR(sprintf "Cannot identify ID for the order %s !",$order);
            return FALSE;
        }

        if (exists $self->{levels}->{$ID}) {
            # this level already exists (from the old pyramid). We have not to remove informations (like extrem tiles)
            next;
        }

        my $tilesperwidth = $self->getTilesPerWidth();
        my $tilesperheight = $self->getTilesPerHeight();

        # base dir image
        my $baseimage = File::Spec->catdir($self->getNewDataDir(), $self->getDirImage(), $ID);

        # base dir nodata
        my $basenodata = File::Spec->catdir($self->getNewDataDir(), $self->getDirNodata(), $ID);

        # TODO : metadata
        #   compression, type ...
        my $basemetadata = File::Spec->catdir($self->getNewDataDir(), $self->getDirMetadata(), $ID);

        # params to level
        my $params = {
            id                => $ID,
            order             => $order,
            dir_image         => File::Spec->abs2rel($baseimage, $self->{new_pyramid}->{desc_path}),
            dir_nodata        => File::Spec->abs2rel($basenodata, $self->{new_pyramid}->{desc_path}),
            dir_metadata      => undef,           # TODO,
            compress_metadata => undef,           # TODO  : raw  => TIFF_RAW_INT8,
            type_metadata     => "INT32_DB_LZW",  # FIXME : type => INT32_DB_LZW,
            size              => [$tilesperwidth, $tilesperheight],
            dir_depth         => $self->getDirDepth(),
            limit             => [undef, undef, undef, undef], # computed
        };
        my $objLevel = BE4::Level->new($params);

        if(! defined  $objLevel) {
            ERROR (sprintf "Can not create the level '%s' !", $ID);
            return FALSE;
        }

        $self->{levels}->{$ID} = $objLevel;
    }

    return TRUE;
}

#
=begin nd
method: addLevel

Store the Level object in the Pyramid object. Return an error if the level already exists.

Parameters
    levelID - TM identifiant
    objLevel - The BE4::Level object to store
=cut
sub addLevel {
    my $self = shift;
    my $levelID = shift;
    my $objLevel = shift;

    TRACE();
    
    if(! defined  $levelID || ! defined  $objLevel) {
        ERROR (sprintf "Level ID or Level object is undefined.");
        return FALSE;
    }
    
    if (ref ($objLevel) ne "BE4::Level") {
        ERROR (sprintf "We must have a Level object for the level $levelID.");
        return FALSE;
    }
    
    if (exists $self->{levels}->{$levelID}) {
        ERROR (sprintf "We have already a Level object for the level $levelID.");
        return FALSE;
    }

    $self->{levels}->{$levelID} = $objLevel;

    return TRUE;
}

#
=begin nd
method: updateTMLimits

Compare old extrems rows/columns of the given level with the news and update values.

Parameters:
    levelID - Level whose extrems have to be updated with following bbox
    bbox - [xmin,ymin,xmax,ymax], to update TM limits
=cut
sub updateTMLimits {
    my $self = shift;
    my ($levelID,@bbox) = @_;

    TRACE();
    
    # We calculate extrem TILES. x -> i = column; y -> j = row
    my $tm = $self->getTileMatrixSet->getTileMatrix($levelID);
    
    my $iMin = $tm->xToColumn($bbox[0]);
    my $iMax = $tm->xToColumn($bbox[2]);
    my $jMin = $tm->yToRow($bbox[3]);
    my $jMax = $tm->yToRow($bbox[1]);
    
    # order in updateExtremTiles : row min, row max, col min, col max
    $self->getLevel($levelID)->updateExtremTiles($jMin,$jMax,$iMin,$iMax);

}


####################################################################################################
#                              FUNCTIONS FOR WRITING PYRAMID'S ELEMENTS                            #
####################################################################################################

#
=begin nd
method: writeConfPyramid

Export the Pyramid object to XML format, write the pyramid's descriptor (pyr_desc_path/pyr_name_new.pyr). Use Level XML export. Levels are written in descending order, from worst to best resolution.

=cut
sub writeConfPyramid {
    my $self    = shift;

    TRACE;
    
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
    my $nodata = $self->getNodataValue();
    $strpyrtmplt =~ s/__NODATAVALUE__/$nodata/;
    #  
    my $interpolation = $self->getInterpolation();
    $strpyrtmplt =~ s/__INTERPOLATION__/$interpolation/;
    #  
    my $photometric = $self->getPhotometric;
    $strpyrtmplt =~ s/__PHOTOMETRIC__/$photometric/;
    
    my @levels = sort {$a->getOrder <=> $b->getOrder} ( values %{$self->getLevels});

    for (my $i = scalar @levels -1; $i >= 0; $i--) {
        # we write levels in pyramid's descriptor from the top to the bottom
        my $levelXML = $levels[$i]->getLevelToXML;
        $strpyrtmplt =~ s/<!-- __LEVELS__ -->\n/$levelXML/;
    }
    #
    $strpyrtmplt =~ s/<!-- __LEVELS__ -->\n//;
    $strpyrtmplt =~ s/^$//g;
    $strpyrtmplt =~ s/^\n$//g;
    #

    # TODO check the new template !
  
    my $filepyramid = $self->getNewDescriptorFile();    

    if (-f $filepyramid) {
        ERROR(sprintf "File Pyramid ('%s') exist, can not overwrite it ! ", $filepyramid);
        return FALSE;
    }
    #
    
    my $dir = dirname($filepyramid);
    if (! -d $dir) {
        DEBUG (sprintf "Create the pyramid's descriptor directory '%s' !", $dir);
        eval { mkpath([$dir]); };
        if ($@) {
            ERROR(sprintf "Can not create the pyramid's descriptor directory '%s' : %s !", $dir , $@);
            return FALSE;
        }
    }
    
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

#
=begin nd
method: writeCachePyramid

Write the Cache Directory Structure (CDS).

If ancestor:
* transpose old cache directories in the new cache (using the cache list).
* create symbolic links toward old cache tiles.
* create nodata tiles which are not in the old cache.
* create the new cache list (just with unchanged images).

If new pyramid:
* create an image directory for each level.
* create the nodata tile for each level.
* create the new cache list (just with unchanged images).

Parameter:
    forest - forest generated by process, to test if an image is among the new cache.
=cut
sub writeCachePyramid {
    my $self = shift;
    my $forest = shift;

    TRACE;

    my $oldcachepyramid = $self->getOldDataDir();
    my $newcachepyramid = $self->getNewDataDir();
    
    DEBUG(sprintf "%s to %s !",$oldcachepyramid , $newcachepyramid);
    my $dirimage      = $self->getDirImage();
    my $dirnodata     = $self->getDirNodata();
    my $dirmetadata   = undef; # TODO ?
    
    my $newcachelist = $self->getNewListFile();
    
    if (-f $newcachelist) {
        ERROR(sprintf "New cache list ('%s') exist, can not overwrite it ! ", $newcachelist);
        return FALSE;
    }
    
    my $dir = dirname($newcachelist);
    if (! -d $dir) {
        DEBUG (sprintf "Create the cache list directory '%s' !", $dir);
        eval { mkpath([$dir]); };
        if ($@) {
            ERROR(sprintf "Can not create the cache list directory '%s' : %s !", $dir , $@);
            return FALSE;
        }
    }
    
    my $NEWLIST;

    if (! open $NEWLIST, ">", $newcachelist) {
        ERROR(sprintf "Cannot open new cache list file : %s",$newcachelist);
        return FALSE;
    }
    
    my %oldCacheRoots;
    $oldCacheRoots{0} = $newcachepyramid;
    printf $NEWLIST "0=%s\n",$newcachepyramid;
    
    # search and create link for only new cache tile
    if (! $self->isNewPyramid) {
        
        my $OLDLIST;

        if (! open $OLDLIST, "<", $self->getOldListFile) {
            ERROR(sprintf "Cannot open old cache list file : %s",$self->getOldListFile);
            return FALSE;
        }
        
        
        while( defined( my $cacheRoot = <$OLDLIST> ) ) {
            chomp $cacheRoot;
            if ($cacheRoot eq "#") {
                # separator between caches' roots and images
                last;
            }
            
            $cacheRoot =~ s/\s+//g; # we remove all spaces
            my @Root = split(/=/,$cacheRoot,-1);
            
            if (scalar @Root != 2) {
                ERROR(sprintf "Bad formatted cache list (root definition) : %s",$cacheRoot);
                return FALSE;
            }
            
            # ID 0 is kept for the new pyramid root, all ID are incremented
            $oldCacheRoots{$Root[0]+1} = $Root[1];
            
            printf $NEWLIST "%s=%s\n",$Root[0]+1,$Root[1];
        }
        
        printf $NEWLIST "#\n";
        
        while( defined( my $oldtile = <$OLDLIST> ) ) {
            chomp $oldtile;
            
            # old tile path is split. Afterwards, only array will be used to compose paths
            my @directories = File::Spec->splitdir($oldtile);
            # @directories = [ RootID, dir_name, levelID, ..., XY.tif]
            #                    0        1        2      3  ... n
            
            # ID 0 is kept for the new pyramid root, all ID are incremented
            $directories[0]++;
            
            my $node = undef;
            
            if ($directories[1] ne $self->{dir_nodata}) {
                my $level = $directories[2];
                my $b36path = "";
                for (my $i = 3; $i < scalar @directories; $i++) {
                    $b36path .= $directories[$i]."/";
                }
                # Extension is removed
                $b36path =~ s/(\.tif|\.tiff|\.TIF|\.TIFF)//;
                my ($x,$y) = BE4::Base36->b36PathToIndices($b36path);
                $node = {level => $level, x => $x, y => $y};
            }
            
            if (! $forest->containsNode($node)) {
                # This image is not in the forest, it won't be modified by this generation.
                # We add it now to the list (real file path)
                printf $NEWLIST "%s\n", File::Spec->catdir(@directories);
            }
            
            # We replace root ID with the root path, to obtain a real path.
            if (! exists $oldCacheRoots{$directories[0]}) {
                ERROR(sprintf "Old cache list uses an undefined root ID : %s",$directories[0]);
                return FALSE;
            }
            $directories[0] = $oldCacheRoots{$directories[0]};
            $oldtile = File::Spec->catdir(@directories);
            
            # We remove the root to replace it by the new cache root
            shift @directories;
            my $newtile = File::Spec->catdir($newcachepyramid,@directories);

            #create folders
            my $dir = dirname($newtile);
            
            if (! -d $dir) {
                eval { mkpath([$dir]); };
                if ($@) {
                    ERROR(sprintf "Can not create the cache directory '%s' : %s !",$dir, $@);
                    return FALSE;
                }
            }

            if (! -f $oldtile || -l $oldtile) {
                ERROR(sprintf "File path in the cache list does not exist or is a link : %s.",$oldtile);
                return FALSE;
            }
            
            my $reloldtile = File::Spec->abs2rel($oldtile, $dir);

            my $result = eval { symlink ($reloldtile, $newtile); };
            if (! $result) {
                ERROR (sprintf "The tile '%s' can not be linked to '%s' (%s) ?",$reloldtile,$newtile,$!);
                return FALSE;
            }
        }
    } else {
        printf $NEWLIST "#\n";
    }
    
    my %levels = %{$self->getLevels};
    foreach my $objLevel (values %levels) {
        #create folders for data and nodata (metadata not implemented) if they don't exist
        
        my $dataDir = File::Spec->rel2abs($objLevel->getDirImage, $self->getNewDescriptorDir);
        if (! -d $dataDir) {
            eval { mkpath([$dataDir]); };
            if ($@) {
                ERROR(sprintf "Can not create the data directory '%s' : %s !", $dataDir , $@);
                return FALSE;
            }
        }
        
        my $nodataDir = File::Spec->rel2abs($objLevel->getDirNodata, $self->getNewDescriptorDir);
        if (! -d $nodataDir) {
            eval { mkpath([$nodataDir]); };
            if ($@) {
                ERROR(sprintf "Can not create the nodata directory '%s' : %s !", $nodataDir , $@);
                return FALSE;
            }
        }
        
        my $nodataTilePath = File::Spec->catfile($nodataDir,$self->{nodata}->getNodataFilename());
        if (! -e $nodataTilePath) {

            my $width = $self->getTileMatrixSet()->getTileWidth($objLevel->getID);
            my $height = $self->getTileMatrixSet()->getTileHeight($objLevel->getID);

            if (! $self->{nodata}->createNodata($nodataDir,$width,$height,$self->getCompression())) {
                ERROR (sprintf "Impossible to create the nodata tile for the level %i !",$objLevel->getID());
                return FALSE;
            }
            
            printf $NEWLIST "%s\n", File::Spec->catdir("0",$self->getDirNodata,$objLevel->getID(),$self->{nodata}->getNodataFilename());
        }
    }
    
    close $NEWLIST;

    return TRUE;
  
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

#################### New pyramid ####################

sub isNewPyramid {
    my $self = shift;
    return (! defined $self->getOldName);
}

sub getNewName {
    my $self = shift;    
    return $self->{new_pyramid}->{name};
}

#
=begin nd
method: getNewDescriptorFile

Returns:
    "pyr_desc_path/pyr_name.pyr", "/home/ign/descriptors/ORTHO.pyr"
=cut
sub getNewDescriptorFile {
    my $self = shift;    
    return File::Spec->catfile($self->{new_pyramid}->{desc_path}, $self->{new_pyramid}->{name}.".pyr");
}

#
=begin nd
method: getNewDescriptorDir

Returns:
    "pyr_desc_path", "/home/ign/descriptors"
=cut
sub getNewDescriptorDir {
    my $self = shift;    
    return $self->{new_pyramid}->{desc_path};
}

#
=begin nd
method: getNewListFile

Returns:
    "pyr_desc_path/pyr_name.list", "/home/ign/descriptors/ORTHO.list"
=cut
sub getNewListFile {
    my $self = shift;
    
    if (! defined $self->{new_pyramid}->{content_path}) {
        $self->{new_pyramid}->{content_path} =
            File::Spec->catfile($self->{new_pyramid}->{desc_path}, $self->{new_pyramid}->{name}.".list");
    }
    
    return $self->{new_pyramid}->{content_path};
}

#
=begin nd
method: getNewDataDir

Returns:
    "pyr_data_path/pyr_name", "/home/ign/pyramids/ORTHO"
=cut
sub getNewDataDir {
    my $self = shift;    
    return File::Spec->catfile($self->{new_pyramid}->{data_path}, $self->{new_pyramid}->{name});
}

#################### Old pyramid ####################

sub getOldName {
    my $self = shift;    
    return $self->{old_pyramid}->{name};
}

sub getOldDescriptorFile {
    my $self = shift;
    return File::Spec->catfile($self->{old_pyramid}->{desc_path}, $self->{old_pyramid}->{name}.".pyr");
}

sub getOldListFile {
    my $self = shift;
    
    if (! defined $self->{old_pyramid}->{content_path}) {
        $self->{old_pyramid}->{content_path} =
            File::Spec->catfile($self->{old_pyramid}->{desc_path}, $self->{old_pyramid}->{name}.".list");
    }
    
    return $self->{old_pyramid}->{content_path};
}

sub getOldDataDir {
    my $self = shift;
    return File::Spec->catfile($self->{old_pyramid}->{data_path}, $self->{old_pyramid}->{name});
}

#################### TMS ####################

sub getTmsName {
    my $self   = shift;
    return $self->{tms}->{name};
}

sub getTileMatrixSet {
    my $self = shift;
    return $self->{tms};
}

################ Directories ################

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

##### Pyramid's images' specifications ######

sub getInterpolation {
    my $self = shift;
    return $self->{pyrImgSpec}->getInterpolation;
}

sub getGamma {
    my $self = shift;
    return $self->{pyrImgSpec}->getGamma;
}

sub getCompression {
    my $self = shift;
    return $self->{pyrImgSpec}->getCompression;
}

sub getCompressionOption {
    my $self = shift;
    return $self->{pyrImgSpec}->getCompressionOption;
}

sub getCode {
    my $self = shift;
    return $self->{pyrImgSpec}->getFormatCode;
}

sub getPixel {
    my $self = shift;
    return $self->{pyrImgSpec}->getPixel;
}

sub getSamplesPerPixel {
    my $self = shift;
    return $self->getPixel->getSamplesPerPixel;
}

sub getPhotometric {
    my $self = shift;
    return $self->getPixel->getPhotometric;
}

sub getBitsPerSample {
    my $self = shift;
    return $self->getPixel->getBitsPerSample;
}

sub getSampleFormat {
    my $self = shift;
    return $self->getPixel->getSampleFormat;
}

################## Nodata ###################

sub getNodata {
    my $self = shift;
    return $self->{nodata};
}

sub getNodataValue {
    my $self = shift;
    return $self->{nodata}->getValue;
}

################### Levels ##################

sub getLevel {
    my $self = shift;
    my $levelID = shift;
    return $self->{levels}->{$levelID};
}

sub getLevels {
    my $self = shift;
    return $self->{levels};
}

#
=begin nd
method: getOrderfromID

Return the tile matrix order (integer) from the ID (string).

* 0 (bottom level, smallest resolution)
* NumberOfTM-1 (top level, biggest resolution).

Parameter:
    levelID - level ID whose we want to know the order.
=cut
sub getOrderfromID {
    my $self = shift;
    my $ID = shift;
    return $self->getTileMatrixSet()->getOrderfromID($ID);
}

#
=begin nd
method: getIDfromOrder

Return the tile matrix ID (string) from the order (integer).

Parameter:
    order - level's order whose we want to know the identifiant.
=cut
sub getIDfromOrder {
    my $self = shift;
    my $order = shift;
    return $self->getTileMatrixSet->getIDfromOrder($order);
}

sub getCacheImageSize {
    my $self = shift;
    my $level = shift;
    # size of cache image in pixel for a defined level !
    return ($self->getCacheImageWidth($level), $self->getCacheImageHeight($level));
}

sub getCacheImageWidth {
    my $self = shift;
    my $level = shift;
    # width of cache image in pixel for a defined level !
    return $self->getTilesPerWidth * $self->getTileMatrixSet->getTileWidth($level);
}

sub getCacheImageHeight {
    my $self = shift;
    my $level = shift;
    # height of cache image in pixel for a defined level !
    return $self->getTilesPerHeight * $self->getTileMatrixSet->getTileHeight($level);
}

sub getTileWidth {
    my $self = shift;
    my $level = shift;
    return $self->getTileMatrixSet()->getTileWidth($level);
}

sub getTileHeight {
    my $self = shift;
    my $level = shift;
    return $self->getTileMatrixSet()->getTileHeight($level);
}

sub getTilesPerWidth {
    my $self = shift;
    return $self->{image_width};
}

sub getTilesPerHeight {
    my $self = shift;
    return $self->{image_height};
}

#
=begin nd
method: getCacheNameOfImage

Return the image relative path, from the cache root directory (pyr_data_path). Tile's indices are convert in base 36, and split to give the path. Use BE4::Base36 tools.

Example:
    $objPyr->getCacheNameOfImage({level => "level_19",x => 4032,y => 18217},"data") returns "IMAGE/level_19/3E/42/01.tif"

Parameters:
    node - node whose name is wanted
    type - tile type : "data" or "metadata".
=cut
sub getCacheNameOfImage {
    my $self = shift;
    my $node = shift;
    my $type = shift;
    
    my $typeDir;
    if ($type eq "metadata"){
      $typeDir=$self->getDirMetadata();
    } else {
      $typeDir=$self->getDirImage();
    }
    
    my $base36path = BE4::Base36->indicesToB36Path($node->{x},$node->{y},$self->getDirDepth()+1);
    
    return File::Spec->catfile($typeDir, $node->{level}, $base36path.".tif"); 
}

#
=begin nd
method: getCachePathOfImage

Return the image absolute path. Use method getCacheNameOfImage.

Example:
    $objPyr->getCachePathOfImage({level => "level_19",x => 4032,y => 18217},"data")
    
    return "/home/ign/BDORTHO/IMAGE/level_19/3E/42/01.tif"

Parameters:
    node - node whose path is wanted.
    type - tile type : data or metadata.
    
See also:
    <getCacheNameOfImage>
=cut
sub getCachePathOfImage {
    my $self = shift;
    my $node = shift;
    my $type = shift;

    my $imageName = $self->getCacheNameOfImage($node, $type);

    return File::Spec->catfile($self->getNewDataDir, $imageName); 
}

1;
__END__

=head1 NAME

BE4::Pyramid - describe a cache (image specifications, levels, ...)

=head1 SYNOPSIS

    use BE4::Pyramid;
    
    # 1. a new pyramid
    
    my $params_options = {
        #
        pyr_name_new => "ORTHO_RAW_LAMB93_D075-O",
        pyr_desc_path => "/home/ign/DATA",
        pyr_data_path => "/home/ign/DATA",
        # 
        tms_name     => "LAMB93_10cm.tms",
        tms_path     => "/home/ign/TMS",
        #
        #
        dir_depth    => 2,
        dir_image    => "IMAGE",
        dir_nodata    => "NODATA",
        #
        image_width  => 16, 
        image_height => 16,
        # 
        color         => "FFFFFF",
        #
        compression         => "raw",
        bitspersample       => 8, 
        sampleformat        => "uint", 
        photometric         => "rgb", 
        samplesperpixel     => 3,
        interpolation       => "bicubic",
    };

    my $objPyr = BE4::Pyramid->new($params_options,$objProcess->{path_temp});
    
    $objPyr->writeConfPyramid(); # write pyramid's descriptor in /home/ign/ORTHO_RAW_LAMB93_D075-O.pyr
 
    $objP->writeCachePyramid($objProcess->{trees});  # root directory is "/home/ign/ORTHO_RAW_LAMB93_D075-O/"
    
    # 2. a update pyramid, with an ancestor
    
    my $params_options  = {
        #
        pyr_name_old        => "ORTHO_RAW_LAMB93_D075-O",
        pyr_data_path_old   => "/home/ign/DATA",
        pyr_desc_path_old   => "/home/ign/DATA",
        #
        pyr_name_new        => "ORTHO_RAW_LAMB93_D075-E",
        pyr_desc_path       => "/home/ign/DATA",
        pyr_data_path       => "/home/ign/DATA",
    };
    
    my $objPyr = BE4::Pyramid->new($params_options,$objProcess->{path_temp});
 
    $objPyr->writeConfPyramid(); # write pyramid's descriptor in /home/ign/ORTHO_RAW_LAMB93_D075-E.pyr
 
    $objPyr->writeCachePyramid($objProcess->{trees});  # root directory is "/home/ign/ORTHO_RAW_LAMB93_D075-E/"

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item new_pyramid

Hash which contains informations about the new pyramid. Keys are 'name' (new cache name, without extension .pyr), 'desc_path' (absolute path, where pyramid's descriptor will be written by default), 'data_path' (absolute path of root directory, where pyramid's cache will be written) and 'content_path' (file path, where to write the new cache's list).

=item old_pyramid

Hash which contains informations about the old pyramid (can be undefined). Keys are 'name' (old cache name, without extension .pyr), 'desc_path' (absolute path, where old pyramid's descriptor is), 'data_path' (absolute path of root directory, where old pyramid's cache is) and 'content_path' (old chache list file path).

=item dir_depth

Image's depth from the level directory. depth = 2 => /.../LevelID/SUB1/SUB2/IMG.tif

=item dir_image, dir_nodata, dir_metadata

Directories' name for images (data), nodata tiles and metadata (not implemented).

=item image_width, image_height

Number of tile in one image (the same for each level), widthwise and heightwise (often 16x16).

=item pyrImgSpec

A PyrImageSpec object. Contains all informations about images : sample format, compression, photometric...

=item tms

A TileMatrixSet object. Define destination SRS and the pyramid mosaiking.

=item nodata

A Nodata object. Contains the value, option nowhite (TRUE if data source contains nodata and have to be removed)

=item levels

An hash of Level objects.

=back

All paramaters are picked in configuration file for a new pyramid or in the old pyramid's descriptor (F<pyr_desc_path_old/pyr_name_old.pyr>) and cache for an update.

=head2 OUTPUT

=over 4

=item Pyramid's Descriptor (F<pyr_desc_path/pyr_name_new.pyr>)

The pyramid descriptor is written in pyr_desc_path contains global informations about the cache:

    <?xml version='1.0' encoding='US-ASCII'?>
    <Pyramid>
        <tileMatrixSet>LAMB93_10cm</tileMatrixSet>
        <format>TIFF_RAW_INT8</format>
        <channels>3</channels>
        <nodataValue>FFFFFF</nodataValue>
        <interpolation>bicubic</interpolation>
        <photometric>rgb</photometric>
            .
        (levels)
            .
    </Pyramid>

And details about each level.

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

For a new pyramid, all level between top and bottom are saved into.

For an update, all level of the existing pyramid are duplicated and we add new levels (between otp and bottom levels). For levels which are present in the old and the new pyramids, we update TMS limits.

=item Cache's List (F<pyr_desc_path/pyr_name_new.list>)

Header : index for caches' roots (used by paths, in the following list). 0 is always for the new cache.

    0=/home/theo/TEST/BE4/PYRAMIDS/ORTHO_RAW_LAMB93_D075-E
    1=/home/theo/TEST/BE4/PYRAMIDS/ORTHO_RAW_LAMB93_D075-O

A separator : #, necessary.

    #

Images' list : just real files, links' targets. 

    1/NODATA/11/nd.tif
    1/NODATA/7/nd.tif
    .
    .
    .
    1/IMAGE/16/00/1A/CV.tif
    1/IMAGE/17/00/2L/PR.tif
    .
    .
    .
    0/IMAGE/0/00/00/00.tif
    0/IMAGE/1/00/00/00.tif
    0/IMAGE/2/00/00/00.tif
    0/IMAGE/3/00/00/00.tif

The new cache's list is written by writeCachePyramid, using the old cache's list. The file is completed by Process, to add generated images.

=item Cache Directory Structure

For a new pyramid, the directory structure is empty, only the level directory for images and directory and tile for nodata are written.

    pyr_data_path/
            |_ pyr_name_new/
                    |__dir_image/
                            |_ ID_LEVEL0/
                            |_ ID_LEVEL1/
                            |_ ID_LEVEL2/
                    |__dir_nodata/
                            |_ ID_LEVEL0/
                                    |_ nd.tif
                            |_ ID_LEVEL1/
                                    |_ nd.tif
                            |_ ID_LEVEL2/
                                    |_ nd.tif

For an existing pyramid, the directory structure is duplicated to the new pyramid with all file linked, thanks to the old cache list.

    pyr_data_path/
            |__pyr_name_new/
                    |__dir_image/
                            |_ ID_LEVEL0/
                                |_ 00/
                                    |_ 7F/
                                    |_ 7G/
                                        |_ CV.tif 
                                |__ ...
                            |__ ID_LEVEL1/
                            |__ ID_LEVEL2/
                            |__ ...
                    |__dir_nodata/
                            |_ ID_LEVEL0/
                                    |_ nd.tif
                            |__ ID_LEVEL1/
                            |__ ID_LEVEL2/
                            |__ ...
                
    with
        ls -l CV.tif
        CV.tif -> /pyr_data_path_old/pyr_name_old/dir_image/ID_LEVEL0/7G/CV.tif
    and
        ls -l nd.tif
        nd.tif -> /pyr_data_path_old/pyr_name_old/dir_nodata/ID_LEVEL0/nd.tif

So be careful when you create a new tile in a update pyramid, you have to test if the link exists, to use image as a background.

=item Rule Image/Directory Naming :

We consider the upper left corner coordinates (X,Y). We know the ground size of a cache image (do not mistake for a tile) : it depends on the level (defined in the TMS).

    For the level:
        * Resolution (2 m)
        * Tile pixel size: tileWidth and tileHeight (256 * 256)
        * Origin (upper left corner): X0,Y0 (0,12000000)
    
    For the cache:
        * image tile size: image_width and image_height (16 * 16)

GroundWidth = tileWidth * image_width * Resolution

GroundHeight = tileHeight * image_height * Resolution

Index X = int (X-X0)/GroundWidth

Index Y = int (Y0-Y)/GroundHeight

Index X base 36 (write with 3 number) = X2X1X0 (example: 0D4)

Index Y base 36 (write with 3 number) = Y2Y1Y0 (example: 18Z)

The image path, from the data root is : dir_image/levelID/X2Y2/X1Y1/X0Y0.tif (example: IMAGE/level_15/01/D8/4Z.tif)

=back

=head1 LIMITATIONS AND BUGS

File name of pyramid must be with extension : pyr or PYR !

All levels must be continuous and unique !

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-TileMatrixSet.html">BE4::TileMatrixSet</A></li>
<li><A HREF="./lib-BE4-DataSource.html">BE4::DataSource</A></li>
<li><A HREF="./lib-BE4-Level.html">BE4::Level</A></li>
<li><A HREF="./lib-BE4-PyrImageSpec.html">BE4::PyrImageSpec</A></li>
<li><A HREF="./lib-BE4-NoData.html">BE4::NoData</A></li>
</ul>

=end html

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jean-philippe.bazonnais@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
