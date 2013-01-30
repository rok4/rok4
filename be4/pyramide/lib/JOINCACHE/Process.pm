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

################################################################################

=begin nd
File: Process.pm

Class: JOINCACHE::Process

Configure, assemble commands used to generate merged pyramid's images, from source pyramids' images. Manage scripts and the final pyramid's content list.

When we have several source images, we have to merge them. 3 ways are available:
    - replace : just the top pyramid's image is kept. That's why order in the *composition* section in configuration is important.
(see merge_replace.png)
    - multiply : samples are multiplied.
(see merge_multiply.png)
    - transparency : alpha blending method. If images dont' own an alpha sample, white is considered as transparent. White is too the background color if we don't want alpha sample in the final images.
(see merge_transparency.png)
    - masks : like replace, but more clever (we can avoid to remove data with nodata thanks to the masks)

Using:
    (start code)
    use JOINCACHE::Process;

    # Process object creation
    my $objProcess = JOINCACHE::Process->new(
        $objPyramid, # BE4::Pyramid object
        $process_params, # Process section as an hash
    );
    (end code)

Attributes:
    pyramid - <BE4::Pyramid> - Allowed to know output format specifications and configure commands.
    mergeMethod - string - Precise way to merge images.

    list - stream - Stream to the pyramid's content list.
    roots - integer hash - Index all pyramid's roots. Values : root - string => ID - integer
    
    splitsNumber - integer - Number of script used for work parallelization.
    scripts - <JOINCACHE::Script> array - *splitsNumber* scripts, which will be executed to generate the pyramid.
    currentScript - integer - Work sharing between splits is the round robin method. We know in which script we have to write the next commands.
=cut

################################################################################

package JOINCACHE::Process;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

use JOINCACHE::Script;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################

use constant TRUE  => 1;
use constant FALSE => 0;

# Variable: COMMANDS
# Define allowed merge methods.
my %COMMANDS;

####################################################################################################
#                                       Group: Commands' calls                                     #
####################################################################################################

=begin nd
Constant: BASHFUNCTIONS
Define bash functions, used to factorize and reduce scripts :
    - PrepareImage
    - Cache2work
    - OverlayNtiff
    - Work2cache
=cut
my $BASHFUNCTIONS   = <<'FUNCTIONS';

PrepareImage () {
  local imgSrc=$1
  local imgDst=$2
  local png=$3
  local opt=$4

  if [ $png ] ; then
    mkdir ${TMP_DIR}/Untiled_PNG
    untile $imgSrc ${TMP_DIR}/Untiled_PNG/
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    montage __montageIn__ ${TMP_DIR}/Untiled_PNG/*.png __montageOut__ $opt ${TMP_DIR}/Untiled_PNG.tif
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    rm -rf ${TMP_DIR}/Untiled_PNG

    tiff2rgba -c zip ${TMP_DIR}/Untiled_PNG.tif $imgDst
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
  else
    tiff2rgba -c zip $imgSrc $imgDst
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
  fi

}

Cache2work () {
  local imgSrc=$1
  local imgDst=$2
  local png=$3
  local opt=$4

  if [ $png ] ; then
    mkdir ${TMP_DIR}/Untiled_PNG
    untile $imgSrc ${TMP_DIR}/Untiled_PNG/
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    montage __montageIn__ ${TMP_DIR}/Untiled_PNG/*.png __montageOut__ $opt $imgDst
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    rm -rf ${TMP_DIR}/Untiled_PNG
  else
    tiffcp __tcp__ $imgSrc $imgDst
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
  fi

}

OverlayNtiff () {
  local inputs=$1

  overlayNtiff __oNt__ -input $inputs -output ${TMP_DIR}/result.tif
  if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
  rm -f $config
  if [ $bg ] ; then
    rm -f $bg
  fi
}

Work2cache () {
  local work=$1
  local cache=$2

  local dir=`dirname $cache`

  if [ -r $cache ] ; then rm -f $cache ; fi
  if [ ! -d  $dir ] ; then mkdir -p $dir ; fi

  if [ -f $work ] ; then
    tiff2tile $work __t2t__  $cache
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
  fi
}

FUNCTIONS

################################################################################

BEGIN {}
INIT {

    %COMMANDS = (
        merge_method => ['replace','transparency','multiply'],
    );
    
}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Process constructor. Bless an instance.

Parameters (list):
    pyr - <BE4::Pyramid> - Images pyramid to generate
    processParams - hash - Parameters in the section *process*
|               merge_method - string - Way to merge several overlayed images
|               job_number - integer - Level of parallelization for scripts
|               path_temp - string - Directory, where to write proper temporary directory
|               path_temp_common - string - Directory, where to write common temporary directory
|               path_shell - string - Directory, where to write scripts
=cut
sub new {
    my $this = shift;
    my $pyr = shift;
    my $processParams = shift;
    
    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        pyramid => undef,
        mergeMethod => undef,
        
        currentScript => 0,
        scripts => [],
        splitsNumber => undef,
        
        list => undef,
        roots => {}
    };
    bless($self, $class);

    TRACE;

    ######### Pyramid #########
    if (! defined $pyr || ref ($pyr) ne "BE4::Pyramid") {
        ERROR("Can not load Pyramid !");
        return undef;
    }
    $self->{pyramid} = $pyr;

    ######### Merge method #########
    if (! exists $processParams->{merge_method} || ! defined $processParams->{merge_method}) {
        ERROR ("'merge_method' required !");
        return undef;
    } else {
        if (! $self->isMergeMethod($processParams->{merge_method})) {
            ERROR (sprintf "Unknown 'merge_method' : %s !", $processParams->{merge_method});
            return undef;
        }
    }
    $self->{mergeMethod} = $processParams->{merge_method};

    ######### Scripts #########

    if (! exists $processParams->{job_number} || ! defined $processParams->{job_number}) {
        ERROR("Parameter required : 'job_number' in section 'Process' !");
        return undef;
    }
    $self->{splitsNumber} = $processParams->{job_number};

    if (! exists $processParams->{path_temp} || ! defined $processParams->{path_temp}) {
        ERROR("Parameter required : 'path_temp' in section 'Process' !");
        return undef;
    }

    if (! exists $processParams->{path_temp_common} || ! defined $processParams->{path_temp_common}) {
        ERROR("Parameter required : 'path_temp_common' in section 'Process' !");
        return undef;
    }

    # Ajout du nom de la pyramide aux dossiers temporaires (pour distinguer de ceux des autres générations)
    $processParams->{path_temp} = File::Spec->catdir($processParams->{path_temp}, $self->{pyramid}->getNewName);
    $processParams->{path_temp_common} = File::Spec->catdir($processParams->{path_temp_common}, $self->{pyramid}->getNewName);

    my $functions = $self->configureFunctions();

    # Création des scripts
    for (my $i = 0; $i < $self->{splitsNumber}; $i++) {
        my $scriptID = sprintf "SCRIPT_%s",$i+1;

        my $script = JOINCACHE::Script->new({
            id => $scriptID,
            tempDir => $processParams->{path_temp},
            commonTempDir => $processParams->{path_temp_common},
            scriptDir => $processParams->{path_shell},
            executedAlone => FALSE
        });

        if (! defined $script) {
            ERROR("Cannot create a Script object !");
            return undef;
        }

        $script->prepare($self->{pyramid}->getNewDataDir(), $functions);

        push @{$self->{scripts}},$script;

    }

    ######### Content list #########

    my $pyramidList = $self->{pyramid}->getNewListFile;
    if (-f $pyramidList) {
        ERROR(sprintf "New pyramid list ('%s') exist, can not overwrite it ! ", $pyramidList);
        return undef;
    }

    my $dir = dirname($pyramidList);
    if (! -d $dir) {
        DEBUG (sprintf "Create the pyramid list directory '%s' !", $dir);
        eval { mkpath([$dir]); };
        if ($@) {
            ERROR(sprintf "Can not create the pyramid list directory '%s' : %s !", $dir , $@);
            return undef;
        }
    }

    my $LIST;
    if (! open $LIST, ">", $pyramidList) {
        ERROR(sprintf "Cannot open new pyramid list file : %s",$pyramidList);
        return undef;
    }
    printf $LIST "#\n";

    $self->{list} = $LIST;

    return $self;
}

####################################################################################################
#                               Group: Process methods                                            #
####################################################################################################

=begin nd
Function: treatImage

3 possibilities:
    - the image is present in just one pyramid or merge_method = replace, and is compatible with the final cache -> we write a symbolic link.
    - the image is present in just one pyramid or merge_method = replace, and is not compatible with the final cache -> we have just to convert image : compression and samples per pixel, commands are written in scripts.
    - the image is preent in seve(ral pyramids and merge method is not 'replace' -> we use tool 'overlayNtiff', commands are written in scripts.

Parameters (list):
    finaleImage - string - Absolute path to the finale pyramid's image (to generate)
    sourceImages - hash array - Source images to merge
|               img - string - Absolute path to the image
|               sourcePyramid - <JOINCACHE::SourcePyramid> - Pyramid which image belong to

See also:
    <makeLink>, <transformImage>, <mergeImages>
=cut
sub treatImage {
    my $self = shift;
    my $finaleImage = shift;
    my @sourceImages = @_;

    my $code = "";

    if (scalar @sourceImages == 1 && $sourceImages[0]{sourcePyramid}->isCompatible() ) {
        # We can just make a symbolic link, no code in a script
        if (! $self->makeLink($finaleImage, $sourceImages[0]{img})) {
            ERROR(sprintf "Cannot create link between %s and %s",$finaleImage,$sourceImages[0]);
            return FALSE;
        }
        
        return TRUE;
    }

    # We have to write commands in a script

    # Use environment to short path, we remove the pyramid's data root
    my $pyramidRoot = $self->{pyramid}->getNewDataDir;
    $finaleImage =~ s/$pyramidRoot\/?//;

    if (scalar @sourceImages == 1) {
        # We have just one source image, but it is not compatible with the final cache
        # We need to transform it.
        $code = $self->transformImage($finaleImage, $sourceImages[0]);
        if ( $code eq "" ) {
            ERROR(sprintf "Cannot transform the image %s",$sourceImages[0]{img});
            return FALSE;
        }
    } else {
        # We have several images, we merge them
        $code = $self->mergeImages($finaleImage,@sourceImages);
        if ( $code eq "" ) {
            ERROR("Cannot merge the images");
            return FALSE;
        }
    }

    # Storages
    $self->storeInList(TRUE, $finaleImage);
    $self->printInScript($code);

    return TRUE;

}

=begin nd
Function: makeLink

Create a symbolic link in the final cache, to a source image. Return TRUE if success, FALSE otherwise.

Parameters:
    finaleImage - string - Absolute path of the final image
    sourceImage - string - Absolute source image's path, to link
=cut
sub makeLink {
    my $self = shift;
    my $finaleImage = shift;
    my $sourceImage = shift;

    TRACE();

    #create folders
    my $finalDir = dirname($finaleImage);
    eval { mkpath([$finalDir]); };
    if ($@) {
        ERROR(sprintf "Can not create the cache directory '%s' : %s !",$finalDir, $@);
        return FALSE;
    }

    my $relativeTarget = undef;
    my $absoluteTarget = undef;

    if (-f $sourceImage && ! -l $sourceImage) {
        $relativeTarget = File::Spec->abs2rel($sourceImage,$finalDir);
        $absoluteTarget = $sourceImage;
    }
    elsif (-f $sourceImage && -l $sourceImage) {
        my $linked   = File::Spec::Link->linked($sourceImage);
        $absoluteTarget = File::Spec::Link->full_resolve($linked);
        $relativeTarget = File::Spec->abs2rel($absoluteTarget, $finalDir);
    } else {
        ERROR(sprintf "The tile '%s' is not a file or a link in '%s' !",basename($sourceImage),dirname($sourceImage));
        return FALSE;
    }

    if (! defined $relativeTarget) {
        ERROR (sprintf "The link '%s' can not be resolved in '%s' ?", basename($sourceImage), dirname($sourceImage));
        return FALSE;
    }

    my $result = eval { symlink ($relativeTarget, $finaleImage); };
    if (! $result) {
        ERROR (sprintf "The tile '%s' can not be linked to '%s' (%s) ?", $relativeTarget,$finaleImage,$!);
        return FALSE;
    }

    $self->storeInList(FALSE, $absoluteTarget);

    return TRUE;
}

=begin nd
Function: transformImage

Write commands in the current script to transform an image in an other format, in the final cache. Can change, compression and samples per pixel.

Returns:
    A string, the code to write in the script to transform image. An empty string if failure.

Parameters (list):
    finaleImage - string - Relative path to the finale pyramid's image (to generate), from the pyramid's root
    sourceImage - hash - Source image to transform
|               img - string - Absolute path to the image
|               sourcePyramid - <JOINCACHE::SourcePyramid> - Pyramid which image belong to
    
=cut
sub transformImage {
    my $self = shift;
    my $finaleImage = shift;
    my $sourceImage = shift;

    my $code = '';

    # Pretreatment

    my $format = $sourceImage->{sourcePyramid}->getFormatCode();
    my $sppSource = $sourceImage->{sourcePyramid}->getSamplesPerPixel();

    if ($format =~ m/PNG/) {
        if ($sppSource == 4) {
            $code .= sprintf "Cache2work %s \${TMP_DIR}/img.tif png \"-type TrueColorMatte -background none\"\n", $sourceImage->{img};
        } elsif ($sppSource == 3) {
            $code .= sprintf "Cache2work %s \${TMP_DIR}/img.tif png \"-type TrueColorMatte\"\n", $sourceImage->{img};
        } elsif ($sppSource == 1) {
            $code .= sprintf "Cache2work %s \${TMP_DIR}/img.tif png \"-type Grayscale\"\n", $sourceImage->{img};
        } else {
            ERROR (sprintf "Samplesperpixel (%s) not supported ", $sppSource);
            return "";
        }
    } else {
        $code .= sprintf "Cache2work %s \${TMP_DIR}/img.tif\n", $sourceImage->{img};
    }

    my $sppFinal = $self->{pyramid}->getSamplesPerPixel();

    # We transform image with the samples per pixel of the final cache
    if ($sppFinal == 3) {
        $code .= "tiff2rgba -c none -n \${TMP_DIR}/img.tif \${TMP_DIR}/transformedImg.tif\n";
    }
    elsif ($sppFinal == 4) {
        $code .= "tiff2rgba -c none \${TMP_DIR}/img.tif \${TMP_DIR}/transformedImg.tif\n";
    }
    elsif ($sppFinal == 1) {
        $code .= "convert \${TMP_DIR}/img.tif -colors 256 -colorspace gray -depth 8 \${TMP_DIR}/transformedImg.tif\n";
    }

    # Final location writting
    $code .= sprintf ("Work2cache \${TMP_DIR}/transformedImg.tif \${PYR_DIR}/%s\n\n", $finaleImage);

    return $code;
}

=begin nd
Function: mergeImages

Write commands in the current script to merge N images according to the merge method. We use *tiff2rgba* to convert into work format and *overlayNtiff* to merge.

Returns:
    A string, the code to write in the script to transform image. An empty string if failure.

Parameters (list):
    finaleImage - string - Relative path to the finale pyramid's image (to generate), from the pyramid's root
    sourceImages - hash array - Source images to merge
|               img - string - Absolute path to the image
|               sourcePyramid - <JOINCACHE::SourcePyramid> - Pyramid which image belong to
=cut
sub mergeImages {
    my $self = shift;
    my $finaleImage = shift;
    my @sourceImages = @_;

    my $code = "";

    # Pretreatment

    for (my $i = 0; $i < scalar @sourceImages; $i++) {

        my $format = $sourceImages[$i]->{sourcePyramid}->getFormatCode();
        my $spp = $sourceImages[$i]->{sourcePyramid}->getSamplesPerPixel();

        if ($format =~ m/PNG/) {
            if ($spp == 4) {
                $code .= sprintf ("PrepareImage %s \${TMP_DIR}/img%s.tif png \"-background none\"\n", $sourceImages[$i]{img} ,$i);
            } elsif ($spp == 3) {
                $code .= sprintf ("PrepareImage %s \${TMP_DIR}/img%s.tif png \"-type TrueColorMatte\"\n", $sourceImages[$i]{img} ,$i);
            } elsif ($spp == 1) {
                $code .= sprintf ("PrepareImage %s \${TMP_DIR}/img%s.tif png \"-type Grayscale\"\n", $sourceImages[$i]{img} ,$i);
            } else {
                ERROR (sprintf "Samplesperpixel (%s) not supported ", $spp);
                return "";
            }
        } else {
            $code .= sprintf ("PrepareImage %s \${TMP_DIR}/img%s.tif\n", $sourceImages[$i]{img} ,$i);
        }
    }

    # Superposition
    my $inputs = "";

    for (my $i = scalar @sourceImages - 1; $i >= 0; $i--) {
        $inputs .= sprintf " \${TMP_DIR}/img%s.tif ",$i;
    }

    $code .= "OverlayNtiff \"$inputs\"\n";

    # Final location writting
    $code .= sprintf ("Work2cache \${TMP_DIR}/result.tif \${PYR_DIR}/%s\n\n", $finaleImage);

    return $code;
}

####################################################################################################
#                                Group: Configuration method                                       #
####################################################################################################

=begin nd
Function: configureFunctions

Configure bash functions to write in scripts' header thanks to pyramid's components.
=cut
sub configureFunctions {
    my $self = shift;

    TRACE;

    my $pyr = $self->{pyramid};
    my $configuredFunc = $BASHFUNCTIONS;

    # congigure overlayNtiff
    my $conf_oNt = "";

    $conf_oNt .= sprintf "-mode %s -transparent 255,255,255 -opaque 255,255,255 -channels %s",
        $self->{mergeMethod}, $self->{pyramid}->getSamplesPerPixel;

    $configuredFunc =~ s/__oNt__/$conf_oNt/;

    # calculate image pixel size

    my $tileWidth = $self->{pyramid}->getTileMatrixSet->getTileWidth;
    my $tileHeight = $self->{pyramid}->getTileMatrixSet->getTileHeight;

    my $imgHeight = $self->{pyramid}->getTilesPerHeight * $tileHeight;

    # congigure tiffcp
    my $conf_tcp = "";

    $conf_tcp .= "-s -r $imgHeight ";

    $configuredFunc =~ s/__tcp__/$conf_tcp/;

    # configure montage
    my $conf_montageIn = "";

    $conf_montageIn .= sprintf "-geometry %sx%s ",$tileWidth,$tileHeight;
    $conf_montageIn .= sprintf "-tile %sx%s",$self->{pyramid}->getTilesPerWidth,$self->{pyramid}->getTilesPerHeight;

    $configuredFunc =~ s/__montageIn__/$conf_montageIn/g;

    my $conf_montageOut = "";

    $conf_montageOut .= sprintf "-depth %s ",$self->{pyramid}->getBitsPerSample;

    $conf_montageOut .= "-define tiff:rows-per-strip=$imgHeight -compress Zip";

    $configuredFunc =~ s/__montageOut__/$conf_montageOut/g;

    # congigure tiff2tile
    my $conf_t2t = "";

    $conf_t2t .= sprintf "-c %s ", $self->{pyramid}->getCompression();

    $conf_t2t .= sprintf "-p %s ",$self->{pyramid}->getPhotometric();
    $conf_t2t .= "-t $tileWidth $tileHeight ";
    $conf_t2t .= sprintf "-b %s ",$self->{pyramid}->getBitsPerSample();
    $conf_t2t .= sprintf "-a %s ",$self->{pyramid}->getSampleFormat();
    $conf_t2t .= sprintf "-s %s ",$self->{pyramid}->getSamplesPerPixel();

    $configuredFunc =~ s/__t2t__/$conf_t2t/;

    return $configuredFunc;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getMergeMethod
sub getMergeMethod {
    my $self = shift;
    return $self->{pyramid}->getNodata; 
}

=begin nd
Function: isMergeMethod

Check merge method value. Possible values: 'replace','transparency','multiply'.

Parameters (list):
    mergeMethod - string - Merge method value

=cut
sub isMergeMethod {
    my $self = shift;
    my $mergeMethod = shift;

    TRACE;

    return FALSE if (! defined $mergeMethod);

    foreach (@{$COMMANDS{merge_method}}) {
        return TRUE if ($mergeMethod eq $_);
    }
    return FALSE;
}

####################################################################################################
#                                   Group: List methods                                            #
####################################################################################################

=begin nd
Function: storeInList

Writes file's path in the pyramid's content list and store the root.

Parameters:
    isNewImage - boolean - If TRUE, root is already remove and ID have to be '0'.
    path - string - Path to write in the cache list : root will be factorize.
=cut
sub storeInList {
    my $self = shift;
    my $isNewImage = shift;
    my $path = shift;

    if ($isNewImage) {
        $path = "0/" . $path;
    } else {
        # We extract from the old tile path, the cache name (without the old cache root path)
        my @directories = File::Spec->splitdir($path);
        # $realName : abs_datapath/dir_image/level/XY/XY/XY.tif
        #                             -5      -4  -3 -2   -1
        #                     => -(3 + dir_depth)

        my $deb = -3 - $self->{pyramid}->getDirDepth;

        my @indexName = ($deb..-1);
        my @indexRoot = (0..@directories+$deb-1);

        my $name = File::Spec->catdir(@directories[@indexName]);
        my $root = File::Spec->catdir(@directories[@indexRoot]);

        my $rootID;
        if (exists $self->{roots}->{$root}) {
            $rootID = $self->{roots}->{$root};
        } else {
            $rootID = scalar (keys %{$self->{roots}}) + 1;
            $self->{roots}->{$root} = $rootID;
        }

        $path = "$rootID/$name";
    }

    my $STREAM = $self->{list};
    printf ($STREAM "%s\n", $path);
}

=begin nd
Function: writeRootsInList

Write roots at the top of the cache list.
=cut
sub writeRootsInList {
    my $self = shift;

    # We write at the top of the list file, caches' roots, using Tie library

    my @LIST;
    if (! tie @LIST, 'Tie::File', $self->{pyramid}->getNewListFile) {
        ERROR(sprintf "Cannot open '%s' with Tie librairy", $self->{pyramid}->getNewListFile);
        return FALSE;
    }

    while( my ($root,$rootID) = each( %{$self->{roots}} ) ) {
        unshift @LIST,(sprintf "%s=%s\n",$rootID,$root);
    }

    # Root of the new cache (first position)
    unshift @LIST,(sprintf "0=%s\n", $self->{pyramid}->getNewDataDir);

    untie @LIST;
}

####################################################################################################
#                                  Group : Scripts methods                                         #
####################################################################################################

=begin nd
Function: closeScripts

Close all scripts and the list.
=cut
sub closeStreams {
    my $self = shift;    

    for (my $i = 0; $i < $self->{splitsNumber}; $i++) {
        $self->{scripts}[$i]->close();
    }

    $self->{list}->close();

    return TRUE;
}

=begin nd
Function: printInScript

Writes commands in the current script. Round robin method is used to fill different scripts.

Parameters (list):
    code - code to write in the current script.
=cut
sub printInScript {
    my $self = shift;
    my $code = shift;

    $self->{scripts}[$self->{currentScript}]->write($code);
    $self->{currentScript} = ($self->{currentScript}+1)%($self->{splitsNumber});
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all commands' informations. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;

    my $export = "";

    $export .= "\nObject JOINCACHE::Process :\n";
    $export .= sprintf "\t Merge method\n", $self->{mergeMethod};

    return $export;
}
  
1;
__END__
