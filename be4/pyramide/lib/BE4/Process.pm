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
use constant RESULT_TEST      => "if [ \$? != 0 ] ; then echo \$0 : Erreur a la ligne \$(( \$LINENO - 1)) >&2 ; exit 1; fi\n";
use constant CACHE_2_WORK_PRG => "tiffcp -s";
use constant WORK_2_CACHE_PRG => "tiff2tile";
use constant MERGE_N_TIFF     => "mergeNtiff";
use constant MERGE_4_TIFF     => "merge4tiff";
use constant UNTILE           => "untile";
# commands' weights
use constant MERGE4TIFF_W => 1;
use constant MERGENTIFF_W => 4;
use constant CACHE2WORK_PNG_W => 3;
use constant WGET_W => 35;
use constant TIFF2TILE_W => 0;
use constant TIFFCP_W => 0;


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
    $self->{pyramid} = $pyr;

    if (! defined $self->{pyramid} || ref ($self->{pyramid}) ne "BE4::Pyramid") {
        ERROR("Can not load Pyramid!");
        return FALSE;
    }

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
    
    for (my $i = 0; $i <= $self->{job_number}; $i++) {
        my $scriptID = sprintf "SCRIPT_%s",$i;
        $scriptID = "SCRIPT_FINISHER" if ($i == 0);
        
        my $header = $self->prepareScript($scriptID);
        
        push @{$self->{scriptsID}},$scriptID;
        push @{$self->{codes}},$header;
        push @{$self->{weights}},0;
    }
    
    while ($self->{currentTree} < scalar @{$self->{trees}}) {
        if (! $self->computeWholeTree()) {
            ERROR(sprintf "Cannot compute tree number %s",$self->{currentTree});
            return FALSE;
        }
        $self->{currentTree}++;
    }
    
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

See Also:
    <computeBranch>, <writeBranchCode>
=cut
sub computeWholeTree {
    my $self = shift;

    TRACE;

    my $pyrName = $self->{pyramid}->getPyrName();
    my $tree = $self->{trees}[$self->{currentTree}];
    
    INFO (sprintf "Tree : %s", $self->{currentTree});
    
    # Pondération de l'arbre en fonction des opérations à réaliser
    # et création du code script (ajoter aux noeuds de l'arbre)
    my @topLevelNodeList = $tree->getNodesOfTopLevel();
    foreach my $topNode (@topLevelNodeList) {
        if (! $self->computeBranch($topNode)) {
            ERROR(sprintf "Can not compute the node of the top level '%s'!", Dumper($topNode));
            return FALSE;
        }
    }

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

    # écriture des scripts
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

    # creation du script final

    $self->{codes}[0] .= $self->writeTopCodes();

    if (! defined $self->{codes}[0]) {
        ERROR("Script finisher undefined !");
        return FALSE;
    }

    return TRUE;
}

####################################################################################################
#                                  COMPUTING/WEIGHTER METHOD                                       #
####################################################################################################

# Group: computing / weighter method

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

    if ($tree->{datasource}->hasHarvesting) {
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
        $code .= $self->mergeNtiff($confFilePathForScript);
    }

    # copie de l'image de travail créée dans le rep temp vers l'image de cache dans la pyramide.
    $weight += TIFF2TILE_W;
    $code .= $self->work2cache($node);

    # Si on a copié une image pour le fond, on la supprime maintenant
    if ( defined($bgImgName) ){
        $code.= "rm -f \${TMP_DIR}/$bgImgName \n";
    }

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
            $bg.=" -b $bgImgPath";

            if ($self->{pyramid}->getCompression() eq 'jpg') {
                # On vérifie d'abord qu'on ne veut pas moissonner une zone trop grande
                if ($tooWide || $tooHigh) {
                    WARN(sprintf "The image would be too high or too wide for this level (%s)",$node->{level});
                } else {
                    # On peut et doit chercher l'image de fond sur le WMS
                    $weight += WGET_W;
                    $code .= $self->wms2work($node, $bgImgName);
                }
            } else {
                # copie avec tiffcp ou untile+montage pour passer du format de cache au format de travail.
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

#
=begin nd
method: computeBranch

Recursive method, which allow to browse tree.

2 cases.
    - the node belong to the bottom level -> computeBottomImage
    - the node does not belong to the bottom level -> computeBranch on each child, and computeAboveImage

Parameter:
    node - level to treat.
    
See Also:
    <computeBottomImage>, <computeAboveImage>
=cut
sub computeBranch {
    my $self = shift;
    my $node = shift;

    my $tree = $self->{trees}[$self->{currentTree}];
    my $weight = 0;

    TRACE;
    DEBUG(sprintf "Search in Level %s (idx: %s - %s)", $node->{level}, $node->{x}, $node->{y});

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
        if (! $self->computeBranch($n)) {
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


####################################################################################################
#                                          WRITER METHODS                                          #
####################################################################################################

# Group: writer methods

# method: writeBranchCode
#  On assemble les bouts de code de chaque noeud de l'arbre, en partant du noeud passé en paramètre
#---------------------------------------------------------------------------------------------------
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

# method: writeTopCodes
#  On assemble les bouts de code de tous les noeuds du haut de l'arbre, jusqu'au cutLevel
#---------------------------------------------------------------------------------------------------
sub writeTopCodes {
    my $self = shift;

    TRACE;

    my $tree = $self->{trees}[$self->{currentTree}];
    
    if ($tree->getTopLevelID() eq $tree->getCutLevelID()){
        INFO("Final script will be empty");
        return "echo \"Final script have nothing to do.\" \n";
    }

    my $code = '';
    my @nodeList = $tree->getNodesOfTopLevel();
    foreach my $node (@nodeList){
        $code .= $self->writeTopCode($node);
    }
    return $code;
}


# method: writeTopCode
#  On assemble les bouts de code d'un noeud du haut de l'arbre, jusqu'au cutLevel
#---------------------------------------------------------------------------------------------------
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

# method: wms2work
#  Récupère par wget l'image correspondant à 'node', et l'enregistre dans le répertoire de travail sous
#  le nom 'fileName'.
#
#  FIXME: - appeler la méthode de l'objet src
#         - parametrer le proxy (placer une option dans le fichier de configuration [harvesting] !)
#---------------------------------------------------------------------------------------------------
sub wms2work {
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
    
    my $nodeName = sprintf "%s_%s_%s",$node->{level},$node->{x},$node->{y};
    
    if (scalar @requests == 0) {
        ERROR("Cannot harvest image for node $nodeName");
        exit 4;
    }
    
    if (scalar @requests > 1) {
        $cmd .= sprintf "mkdir \${TMP_DIR}/%s\n",$nodeName;
    }
    
    for (my $i = 0; $i < scalar @requests; $i++) {
        my $partFileName = $fileName;
        
        if (scalar @requests > 1) {
            my $suffix = sprintf "_%02d",$i;
            $partFileName =~ s/\.tif/$suffix\.tif/;
            $partFileName = "$nodeName/".$partFileName;
        }
        
        $cmd .= "count=0; wait_delay=60\n";
        $cmd .= "while :\n";
        $cmd .= "do\n";
        $cmd .= "  let count=count+1\n";
        $cmd .= sprintf "  wget --no-verbose -O \${TMP_DIR}/%s \"%s\" \n",$partFileName,$requests[$i];
        $cmd .= "  if tiffck \${TMP_DIR}/$partFileName ; then break ; fi\n";
        $cmd .= "  echo \"echec \$count : wait for \$wait_delay s\"\n";
        $cmd .= "  sleep \$wait_delay\n";
        $cmd .= "  let wait_delay=wait_delay*2\n";
        $cmd .= "  if [ 3600 -lt \$wait_delay ] ; then \n";
        $cmd .= "    let wait_delay=3600\n";
        $cmd .= "  fi\n";
        $cmd .= "done\n";
    }
    
    if (scalar @requests > 1) {
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

# method: cache2work
#  Copie une dalle de la pyramide dans le répertoire de travail en la passant au format de cache.
#---------------------------------------------------------------------------------------------------
sub cache2work {
    my ($self, $node, $workName) = @_;

    my $tree = $self->{trees}[$self->{currentTree}];
    my @imgSize   = $self->{pyramid}->getCacheImageSize($node->{level}); # ie size tile image in pixel !
    my $cacheName = $self->{pyramid}->getCacheNameOfImage($node->{level}, $node->{x}, $node->{y}, 'data');

    INFO(sprintf "'%s'(cache) === '%s'(work)", $cacheName, $workName);

    if ($self->{pyramid}->getCompression() eq 'png') {
        # Dans le cas du png, l'opération de copie doit se faire en 3 étapes :
        #       - la copie du fichier dans le dossier temporaire
        #       - le détuilage (untile)
        #       - la fusion de tous les png en un tiff
        $tree->updateWeightOfNode($node,CACHE2WORK_PNG_W);
        my $dirName = $workName;
        $dirName =~ s/\.tif//;
        my $cmd =  sprintf ("cp \${PYR_DIR}/%s \${TMP_DIR}/%s\n", $cacheName , $workName);
        $cmd .=  sprintf ("mkdir \${TMP_DIR}/%s\n", $dirName);
        $cmd .=  sprintf ("%s \${TMP_DIR}/%s \${TMP_DIR}/%s/\n%s", UNTILE, $workName,$dirName, RESULT_TEST);
        
        $cmd .=  sprintf "montage -geometry %sx%s -tile %sx%s \${TMP_DIR}/%s/*.png -depth %s",
            $self->{pyramid}->getTileWidth(),$self->{pyramid}->getTileHeight(),
            $self->{pyramid}->getTilePerWidth(),$self->{pyramid}->getTilePerHeight(),
            $dirName,$self->{pyramid}->getBitsPerSample();
            
        $cmd .=  " -background none" if (int($self->{pyramid}->getSamplesPerPixel()) == 4);
        
        $cmd .=  sprintf " -define tiff:rows-per-strip=%s  \${TMP_DIR}/%s\n%s",
            $self->{pyramid}->getCacheImageWidth($node->{level}),$workName, RESULT_TEST;

        $cmd .=  sprintf ("rm -rf \${TMP_DIR}/%s/\n",$dirName);

        return $cmd;
    } else {
        # Pour le tiffcp on fixe le rowPerStrip au nombre de ligne de l'image ($imgSize[1])
        $tree->updateWeightOfNode($node,TIFFCP_W);
        my $cmd =  sprintf ("%s -r %s \${PYR_DIR}/%s \${TMP_DIR}/%s\n%s", CACHE_2_WORK_PRG, $imgSize[1], $cacheName , $workName, RESULT_TEST);
        return $cmd;
    }
}

# method: work2cache
#  Copie l'image de travail qui se trouve dans le répertoire temporaire vers la
#  pyramide en la passant au format de cache et en calculant sont nom de cache.
#
#  C'est donc un peu l'inverse de cache2work.
#---------------------------------------------------------------------------------------------------
sub work2cache {
    my $self = shift;
    my $node = shift;
  
    my $tree = $self->{trees}[$self->{currentTree}];
    my $workImgName  = $self->workNameOfNode($node);
    my $cacheImgName = $self->{pyramid}->getCacheNameOfImage($node->{level}, $node->{x}, $node->{y}, 'data'); 
    
    my $tm = $self->{pyramid}->getTileMatrixSet()->getTileMatrix($node->{level});
    my $compression = $self->{pyramid}->getCompression();
    my $compressionoption = $self->{pyramid}->getCompressionOption();
    
    # cas particulier de la commande tiff2tile :
    $compression = ($compression eq 'raw'?'none':$compression);
    
    # DEBUG: On pourra mettre ici un appel à convert pour ajouter des infos
    # complémentaire comme le quadrillage des dalles et le numéro du node, 
    # ou celui des tuiles et leur identifiant.
    DEBUG(sprintf "'%s'(work) === '%s'(cache)", $workImgName, $cacheImgName);
    
    # Suppression du lien pour ne pas corrompre les autres pyramides.
    my $cmd = sprintf ("if [ -r \"\${PYR_DIR}/%s\" ] ; then rm -f \${PYR_DIR}/%s ; fi\n", $cacheImgName, $cacheImgName);
    $cmd .= sprintf ("if [ ! -d \"\${PYR_DIR}/%s\" ] ; then mkdir -p \${PYR_DIR}/%s ; fi\n", dirname($cacheImgName), dirname($cacheImgName));
    $cmd .= sprintf ("%s \${TMP_DIR}/%s ", WORK_2_CACHE_PRG, $workImgName);
    $cmd .= sprintf ("-c %s ",    $compression);
    
    if ($compressionoption eq 'crop') {
      $cmd .= sprintf ("-crop ");
    }
    
    $cmd .= sprintf ("-p %s ",    $self->{pyramid}->getPhotometric());
    $cmd .= sprintf ("-t %s %s ", $tm->getTileWidth(), $tm->getTileHeight());
    $cmd .= sprintf ("-b %s ",    $self->{pyramid}->getBitsPerSample());
    $cmd .= sprintf ("-a %s ",    $self->{pyramid}->getSampleFormat());
    $cmd .= sprintf ("-s %s ",    $self->{pyramid}->getSamplesPerPixel());
    $cmd .= sprintf (" \${PYR_DIR}/%s\n", $cacheImgName);
    $cmd .= sprintf ("%s", RESULT_TEST);
    
    # Si on est au niveau du haut, il faut supprimer les images, elles ne seront plus utilisées
    
    if ($node->{level} eq $tree->{topLevelID}) {
        $cmd .= sprintf ("rm -f \${TMP_DIR}/%s\n", $workImgName);
    }
    
    return $cmd;
}

# method: mergeNtiff
#  compose la commande qui fusionne des images (mergeNtiff).
#---------------------------------------------------------------------------------------------------
sub mergeNtiff {
    my $self = shift;
    my $confFile = shift;
    my $dataType = shift; # param. are image or mtd to mergeNtiff script !

    TRACE;

    $dataType = 'image' if (  defined $dataType && $dataType eq 'data');
    $dataType = 'image' if (! defined $dataType);
    $dataType = 'mtd'   if (  defined $dataType && $dataType eq 'metadata');

    my $pyr = $self->{pyramid};
    #"bicubic"; # TODO l'interpolateur pour les mtd sera "nn"
    # TODO pour les métadonnées ce sera 0

    my $cmd = sprintf ("%s -f %s ",MERGE_N_TIFF, $confFile);
    $cmd .= sprintf ( " -i %s ", $pyr->getInterpolation());
    $cmd .= sprintf ( " -n %s ", $self->{nodata}->getValue() );
    $cmd .= sprintf (" -nowhite ") if ($self->{nodata}->{nowhite});
    $cmd .= sprintf ( " -t %s ", $dataType);
    $cmd .= sprintf ( " -s %s ", $pyr->getSamplesPerPixel());
    $cmd .= sprintf ( " -b %s ", $pyr->getBitsPerSample() );
    $cmd .= sprintf ( " -p %s ", $pyr->getPhotometric() );
    $cmd .= sprintf ( " -a %s\n",$pyr->getSampleFormat());
    $cmd .= sprintf ("%s" ,RESULT_TEST);

    $cmd .= sprintf ("rm -f %s\n" ,$confFile);

    return $cmd;
}

# method:merge4tiff
#  compose la commande qui calcule une dalle d'un niveau de la pyramide en sous échantillonnant 4 dalles du niveau inférieur
#  et en les répartissant ainsi: NO, NE, SO, SE. 
#---------------------------------------------------------------------------------------------------
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



# method: workNameOfNode
#  Renvoie le nom de l'image de travail désignée par node. C'est un nom conventionnel facilitant 
#  l'analyse du répertoire de travail lors du débug.
#
#  ex: 'level'_'idxX'_'idxY'.tif
#---------------------------------------------------------------------------------------------------
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

# method: getRootTmpDir
#  Retourne le répertoire de travail de la pyramide
#-------------------------------------------------------------------------------
sub getRootTmpDir {
  my $self = shift;
  my $pyrName = $self->{pyramid}->getPyrName();
  return File::Spec->catdir($self->{path_temp}, $pyrName);
  # ie .../TMP/PYRNAME/
}

# method: getScriptTmpDir
#  Retourne le répertoire de travail du script
#  NOTE
#  le code commenté permettra de rétablir un tri des fichiers temporaires dans différents dossiers
#-------------------------------------------------------------------------------------------------
sub getScriptTmpDir {
  my $self = shift;
  my $pyrName = $self->{pyramid}->getPyrName();
  return File::Spec->catdir($self->{path_temp}, $pyrName);
  #return File::Spec->catdir($self->{path_temp}, $pyrName, "\${SCRIPT_ID}/");
  # ie .../TMP/PYRNAME/SCRIPT_X/
}

# method: prepareScript
#  Initilise le script avec les chemins du répertoire temporaire, de la pyramide et des dalles noData.
#-------------------------------------------------------------------------------------------------
sub prepareScript {
    my $self = shift;
    my $scriptId = shift;

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

    # creation du répertoire de travail:
    $code .= "# creation du repertoire de travail\n";
    $code .= "if [ ! -d \"\${TMP_DIR}\" ] ; then mkdir -p \${TMP_DIR} ; fi\n\n";

    return $code;
}

# method: saveScript
#  ajoute la commande de suppression des tuiles le cas échéant
#  sauvegarde le script.
#-------------------------------------------------------------------------------
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

# method: collectWorkImage
#  Récupère les images au format de travail dans les répertoires temporaires des
#  scripts de calcul du bas de la pyramide, pour les copier dans le répertoire
#  temporaire du script final.
#  NOTE
#  Pas utilisé pour le moment
#--------------------------------------------------------------------------------------------
sub collectWorkImage(){
  my $self = shift;
  my ($node, $scriptId, $finishId) = @_;
  
  TRACE;
  
  my $source = File::Spec->catfile( '${ROOT_TMP_DIR}', $scriptId, $self->workNameOfNode($node));
  my $code   = sprintf ("mv %s \$TMP_DIR \n", $source);

  return $code;
}



# method: processScript
#  Execution des scripts les uns apres les autres.
#  (Orienté maintenance)
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
        $harvestingParams,
        $objPyramid
    );

    # Scripts generation
    if (! $objProcess->computeWholeTree()) {
        ERROR ("Can not compute process !");
        return FALSE;
    }

=head1 DESCRIPTION

    A Process object

        * pyramid : Pyramid object
        * nodata : NoData object
        * harvesting : Harvesting object
        * trees : array of Tree objects
        * job_number : work will be shared between job_number scripts, and a SCRIPT_FINISHER
        * scripts : array which contains scripts' names
        * path_temp : work directory, for temporary files
        * path_shell : be4 execution directory


=head1 LIMITATION & BUGS


=head1 SEE ALSO

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

    Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

    Copyright (C) 2011 by Satabin Théo

    This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself,
    either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
