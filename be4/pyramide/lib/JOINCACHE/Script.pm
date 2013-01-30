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
File: Script.pm

Class: JOINCACHE::Script

Describe a script, allowed to write in.

Using:
    (start code)
    use JOINCACHE::Script;

    my $objScript = JOINCACHE::Script->new({
        id => "SCRIPT_1",
        tempDir => "/home/ign/TMP/",
        commonTempDir => "/home/ign/TMP/",
        scriptDir => "/home/ign/SCRIPTS",
        executedAlone => "FALSE"
    });
    (end code)

Attributes:
    id - string - Identifiant, like "SCRIPT_2". It used to name the file and temporary directories.
    executedAlone - boolean - If we know a script will be executed ALONE. it can change some working.
    filePath - string - Complete absolute script file path.
    tempDir - string - Directory used to write temporary images.
    commonTempDir - string - Directory used to write temporary images which have to be shared between different scripts.
    stream - stream - Stream to the script file, to write in.
=cut

################################################################################

package JOINCACHE::Script;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use File::Path;
use File::Basename;

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

Script constructor. Bless an instance. Checks parameters and creates directories.

Parameters (hash):
    id - string - Identifiant, used to name the file, like 'SCRIPT_1'.
    scriptDir - string - Directory path, where to write the script.
    tempDir - string - Root directory, in which own temporary directory will be created.
    commonTempDir - string - Common temporary directory, to allowed scripts to share files
    executedAlone - boolean - Optionnal, FALSE by default.
=cut
sub new {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        id => undef,
        executedAlone => FALSE,
        filePath => undef,
        tempDir => undef,
        commonTempDir => undef,
        stream => undef,
    };

    bless($self, $class);

    TRACE;

    ########## ID

    if ( ! exists $params->{id} || ! defined $params->{id}) {
        ERROR ("'id' mandatory to create a Script object !");
        return undef;
    }
    $self->{id} = $params->{id};

    ########## Will be executed alone ?

    if ( exists $params->{executedAlone} && defined $params->{executedAlone} && $params->{executedAlone}) {
        $self->{executedAlone} = TRUE;
    }

    ########## Chemin d'écriture du script
    
    if ( ! exists $params->{scriptDir} || ! defined $params->{scriptDir}) {
        ERROR ("'scriptDir' mandatory to create a Script object !");
        return undef;
    }
    $self->{filePath} = File::Spec->catfile($params->{scriptDir},$self->{id}.".sh");

    ########## Dossier temporaire dédié à ce script
    
    if ( ! exists $params->{tempDir} || ! defined $params->{tempDir}) {
        ERROR ("'tempDir' mandatory to create a Script object !");
        return undef;
    }
    $self->{tempDir} = File::Spec->catdir($params->{tempDir},$self->{id});

    ########## Dossier commun à tous les scripts

    if ( ! exists $params->{commonTempDir} || ! defined $params->{commonTempDir}) {
        ERROR ("'commonTempDir mandatory to create a Script object !");
        return undef;
    }
    $self->{commonTempDir} = File::Spec->catdir($params->{commonTempDir},"COMMON");

    ########## Tests et création de l'ensemble des dossiers
    
    # Temporary directory
    if (! -d $self->{tempDir}) {
        DEBUG (sprintf "Create the temporary directory '%s' !", $self->{tempDir});
        eval { mkpath([$self->{tempDir}]); };
        if ($@) {
            ERROR(sprintf "Can not create the temporary directory '%s' : %s !", $self->{tempDir}, $@);
            return undef;
        }
    }

    # Common directory
    if (! -d $self->{commonTempDir}) {
        DEBUG (sprintf "Create the common temporary directory '%s' !", $self->{commonTempDir});
        eval { mkpath([$self->{commonTempDir}]); };
        if ($@) {
            ERROR(sprintf "Can not create the common temporary directory '%s' : %s !", $self->{commonTempDir}, $@);
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
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getID
sub getID {
    my $self = shift;
    return $self->{id};
}

# Function: getTempDir
sub getTempDir {
    my $self = shift;
    return $self->{tempDir};
}

####################################################################################################
#                                   Group: Stream methods                                          #
####################################################################################################

=begin nd
Function: prepare

Write script's header, which contains environment variables: the script ID, path to work directory, cache... And functions to factorize code.

Parameters (list):
    pyrDir - string - Pyramid root directory.
    functions - string - Configured bash functions, used in the script (OverlayNtiff, Cache2work...).

Example:
    (start code)
    # Variables d'environnement
    SCRIPT_ID="SCRIPT_1"
    COMMON_TMP_DIR="/tmp/ORTHO/COMMON"
    TMP_DIR="/tmp/ORTHO/SCRIPT_1"
    PYR_DIR="/home/ign/PYR/ORTHO"
    (end code)

=cut
sub prepare {
    my $self = shift;
    my $pyrDir = shift;
    my $functions = shift;

    TRACE;

    # definition des variables d'environnement du script
    my $code = sprintf ("# Variables d'environnement\n");
    $code   .= sprintf ("SCRIPT_ID=\"%s\"\n", $self->{id});
    $code   .= sprintf ("COMMON_TMP_DIR=\"%s\"\n", $self->{commonTempDir});
    $code   .= sprintf ("TMP_DIR=\"%s\"\n", $self->{tempDir});
    $code   .= sprintf ("PYR_DIR=\"%s\"\n", $pyrDir);
    $code   .= "\n";
    
    # Fonctions
    $code   .= "# Fonctions\n";
    $code   .= "$functions\n";

    # creation du répertoire de travail:
    $code .= "# Création du repertoire de travail\n";
    $code .= "if [ ! -d \"\${TMP_DIR}\" ] ; then mkdir -p \${TMP_DIR} ; fi\n\n";

    $self->write($code);
}

=begin nd
Function: write
Print text in the script's file, using the opened stream.

Parameters (list):
    text - string - Text to write in file.
=cut
sub write {
    my $self = shift;
    my $text = shift;
    
    my $stream = $self->{stream};
    printf $stream "%s", $text;
}

# Function: close
sub close {
    my $self = shift;
    
    my $stream = $self->{stream};
    close $stream;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all informations about the script. Useful for debug.

Example:
    (start code)
    Object JOINCACHE::Script :
        ID : SCRIPT_2
        Will NOT be executed alone
        Script path : /home/IGN/SCRIPTS/SCRIPT_2.sh 
        Temporary directory : /home/IGN/TEMP/SCRIPT_2
        Common temporary directory : /home/IGN/TEMP/COMMON
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject JOINCACHE::Script :\n";
    $export .= sprintf "\t ID : %s\n", $self->{id};
    
    $export .= sprintf "\t Will NOT be executed alone\n" if (! $self->{executedAlone});
    $export .= sprintf "\t Will be executed ALONE\n" if ($self->{executedAlone});
    
    $export .= sprintf "\t Script path : %s\n", $self->{filePath};
    $export .= sprintf "\t Temporary directory : %s\n", $self->{tempDir};
    $export .= sprintf "\t Common temporary directory : %s\n", $self->{commonTempDir};

    return $export;
}

1;
__END__
