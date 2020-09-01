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

Class: COMMON::Script

(see ROK4GENERATION/libperlauto/COMMON_Script.png)

Describe a script and allowed to write in.

Using:
    (start code)
    use COMMON::Script;

    my $objC = COMMON::Script->new({
        id => "SCRIPT_1",
        finisher => "FALSE"
        shellClass => 'BE4::Shell',
        initialisation => "echo 'Hello world'"
    });
    (end code)

Attributes:
    id - string - Identifiant, like "SCRIPT_2". It used to name the file and temporary directories.
    finisher - boolean - If we know a script is the finisher (executed alone at the end). it can change some working.
    filePath - string - Complete absolute script file path.
    tempDir - string - Directory used to write temporary images.
    stream - stream - Stream to the script file, to write in.
=cut

################################################################################

package COMMON::Script;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use File::Path;
use File::Basename;

use BE4::Shell;
use FOURALAMO::Shell;
use JOINCACHE::Shell;

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
    finisher - boolean - Precise if script is the finisher (executed alone at the end)
    shellClass - <BE4::Shell> or <FOURALAMO::Shell> or <JOINCACHE::Shell> class - Shell class where some information are stored as class variables
    initialisation - string - Text to print into script
=cut
sub new {
    my $class = shift;
    my $params = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        id => undef,
        finisher => undef,
        filePath => undef,

        tempDir => undef,

        stream => undef
    };

    bless($this, $class);

    ########## ID

    if ( ! exists $params->{id} || ! defined $params->{id}) {
        ERROR ("'id' mandatory to create a Script object !");
        return undef;
    }
    $this->{id} = $params->{id};

    ########## Will be executed alone ?

    if ( ! exists $params->{finisher} || ! defined $params->{finisher}) {
        ERROR ("'finisher' mandatory to create a Script object !");
        return undef;
    }
    $this->{finisher} = $params->{finisher};

    ########## Shell class

    if ( ! exists $params->{shellClass} || ! defined $params->{shellClass}) {
        ERROR ("'shellClass' mandatory to create a Script object !");
        return undef;
    }

    my $shellClass = $params->{shellClass};

    ########## Chemin d'écriture du script

    $this->{filePath} = File::Spec->catfile($shellClass->getScriptDirectory(), $this->{id}.".sh");

    if (! -d $shellClass->getScriptDirectory()) {
        DEBUG (sprintf "Create the script directory '%s' !", $shellClass->getScriptDirectory());
        eval { mkpath([$shellClass->getScriptDirectory()]); };
        if ($@) {
            ERROR(sprintf "Can not create the script directory '%s' : %s !", $shellClass->getScriptDirectory(), $@);
            return undef;
        }
    }

    ########## Dossier temporaire dédié à ce script
    
    $this->{tempDir} = File::Spec->catdir($shellClass->getPersonnalTempDirectory(),$this->{id});
    
    
    ########## Open stream
    my $STREAM;
    if ( ! (open $STREAM,">", $this->{filePath})) {
        ERROR(sprintf "Can not open stream to '%s' !", $this->{filePath});
        return undef;
    }
    $this->{stream} = $STREAM;

    $this->prepare($params->{initialisation});

    return $this;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getID
# Returns the script's identifiant
sub getID {
    my $this = shift;
    return $this->{id};
}

# Function: getTempDir
# Returns the script's temporary directories
sub getTempDir {
    my $this = shift;
    return $this->{tempDir};
}

####################################################################################################
#                                   Group: Stream methods                                          #
####################################################################################################

=begin nd
Function: prepare

Write script's header, which contains script's environment variables: the script ID, path to work directory.

Parameters (list):
    initialisation - string - Shell method and variables usefull to generate the pyramid
=cut
sub prepare {
    my $this = shift;
    my $initialisation = shift;

    # definition des variables d'environnement du script
    my $code = sprintf ("# Variables d'environnement\n");
    $code .= sprintf ("SCRIPT_ID=\"%s\"\n", $this->{id});
    $code .= sprintf ("TMP_DIR=\"%s\"\n", $this->{tempDir});

    my $tmpListFile = File::Spec->catdir($this->{tempDir},"list_".$this->{id}.".txt");
    $code .= sprintf ("TMP_LIST_FILE=\"%s\"\n", $tmpListFile);
    $code .= "\n";

    $code .= "# Création du dossier temporaire\n";
    $code .= "mkdir -p \${TMP_DIR}\n\n";

    if (defined $initialisation) {
        $code .= $initialisation;
    }

    $this->write($code);

    return TRUE;
}

=begin nd
Function: write
Print text in the script's file, using the opened stream.

Parameters (list):
    text - string - Text to write in file.
=cut
sub write {
    my $this = shift;
    my $text = shift;
    
    my $stream = $this->{stream};
    printf $stream "%s", $text;
}

# Function: close
sub close {
    my $this = shift;
    
    my $stream = $this->{stream};

    # On copie la liste temporaire de ce script vers le dossier commun si il existe
    printf $stream "if [ -f \"\${TMP_LIST_FILE}\" ];then\n";
    printf $stream "    echo \"Temporary files list is moving to the common directory\"\n";
    printf $stream "    mv \${TMP_LIST_FILE} \${COMMON_TMP_DIR}\n";
    printf $stream "fi\n\n";

    if ($this->{finisher}) {
        printf $stream "echo \"Temporary files lists (list_*.txt in the common directory) are added to the global files list, then removed\"\n";
        printf $stream "cat \${COMMON_TMP_DIR}/list_*.txt >>\${LIST_FILE}\n";
        printf $stream "rm -f \${COMMON_TMP_DIR}/list_*.txt\n";
        printf $stream "BackupListFile\n\n";
    }

    printf $stream "if [ -f \"\${progression_file}\" ];then\n";
    printf $stream "    echo '100' >\$progression_file\n";
    printf $stream "fi\n\n";
    
    close $stream;
}

1;
__END__
