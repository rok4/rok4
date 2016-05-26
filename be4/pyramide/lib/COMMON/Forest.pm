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
File: Forest.pm

Class: COMMON::Forest

Creates and manages all graphs, <NNGraph> and <QTree>.

(see forest.png)

We have several kinds of graphs and their using have to be transparent for the forest. That's why we must define functions for all graph's types (as an interface) :
    - computeYourself() : <NNGraph::computeYourself>, <QTree::computeYourself>
    - containsNode(level, i, j) : <NNGraph::containsNode>, <QTree::containsNode>
    - exportForDebug() : <NNGraph::exportForDebug>, <QTree::exportForDebug>

Using:
    (start code)
    use COMMON::Forest

    my $Forest = COMMON::Forest->new(
        $objPyramid, # a BE4::Pyramid object
        $objDSL, # a COMMON::DataSourceLoader object
        $param_process, # a hash with following keys : job_number, path_temp, path_temp_common and path_shell
        $storageType, # final storage : FS, CEPH or SWIFT
    );
    (end code)

Attributes:
    pyramid - <Pyramid> - Images' pyramid to generate, thanks to one or several graphs.
    commands - <Commands> - To compose generation commands (mergeNtiff, tiff2tile...).
    graphs - <QTree> or <NNGraph> array - Graphs composing the forest, one per data source.
    scripts - <Script> array - Scripts, whose execution generate the images' pyramid.
    splitNumber - integer - Number of script used for work parallelization.
    storageType - string - Pyramid final storage type : FS, CEPH or SWIFT

=cut

################################################################################

package COMMON::Forest;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use List::Util qw(min max);

use Geo::GDAL;

# My module
use COMMON::QTree;
use COMMON::NNGraph;
use BE4::Commands;
use BE4CEPH::Commands;
use BE4::Pyramid;
use BE4CEPH::Pyramid;
use COMMON::GraphScript;
use COMMON::DataSourceLoader;
use COMMON::DataSource;

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

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Forest constructor. Bless an instance.

Parameters (list):
    pyr - <BE4::Pyramid> or <BE4CEPH::Pyramid> - Contains output format specifications, needed by generations command's.
    DSL - <DataSourceLoader> - Contains one or several data sources
    params_process - hash - Informations for scripts
|               job_number - integer - Parallelization level
|               path_temp - string - Temporary directory
|               path_temp_common - string - Common temporary directory
|               path_shell - string - Script directory
    storageType - string - Pyramid final storage type : FS, CEPH or SWIFT

See also:
    <_init>, <_load>
=cut
sub new {
    my $this = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        pyramid     => undef,
        commands    => undef,
        graphs      => [],
        scripts     => [],
        splitNumber => undef,
        storageType => undef
    };

    bless($self, $class);

    # init. class
    return undef if (! $self->_init(@_));
    
    # load. class
    return undef if (! $self->_load(@_));
    
    INFO (sprintf "Graphs' number : %s",scalar @{$self->{graphs}});

    return $self;
}

=begin nd
Function: _init

Checks parameters and stores the pyramid.

Parameters (list):
    pyr - <BE4::Pyramid> or <BE4CEPH::Pyramid> - Contains output format specifications, needed by generations command's.
    DSL - <COMMON::DataSourceLoader> - Contains one or several data sources
    params_process - hash - Informations for scipts, where to write them, temporary directory to use...
|               job_number - integer - Parallelization level
|               path_temp - string - Temporary directory
|               path_temp_common - string - Common temporary directory
|               path_shell - string - Script directory
    storageType - string - Pyramid final storage type : FS, CEPH or SWIFT

=cut
sub _init {
    my $self = shift;
    my $pyr = shift;
    my $DSL = shift;
    my $params_process = shift;
    my $storageType = shift;

    # it's an object and it's mandatory !
    if (! defined $pyr || (ref ($pyr) ne "BE4::Pyramid" && ref ($pyr) ne "BE4CEPH::Pyramid")) {
        ERROR("Can not load Pyramid !");
        return FALSE;
    }
    $self->{pyramid} = $pyr;
    
    # it's an object and it's mandatory !
    if (! defined $DSL || ref ($DSL) ne "COMMON::DataSourceLoader") {
        ERROR("Can not load data sources !");
        return FALSE;
    }

    if (! defined $params_process) {
        ERROR("We need process' parameters !");
        return FALSE;
    }

    if (! defined $storageType || "|FS|SWIFT|CEPH|" !~ m/\|$storageType\|/) {
        ERROR("Forest's storage type is undef or not valid !");
        return FALSE;
    }
    $self->{storageType} = $storageType;
    
    return TRUE;
}

=begin nd
Function: _load

Creates a <NNGraph> or a <QTree> object per data source and a <Commands> object. Using a QTree is faster but it does'nt match all cases.

All differences between different kinds of graphs are handled in respective classes, in order to be imperceptible for users.

Only scripts creation and initial organization are managed by the forest.

Parameters (list):
    pyr - <Pyramid> - Contains output format specifications, needed by generations command's.
    DSL - <DataSourceLoader> - Contains one or several data sources
    params_process - hash - Informations for scipts, where to write them, temporary directory to use...
|               job_number - integer - Parallelization level
|               path_temp - string - Temporary directory
|               path_temp_common - string - Common temporary directory
|               path_shell - string - Script directory
    storageType - string - Pyramid final storage type : FS, CEPH or SWIFT

=cut
sub _load {
    my $self = shift;
    my $pyr = shift;
    my $DSL = shift;
    my $params_process = shift;
    my $storageType = shift;

    my $dataSources = $DSL->getDataSources;
    my $TMS = $pyr->getTileMatrixSet;
    my $isQTree = $TMS->isQTree();
    
    ######### PARAM PROCESS ###########
    
    my $splitNumber = $params_process->{job_number};
    my $tempDir = $params_process->{path_temp};
    my $commonTempDir = $params_process->{path_temp_common};
    my $scriptDir = $params_process->{path_shell};

    if (! defined $splitNumber) {
        ERROR("Parameter required : 'job_number' in section 'Process' !");
        return FALSE;
    }
    $self->{splitNumber} = $splitNumber;

    if (! defined $tempDir) {
        ERROR("Parameter required : 'path_temp' in section 'Process' !");
        return FALSE;
    }

    if (! defined $commonTempDir) {
        ERROR("Parameter required : 'path_temp_common' in section 'Process' !");
        return FALSE;
    }

    if (! defined $scriptDir) {
        ERROR("Parameter required : 'path_shell' in section 'Process' !");
        return FALSE;
    }

    # Ajout du nom de la pyramide aux dossiers temporaires (pour distinguer de ceux des autres générations)
    $tempDir = File::Spec->catdir($tempDir,$self->{pyramid}->getNewName);
    $commonTempDir = File::Spec->catdir($commonTempDir,$self->{pyramid}->getNewName);

    ############# PROCESS #############

    my $commands;

    if ($storageType eq 'FS') {
        $commands = BE4::Commands->new($pyr,$params_process->{use_masks});
    }
    elsif ($storageType eq 'CEPH') {
        $commands = BE4CEPH::Commands->new($pyr,$params_process->{use_masks});
    }
    elsif ($storageType eq 'SWIFT') {
        ERROR("Swift storage not yet implemented");
        return FALSE;
    }

    if (! defined $commands) {
        ERROR ("Can not load Commands !");
        return FALSE;
    }
    $self->{commands} = $commands;
    DEBUG(sprintf "COMMANDS (debug export) = %s", $commands->exportForDebug);
    
    ############# SCRIPTS #############
    # We create COMMON::GraphScript objects and initialize them (header)

    my $functions = $commands->configureFunctions();
    $functions .= COMMON::Commands::configureFunctions();

    if ($isQTree) {
        #### QTREE CASE

        for (my $i = 0; $i <= $self->getSplitNumber; $i++) {
            my $scriptID = sprintf "SCRIPT_%s",$i;
            my $executedAlone = FALSE;

            if ($i == 0) {
                $scriptID = "SCRIPT_FINISHER";
                $executedAlone = TRUE;
            }

            my $script = COMMON::GraphScript->new({
                id => $scriptID,
                tempDir => $tempDir,
                commonTempDir => $commonTempDir,
                scriptDir => $scriptDir,
                executedAlone => $executedAlone
            });

            my $listFile = $self->{pyramid}->getNewListFile;
            $script->prepare($self->{pyramid},$listFile,$functions);

            push @{$self->{scripts}}, $script;
        }
    } else {
        #### GRAPH CASE

        # Boucle sur les levels et sur le nb de scripts/jobs
        # On commence par les finishers
        # On continue avec les autres scripts, par level
        for (my $i = $pyr->getBottomOrder - 1; $i <= $pyr->getTopOrder; $i++) {
            for (my $j = 1; $j <= $self->getSplitNumber; $j++) {
                my $scriptID;
                if ($i == $pyr->getBottomOrder - 1) {
                    $scriptID = sprintf "SCRIPT_FINISHER_%s", $j;
                } else {
                    my $levelID = $self->getPyramid()->getIDfromOrder($i);
                    $scriptID = sprintf "LEVEL_%s_SCRIPT_%s", $levelID, $j;
                }

                my $script = COMMON::GraphScript->new({
                    id => $scriptID,
                    tempDir => $tempDir,
                    commonTempDir => $commonTempDir,
                    scriptDir => $scriptDir,
                    executedAlone => FALSE
                });

                my $listFile = $self->{pyramid}->getNewListFile;
                $script->prepare($self->{pyramid},$listFile,$functions);

                push @{$self->{scripts}},$script;
            }
        }

        # Le SUPER finisher
        my $script = COMMON::GraphScript->new({
            id => "SCRIPT_FINISHER",
            tempDir => $tempDir,
            commonTempDir => $commonTempDir,
            scriptDir => $scriptDir,
            executedAlone => TRUE
        });

        my $listFile = $self->{pyramid}->getNewListFile;
        $script->prepare($self->{pyramid}->getNewDataDir,$listFile,$functions);

        push @{$self->{scripts}},$script;
    }
    
    ######## PROCESS (suite) #########

    $commands->setConfDir($self->{scripts}[0]->getMntConfDir(), $self->{scripts}[0]->getDntConfDir());
    
    ############# GRAPHS #############

    foreach my $datasource (@{$dataSources}) {
        
        # Now, if datasource contains a WMS service, we have to use it
        
        # Creation of QTree or NNGraph object
        my $graph = undef;
        if ($isQTree) {
            $graph = COMMON::QTree->new($self, $datasource, $self->{pyramid}, $self->{commands});
        } else {
            $graph = COMMON::NNGraph->new($self,$datasource, $self->{pyramid}, $self->{commands});
        };
                
        if (! defined $graph) {
            ERROR(sprintf "Can not create a graph for datasource with bottom level %s !",$datasource->getBottomID);
            return FALSE;
        }
        
        push @{$self->{graphs}},$graph;
    }

    return TRUE;
}


####################################################################################################
#                                  Group: Graphs tools                                             #
####################################################################################################

=begin nd
Function: containsNode

Returns a boolean : TRUE if the node belong to this forest, FALSE otherwise (if a parameter is not defined too).

Parameters (list):
    level - string - Level ID of the node we want to know if it is in the forest.
    i - integer - Column of the node we want to know if it is in the forest.
    j - integer - Row of the node we want to know if it is in the forest.
=cut
sub containsNode {
    my $self = shift;
    my $level = shift;
    my $i = shift;
    my $j = shift;

    return FALSE if (! defined $level || ! defined $i || ! defined $j);
    
    foreach my $graph (@{$self->{graphs}}) {
        return TRUE if ($graph->containsNode($level,$i,$j));
    }
    
    return FALSE;
}

=begin nd
Function: computeGraphs

Computes each <NNGraph> or <QTree> one after the other and closes scripts to finish.

See Also:
    <NNGraph::computeYourself>, <QTree::computeYourself>
=cut
sub computeGraphs {
    my $self = shift;

    TRACE;
    
    my $graphInd = 1;
    my $graphNumber = scalar @{$self->{graphs}};
    
    foreach my $graph (@{$self->{graphs}}) {
        if (! $graph->computeYourself) {
            ERROR(sprintf "Cannot compute graph $graphInd/$graphNumber");
            return FALSE;
        }
        INFO("Graph $graphInd/$graphNumber computed");
        DEBUG($graph->exportForDebug);
        $graphInd++;
    }
    
    foreach my $script (@{$self->{scripts}}) {
        $script->close;
    }
    
    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getGraphs
sub getGraphs {
    my $self = shift;
    return $self->{graphs}; 
}

# Function: getStorageType
sub getStorageType {
    my $self = shift;
    return $self->{storageType}; 
}

# Function: getPyramid
sub getPyramid {
    my $self = shift;
    return $self->{pyramid}; 
}

# Function: getScripts
sub getScripts {
    my $self = shift;
    return $self->{scripts};
}

=begin nd
Function: getScript

Parameters (list):
    ind - integer - Script's indice in the array
=cut
sub getScript {
    my $self = shift;
    my $ind = shift;
    
    return $self->{scripts}[$ind];
}

=begin nd
Function: getWeightOfScript

Parameters (list):
    ind - integer - Script's indice in the array
=cut 
sub getWeightOfScript {
    my $self = shift;
    my $ind = shift;
    
    return $self->{scripts}[$ind]->getWeight;
}

=begin nd
Function: setWeightOfScript

Parameters (list):
    ind - integer - Script's indice in the array
    weight - integer - Script's weight to set
=cut
sub setWeightOfScript {
    my $self = shift;
    my $ind = shift;
    my $weight = shift;
    
    $self->{scripts}[$ind]->setWeight($weight);
}

# Function: getSplitNumber
sub getSplitNumber {
    my $self = shift;
    return $self->{splitNumber};
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all informations about the forest. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= sprintf "\n Object COMMON::Forest :\n";

    $export .= "\t Graph :\n";
    $export .= sprintf "\t Number of graphs in the forest : %s\n", scalar @{$self->{graphs}};
    
    $export .= "\t Scripts :\n";
    $export .= sprintf "\t Number of split : %s\n", $self->{splitNumber};
    $export .= sprintf "\t Number of script : %s\n", scalar @{$self->{scripts}};
    
    return $export;
}

1;
__END__
