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
    - alphatop : alpha blending method. If images dont' own an alpha sample, white is considered as transparent. White is too the background color if we don't want alpha sample in the final images.
(see merge_transparency.png)
    - top : like replace, but more clever (we can avoid to remove data with nodata thanks to the masks)
(see merge_mask.png)

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
    mergeMethod - string - Precise way to merge images (in upper case).
    ontConfDir - string - Directory, where to write overlayNtiff configuration files.
    useMasks - boolean - Precise if masks have to be search and used. They are used if : user precise it (through the *use_masks" parameter OR the merge method is "TOP" OR user want masks in the final pyramid.

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
use JOINCACHE::Node;

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
    - Cache2work
    - OverlayNtiff
    - Work2cache
=cut
my $BASHFUNCTIONS   = <<'FUNCTIONS';

Cache2work () {
    local imgSrc=$1
    local imgDst=$2

    cache2work __c2w__ $imgSrc ${TMP_DIR}/$imgDst
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

OverlayNtiff () {
    local config=$1
    local inTemplate=$2

    overlayNtiff -f ${ONT_CONF_DIR}/$config __oNt__
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    rm -f ${TMP_DIR}/$inTemplate
    rm -f ${ONT_CONF_DIR}/$config
}

Work2cache () {

    local workImgName=$1
    local imgName=$2
    local workMskName=$3
    local mskName=$4

    local dir=`dirname ${PYR_DIR}/$imgName`

    if [ -r ${TMP_DIR}/$workImgName ] ; then rm -f ${PYR_DIR}/$imgName ; fi
    if [ ! -d $dir ] ; then mkdir -p $dir ; fi

    work2cache ${TMP_DIR}/$workImgName __t2tI__ ${PYR_DIR}/$imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    rm -f ${TMP_DIR}/$workImgName

    if [ $workMskName ] ; then

        dir=`dirname ${PYR_DIR}/$mskName`

        if [ -r ${TMP_DIR}/$workMskName ] ; then rm -f ${PYR_DIR}/$mskName ; fi
        if [ ! -d $dir ] ; then mkdir -p $dir ; fi

        work2cache ${TMP_DIR}/$workMskName __t2tM__ ${PYR_DIR}/$mskName
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
        rm -f ${TMP_DIR}/$workMskName
    fi
}

FUNCTIONS

################################################################################

BEGIN {}
INIT {

    %COMMANDS = (
        merge_method => ['REPLACE','ALPHATOP','MULTIPLY','TOP'],
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
|               merge_method - string - Way to merge several overlayed images (can be in lower case)
|               job_number - integer - Level of parallelization for scripts
|               path_temp - string - Directory, where to write proper temporary directory
|               path_temp_common - string - Directory, where to write common temporary directory
|               path_shell - string - Directory, where to write scripts
|               use_masks - string - TRUE or FALSE, to precise if masks have to be used
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
        ontConfDir => undef,
        
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
        if (! $self->isMergeMethod(uc($processParams->{merge_method}))) {
            ERROR (sprintf "Unknown 'merge_method' : %s !", $processParams->{merge_method});
            return undef;
        }
    }
    $self->{mergeMethod} = uc($processParams->{merge_method});

    ######### Use masks ? #########
    my $useMasks = $processParams->{use_masks};
    if ( $pyr->ownMasks() || (defined $useMasks && uc($useMasks) eq "TRUE")) {
        DEBUG("Masks will be used");
        $self->{useMasks} = TRUE;
    }

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

    ######### OverlayNtiff configuration directory #########

    $self->{ontConfDir} = $self->getCurrentScript()->getOntConfDir();

    return $self;
}

####################################################################################################
#                               Group: Process methods                                            #
####################################################################################################

=begin nd
Function: treatImage

3 possibilities:
    - the node owns just one source image and is compatible with the final cache -> we write a symbolic link : <makeLink>.
    - the node owns just one source image and is not compatible with the final cache -> we have just to convert image : compression and samples per pixel, commands are written in scripts : <mergeImages>.
    - the node owns several source images -> we use tool 'overlayNtiff', commands are written in scripts : <mergeImages>.

Parameters (list):
    node - <Node> - Node to treat
=cut
sub treatImage {
    my $self = shift;
    my $node = shift;

    my $code = "";

    my $path = $node->getPyramidName();

    my $finalImage = File::Spec->catfile($self->{pyramid}->getDirImage(TRUE), $node->getLevel(),$path);

    my $finalMask = undef;
    if ($self->{pyramid}->ownMasks()) {
        $finalMask = File::Spec->catfile($self->{pyramid}->getDirMask(TRUE), $node->getLevel(),$path);
    }

    # On affecte le script courant au noeud. Le script courant n'est pas incrémenté dans le cas d'un lien, car rien n'est écrit dans le script.
    $node->setScript($self->getCurrentScript());

    if ($node->getSourcesNumber() == 1) {
        my $sourceImage = $node->getSource(0);

        if ($sourceImage->{sourcePyramid}->isCompatible()) {
            # We can just make a symbolic link, no code in a script
            if (! $self->makeLink($finalImage, $sourceImage->{img})) {
                ERROR(sprintf "Cannot create link %s to %s", $sourceImage->{img}, $finalImage);
                return FALSE;
            }
            
        } else {
            # We have just one source image, but it is not compatible with the final cache
            # We need to transform it.
            if ( ! $self->mergeImages($node) ) {
                ERROR(sprintf "Cannot transform the image");
                return FALSE;
            }
            
            $node->writeInScript();
            $self->{currentScript} = ($self->{currentScript}+1)%($self->{splitsNumber});
        }

        # Export éventuel du masque : il est forcément dans le bon format, on peut donc le lier.
        if (exists $sourceImage->{msk} && $self->{pyramid}->ownMasks()) {
            # We can just make a symbolic link, no code in a script
            if (! $self->makeLink($finalMask, $sourceImage->{msk})) {
                ERROR(sprintf "Cannot create link %s to %s", $sourceImage->{msk}, $finalMask);
                return FALSE;
            }
        }
        
    } else {
        # We have several images, we merge them
        if ( ! $self->mergeImages($node) ) {
            ERROR(sprintf "Cannot merge the images");
            return FALSE;
        }
        
        $node->writeInScript($code);
        $self->{currentScript} = ($self->{currentScript}+1)%($self->{splitsNumber});
    }

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

    $self->storeInList($absoluteTarget);

    return TRUE;
}

=begin nd
Function: mergeImages

Write commands in the current script to merge N (N could be 1) images according to the merge method. We use *tiff2rgba* to convert into work format and *overlayNtiff* to merge. Masks are treated if needed. Code is store into the node.

If just one input image, overlayNtiff is used to change the image's properties (samples per pixel for example). Mask is not treated (masks have always the same properties and a symbolic link have been created).

Returns:
    A boolean, TRUE if success, FALSE otherwise.

Parameters (list):
    node - <Node> - Node to treat
=cut
sub mergeImages {
    my $self = shift;
    my $node = shift;

    my $code = "";
    my $LIST = $self->{list};
    my $nodeName = $node->getWorkBaseName();
    my $inNumber = $node->getSourcesNumber();

    #### Fichier de configuration ####
    my $oNtConfFilename = "$nodeName.txt";
    my $oNtConfFile = File::Spec->catfile($self->{ontConfDir},$oNtConfFilename);
    
    if (! open CFGF, ">", $oNtConfFile ) {
        ERROR(sprintf "Impossible de creer le fichier $oNtConfFile, en écriture.");
        return FALSE;
    }

    #### Sorties ####

    my $outImgName = $node->getWorkName("I");
    my $outImgPath = File::Spec->catfile($node->getScript->getTempDir, $outImgName);
    
    my $outMskPath = undef;
    # Pas de masque de sortie si on a juste une image : le masque a été lié symboliquement
    if ($self->{pyramid}->ownMasks() && $inNumber != 1) {
        $outMskPath = File::Spec->catfile($node->getScript->getTempDir, $node->getWorkName("M"));
    }
    
    printf CFGF $node->exportForOntConf($outImgPath, $outMskPath);
    
    #### Entrées ####
    my $inTemplate = $node->getWorkName("*_*");
    
    for (my $i = $inNumber - 1; $i >= 0; $i--) {
        # Les images sont dans l'ordre suivant : du dessus vers le dessous
        # Dans le fichier de configuration de overlayNtiff, elles doivent être dans l'autre sens, d'où la lecture depuis la fin.
        my $sourceImage = $node->getSource($i);

        my $inImgName = $node->getWorkName($i."_I");
        my $inImgPath = File::Spec->catfile($node->getScript->getTempDir, $inImgName);

        # Pretreatment
        my $format = $sourceImage->{sourcePyramid}->getFormatCode();
        my $spp = $sourceImage->{sourcePyramid}->getSamplesPerPixel();
        my $bps = $sourceImage->{sourcePyramid}->getBitsPerSample();

        $code .= sprintf "Cache2work %s $inImgName\n", $sourceImage->{img};

        my $inMskPath = undef;
        if (exists $sourceImage->{msk}) {
            my $inMskName = $node->getWorkName($i."_M");
            $inMskPath = File::Spec->catfile($node->getScript->getTempDir, $inMskName);
            $code .= sprintf "Cache2work %s $inMskName\n", $sourceImage->{msk};
        }

        printf CFGF $node->exportForOntConf($inImgPath, $inMskPath);
    }

    close CFGF;

    $code .= "OverlayNtiff $oNtConfFilename $inTemplate\n";

    # Final location writting
    my $imgCacheName = File::Spec->catfile($self->{pyramid}->getDirImage(), $node->getLevel(), $node->getPyramidName());
    $code .= sprintf ("Work2cache $outImgName $imgCacheName");
    printf ($LIST "0/%s\n", $imgCacheName);
    
    # Pas de masque à tuiler si on a juste une image : le masque a été lié symboliquement
    if (defined $outMskPath && $inNumber != 1) {
        my $outMskName = $node->getWorkName("M");
        my $mskCacheName = File::Spec->catfile($self->{pyramid}->getDirMask(), $node->getLevel(), $node->getPyramidName());
        $code .= sprintf (" $outMskName $mskCacheName");
        printf ($LIST "0/%s\n", $mskCacheName);
    }

    $code .= "\n\n";

    $node->setCode($code);
    
    return TRUE;
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

    my $mm = $self->{mergeMethod};
    my $spp = $pyr->getSamplesPerPixel;
    my $ph = $pyr->getPhotometric;
    my $nd = $self->{pyramid}->getNodata()->getValue();

    ######## congigure overlayNtiff ########
    
    my $conf_oNt = "-c zip -s $spp -p $ph -b $nd ";
    
    if ($mm eq "REPLACE") {
        # Dans le cas REPLACE, overlayNtiff est utilisé uniquement pour modifier les caractéristiques
        # d'une image (passer en noir et blanc par exemple)
        # overlayNtiff sera appelé sur une image unique, qu'on "fusionne" en mode TOP
        $conf_oNt .= "-m TOP ";
    } else {
        $conf_oNt .= "-m $mm ";        
    }

    if ($mm eq "ALPHATOP") {
        $conf_oNt .= "-t 255,255,255 ";
    }

    $configuredFunc =~ s/__oNt__/$conf_oNt/;

    # calculate image pixel size

    my $tileWidth = $pyr->getTileMatrixSet->getTileWidth;
    my $tileHeight = $pyr->getTileMatrixSet->getTileHeight;

    my $imgHeight = $pyr->getTilesPerHeight * $tileHeight;

    ######## work2cache ########

    my $conf_c2w = "-c zip";
    $configuredFunc =~ s/__c2w__/$conf_c2w/;

    ######## configure work2cache ########
    my $conf_t2t = "";

    # pour les images
    $conf_t2t .= sprintf "-c %s ", $pyr->getCompression();
    $conf_t2t .= "-t $tileWidth $tileHeight";

    $configuredFunc =~ s/__t2tI__/$conf_t2t/;

    # pour les masques
    $conf_t2t = "-c zip -t $tileWidth $tileHeight";
    $configuredFunc =~ s/__t2tM__/$conf_t2t/;

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

# Function: useMasks
sub useMasks {
    my $self = shift;
    return $self->{useMasks};
}

=begin nd
Function: isMergeMethod

Check merge method value. Possible values: 'REPLACE','ALPHATOP','MULTIPLY','TOP'.

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
    path - string - Path to write in the cache list : root will be factorize.
=cut
sub storeInList {
    my $self = shift;
    my $path = shift;

    # We extract from the old tile path, the cache name (without the old cache root path)
    my @directories = File::Spec->splitdir($path);
    # $realName : abs_datapath/IMAGE/level/XY/XY/XY.tif
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

# Function: getCurrentScript
sub getCurrentScript {
    my $self = shift;
    return $self->{scripts}->[$self->{currentScript}];
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all commands' informations. Useful for debug.

Example:
    (start code)
    Object JOINCACHE::Process :
         Merge method : MULTIPLY
         Use masks
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;

    my $export = "";

    $export .= "\nObject JOINCACHE::Process :\n";
    $export .= sprintf "\t Merge method : %s\n", $self->{mergeMethod};
    $export .= "\t Use masks\n" if $self->{useMasks};
    $export .= "\t Doesn't use masks\n" if (! $self->{useMasks});

    return $export;
}
  
1;
__END__
