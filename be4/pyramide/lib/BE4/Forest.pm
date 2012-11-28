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

package BE4::Forest;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use List::Util qw(min max);

use Data::Dumper;
use Geo::GDAL;

# My module
use BE4::QTree;
use BE4::Graph;
use BE4::Commands;
use BE4::Pyramid;
use BE4::Script;
use BE4::DataSourceLoader;
use BE4::DataSource;

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

################################################################################
=begin nd
Group: variable

variable: $self
    * pyramid : BE4::Pyramid
    * commands : BE4::Commands
    * graphs : array of BE4::QTree or BE4::Graph
    * scripts : array of BE4::Script
    * splitNumber : integer - the number of script used
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        pyramid     => undef,
        commands     => undef,
        graphs  => [],
        scripts => [],
        splitNumber => undef,
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init(@_));
    
    # load. class
    return undef if (! $self->_load(@_));
    
    INFO (sprintf "Graphs' number : %s",scalar @{$self->{graphs}});

    return $self;
}

#
=begin nd
method: _init

Check the Pyramid and DataSourceLoader objects and Commands parameters.

Parameters:
    pyr - a BE4::Pyramid object.
    DSL - a BE4::DataSourceLoader object
    params_process - job_number, path_temp and path_shell.
=cut
sub _init {
    my ($self, $pyr, $DSL, $params_process) = @_;

    TRACE;

    # it's an object and it's mandatory !
    if (! defined $pyr || ref ($pyr) ne "BE4::Pyramid") {
        ERROR("Can not load Pyramid !");
        return FALSE;
    }
    $self->{pyramid} = $pyr;
    
    # it's an object and it's mandatory !
    if (! defined $DSL || ref ($DSL) ne "BE4::DataSourceLoader") {
        ERROR("Can not load data sources !");
        return FALSE;
    }

    if (! defined $params_process) {
        ERROR("We need process' parameters !");
        return FALSE;
    }
    
    return TRUE;
}

#
=begin nd
method: _load

Create a Graph or a QTree object per data source and a Commands object. Using a QTree is faster but it does'nt match all cases. Graph is a more general case.

Parameters:
    pyr - a BE4::Pyramid object.
    DSL - a BE4::DataSourceLoader object
    params_process - job_number, path_temp and path_shell.
=cut
sub _load {
    my ($self, $pyr, $DSL, $params_process) = @_;

    TRACE;

    my $dataSources = $DSL->getDataSources;
    my $TMS = $pyr->getTileMatrixSet;
    my $isQTree = $TMS->isQTree();
    
    ######### PARAM PROCESS ###########
    
    my $splitNumber = $params_process->{job_number};
    my $tempDir = $params_process->{path_temp};
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

    if (! defined $scriptDir) {
        ERROR("Parameter required : 'path_shell' in section 'Process' !");
        return FALSE;
    }
    
    ############# PROCESS #############
    
    my $commands = BE4::Commands->new($pyr,$params_process->{use_masks});

    if (! defined $commands) {
        ERROR ("Can not load Commands !");
        return FALSE;
    }
    $self->{commands} = $commands;
    
    ############# GRAPHS #############

    foreach my $datasource (@{$dataSources}) {
        
        if ($datasource->hasImages) {
            
            if (($datasource->getSRS ne $TMS->getSRS) ||
                (! $self->{pyramid}->isNewPyramid && ($pyr->getCompression eq 'jpg'))) {
                
                if (! $datasource->hasHarvesting) {
                    ERROR (sprintf "We need a WMS service for a reprojection (from %s to %s) or because of a lossy compression cache update (%s) for the base level %s",
                        $datasource->getSRS, $TMS->getSRS,
                        $pyr->getCompression, $datasource->getBottomID);
                    return FALSE;
                }
                
            } else {
                if ($datasource->hasHarvesting()) {
                    WARN(sprintf "We don't need WMS service for the datasource with base level '%s'. We remove it.",$datasource->getBottomID);
                    $datasource->removeHarvesting();
                }
            }
        }
        
        # Now, if datasource contains a WMS service, we have to use it
        
        # Creation of QTree or Graph object
        my $graph = undef;
        if ($isQTree) {
          $graph = BE4::QTree->new($self, $datasource, $self->{pyramid}, $self->{commands});
        } else {
          $graph = BE4::Graph->new($self,$datasource, $self->{pyramid}, $self->{commands});
        };
                
        if (! defined $graph) {
            ERROR(sprintf "Can not create nor QTree nor Graph object for datasource with bottom level %s !",
                  $datasource->getBottomID);
            return FALSE;
        }
        
        push @{$self->{graphs}},$graph;
    }
    
    
    ############# SCRIPTS #############
    # We create BE4::Script objects and initialize them (header)
    
    my $functions = $commands->configureFunctions;
    
    my $rootTempDir = File::Spec->catdir($tempDir,$self->{pyramid}->getNewName);
    
    if ($isQTree) {
        #### QTREE CASE
        
        for (my $i = 0; $i <= $self->getSplitNumber; $i++) {
            my $scriptID = sprintf "SCRIPT_%s",$i;
            
            if ($i == 0) {
                $scriptID = "SCRIPT_FINISHER";
            }
            
            my $script = BE4::Script->new({
                id => $scriptID,
                rootTempDir => $rootTempDir,
                scriptDir => $scriptDir,
                rootAsTempDir => ($i == 0),
            });
            
            my $listFile = $self->{pyramid}->getNewListFile;
            $script->prepare($rootTempDir,$self->{pyramid}->getNewDataDir,$listFile,$functions);
            
            push @{$self->{scripts}},$script;
        }
        
    } else {
        #### GRAPH CASE
        
        # Boucle sur les levels et sur le nb de scripts/jobs
        # On commence par les finishers
        # On continue avec les autres scripts, par level  
        for (my $i = $pyr->getBottomOrder - 1; $i <= $pyr->getTopOrder; $i++) {
            for (my $j = 1; $j <= $self->getSplitNumber; $j++) {
                my $scriptID ;
                if ($i == $pyr->getBottomOrder - 1) {
                    $scriptID = sprintf "SCRIPT_FINISHER_%s", $j;
                } else {
                    my $levelID = $self->getPyramid()->getTileMatrixSet()->getIDfromOrder($i);
                    $scriptID = sprintf "LEVEL_%s_SCRIPT_%s", $levelID, $j;
                }
                
                my $script = BE4::Script->new({
                    id => $scriptID,
                    rootTempDir => $rootTempDir,
                    scriptDir => $scriptDir,
                });
                
                my $listFile = $self->{pyramid}->getNewListFile;
                $script->prepare($rootTempDir,$self->{pyramid}->getNewDataDir,$listFile,$functions);
            
                push @{$self->{scripts}},$script;
            }
        }   
    }

    return TRUE;
}


####################################################################################################
#                                          Graph TOOLS                                             #
####################################################################################################

# Group: Graph and QTree tools

#
=begin nd
method: containsNode

Check if a Graph (or a QTree) in the forest contain a particular node (level,x,y).

Parameters:
    level - level of the node we want to know if it is in the forest.
    x - x coordinate of the node we want to know if it is in the forest.
    y - y coordinate of the node we want to know if it is in the forest.

Returns:
    A boolean : TRUE if the node exists, FALSE otherwise.
=cut
sub containsNode {
    my $self = shift;
    my $level = shift;
    my $x = shift;
    my $y = shift;
    
    foreach my $graph (@{$self->{graphs}}) {
        return TRUE if ($graph->containsNode($level,$x,$y));
    }
    
    return FALSE;
}

#
=begin nd
method: computeGraphs

Compute each Graph or QTree one after the other and close scripts to finish.

See Also:
    <computeYourself>
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
    }
    
    foreach my $script (@{$self->{scripts}}) {
        $script->close;
    }
    
    return TRUE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getGraphs {
    my $self = shift;
    return $self->{graphs}; 
}

sub getPyramid {
    my $self = shift;
    return $self->{pyramid}; 
}

sub getScripts {
    my $self = shift;
    return $self->{scripts};
}

sub getScript {
    my $self = shift;
    my $ind = shift;
    
    return $self->{scripts}[$ind];
}

sub getWeightOfScript {
    my $self = shift;
    my $ind = shift;
    
    return $self->{scripts}[$ind]->getWeight;
}

sub setWeightOfScript {
    my $self = shift;
    my $ind = shift;
    my $weight = shift;
    
    $self->{scripts}[$ind]->setWeight($weight);
}

sub getSplitNumber {
    my $self = shift;
    return $self->{splitNumber};
}

####################################################################################################
#                                          EXPORT METHODS                                          #
####################################################################################################

# Group: export methods

sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= sprintf "\n Object BE4::Forest :\n";

    $export .= "\t Graph :\n";
    $export .= sprintf "\t Number of graphs in the forest : %s\n", scalar @{$self->{graphs}};
    
    $export .= "\t Scripts :\n";
    $export .= sprintf "\t Number of split : %s\n", $self->{splitNumber};
    $export .= sprintf "\t Number of script : %s\n", scalar @{$self->{scripts}};
    
    return $export;
}

1;
__END__

=head1 NAME

BE4::Forest - Create and compute Graphs (including QTrees)

=head1 SYNOPSIS

    use BE4::Forest
    
    my $Forest = BE4::Forest->new(
        $objPyramid, # a BE4::Pyramid object
        $objDSL, # a BE4::DataSourceLoader object
        $param_process, # a hash with following keys : job_number, path_temp and path_shell
    );

=head1 DESCRIPTION

=over 4

=item pyramid

A BE4::Pyramid object.

=item commands

A BE4::Commands object, to compose generating commands (mergeNtiff, tiff2tile...).

=item graphs

Array of BE4::QTree or BE4::Graph.

=item scripts

Array of BE4::Script.

=item splitNumber

Number of script used to divide works.

=back

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-Commands.html">BE4::Commands</A></li>
<li><A HREF="./lib-BE4-Pyramid.html">BE4::Pyramid</A></li>
<li><A HREF="./lib-BE4-QTree.html">BE4::QTree</A></li>
<li><A HREF="./lib-BE4-Graph.html">BE4::Graph</A></li>
</ul>

=end html

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHORS

Chevereau Simon, E<lt>simon.chevereaun@ign.frE<gt>
Satabin Théo, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
