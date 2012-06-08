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
# booleans
use constant TRUE  => 1;
use constant FALSE => 0;
# commands
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
    * harvesting => undef, # Harvesting object
    * trees => [], # array of Tree objects
    * job_number => undef,
    * scripts => [],    # list of script
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
        pyramid    => undef,
        #
        job_number => undef,
        path_temp  => undef,
        path_shell => undef,
        # 
        trees       => [],
        nodata     => undef,
        harvesting => undef,
        # out
        scripts    => [],
    };
    bless($self, $class);

    TRACE;
    
    return undef if (! $self->_init(@_));

    return $self;
}

sub _init {
    my ($self, $params_process, $params_harvest, $pyr) = @_;

    TRACE;

    # it's an object and it's mandatory !
    $self->{pyramid} = $pyr;

    if (! defined $self->{pyramid}  || ref ($self->{pyramid}) ne "BE4::Pyramid") {
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
        ERROR("Can not load NoData Tile !");
        return FALSE;
    }

    if ($self->{pyramid}->getDataSource()->getType() eq 'image' &&
        (($self->{pyramid}->getDataSource()->getSRS() ne $self->{pyramid}->getTileMatrixSet()->getSRS()) ||
        (! $self->{pyramid}->isNewPyramid() && ($self->{pyramid}->getCompression() eq 'jpg'))))
    {
        $self->{harvesting} = BE4::Harvesting->new($params_harvest);

        if (! defined $self->{harvesting}) {
            ERROR ("Can not load Harvest service !");
            return FALSE;
        }

        DEBUG (sprintf "HARVESTING = %s", Dumper($self->{harvesting}));
    }

    my $dataSource = $self->{pyramid}->getDataSource();
    my $TMS = $self->{pyramid}->getTileMatrixSet();

    push @{$self->{trees}},BE4::Tree->new($self->{pyramid}->getDataSource(), $self->{pyramid}, $self->{job_number});

    if (! defined $self->{tree}) {
        ERROR("Can not create Tree object !");
        return FALSE;
    }
    
    DEBUG (sprintf "TREE = %s", Dumper($self->{tree}));

    return TRUE;
}


####################################################################################################
#                                  GENERAL COMPUTING METHOD                                        #
####################################################################################################

# Group: general computing method

# method: computeWholeTree
#  Crée tous les script permettant de calculer les images de la pyramide.
#
#  NOTE
#  Il y a exactement jobNbr +1 scripts crées :
#       - Les scripts du début (du bottomLevel au cutLevel) se nomment SCRIPT_X.sh
#       - Le script final (du cutLevel au topLevel) se nomme SCRIPT_FINISHER.sh
#  Ils peuvent être vides.
#
#  Pour ce faire, on exécute 3 étapes :
#       - Le parcours de l'arbre : à chaque noeud, on définit le poids et le code correspondant à
#         sa construction (image)
#       - L'arbre étant pondéré, on peut définir le cutLevel, qui optimisera la répartition et le
#         temps total de génération de la pyramide.
#       - Les commande étant déjà écrites, il ne reste plus qu'à parcourir l'arbre et concaténer
#         les bouts de code, et les écrire dans les scripts.
#---------------------------------------------------------------------------------------------------
sub computeWholeTree {
    my $self = shift;

    TRACE;

    # initialisation du script final
    my $pyrName = $self->{pyramid}->getPyrName();
    my $finishScriptId   = "SCRIPT_FINISHER";
    my $finishScriptCode = $self->prepareScript($finishScriptId);

    # Pondération de l'arbre en fonction des opérations à réaliser
    # et création du code script (ajoter aux noeuds de l'arbre
    my @topLevelNodeList = $self->{tree}->getNodesOfTopLevel();
    foreach my $topNode (@topLevelNodeList) {
        if (! $self->computeBranch($topNode)) {
            ERROR(sprintf "Can not compute the node of the top level '%s'!", Dumper($topNode));
            return FALSE;
        }
    }

    # Détermination du cutLevel optimal et répartition des noeuds sur les jobs
    my @nodeRack = $self->{tree}->shareNodesOnJobs();
    if (! scalar @nodeRack) {
        ERROR("Cut Level Node List is empty !");
        return FALSE;
    }
    INFO (sprintf "CutLevel : %s", $self->{tree}->{cutLevelId});

    $finishScriptCode .= "#recuperation des images calculees par les scripts precedents\n";

    # écriture des scripts
    for (my $scriptCount=1; $scriptCount<=$self->{job_number}; $scriptCount++){
        my $scriptId   = sprintf "SCRIPT_%s", $scriptCount;
        # record scripid
        push @{$self->{scripts}}, $scriptId;
        #
        my $scriptCode;
        if (! defined($nodeRack[$scriptCount-1])){
            $scriptCode = "echo \"Le script \$0 n'a rien a faire. Tout va bien, c'est normal, on n'a pas de travail pour lui.\"";
        } else {
            $scriptCode = $self->prepareScript($scriptId);
            foreach my $node (@{$nodeRack[$scriptCount-1]}){
                INFO (sprintf "Node '%s-%s-%s' into 'SCRIPT_%s'.", $node->{level} ,$node->{x}, $node->{y}, $scriptCount);
                $scriptCode .= sprintf "\necho \"PYRAMIDE:%s   LEVEL:%s X:%s Y:%s\"", $pyrName, $node->{level} ,$node->{x}, $node->{y}; 
                $scriptCode .= $self->writeBranchCode($node);
                # NOTE : pas d'image à collecter car non séparation par script du dossier temporaire, de travail
                # A rétablir si possible
                # on récupère l'image de travail finale pour le job de fin.
                # $finishScriptCode .= $self->collectWorkImage($node, $scriptId, $finishScriptId);
            }
        }
        if (! $self->saveScript($scriptCode,$scriptId)) {
            ERROR(sprintf "Can not save the script '%s' !", $scriptId);
            return FALSE;
        }
    }

    # creation du script final

    $finishScriptCode .= $self->writeTopCodes();

    if (! defined $finishScriptCode) {
        ERROR();
        return FALSE;
    }

    if (! $self->saveScript($finishScriptCode, $finishScriptId)) {
        ERROR(sprintf "Can not save the script FINISHER '%s' !", $finishScriptId);
        return FALSE;
    }

    # record scripid
    push @{$self->{scripts}}, $finishScriptId;

    return TRUE;
}

####################################################################################################
#                                  COMPUTING/WEIGHTER METHOD                                       #
####################################################################################################

# Group: computing / weighter method

# method: computeBottomImage 
#  Construction de l'image du bas de la pyramide désignée par 'node'.
#  On détermine le poids et le code correspondant à la génération de l'image du bas
#
# NOTE:
#  Si les dalles sources ne sont pas dans la projection de la pyramide on utilise le WMS pour
# obtenir des données dans la bonne projection. Du coup autant demander la dalle entière
# et on n'a pas besoin d'appeller mergeNtiff.
#
#  Si les dalles sources sont dans la projection de la pyramides MAIS que la pyramide utilise
# une compression avec perte (JPEG), on est contraint d'utiliser le WMS pour obtenir 
# la dalle de fond. Du coup, on peut récupérer la nouvelle dalle complète et se passer de 
# mergeNtiff.
#
#  Dans le cas JPEG en projection native, on a effectivement besoin d'une image de 
# fond pour le mergeNtiff que quand les données sources ne couvrent pas entièrement la
# dalle. On ne fait pas la distinction ici par soucis de simplicité du code et parce que 
# l'efficacité n'est probablement pas moins bonne.
#
# TODO:
#  Dans le cas du PNG qui ne dégrade pas les images, on utilise untile+montage. Il faudrait écrire
#  un outil améliorant cette tâche.
#---------------------------------------------------------------------------------------------------
sub computeBottomImage {
    my $self = shift;
    my $node = shift;

    TRACE;

    my $weight  = 0;
    my $code  = "\n";

    my $bgImgPath=undef;
    my $bgImgName=undef;
  
# FIXME (TOS) Ici, on fait appel au service WMS sans vérifier que la zone demandée n'est pas trop grande.
# On considère que le niveau le plus bas de la pyramide que l'on est en train de construire n'est pas trop élevé.
# A terme, il faudra vérifier cette zone et ne demander que les tuiles contenant des données, et reconstruire une
# image entière à partir de là (en ajoutant sur place des tuiles de nodata pour compléter).

    if (($self->{pyramid}->getDataSource()->getSRS() ne $self->{pyramid}->getTileMatrixSet()->getSRS())
    ||
    (! $self->{pyramid}->isNewPyramid() && ($self->{pyramid}->getCompression() eq 'jpg')))
    {
        $code .= $self->wms2work($node,$self->workNameOfNode($node));
        $self->{tree}->updateWeightOfNode($node,WGET_W);
    } else {
        my $newImgDesc = $self->{tree}->getImgDescOfNode($node);
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
        my $listDesc = $self->{tree}->getImgDescOfBottomNode($node);
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

    $self->{tree}->updateWeightOfNode($node,$weight);
    $self->{tree}->setComputingCode($node,$code);

    return TRUE;
}

# method: computeAboveImage
#  Construction d'une image pyramide qui n'est pas au bottomLevel.
#  On détermine le poids et le code correspondant à la génération de l'image qui n'est pas en bas
#
#  NOTE:
#  Le choix d'utiliser ou non le WMS pour les niveaux supérieurs ne dépend pas de la 
# projection puisque les données viennent du niveau inférieur de la pyramide.
#
#  Par contre, on a le problème avec les images de fond en JPEG et PNG. Mais contrairement a
# ce qui est fait dans computeBottomImage(),
# on ne souhaite pas ne faire que récupérer la nouvelle dalle sur le WMS parce que cela ne 
# permettrait pas de faire remonter un défaut d'un niveau inférieur dans les niveaux du 
# dessus, et l'opérateur de validation ne pourait pas utiliser les petites échelles pour avoir
# une vision globale de son chantier. Il devrait inspecter chaque niveau de la pyramide ce 
# qui n'est pas envisageable.
#
#  Conclusion, on ne fait de wget pour l'image de fond que quand on en a réellement besoin,
# c'est à dire quand on a pas mis à jour les 4 dalles correspondantes au niveau inférieur. Et
# même quand on fait le wget, on fait quand même le merge4tiff pour faire remonter les 
# éventuels défauts.
#---------------------------------------------------------------------------------------------------
sub computeAboveImage {
    my $self = shift;
    my $node = shift;

    TRACE;

    my $code = "\n";
    my $weight = 0;
    my $newImgDesc = $self->{tree}->getImgDescOfNode($node);
    my @childList = $self->{tree}->getChilds($node);

    my $bgImgPath=undef;
    my $bgImgName=undef;

    # A-t-on besoin de quelque chose en fond d'image?
    my $bg="";
    if (scalar @childList != 4){

        # Pour cela, on va récupérer le nombre de tuiles (en largeur et en hauteur) du niveau, et 
        # le comparer avec le nombre de tuile dans une image (qui est potentiellement demandée à 
        # rok4, qui n'aime pas). Si l'image contient plus de tuile que le niveau, on ne demande plus
        # (c'est qu'on a déjà tout ce qui faut avec les niveaux inférieurs).

        # WARNING (TOS) cette solution n'est valable que si la structure de l'image (nombre de tuile dans l'image si
        # tuilage il y a) est la même entre le cache moissonné et la pyramide en cours d'écriture.
        # On a à l'heure actuelle du 16 sur 16 sur toute les pyramides et pas de JPEG 2000. 

        my $tm = $self->{tree}->getTileMatrix($node->{level});
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
            $bg="-b $bgImgPath";

            if ($self->{pyramid}->getCompression() eq 'jpg') {
                # On vérifie d'abord qu'on ne veut pas moissonner une zone trop grande
                if ($tooWide || $tooHigh) {
                    WARN(sprintf "The image would be too high or too wide for this level (%s)",$node->{level});
                    # On ne peut pas avoir d'image alors on donne une couleur de nodata
                    $bg='-n ' . $self->{nodata}->getColor();
                } else {
                    # On peut et doit chercher l'image de fond sur le WMS
                    $weight += WGET_W;
                    $code .= $self->wms2work($node, $bgImgName);
                }
            } else {
                # copie avec tiffcp ou untile+montage pour passer du format de cache au format de travail.
                $code .= $self->cache2work($node, $bgImgName);
            }
        } else {
            # On a pas d'image alors on donne une couleur de nodata
            $bg='-n ' . $self->{nodata}->getColor();
        }
    }

    # Maintenant on constitue la liste des images à passer à merge4tiff.
    my $childImgParam=''; 
    my $imgCount=0;
    foreach my $childNode ($self->{tree}->getPossibleChilds($node)){
        $imgCount++;
        if ($self->{tree}->isInTree($childNode)){
            $childImgParam.=' -i'.$imgCount.' $TMP_DIR/' . $self->workNameOfNode($childNode)
        }
    }
    $code .= $self->merge4tiff('$TMP_DIR/' . $self->workNameOfNode($node), $bg, $childImgParam);

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

    $self->{tree}->updateWeightOfNode($node,$weight);
    $self->{tree}->setComputingCode($node,$code);

    return TRUE;
}

# method: computeBranch
#  On détermine le poids et le code correspondant à la génération de toute les images d'une branche
#---------------------------------------------------------------------------------------------------
sub computeBranch {
    my $self = shift;
    my $node = shift;

    my $weight = 0;

    TRACE;
    DEBUG(sprintf "Search in Level %s (idx: %s - %s)", $node->{level}, $node->{x}, $node->{y});

    my $res = '';
    my @childList = $self->{tree}->getChilds($node);
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
        $weight += $self->{tree}->getAccumulatedWeightOfNode($n);
    }

    if (! $self->computeAboveImage($node)) {
        ERROR(sprintf "Cannot compute the above image : %s_%s, level %s)", $node->{x}, $node->{y}, $node->{level});
        return FALSE;
    }

    $self->{tree}->setAccumulatedWeightOfNode($node,$weight);

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

    my $code = '';
    my @childList = $self->{tree}->getChilds($node);

    # Le noeud est une feuille
    if (scalar @childList == 0){
        return $self->{tree}->getComputingCode($node);
    }

    # Le noeud a des enfants
    foreach my $n (@childList){
        $code .= $self->writeBranchCode($n);
    }
    $code .= $self->{tree}->getComputingCode($node);

    return $code;
}

# method: writeTopCodes
#  On assemble les bouts de code de tous les noeuds du haut de l'arbre, jusqu'au cutLevel
#---------------------------------------------------------------------------------------------------
sub writeTopCodes {
    my $self = shift;

    TRACE;

    if ($self->{tree}->getTopLevelId() eq $self->{tree}->getCutLevelId()){
        INFO("Final script will be empty");
        return "echo \"Final script have nothing to do.\" \n";
    }

    my $code = '';
    my @nodeList = $self->{tree}->getNodesOfTopLevel();
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

    # Rien à faire, le niveau CutLevel est déjà fait et les images de travail sont déjà là. 
    return '' if ($node->{level} eq $self->{tree}->getCutLevelId());

    my $code = '';
    my @childList = $self->{tree}->getChilds($node);
    foreach my $n (@childList){
        $code .= $self->writeTopCode($n);
    }
    $code .= $self->{tree}->getComputingCode($node);

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
  
  my $imgDesc = $self->{tree}->getImgDescOfNode($node);
  my @imgSize = $self->{pyramid}->getCacheImageSize(); # ie size tile image in pixel !
  my $tms     = $self->{pyramid}->getTileMatrixSet();
  my $url     = $self->{harvesting}->doRequestUrl(
                                                   srs      => $tms->getSRS(),
                                                   bbox     => [$imgDesc->{xMin},
                                                                $imgDesc->{yMin},
                                                                $imgDesc->{xMax},
                                                                $imgDesc->{yMax}],
                                                   imagesize => [$imgSize[0], $imgSize[1]]
                                                   );
  
  my $cmd="";

  $cmd .= "count=0; wait_delay=60\n";
  $cmd .= "while :\n";
  $cmd .= "do\n";
  $cmd .= "  let count=count+1\n";
  $cmd .= "  wget --no-verbose -O \${TMP_DIR}/$fileName \"$url\" \n";
  $cmd .= "  if tiffck \${TMP_DIR}/$fileName ; then break ; fi\n";
  $cmd .= "  echo \"echec \$count : wait for \$wait_delay s\"\n";
  $cmd .= "  sleep \$wait_delay\n";
  $cmd .= "  let wait_delay=wait_delay*2\n";
  $cmd .= "  if [ 3600 -lt \$wait_delay ] ; then \n";
  $cmd .= "    let wait_delay=3600\n";
  $cmd .= "  fi\n";
  $cmd .= "done\n";
  
  return $cmd;
}

# method: cache2work
#  Copie une dalle de la pyramide dans le répertoire de travail en la passant au format de cache.
#---------------------------------------------------------------------------------------------------
sub cache2work {
    my ($self, $node, $workName) = @_;

    my @imgSize   = $self->{pyramid}->getCacheImageSize(); # ie size tile image in pixel !
    my $cacheName = $self->{pyramid}->getCacheNameOfImage($node->{level}, $node->{x}, $node->{y}, 'data');

    INFO(sprintf "'%s'(cache) === '%s'(work)", $cacheName, $workName);

    if ($self->{pyramid}->getCompression() eq 'png') {
        # Dans le cas du png, l'opération de copie doit se faire en 3 étapes :
        #       - la copie du fichier dans le dossier temporaire
        #       - le détuilage (untile)
        #       - la fusion de tous les png en un tiff
        $self->{tree}->updateWeightOfNode($node,CACHE2WORK_PNG_W);
        my $dirName = $workName;
        $dirName =~ s/\.tif//;
        my $cmd =  sprintf ("cp \${PYR_DIR}/%s \${TMP_DIR}/%s\n", $cacheName , $workName);
        $cmd .=  sprintf ("mkdir \${TMP_DIR}/%s\n", $dirName);
        $cmd .=  sprintf ("%s \${TMP_DIR}/%s \${TMP_DIR}/%s/\n%s", UNTILE, $workName,$dirName, RESULT_TEST);
        
        if (int($self->{pyramid}->getSamplesPerPixel()) == 4) {
            # Option supplémentaire pour conserver le canal alpha
            $cmd .=  sprintf ("montage -geometry 256x256 -tile 16x16 \${TMP_DIR}/%s/*.png -depth %s -background none -define tiff:rows-per-strip=4096  \${TMP_DIR}/%s\n%s",$dirName, $self->{pyramid}->getBitsPerSample(), $workName, RESULT_TEST);
        } else {
            $cmd .=  sprintf ("montage -geometry 256x256 -tile 16x16 \${TMP_DIR}/%s/*.png -depth %s -define tiff:rows-per-strip=4096  \${TMP_DIR}/%s\n%s",$dirName, $self->{pyramid}->getBitsPerSample(), $workName, RESULT_TEST);
        }

        $cmd .=  sprintf ("rm -rf \${TMP_DIR}/%s/\n",$dirName);

        return $cmd;
    } else {
        # Pour le tiffcp on fixe le rowPerStrip au nombre de ligne de l'image ($imgSize[1])
        $self->{tree}->updateWeightOfNode($node,TIFFCP_W);
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
  
  my $workImgName  = $self->workNameOfNode($node);
  my $cacheImgName = $self->{pyramid}->getCacheNameOfImage($node->{level}, $node->{x}, $node->{y}, 'data'); 
  
  my $tms = $self->{pyramid}->getTileMatrixSet();
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
  $cmd .= sprintf ("-t %s %s ", $tms->getTileWidth(), $tms->getTileHeight()); # ie size tile 256 256 pix !
  $cmd .= sprintf ("-b %s ",    $self->{pyramid}->getBitsPerSample());
  $cmd .= sprintf ("-a %s ",    $self->{pyramid}->getSampleFormat());
  $cmd .= sprintf ("-s %s ",    $self->{pyramid}->getSamplesPerPixel());
  $cmd .= sprintf (" \${PYR_DIR}/%s\n", $cacheImgName);
  $cmd .= sprintf ("%s", RESULT_TEST);

  # Si on est au niveau du haut, il faut supprimer les images, elles ne seront plus utilisées

  if ($node->{level} eq $self->{tree}->{topLevelId}) {
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
    $cmd .= sprintf ( " -n %s ", $self->{nodata}->getColor() );
    $cmd .= sprintf (" -nowhite ") if ($self->{nodata}->{nowhite});
    $cmd .= sprintf ( " -t %s ", $dataType);
    $cmd .= sprintf ( " -s %s ", $pyr->getSamplesPerPixel());
    $cmd .= sprintf ( " -b %s ", $pyr->getBitsPerSample() );
    $cmd .= sprintf ( " -p %s ", $pyr->getPhotometric() );
    $cmd .= sprintf ( " -a %s\n",$pyr->getSampleFormat());
    $cmd .= sprintf ("%s" ,RESULT_TEST);
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
    $code   .= sprintf ("NODATA_DIR=\"%s\"\n", $self->{nodata}->getPath());
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
    
    BE4::Tree
    BE4::NoData
    BE4::Pyramid
    BE4::Harvesting

=head1 AUTHOR

    Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

    Copyright (C) 2011 by Satabin Théo

    This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself,
    either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
