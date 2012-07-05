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

package BE4::Process;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

use BE4::Tree;
use BE4::Harvesting;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constantes
# Booleans
use constant TRUE  => 1;
use constant FALSE => 0;
# Commands
use constant RESULT_TEST => "if [ \$? != 0 ] ; then echo \$0 : Erreur a la ligne \$(( \$LINENO - 1)) >&2 ; exit 1; fi\n";
use constant CACHE_2_WORK_PRG => "tiffcp -s";
use constant WORK_2_CACHE_PRG => "tiff2tile";
use constant MERGE_N_TIFF     => "mergeNtiff";
use constant MERGE_4_TIFF     => "merge4tiff";
use constant UNTILE           => "untile";
# Commands' weights
use constant MERGE4TIFF_W => 1;
use constant MERGENTIFF_W => 4;
use constant CACHE2WORK_PNG_W => 3;
use constant WGET_W => 35;
use constant TIFF2TILE_W => 0;
use constant TIFFCP_W => 0;

################################################################################
# Global

my $BASHFUNCTIONS   = <<'FUNCTIONS';

Wms2work () {
  local img_dst=$1
  local url=$2
  local count=0; local wait_delay=60
  while :
  do
    let count=count+1
    wget --no-verbose -O $img_dst $url 
    if tiffck $img_dst ; then break ; fi
    echo "Failure $count : wait for $wait_delay s"
    sleep $wait_delay
    let wait_delay=wait_delay*2
    if [ 3600 -lt $wait_delay ] ; then 
      let wait_delay=3600
    fi
  done
}

Cache2work () {
  local imgSrc=$1
  local workName=$2
  local png=$3

  if [ $png ] ; then
    cp $imgSrc $workName.tif
    mkdir $workName
    untile $workName.tif $workName/
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    montage -geometry 256x256 -tile 16x16 $workName/*.png __montage__ -define tiff:rows-per-strip=4096 $workName.tif
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    rm -rf $workName/
  else
    tiffcp __tcp__ $imgSrc $workName.tif
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
  fi

}

Work2cache () {
  local work=$1
  local cache=$2
  
  local dir=`dirname $cache`
  
  if [ -r $cache ] ; then rm -f $cache ; fi
  if [ ! -d  $dir ] ; then mkdir -p $dir ; fi
  
  tiff2tile $work __t2t__  $cache
  if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

MergeNtiff () {
  local type=$1
  local config=$2
  local bg=$3
  mergeNtiff -f $config -t $type __mNt__
  if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
  rm -f $config
  if [ $bg ] ; then
    rm -f $bg
  fi
}

FUNCTIONS

################################################################################

BEGIN {}
INIT {}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    * pyramid    => undef, # Pyramid object
    * nodata => undef, # NoData object
    * trees => [], # array of Tree objects
    * scripts   => [], # name of each jobs (split and finisher)
    * codes       => [], # code of each jobs (split and finisher)
    * weights     => [], # weight of each jobs (split and finisher)
    * currentTree => 0, # tree in process
    * job_number => undef,
    * path_temp  => undef,
    * path_shell => undef,
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;
    
    my $class= ref($this) || $this;
    my $self = {
        # in
        pyramid     => undef,
        #
        job_number  => undef,
        path_temp   => undef,
        path_shell  => undef,
        # 
        trees       => [],
        nodata      => undef,
        # out
        scripts   => [],
        codes       => [],
        weights     => [],
        currentTree => 0,
    };
    bless($self, $class);

    TRACE;
    
    return undef if (! $self->_init(@_));

    return $self;
}

#
=begin nd
method: _init

Load process' parameters, create a tree per data source.

Parameters:
    params_process - job_number, path_temp and path_shell.
    pyr - Pyramid object.
=cut
sub _init {
    my ($self, $params_process, $pyr) = @_;

    TRACE;

    # it's an object and it's mandatory !
    if (! defined $pyr || ref ($pyr) ne "BE4::Pyramid") {
        ERROR("Can not load Pyramid!");
        return FALSE;
    }
    $self->{pyramid} = $pyr;


    # manadatory !
    $self->{job_number} = $params_process->{job_number}; 
    $self->{path_temp}  = $params_process->{path_temp};
    $self->{path_shell} = $params_process->{path_shell};

    if (! defined $self->{job_number}) {
        ERROR("Parameter required to 'job_number' !");
        return FALSE;
    }

    if (! defined $self->{path_temp}) {
        ERROR("Parameter required to 'path_temp' !");
        return FALSE;
    }

    if (! defined $self->{path_shell}) {
        ERROR("Parameter required to 'path_shell' !");
        return FALSE;
    }

    # it's an object !
    $self->{nodata} = $self->{pyramid}->getNodata();

    if (! defined $self->{nodata} || ref ($self->{nodata}) ne "BE4::NoData") {
        ERROR("Can not load NoData object !");
        return FALSE;
    }

    my $dataSources = $self->{pyramid}->getDataSources();
    my $TMS = $self->{pyramid}->getTileMatrixSet();

    foreach my $datasource (@{$dataSources}) {
        
        if ($datasource->hasImages()) {
            
            if (($datasource->getSRS() ne $TMS->getSRS()) ||
                (! $self->{pyramid}->isNewPyramid() && ($self->{pyramid}->getCompression() eq 'jpg'))) {
                
                if (! $datasource->hasHarvesting()) {
                    ERROR ("We need a WMS service to harvest in this case (Reprojection with image source, or update of JPEG cache");
                    return FALSE;
                }
                
            } else {
                WARN(sprintf "We don't need WMS service for the datasource with base level '%s'. We remove it.",$datasource->{bottomLevelID});
                $datasource->removeHarvesting();
            }
        }
        
        # Now, if datasource contains a WMS service, we have to use it
        
        my $tree = BE4::Tree->new($datasource, $self->{pyramid}, $self->{job_number});
        
        if (! defined $tree) {
            ERROR(sprintf "Can not create a Tree object for datasource with bottom level %s !",
                  $datasource->{bottomLevelID});
            return FALSE;
        }
        
        push @{$self->{trees}},$tree;
    }
    
    DEBUG (sprintf "TREES = %s", Dumper($self->{trees}));

    return TRUE;
}

####################################################################################################
#                                  GENERAL COMPUTING METHOD                                        #
####################################################################################################

# Group: general computing method

#
=begin nd
method: computeTrees

Initialize each script, compute each tree one after the other and save scripts to finish.

See Also:
    <computeWholeTree>
=cut
sub computeTrees {
    my $self = shift;

    TRACE;
    
    # -------------------------------------------------------------------
    # We initialize scripts (name, header, weights)
    
    my $functions = $self->configureFunctions();
    
    for (my $i = 0; $i <= $self->{job_number}; $i++) {
        my $scriptID = sprintf "SCRIPT_%s",$i;
        $scriptID = "SCRIPT_FINISHER" if ($i == 0);
        
        my $header = $self->prepareScript($scriptID,$functions);
        
        push @{$self->{scriptsID}},$scriptID;
        push @{$self->{codes}},$header;
        push @{$self->{weights}},0;
    }
    
    # -------------------------------------------------------------------
    # We open stream to the new cache list, to add generated tile when we browse tree.    
    my $NEWLIST;
    if (! open $NEWLIST, ">>", $self->{pyramid}->{new_pyramid}->{content_path}) {
        ERROR(sprintf "Cannot open new cache list file : %s",$self->{pyramid}->{new_pyramid}->{content_path});
        return FALSE;
    }
    
    while ($self->{currentTree} < scalar @{$self->{trees}}) {
        if (! $self->computeWholeTree($NEWLIST)) {
            ERROR(sprintf "Cannot compute tree number %s",$self->{currentTree});
            return FALSE;
        }
        $self->{currentTree}++;
    }
    
    close $NEWLIST;
    
    # -------------------------------------------------------------------
    # We save codes in files
    
    for (my $i = 0; $i <= $self->{job_number}; $i++) {
        if (! $self->saveScript($self->{codes}[$i], $self->{scriptsID}[$i])) {
            ERROR(sprintf "Can not save the script '%s' !", $self->{scriptsID}[$i]);
            return FALSE;
        }
    }
    
    return TRUE;
}

#
=begin nd
method: computeWholeTree

Determine codes and weights for each node of the current tree, and share work on scripts, so as to optimize execution time.

Three steps:
    - browse tree : add weight and code to the nodes.
    - determine the cut level, to distribute fairly work.
    - browse the tree once again: we concatenate node's commands and write them in different scripts.

Parameter:
    NEWLIST - stream to the cache's list, to add new images.
    
See Also:
    <computeBranch>, <writeBranchCode>
=cut
sub computeWholeTree {
    my $self = shift;
    my $NEWLIST = shift;

    TRACE;

    my $pyrName = $self->{pyramid}->getPyrName();
    my $tree = $self->{trees}[$self->{currentTree}];
    
    INFO (sprintf "Tree : %s", $self->{currentTree});
    
    # -------------------------------------------------------------------
    # Pondération de l'arbre en fonction des opérations à réaliser,
    # création du code script (ajouté aux noeuds de l'arbre) et mise à
    # jour de la liste des fichiers du cache avec les nouvelles images.
    my @topLevelNodeList = $tree->getNodesOfTopLevel();
    foreach my $topNode (@topLevelNodeList) {
        if (! $self->computeBranch($topNode,$NEWLIST)) {
            ERROR(sprintf "Can not compute the node of the top level '%s'!", Dumper($topNode));
            return FALSE;
        }
    }

    # -------------------------------------------------------------------
    # Détermination du cutLevel optimal et répartition des noeuds sur les jobs,
    # en tenant compte du fait qu'ils peuvent déjà contenir du travail, du fait
    # de la pluralité des arbres à traiter.
    
    my @nodeRack;
    
    $tree->shareNodesOnJobs(\@nodeRack,\@{$self->{weights}});
    
    if (! scalar @nodeRack) {
        ERROR("Cut Level Node List is empty !");
        return FALSE;
    }
    INFO (sprintf "CutLevel : %s", $tree->{cutLevelID});

    # -------------------------------------------------------------------
    # Split scripts
    for (my $scriptCount = 1; $scriptCount <= $self->{job_number}; $scriptCount++){
        #
        my $scriptCode;
        if (! defined($nodeRack[$scriptCount-1])){
            $self->{codes}[$scriptCount] .= "echo \"Nothing to do : not a problem.\"";
        } else {
            foreach my $node (@{$nodeRack[$scriptCount-1]}){
                INFO (sprintf "Node '%s-%s-%s' into 'SCRIPT_%s'.", $node->{level} ,$node->{x}, $node->{y}, $scriptCount);
                $self->{codes}[$scriptCount] .= sprintf "\necho \"PYRAMIDE:%s SOURCE:%s  LEVEL:%s X:%s Y:%s\"", $pyrName, $self->{currentTree}, $node->{level} ,$node->{x}, $node->{y};
                $self->{codes}[$scriptCount] .= $self->writeBranchCode($node);
                # NOTE : pas d'image à collecter car non séparation par script du dossier temporaire, de travail
                # A rétablir si possible
                # on récupère l'image de travail finale pour le job de fin.
                # $finishScriptCode .= $self->collectWorkImage($node, $scriptId, $finishScriptId);
            }
        }
    }

    # -------------------------------------------------------------------
    # Final script    
    if ($tree->getTopLevelID() eq $tree->getCutLevelID()){
        INFO("Final script will be empty");
        $self->{codes}[0] .= "echo \"Final script have nothing to do.\" \n";
    } else {
        my @nodeList = $tree->getNodesOfTopLevel();
        foreach my $node (@nodeList){
            $self->{codes}[0] .= $self->writeTopCode($node);
        }
    }
    
    return TRUE;
}

####################################################################################################
#                                  COMPUTING/WEIGHTER METHOD                                       #
####################################################################################################

# Group: computing / weighter method

#
=begin nd
method: computeBranch

Recursive method, which allow to browse tree downward.

2 cases.
    - the node belong to the bottom level -> computeBottomImage
    - the node does not belong to the bottom level -> computeBranch on each child, and computeAboveImage

Parameter:
    node - node to treat.
    NEWLIST - stream to the cache's list, to add new images.
    
See Also:
    <computeBottomImage>, <computeAboveImage>
=cut
sub computeBranch {
    my $self = shift;
    my $node = shift;
    my $NEWLIST = shift;

    my $tree = $self->{trees}[$self->{currentTree}];
    my $weight = 0;

    TRACE;
    
    printf $NEWLIST "0/%s\n", $self->{pyramid}->getCacheNameOfImage($node,'data');

    my $res = '';
    my @childList = $tree->getChildren($node);
    if (scalar @childList == 0){
        if (! $self->computeBottomImage($node)) {
            ERROR(sprintf "Cannot compute the bottom image : %s_%s, level %s)", $node->{x}, $node->{y}, $node->{level});
            return FALSE;
        }
        return TRUE;
    }
    foreach my $n (@childList){
        if (! $self->computeBranch($n,$NEWLIST)) {
            ERROR(sprintf "Cannot compute the branch from node %s_%s , level %s)", $node->{x}, $node->{y}, $node->{level});
            return FALSE;
        }
        $weight += $tree->getAccumulatedWeightOfNode($n);
    }

    if (! $self->computeAboveImage($node)) {
        ERROR(sprintf "Cannot compute the above image : %s_%s, level %s)", $node->{x}, $node->{y}, $node->{level});
        return FALSE;
    }

    $tree->setAccumulatedWeightOfNode($node,$weight);

    return TRUE;
}

#
=begin nd
method: computeBottomImage

Treat a bottom node : determine code and weight.

2 cases.
    - native projection, lossless compression and images as data -> mergeNtiff
    - reprojection or lossy compression or just a WMS service as data -> wget

Parameter:
    node - bottom level's node, to treat.
    
See Also:
    <wms2work>, <cache2work>, <mergeNtiff>, <work2cache>
=cut
sub computeBottomImage {
    my $self = shift;
    my $node = shift;

    TRACE;

    my $tree = $self->{trees}[$self->{currentTree}];
    my $weight  = 0;
    my $code  = "\n";

    my $bgImgPath=undef;
    my $bgImgName=undef;
  
# FIXME (TOS) Ici, on fait appel au service WMS sans vérifier que la zone demandée n'est pas trop grande.
# On considère que le niveau le plus bas de la pyramide que l'on est en train de construire n'est pas trop élevé.
# A terme, il faudra vérifier cette zone et ne demander que les tuiles contenant des données, et reconstruire une
# image entière à partir de là (en ajoutant sur place des tuiles de nodata pour compléter).

    if ($tree->{datasource}->hasHarvesting()) {
        # Datasource has a WMS service : we have to use it
        $code .= $self->wms2work($node,$self->workNameOfNode($node));
        $tree->updateWeightOfNode($node,WGET_W);
    } else {
        my $newImgDesc = $tree->getImgDescOfNode($node);
        my $workImgFilePath = File::Spec->catfile($self->getScriptTmpDir(), $self->workNameOfNode($node));
        my $workImgDesc = $newImgDesc->copy($workImgFilePath); # copie du descripteur avec changement du nom de fichier

        # Si elle existe, on copie la dalle de la pyramide de base dans le repertoire de travail 
        # en la convertissant du format cache au format de travail: c'est notre image de fond.
        # Si la dalle de la pyramide de base existe, on a créé un lien, donc il existe un fichier 
        # correspondant dans la nouvelle pyramide.
        if ( -f $newImgDesc->getFilePath() ){
            $bgImgName = join("_","bgImg",$node->{level},$node->{x},$node->{y}).".tif";
            $bgImgPath = File::Spec->catfile($self->getScriptTmpDir(),$bgImgName);
            # copie avec tiffcp ou untile+montage pour passer du format de cache au format de travail.
            $code .= $self->cache2work($node, $bgImgName);
        }

        # On cree maintenant le fichier de config pour l'outil mergeNtiff
        my $confDirPath  = File::Spec->catdir($self->getScriptTmpDir(), "mergeNtiff");
        if (! -d $confDirPath) {
            DEBUG (sprintf "create dir mergeNtiff");
            eval { mkpath([$confDirPath]); };
            if ($@) {
                ERROR(sprintf "Can not create the script directory '%s' : %s !", $confDirPath, $@);
                return FALSE;
            }
        }

        my $confFilePath = File::Spec->catfile($confDirPath,
            join("_","mergeNtiffConfig", $node->{level}, $node->{x}, $node->{y}).".txt");
        my $confFilePathForScript = File::Spec->catfile('${ROOT_TMP_DIR}/mergeNtiff',
            join("_","mergeNtiffConfig", $node->{level}, $node->{x}, $node->{y}).".txt");

        DEBUG (sprintf "create mergeNtiff");
        if (! open CFGF, ">", $confFilePath ){
            ERROR(sprintf "Impossible de creer le fichier $confFilePath.");
            return FALSE;
        }
        # La premiere ligne correspond à la dalle résultat: La version de travail de la dalle à calculer.
        printf CFGF $workImgDesc->to_string();

        # Maintenant les dalles en entrée:
        my $bgImgDesc;
        if (defined $bgImgPath){
            # L'image de fond (qui est la dalle de la pyramide de base ou la dalle nodata si elle n'existe pas)
            $bgImgDesc = $newImgDesc->copy($bgImgPath);
            printf CFGF "%s", $bgImgDesc->to_string();
        }
        # ajout des images sources
        my $listDesc = $tree->getGeoImgOfBottomNode($node);
        foreach my $desc (@{$listDesc}){
            printf CFGF "%s", $desc->to_string();
        }
        close CFGF;

        $weight += MERGENTIFF_W;
        $code .= $self->mergeNtiff($confFilePathForScript, $bgImgName);
    }

    # copie de l'image de travail créée dans le rep temp vers l'image de cache dans la pyramide.
    $weight += TIFF2TILE_W;
    $code .= $self->work2cache($node);

    $tree->updateWeightOfNode($node,$weight);
    $tree->setComputingCode($node,$code);

    return TRUE;
}

#
=begin nd
method: computeAboveImage

Treat an above node (different to the bottom level) : determine code and weight.

To generate an above node, we use children (merge4tiff). If we have not 4 children or if children contain nodata, we have to supply a background, a color or an image if exists.

Parameter:
    node - above level's node, to treat.
    
See Also:
    <wms2work>, <cache2work>, <merge4tiff>, <work2cache>
=cut
sub computeAboveImage {
    my $self = shift;
    my $node = shift;

    TRACE;

    my $tree = $self->{trees}[$self->{currentTree}];
    my $code = "\n";
    my $weight = 0;
    my $newImgDesc = $tree->getImgDescOfNode($node);
    my @childList = $tree->getChildren($node);

    my $bgImgPath=undef;
    my $bgImgName=undef;

    # On renseigne dans tous les cas la couleur de nodata, et on donne un fond s'il existe, même s'il y a 4 images,
    # si on a l'option nowhite
    my $bg='-n ' . $self->{nodata}->getValue();


    if (scalar @childList != 4 || $self->{nodata}->{nowhite}) {
        # Pour cela, on va récupérer le nombre de tuiles (en largeur et en hauteur) du niveau, et 
        # le comparer avec le nombre de tuile dans une image (qui est potentiellement demandée à 
        # rok4, qui n'aime pas). Si l'image contient plus de tuile que le niveau, on ne demande plus
        # (c'est qu'on a déjà tout ce qui faut avec les niveaux inférieurs).

        # WARNING (TOS) cette solution n'est valable que si la structure de l'image (nombre de tuile dans l'image si
        # tuilage il y a) est la même entre le cache moissonné et la pyramide en cours d'écriture.
        # On a à l'heure actuelle du 16 sur 16 sur toute les pyramides et pas de JPEG 2000. 

        my $tm = $tree->{tms}->getTileMatrix($node->{level});
        if (! defined $tm) {
            ERROR(sprintf "Cannot load the Tile Matrix for the level %s",$node->{level});
            return undef;
        }

        my $tooWide =  $tm->getMatrixWidth() < $self->{pyramid}->getTilePerWidth();
        my $tooHigh =  $tm->getMatrixHeight() < $self->{pyramid}->getTilePerHeight();

        if (-f $newImgDesc->getFilePath()) {
            # Il y a dans la pyramide une dalle pour faire image de fond de notre nouvelle dalle.
            $bgImgName = join("_","bgImg",$node->{level},$node->{x},$node->{y}).".tif";
            $bgImgPath = File::Spec->catfile('${TMP_DIR}',$bgImgName);

            if ($self->{pyramid}->getCompression() eq 'jpg') {
                # On vérifie d'abord qu'on ne veut pas moissonner une zone trop grande
                if ($tooWide || $tooHigh) {
                    WARN(sprintf "The image would be too high or too wide for this level (%s)",$node->{level});
                } else {
                    # On peut et doit chercher l'image de fond sur le WMS
                    $weight += WGET_W;
                    $bg.=" -b $bgImgPath";
                    $code .= $self->wms2work($node, $bgImgName);
                }
            } else {
                # copie avec tiffcp ou untile+montage pour passer du format de cache au format de travail.
                $bg.=" -b $bgImgPath";
                $code .= $self->cache2work($node, $bgImgName);
            }
        }
    }


    # Maintenant on constitue la liste des images à passer à merge4tiff.
    my $childImgParam=''; 
    my $imgCount=0;
    foreach my $childNode ($tree->getPossibleChildren($node)) {
        $imgCount++;
        if ($tree->isInTree($childNode)){
            $childImgParam.=' -i'.$imgCount.' $TMP_DIR/' . $self->workNameOfNode($childNode)
        }
    }
    $code .= $self->merge4tiff('$TMP_DIR/' . $self->workNameOfNode($node), $bg, $childImgParam);
    $weight += MERGE4TIFF_W;

    # Suppression des images de travail dont on a plus besoin.
    foreach my $node (@childList){
        my $workName = $self->workNameOfNode($node);
        $code .= "rm -f \${TMP_DIR}/$workName \n";
    }

    # Si on a copié une image pour le fond, on la supprime maintenant
    if ( defined($bgImgName) ){
        $code.= "rm -f \${TMP_DIR}/$bgImgName \n";
    }

    # copie de l'image de travail crée dans le rep temp vers l'image de cache dans la pyramide.
    $code .= $self->work2cache($node);

    $tree->updateWeightOfNode($node,$weight);
    $tree->setComputingCode($node,$code);

    return TRUE;
}

####################################################################################################
#                                          WRITER METHODS                                          #
####################################################################################################

# Group: writer methods

#
=begin nd
method: writeBranchCode

Recursive method, which allow to browse tree (downward) and concatenate node's commands.

Parameter:
    node - node whose code is written.
=cut
sub writeBranchCode {
    my $self = shift;
    my $node = shift;

    TRACE;

    my $tree = $self->{trees}[$self->{currentTree}];
    my $code = '';
    my @childList = $tree->getChildren($node);

    # Le noeud est une feuille
    if (scalar @childList == 0){
        return $tree->getComputingCode($node);
    }

    # Le noeud a des enfants
    foreach my $n (@childList){
        $code .= $self->writeBranchCode($n);
    }
    $code .= $tree->getComputingCode($node);

    return $code;
}

#
=begin nd
method: writeTopCode

Recursive method, which allow to browse downward the tree, from the top, to the cut level and concatenate node's commands.

Parameter:
    node - node whose code is written.
=cut
sub writeTopCode {
    my $self = shift;
    my $node = shift;

    TRACE;

    my $tree = $self->{trees}[$self->{currentTree}];
    
    # Rien à faire, le niveau CutLevel est déjà fait et les images de travail sont déjà là. 
    return '' if ($node->{level} eq $tree->getCutLevelID());

    my $code = '';
    my @childList = $tree->getChildren($node);
    foreach my $n (@childList){
        $code .= $self->writeTopCode($n);
    }
    $code .= $tree->getComputingCode($node);

    return $code;
}

####################################################################################################
#                                      COMMANDS METHODS                                            #
####################################################################################################

# Group: commands methods

#
=begin nd
method: wms2work

Fetch image corresponding to the node thanks to 'wget', in one or more steps at a time. WMS service is described in the current tree's datasource. Use the 'Wms2work' bash function.

Example:
    Wms2work ${TMP_DIR}/18_8300_5638.tif "http://localhost/wmts/rok4?LAYERS=ORTHO_RAW_LAMB93_D075-E&SERVICE=WMS&VERSION=1.3.0&REQUEST=getMap&FORMAT=image/tiff&CRS=EPSG:3857&BBOX=264166.3697535659936,6244599.462785762557312,266612.354658691633792,6247045.447690888197504&WIDTH=4096&HEIGHT=4096&STYLES="

Parameters:
    node - node whose image have to be harvested.
    fileName - file to save the download (level_x_y.tif).
=cut
sub wms2work {
    #  FIXME: - appeler la méthode de l'objet src
    #         - parametrer le proxy (placer une option dans le fichier de configuration [harvesting] !)
    my ($self, $node, $fileName) = @_;
    
    TRACE;
    
    my $tree = $self->{trees}[$self->{currentTree}];
    
    my $imgDesc = $tree->getImgDescOfNode($node);
    my @imgSize = $self->{pyramid}->getCacheImageSize($node->{level}); # ie size tile image in pixel !
    my $tms     = $self->{pyramid}->getTileMatrixSet();
    
    my @requests = $tree->{datasource}->{harvesting}->doRequestUrl(
        srs      => $tms->getSRS(),
        bbox     => [$imgDesc->{xMin},$imgDesc->{yMin},$imgDesc->{xMax},$imgDesc->{yMax}],
        imagesize => [$imgSize[0], $imgSize[1]]
    );
  
    my $cmd="";
    
    my $nodeName = $self->workNameOfNode($node);
    
    if (scalar @requests == 0) {
        ERROR("Cannot harvest image for node $nodeName");
        exit 4;
    }
    
    # Si plusieurs requêtes :
    #   - on récupère toutes les images dans un dossier ayant pour nom le noeud (level_x_y)
    #   - on fait un 'montage' pour reformer l'image complète sous le nom fileName
    # Si une seule :
    #   - enregistrée directement sous le nom fileName
    
    if (scalar @requests > 1) {
        $cmd .= sprintf "mkdir \${TMP_DIR}/%s\n",$nodeName;
    }
    
    for (my $i = 0; $i < scalar @requests; $i++) {
        # On requête toutes les images avec éventuellement plusieurs tentatives
        my $partFileName = $fileName;
        
        if (scalar @requests > 1) {
            # Si il y a plusieurs images, on ajoute au chemin d'écriture un dossier et on suffixe les images
            my $suffix = sprintf "_%02d",$i;
            $partFileName =~ s/\.tif/$suffix\.tif/;
            $partFileName = "$nodeName/".$partFileName;
        }
        
        $cmd .= sprintf "Wms2work \${TMP_DIR}/%s \"%s\"\n",$partFileName,$requests[$i];
        
    }
    
    if (scalar @requests > 1) {
        # On configure le montage, les images sont déjà nommées dans le bon ordre
        my $width = $tree->{datasource}->{harvesting}->{image_width};
        my $height = $tree->{datasource}->{harvesting}->{image_height};
        $cmd .=  sprintf "montage -geometry %sx%s -tile %sx%s \${TMP_DIR}/%s/*.tif -depth %s",
            $width,$height,$imgSize[0]/$width,$imgSize[1]/$height,
            $nodeName,$self->{pyramid}->getBitsPerSample();
            
        $cmd .=  " -background none" if (int($self->{pyramid}->getSamplesPerPixel()) == 4);
        
        $cmd .=  sprintf " -define tiff:rows-per-strip=%s  \${TMP_DIR}/%s\n%s",
            $self->{pyramid}->getCacheImageWidth($node->{level}),$fileName, RESULT_TEST;

        $cmd .=  sprintf ("rm -rf \${TMP_DIR}/%s/\n",$nodeName);
    }
    
    return $cmd;
}

#
=begin nd
method: cache2work

Copy image from cache to work directory and transform (work format : untiles, uncompressed).
Use the 'Cache2work' bash function.

2 cases:
    - legal tiff compression -> tiffcp
    - png -> untile + montage
    
Examples:
    Cache2work ${PYR_DIR}/IMAGE/19/02/BF/24.tif ${TMP_DIR}/bgImg_19_398_3136 (png)
    
Parameters:
    node - node whose image have to be transfered in the work directory.
    workName - work file name (level_x_y.tif).
=cut
sub cache2work {
    my ($self, $node, $workName) = @_;

    my $tree = $self->{trees}[$self->{currentTree}];
    my @imgSize   = $self->{pyramid}->getCacheImageSize($node->{level}); # ie size tile image in pixel !
    my $cacheName = $self->{pyramid}->getCacheNameOfImage($node, 'data');

    DEBUG(sprintf "'%s'(cache) === '%s'(work)", $cacheName, $workName);
    
    my $dirName = $workName;
    $dirName =~ s/\.tif//;

    if ($self->{pyramid}->getCompression() eq 'png') {
        # Dans le cas du png, l'opération de copie doit se faire en 3 étapes :
        #       - la copie du fichier dans le dossier temporaire
        #       - le détuilage (untile)
        #       - la fusion de tous les png en un tiff
        $self->{tree}->updateWeightOfNode($node,CACHE2WORK_PNG_W);

        my $cmd =  sprintf ("Cache2work \${PYR_DIR}/%s \${TMP_DIR}/%s png\n", $cacheName , $dirName);

        return $cmd;
    } else {
        # Pour le tiffcp on fixe le rowPerStrip au nombre de ligne de l'image ($imgSize[1])
        $self->{tree}->updateWeightOfNode($node,TIFFCP_W);
        my $cmd =  sprintf ("Cache2work \${PYR_DIR}/%s \${TMP_DIR}/%s\n", $cacheName ,$dirName);
        return $cmd;
    }
}

#
=begin nd
method: work2cache

Copy image from work directory to cache and transform it (tiles and compressed) thanks to the 'Work2cache' bash function (tiff2tile).

Example:
    Work2cache ${TMP_DIR}/19_395_3137.tif ${PYR_DIR}/IMAGE/19/02/AF/Z5.tif

Parameter:
    node - node whose image have to be transfered in the cache.
=cut
sub work2cache {
    my $self = shift;
    my $node = shift;
  
    my $tree = $self->{trees}[$self->{currentTree}];
    my $workImgName  = $self->workNameOfNode($node);
    my $cacheImgName = $self->{pyramid}->getCacheNameOfImage($node, 'data'); 
    
    my $tm = $self->{pyramid}->getTileMatrixSet()->getTileMatrix($node->{level});
    
    # DEBUG: On pourra mettre ici un appel à convert pour ajouter des infos
    # complémentaire comme le quadrillage des dalles et le numéro du node, 
    # ou celui des tuiles et leur identifiant.
    DEBUG(sprintf "'%s'(work) === '%s'(cache)", $workImgName, $cacheImgName);
    
    # Suppression du lien pour ne pas corrompre les autres pyramides.
    my $cmd   .= sprintf ("Work2cache \${TMP_DIR}/%s \${PYR_DIR}/%s\n", $workImgName, $cacheImgName);
    
    # Si on est au niveau du haut, il faut supprimer les images, elles ne seront plus utilisées
    
    if ($node->{level} eq $tree->{topLevelID}) {
        $cmd .= sprintf ("rm -f \${TMP_DIR}/%s\n", $workImgName);
    }
    
    return $cmd;
}

#
=begin nd
method: mergeNtiff

Use the 'MergeNtiff' bash function.

Parameters:
    confFile - complete absolute file path to the configuration (list of used images).
    bg - Name of the eventual background (undefined if none).
    dataType - 'data' or 'metadata'.
    
Example:
    MergeNtiff image ${ROOT_TMP_DIR}/mergeNtiff/mergeNtiffConfig_19_397_3134.txt
=cut
sub mergeNtiff {
    my $self = shift;
    my $confFile = shift;
    my $bg = shift; # optionnal
    my $dataType = shift; # param. are image or mtd to mergeNtiff script !

    TRACE;

    $dataType = 'image' if (  defined $dataType && $dataType eq 'data');
    $dataType = 'image' if (! defined $dataType);
    $dataType = 'mtd'   if (  defined $dataType && $dataType eq 'metadata');

    my $pyr = $self->{pyramid};
    #"bicubic"; # TODO l'interpolateur pour les mtd sera "nn"
    # TODO pour les métadonnées ce sera 0

    my $cmd = sprintf "MergeNtiff %s %s",$dataType,$confFile;
    
    if (defined $bg) {
        $cmd .= " \${TMP_DIR}/$bg\n";
    } else {
        $cmd .= "\n";
    }

    return $cmd;
}

#
=begin nd
method: merge4tiff

Compose the 'merge4tiff' command.

Parameters:
    resultImg - path to write 'merge4tiff' result.
    backGround - a color or an image.
    childImgParam - images to merge (and their places), from 1 to 4 ("-i1 img1.tif -i3 img2.tif -i4 img3.tif").
|                   i1  i2
| backGround    +              =  resultImg
|                   i3  i4

Example:
    merge4tiff -g 1 -n FFFFFF  -i1 $TMP_DIR/19_396_3134.tif -i2 $TMP_DIR/19_397_3134.tif -i3 $TMP_DIR/19_396_3135.tif -i4 $TMP_DIR/19_397_3135.tif $TMP_DIR/18_198_1567.tif
=cut
sub merge4tiff {
  my $self = shift;
  my $resultImg = shift;
  my $backGround = shift;
  my $childImgParam  = shift;
  
  TRACE;
  
  # Change :
  #   getGamma() retourne 1 par defaut, ou une valeur decimale 0->1

  my $pyr = $self->{pyramid};
  
  my $cmd = sprintf "%s -g %s ", MERGE_4_TIFF, $pyr->getGamma();
  
  $cmd .= "$backGround ";
  
  $cmd .= "$childImgParam ";
  
  $cmd .= sprintf "%s\n%s",$resultImg, RESULT_TEST;
  
  return $cmd;
}

#
=begin nd
method: workNameOfNode

Compose the name corresponding to the node

Parameter:
    node - node whose we want name

Example:
    level-15_132_3654
=cut
sub workNameOfNode {
  my $self = shift;
  my $node = shift;
  
  TRACE;
  
  return sprintf ("%s_%s_%s.tif", $node->{level}, $node->{x}, $node->{y}); 
}




####################################################################################################
#                                        PUBLIC METHODS                                            #
####################################################################################################

# Group: public methods

sub getRootTmpDir {
  my $self = shift;
  my $pyrName = $self->{pyramid}->getPyrName();
  return File::Spec->catdir($self->{path_temp}, $pyrName);
  # ie .../TMP/PYRNAME/
}

sub getScriptTmpDir {
  my $self = shift;
  my $pyrName = $self->{pyramid}->getPyrName();
  return File::Spec->catdir($self->{path_temp}, $pyrName);
  #return File::Spec->catdir($self->{path_temp}, $pyrName, "\${SCRIPT_ID}/");
  # ie .../TMP/PYRNAME/SCRIPT_X/
}

#
=begin nd
method: configureFunctions

Configure bash functions to write in scripts' header.
=cut
sub configureFunctions {
    my $self = shift;

    TRACE;

    my $pyr = $self->{pyramid};
    my $configuredFunc = $BASHFUNCTIONS;

    # congigure mergeNtiff
    my $conf_mNt = "";

    my $ip = $pyr->getInterpolation();
    $conf_mNt .= "-i $ip ";
    my $spp = $pyr->getSamplesPerPixel();
    $conf_mNt .= "-s $spp ";
    my $bps = $pyr->getBitsPerSample();
    $conf_mNt .= "-b $bps ";
    my $ph = $pyr->getPhotometric();
    $conf_mNt .= "-p $ph ";
    my $sf = $pyr->getSampleFormat();
    $conf_mNt .= "-a $sf ";

    if ($self->{nodata}->{nowhite}) {
        $conf_mNt .= "-nowhite ";
    }

    my $nd = $self->{nodata}->getValue();
    $conf_mNt .= "-n $nd ";

    $configuredFunc =~ s/__mNt__/$conf_mNt/;

    # congigure montage
    my $conf_montage = "";

    $conf_montage .= "-depth $bps ";
    if ($spp == 4) {
        $conf_montage .= "-background none ";
    }

    $configuredFunc =~ s/__montage__/$conf_montage/;

    # congigure tiffcp
    my $conf_tcp = "";

    my @imgSize   = $self->{pyramid}->getCacheImageSize(); # ie size tile image in pixel !
    my $imgS = $imgSize[1];
    $conf_tcp .= "-s -r $imgS ";

    $configuredFunc =~ s/__tcp__/$conf_tcp/;

    # congigure tiff2tile
    my $conf_t2t = "";

    my $compression = $pyr->getCompression();
    my $compressionoption = $pyr->getCompressionOption();
    $compression = ($compression eq 'raw'?'none':$compression);
    $conf_t2t .= "-c $compression ";
    if ($pyr->getCompressionOption() eq 'crop') {
        $conf_t2t .= "-crop ";
    }

    $conf_t2t .= "-p $ph ";
    $conf_t2t .= sprintf "-t %s %s ",$pyr->getTileMatrixSet()->getTileWidth(),$pyr->getTileMatrixSet()->getTileHeight();
    $conf_t2t .= "-b $bps ";
    $conf_t2t .= "-a $sf ";
    $conf_t2t .= "-s $spp ";

    $configuredFunc =~ s/__t2t__/$conf_t2t/;

    return $configuredFunc;
}

#
=begin nd
method: prepareScript

Compose script's header, which contains environment variables: the script ID, path to work directory, cache... And functions to factorize code.

Parameter:
    scriptId - script whose header have to be add.
    functions - Configured functions, used in the script (mergeNtiff, wget...).

Example:
|   # Variables d'environnement
|   SCRIPT_ID="SCRIPT_1"
|   ROOT_TMP_DIR="/home/TMP/ORTHO"
|   TMP_DIR="/home/ign/TMP/ORTHO"
|   PYR_DIR="/home/ign/PYR/ORTHO"
|
|   # fonctions de factorisation
|   Wms2work () {
|     local img_dst=$1
|     local url=$2
|     local count=0; local wait_delay=60
|     while :
|     do
|       let count=count+1
|       wget --no-verbose -O $img_dst $url 
|       if tiffck $img_dst ; then break ; fi
|       echo "Failure $count : wait for $wait_delay s"
|       sleep $wait_delay
|       let wait_delay=wait_delay*2
|       if [ 3600 -lt $wait_delay ] ; then 
|         let wait_delay=3600
|       fi
|     done
|   }
=cut
sub prepareScript {
    my $self = shift;
    my $scriptId = shift;
    my $functions = shift;

    TRACE;

    # definition des variables d'environnement du script
    my $pyrName = $self->{pyramid}->getPyrName();
    my $tmpDir  = $self->getScriptTmpDir();
    my $pyrpath = File::Spec->catdir($self->{pyramid}->getPyrDataPath(),$pyrName);

    my $code = sprintf ("# Variables d'environnement\n");
    $code   .= sprintf ("SCRIPT_ID=\"%s\"\n", $scriptId);
    $code   .= sprintf ("ROOT_TMP_DIR=\"%s\"\n", File::Spec->catdir($self->{path_temp}, $pyrName));
    $code   .= sprintf ("TMP_DIR=\"%s\"\n", $tmpDir);
    $code   .= sprintf ("PYR_DIR=\"%s\"\n", $pyrpath);
    $code   .= "\n";
    
    # Fonctions
    $code   .= "# Fonctions\n";
    $code   .= "$functions\n";

    # creation du répertoire de travail:
    $code .= "# creation du repertoire de travail\n";
    $code .= "if [ ! -d \"\${TMP_DIR}\" ] ; then mkdir -p \${TMP_DIR} ; fi\n\n";

    return $code;
}

#
=begin nd
method: saveScript

Write scripts in files, remove temporary images if exist.

Parameter:
    code - code to write in file
    scriptId - script identifiant, file will be "scriptId.sh", in the "path_shell" directory.
=cut
sub saveScript {
    my $self = shift;
    my $code = shift;
    my $scriptId = shift;

    TRACE;

    if (! defined $code) {
        ERROR("No code to pass into the script ?"); 
        return FALSE;
    }
    if (! defined $scriptId) {
        ERROR("No ScriptId to save the script ?");
        return FALSE;
    }

    my $scriptName     = join('.',$scriptId,'sh');
    my $scriptFilePath = File::Spec->catfile($self->{path_shell}, $scriptName);

    if (! -d dirname($scriptFilePath)) {
        my $dir = dirname($scriptFilePath);
        DEBUG (sprintf "Create the script directory'%s' !", $dir);
        eval { mkpath([$dir]); };
        if ($@) {
            ERROR(sprintf "Can not create the script directory '%s' : %s !", $dir , $@);
            return FALSE;
        }
    }

    if ( ! (open SCRIPT,">", $scriptFilePath)) {
        ERROR(sprintf "Can not save the script '%s' into directory '%s' !.", $scriptName, dirname($scriptFilePath));
        return FALSE;
    }

    # Would Be Nice: On pourra faire une évaluation de la charge de ce script ici, par analyse de son contenu.
    #                c'est parfois utilisé par les orchestrateurs (ex: Torque)
    # Utilisation du poids calculé des branches traitées dans ce script

    if ($self->{pyramid}->getCompression() eq 'png') {
        # Dans le cas du png, on doit éventuellement supprimer les *.png dans le dossier temporaire
        $code .= sprintf ("rm -f \${TMP_DIR}/*.png\n");
    }

    printf SCRIPT "%s", $code;
    close SCRIPT;

    return TRUE;
}

# collectWorkImage
# Récupère les images au format de travail dans les répertoires temporaires des
# scripts de calcul du bas de la pyramide, pour les copier dans le répertoire
# temporaire du script final.
# NOTE
# Pas utilisé pour le moment
#--------------------------------------------------------------------------------------------
sub collectWorkImage(){
  my $self = shift;
  my ($node, $scriptId, $finishId) = @_;
  
  TRACE;
  
  my $source = File::Spec->catfile( '${ROOT_TMP_DIR}', $scriptId, $self->workNameOfNode($node));
  my $code   = sprintf ("mv %s \$TMP_DIR \n", $source);

  return $code;
}



# processScript
# Execution des scripts les uns apres les autres.
# (Orienté maintenance)
#--------------------------------------------------------------------------------------------
sub processScript {
  my $self = shift;
  
  TRACE;
  
  foreach my $scriptId (@{$self->{scripts}}) {
    
    my $scriptName     = join('.',$scriptId,'sh');
    my $scriptFilePath = File::Spec->catfile($self->{path_shell}, $scriptName);
    
    if (! -f $scriptFilePath) {
      ERROR("Can not find script file path !");
      return FALSE;
    }
    
    ALWAYS(sprintf ">>> Process script : %s ...", $scriptName);
    
    chmod 0755, $scriptFilePath;
    
    if (! open OUT, "$scriptFilePath |") {
      ERROR(sprintf "impossible de dupliquer le processus : %s !", $!);
      return FALSE;
    }
    
    while( defined( my $ligne = <OUT> ) ) {
      ALWAYS($ligne);
    }
  }
  
  return TRUE;
}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

BE4::Process - compose scripts to generate the cache

=head1 SYNOPSIS

    use BE4::Process;
  
    # Process object creation
    my $objProcess = BE4::Process->new(
        $params->{process},
        $objPyramid
    );

    # Scripts generation
    if (! $objProcess->computeTrees()) {
        ERROR ("Can not compute process !");
        return FALSE;
    }

=head1 DESCRIPTION

=over 4

=item pyramid

A Pyramid object.

=item nodata

A NoData object.

=item trees

An array of Tree objects, one per data source.

=item scripts

Array of string, names of each jobs (split and finisher)

=item codes

Array of string, codes of each jobs (split and finisher)

=item weights

Array of integer, weights of each jobs (split and finisher)

=item currentTree

Integer, indice of the tree in process.

=item job_number

Number of split scripts. If job_number = 5 -> 5 split scripts (can run in parallel) + one finisher (when splits are finished) = 6 scripts. Splits generate cache from bottom level to cut level, and the finisher from the cut level to the top level.

=item path_temp

Temporary directory path in which we create a directory named like the new cache : temporary files are written in F<path_temp/pyr_name_new>. This directory is used to store raw images (uncompressed and untiled). They are removed as and when we don't need them any more. We write in mergeNtiff configurations too.

=item path_shell

Directory path, to write scripts in. Scripts are named F<SCRIPT_1.sh>,F<SCRIPT_2.sh>... and F<SCRIPT_FINISHER.sh> for all generation. That's why the path_shell must be specific to the generation (contains the name of the pyramid for example).

=back

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-Tree.html">BE4::Tree</A></li>
<li><A HREF="./lib-BE4-NoData.html">BE4::NoData</A></li>
<li><A HREF="./lib-BE4-Pyramid.html">BE4::Pyramid</A></li>
<li><A HREF="./lib-BE4-Harvesting.html">BE4::Harvesting</A></li>
</ul>

=end html

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHOR

Satabin Théo, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
