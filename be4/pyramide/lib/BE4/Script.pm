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

package BE4::Script;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use File::Path;

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
    * id
    * filePath
    * tempDir
    * mntConfDir
    * weight
    * stream
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

#
=begin nd
method: new

Script constructor

Parameters (hash keys):
    id - string ID, like 'SCRIPT_1', used to name the file
    scriptDir - directory, where to write the script
    tempDir - directory, used for temporary file by the script
=cut
sub new {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        id => undef,
        filePath => undef,
        tempDir => undef,
        mntConfDir => undef,
        weight => 0,
        stream => undef,
    };

    bless($self, $class);

    TRACE;

    if ( ! exists $params->{id} || ! defined $params->{id}) {
        ERROR ("'id' mandatory to create a Script object !");
        return undef;
    }
    $self->{id} = $params->{id};
    
    if ( ! exists $params->{scriptDir} || ! defined $params->{scriptDir}) {
        ERROR ("'scriptDir' mandatory to create a Script object !");
        return undef;
    }
    
    $self->{filePath} = File::Spec->catfile($params->{scriptDir},$self->{id}.".sh");
    
    if ( ! exists $params->{tempDir} || ! defined $params->{tempDir}) {
        ERROR ("'tempDir' mandatory to create a Script object !");
        return undef;
    }
    $self->{tempDir} = $params->{tempDir};
    
    $self->{mntConfDir} = File::Spec->catfile($self->{tempDir},"mergeNtiff");
    
    # Temporary directory
    if (! -d $self->{tempDir}) {
        DEBUG (sprintf "Create the temporary directory '%s' !", $self->{tempDir});
        eval { mkpath([$self->{tempDir}]); };
        if ($@) {
            ERROR(sprintf "Can not create the temporary directory '%s' : %s !", $self->{tempDir}, $@);
            return undef;
        }
    }
    
    # Script's directory
    if (! -d $params->{scriptDir}) {
        DEBUG (sprintf "Create the script directory '%s' !", $params->{scriptDir});
        eval { mkpath([$params->{scriptDir}]); };
        if ($@) {
            ERROR(sprintf "Can not create the script directory '%s' : %s !", $params->{scriptDir}, $@);
            return undef;
        }
    }
    
    # Open stream
    my $STREAM;
    if ( ! (open $STREAM,">", $self->{filePath})) {
        ERROR(sprintf "Can not open stream to '%s' !", $self->{filePath});
        return undef;
    }
    $self->{stream} = $STREAM;

    return $self;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getID {
    my $self = shift;
    return $self->{id};
}

####################################################################################################
#                                       STREAM METHODS                                             #
####################################################################################################

# Group: stream methods

sub print {
    my $self = shift;
    my $text = shift;
    
    my $stream = $self->{stream};
    printf $stream "%s", $text;
}

sub close {
    my $self = shift;
    
    my $stream = $self->{stream};
    close $stream;
}

1;
__END__

=head1 NAME

BE4::Script - Describe a script, its weight and allowed to write in.

=head1 SYNOPSIS

    use BE4::Script;
  
    my $objC = BE4::Pixel->new({
        id => "SCRIPT_1",
        tempDir => "/home/ign/TMP/SCRIPT_1",
        scriptDir => "/home/ign/SCRIPTS",
    });

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

item id

item filePath

Complete absolute file path.

item tempDir

Directory used to write temporary images.

item mntConfDir

Directory used to write mergeNtiff configuration files.

item weight

Weight of the script, according to its content.

item stream

Stream to the script file, to write in.

=back

=head1 SEE ALSO

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
