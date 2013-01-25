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

Class: BE4::Script

Describe a script, its weight and allowed to write in.

Using:
    (start code)
    use BE4::Script;

    my $objC = BE4::Script->new({
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
    mntConfDir - string - Directory used to write mergeNtiff configuration files. *mntConfDir* is a subdirectory of *commonTempDir*.
    weight - integer - Weight of the script, according to its content.
    stream - stream - Stream to the script file, to write in.
=cut

################################################################################

package BE4::Script;

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

Script constructor. Bless an instance.

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
        mntConfDir => undef,
        weight => 0,
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

    ########## Dossier des configurations des mergeNtiff pour ce script
    
    $self->{mntConfDir} = File::Spec->catfile($self->{commonTempDir},"mergeNtiff");

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
    
    # MergeNtiff configurations directory
    if (! -d $self->{mntConfDir}) {
        DEBUG (sprintf "Create the MergeNtiff configurations directory '%s' !", $self->{mntConfDir});
        eval { mkpath([$self->{mntConfDir}]); };
        if ($@) {
            ERROR(sprintf "Can not create the MergeNtiff configurations directory '%s' : %s !",
                $self->{mntConfDir}, $@);
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
# Returns the script's identifiant
sub getID {
    my $self = shift;
    return $self->{id};
}

# Function: getTempDir
# Returns the script's temporary directories
sub getTempDir {
    my $self = shift;
    return $self->{tempDir};
}

# Function: getMntConfDir
# Returns the mergeNtiff configuration's directory
sub getMntConfDir {
    my $self = shift;
    return $self->{mntConfDir};
}

# Function: getWeight
# Returns the script's weight
sub getWeight {
    my $self = shift;
    return $self->{weight};
}

=begin nd
Function: addWeight
Add provided weight to script's weight.

Parameters (list):
    weight - integer - weight to add to script's one.
=cut
sub addWeight {
    my $self = shift;
    my $weight = shift;
    
    $self->{weight} += $weight;
}

=begin nd
Function: setWeight
Define the script's weight.

Parameters (list):
    weight - integer - weight to set as script's one.
=cut
sub setWeight {
    my $self = shift;
    my $weight = shift;
    
    $self->{weight} = $weight;
}

####################################################################################################
#                                   Group: Stream methods                                          #
####################################################################################################

=begin nd
Function: prepare

Write script's header, which contains environment variables: the script ID, path to work directory, cache... And functions to factorize code.

Parameters (list):
    pyrDir - string - Pyramid root directory.
    listFile - string - Path to the list file.
    functions - string - Configured functions, used in the script (mergeNtiff, wget...).

Example:
    (start code)
    # Variables d'environnement
    SCRIPT_ID="SCRIPT_1"
    COMMON_TMP_DIR="/tmp/ORTHO/COMMON"
    ROOT_TMP_DIR="/tmp/ORTHO/"
    TMP_DIR="/tmp/ORTHO/SCRIPT_1"
    MNT_CONF_DIR="/home/ign/TMP/ORTHO/SCRIPT_1/mergeNtiff"
    PYR_DIR="/home/ign/PYR/ORTHO"
    LIST_FILE="/home/ign/PYR/ORTHO.list"

    # fonctions de factorisation
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
    (end code)

=cut
sub prepare {
    my $self = shift;
    my $pyrDir = shift;
    my $listFile = shift;
    my $functions = shift;

    TRACE;

    # definition des variables d'environnement du script
    my $code = sprintf ("# Variables d'environnement\n");
    $code   .= sprintf ("SCRIPT_ID=\"%s\"\n", $self->{id});
    $code   .= sprintf ("COMMON_TMP_DIR=\"%s\"\n", $self->{commonTempDir});
    $code   .= sprintf ("ROOT_TMP_DIR=\"%s\"\n", dirname($self->{tempDir}));
    $code   .= sprintf ("TMP_DIR=\"%s\"\n", $self->{tempDir});
    $code   .= sprintf ("MNT_CONF_DIR=\"%s\"\n", $self->{mntConfDir});
    $code   .= sprintf ("PYR_DIR=\"%s\"\n", $pyrDir);
    $code   .= sprintf ("LIST_FILE=\"%s\"\n", $listFile);

    my $tmpListFile = File::Spec->catdir($self->{tempDir},"list_".$self->{id}.".txt");
    $code   .= sprintf ("TMP_LIST_FILE=\"%s\"\n", $tmpListFile);
    $code   .= "\n";
    
    # Fonctions
    $code   .= "# Fonctions\n";
    $code   .= "$functions\n";

    # creation du répertoire de travail:
    $code .= "# creation du repertoire de travail\n";
    $code .= "if [ ! -d \"\${TMP_DIR}\" ] ; then mkdir -p \${TMP_DIR} ; fi\n\n";

    $code .= "# creation de la liste temporaire\n";
    $code .= "if [ ! -f \"\${TMP_LIST_FILE}\" ] ; then touch \${TMP_LIST_FILE} ; fi\n\n";

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

sub mergeTemporaryList {
    my $self = shift;
    my $scriptID = shift;

    my $stream = $self->{stream};
    
    printf $stream "rm -f \${COMMON_TMP_DIR}/list_%s.txt\n", $scriptID;
}

# Function: close
sub close {
    my $self = shift;
    
    my $stream = $self->{stream};

    if ($self->{weight} == 0) {
        printf $stream "\necho \"No image to generate (null weight)\"\n";
    } else {
        printf $stream "\necho \"Theorical weight was : %s\"\n", $self->{weight};
    }

    # On copie la liste temporaire de ce script vers le dossier commun
    printf $stream "\necho \"Temporary files list is moving to the common directory\"\n";
    printf $stream "mv \${TMP_LIST_FILE} \${COMMON_TMP_DIR}\n";

    if ($self->{executedAlone}) {
        printf $stream "\necho \"Temporary files lists (list_<?>.txt in the common directory) are added to the global files list, then removed\"\n";
        printf $stream "cat \${COMMON_TMP_DIR}/list_*.txt >>\${LIST_FILE}\n";
        printf $stream "rm -f \${COMMON_TMP_DIR}/list_*.txt\n";
    }
    
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
    Object BE4::Script :
        ID : SCRIPT_2
        Will NOT be executed alone
        Script path : /home/IGN/SCRIPTS/SCRIPT_2.sh 
        Temporary directory : /home/IGN/TEMP/SCRIPT_2
        Common temporary directory : /home/IGN/TEMP/COMMON
        MergeNtiff configuration directory : /home/IGN/TEMP/COMMON/mergeNtiff/SCRIPT_2
        Weight : 59
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject BE4::Script :\n";
    $export .= sprintf "\t ID : %s\n", $self->{id};
    
    $export .= sprintf "\t Will NOT be executed alone\n" if (! $self->{executedAlone});
    $export .= sprintf "\t Will be executed ALONE\n" if ($self->{executedAlone});
    
    $export .= sprintf "\t Script path : %s\n", $self->{filePath};
    $export .= sprintf "\t Temporary directory : %s\n", $self->{tempDir};
    $export .= sprintf "\t Common temporary directory : %s\n", $self->{commonTempDir};
    $export .= sprintf "\t MergeNtiff configuration directory : %s\n", $self->{mntConfDir};
    $export .= sprintf "\t Weight : %s\n", $self->{weight};

    return $export;
}

1;
__END__
