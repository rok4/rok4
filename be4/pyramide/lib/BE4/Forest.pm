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
use BE4::Tree;
use BE4::Pyramid;
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
    * pyramid => undef, # Pyramid object
    * trees => [], # array of Tree objects
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
        trees  => []
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init(@_));
    
    # load. class
    return undef if (! $self->_load(@_));
    
    INFO (sprintf "Trees' number : %s",scalar @{$self->{trees}});

    return $self;
}

#
=begin nd
method: _init

Check the Pyramid and DataSourceLoader objects.

Parameters:
    pyr - a BE4::Pyramid object.
    DSL - a BE4::DataSourceLoader object
=cut
sub _init {
    my ($self, $pyr, $DSL) = @_;

    TRACE;

    # it's an object and it's mandatory !
    if (! defined $pyr || ref ($pyr) ne "BE4::Pyramid") {
        ERROR("Can not load Pyramid!");
        return FALSE;
    }
    $self->{pyramid} = $pyr;
    
    # it's an object and it's mandatory !
    if (! defined $DSL || ref ($DSL) ne "BE4::DataSourceLoader") {
        ERROR("Can not load data sources !");
        return FALSE;
    }

    return TRUE;
}

#
=begin nd
method: _load

Create a Tree object per data source.

Parameters:
    pyr - a BE4::Pyramid object.
    DSL - a BE4::DataSourceLoader object
=cut
sub _load {
    my ($self, $pyr, $DSL) = @_;

    TRACE;

    my $dataSources = $DSL->getDataSources;
    my $TMS = $pyr->getTileMatrixSet;

    foreach my $datasource (@{$dataSources}) {
        
        if ($datasource->hasImages()) {
            
            if (($datasource->getSRS() ne $TMS->getSRS()) ||
                (! $self->{pyramid}->isNewPyramid() && ($pyr->getCompression() eq 'jpg'))) {
                
                if (! $datasource->hasHarvesting()) {
                    ERROR (sprintf "We need a WMS service for a reprojection (from %s to %s) or because of a lossy compression cache update (%s) for the base level %s",
                        $pyr->getDataSource()->getSRS(), $pyr->getTileMatrixSet()->getSRS(),
                        $pyr->getCompression(), $datasource->getBottomID);
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
        
        my $tree = BE4::Tree->new($datasource, $pyr, $self->{job_number});
        
        if (! defined $tree) {
            ERROR(sprintf "Can not create a Tree object for datasource with bottom level %s !",
                  $datasource->getBottomID);
            return FALSE;
        }
        
        push @{$self->{trees}},$tree;
    }

    return TRUE;
}

####################################################################################################
#                                          TREE TOOLS                                              #
####################################################################################################

# Group: tree tools

sub containsNode {
    my $self = shift;
    my $node = shift;
    
    foreach my $tree (@{$self->{trees}}) {
        return TRUE if ($tree->containsNode($node));
    }
    
    return FALSE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

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
    # We create the mergeNtiff configuration directory "path_temp/mergNtiff"
    my $mergNtiffConfDir  = File::Spec->catdir($self->getScriptTmpDir(), "mergeNtiff");
    if (! -d $mergNtiffConfDir) {
        DEBUG (sprintf "Create mergeNtiff configuration directory");
        eval { mkpath([$mergNtiffConfDir]); };
        if ($@) {
            ERROR(sprintf "Can not create the temporary directory '%s' : %s !", $mergNtiffConfDir, $@);
            return FALSE;
        }
    }
    
    # -------------------------------------------------------------------
    # We open stream to the new cache list, to add generated tile when we browse tree.    
    my $NEWLIST;
    if (! open $NEWLIST, ">>", $self->{pyramid}->getNewListFile) {
        ERROR(sprintf "Cannot open new cache list file : %s",$self->{pyramid}->getNewListFile);
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

sub getTrees {
    my $self = shift;
    return $self->{trees}; 
}

1;
__END__

=head1 NAME

BE4::Forest - Create and compute trees

=head1 SYNOPSIS

    use BE4::DataSourceLoader

    # DataSourceLoader object creation
    my $objDataSource = BE4::DataSource->new({
        filepath_conf => "/home/IGN/CONF/source.txt",
    });

=head1 DESCRIPTION

=over 4

=item trees

An array of Tree objects

=back

=head1 LIMITATION & BUGS

Metadata managing not yet implemented.

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-DataSource.html">BE4::Tree</A></li>
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
