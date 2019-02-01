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

(see COMMON_Script.png)

Describe a script, its weight and allowed to write in.

Using:
    (start code)
    use COMMON::Script;

    my $objC = COMMON::Script->new({
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
    dntConfDir - string - Directory used to write decimateNtiff configuration files (used by <COMMON::NNGraph> generation). *dntConfDir* is a subdirectory of *commonTempDir*.
    ontConfDir - string - Directory used to write overlayNtiff configuration files (used by JoinCache tool). *ontConfDir* is a subdirectory of *commonTempDir*.
    currentweight - integer - Current weight of the script (already written in the file)
    weight - integer - Total weight of the script, according to its content.
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
    my $class = shift;
    my $params = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        id => undef,
        executedAlone => FALSE,
        filePath => undef,

        tempDir => undef,
        commonTempDir => undef,
        mntConfDir => undef,
        dntConfDir => undef,
        ontConfDir => undef,

        weight => 0,
        currentweight => 0,

        stream => undef,
    };

    bless($this, $class);


    ########## ID

    if ( ! exists $params->{id} || ! defined $params->{id}) {
        ERROR ("'id' mandatory to create a Script object !");
        return undef;
    }
    $this->{id} = $params->{id};

    ########## Will be executed alone ?

    if ( exists $params->{executedAlone} && defined $params->{executedAlone} && $params->{executedAlone}) {
        $this->{executedAlone} = TRUE;
    }

    ########## Chemin d'écriture du script
    
    if ( ! exists $params->{scriptDir} || ! defined $params->{scriptDir}) {
        ERROR ("'scriptDir' mandatory to create a Script object !");
        return undef;
    }
    $this->{filePath} = File::Spec->catfile($params->{scriptDir},$this->{id}.".sh");

    ########## Dossier temporaire dédié à ce script
    
    if ( ! exists $params->{tempDir} || ! defined $params->{tempDir}) {
        ERROR ("'tempDir' mandatory to create a Script object !");
        return undef;
    }
    $this->{tempDir} = File::Spec->catdir($params->{tempDir},$this->{id});

    ########## Dossier commun à tous les scripts

    if ( ! exists $params->{commonTempDir} || ! defined $params->{commonTempDir}) {
        ERROR ("'commonTempDir mandatory to create a Script object !");
        return undef;
    }
    $this->{commonTempDir} = File::Spec->catdir($params->{commonTempDir},"COMMON");

    ########## Dossier des configurations des mergeNtiff pour ce script
    
    $this->{mntConfDir} = File::Spec->catfile($this->{commonTempDir},"mergeNtiff");
    
    ########## Dossier des configurations des decimateNtiff pour ce script
    
    $this->{dntConfDir} = File::Spec->catfile($this->{commonTempDir},"decimateNtiff");
    
    ########## Dossier des configurations des overlayNtiff pour ce script
    
    $this->{ontConfDir} = File::Spec->catfile($this->{commonTempDir},"overlayNtiff");

    ########## Tests et création de l'ensemble des dossiers

    # Common directory
    if (! -d $this->{commonTempDir}) {
        DEBUG (sprintf "Create the common temporary directory '%s' !", $this->{commonTempDir});
        eval { mkpath([$this->{commonTempDir}]); };
        if ($@) {
            ERROR(sprintf "Can not create the common temporary directory '%s' : %s !", $this->{commonTempDir}, $@);
            return undef;
        }
    }
    
    # MergeNtiff configurations directory
    if (! -d $this->{mntConfDir}) {
        DEBUG (sprintf "Create the MergeNtiff configurations directory '%s' !", $this->{mntConfDir});
        eval { mkpath([$this->{mntConfDir}]); };
        if ($@) {
            ERROR(sprintf "Can not create the MergeNtiff configurations directory '%s' : %s !",
                $this->{mntConfDir}, $@);
            return undef;
        }
    }
    
    # DecimateNtiff configurations directory
    if (! -d $this->{dntConfDir}) {
        DEBUG (sprintf "Create the DecimateNtiff configurations directory '%s' !", $this->{dntConfDir});
        eval { mkpath([$this->{dntConfDir}]); };
        if ($@) {
            ERROR(sprintf "Can not create the DecimateNtiff configurations directory '%s' : %s !",
                $this->{dntConfDir}, $@);
            return undef;
        }
    }
    
    # OverlayNtiff configurations directory
    if (! -d $this->{ontConfDir}) {
        DEBUG (sprintf "Create the OverlayNtiff configurations directory '%s' !", $this->{ontConfDir});
        eval { mkpath([$this->{ontConfDir}]); };
        if ($@) {
            ERROR(sprintf "Can not create the OverlayNtiff configurations directory '%s' : %s !",
                $this->{ontConfDir}, $@);
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
    if ( ! (open $STREAM,">", $this->{filePath})) {
        ERROR(sprintf "Can not open stream to '%s' !", $this->{filePath});
        return undef;
    }
    $this->{stream} = $STREAM;

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

# Function: isExecutedAlone
sub isExecutedAlone {
    my $this = shift;
    return $this->{executedAlone};
}

# Function: getTempDir
# Returns the script's temporary directories
sub getTempDir {
    my $this = shift;
    return $this->{tempDir};
}

# Function: getMntConfDir
# Returns the mergeNtiff configuration's directory
sub getMntConfDir {
    my $this = shift;
    return $this->{mntConfDir};
}

# Function: getDntConfDir
# Returns the decimateNtiff configuration's directory
sub getDntConfDir {
    my $this = shift;
    return $this->{dntConfDir};
}

# Function: getDntConfDir
# Returns the overlayNtiff configuration's directory
sub getOntConfDir {
    my $this = shift;
    return $this->{ontConfDir};
}


# Function: getWeight
# Returns the script's weight
sub getWeight {
    my $this = shift;
    return $this->{weight};
}

=begin nd
Function: addWeight
Add provided weight to script's weight.

Parameters (list):
    weight - integer - weight to add to script's one.
=cut
sub addWeight {
    my $this = shift;
    my $weight = shift;
    
    $this->{weight} += $weight;
}

=begin nd
Function: setWeight
Define the script's weight.

Parameters (list):
    weight - integer - weight to set as script's one.
=cut
sub setWeight {
    my $this = shift;
    my $weight = shift;
    
    $this->{weight} = $weight;
}

####################################################################################################
#                                   Group: Stream methods                                          #
####################################################################################################

=begin nd
Function: prepare

Write script's header, which contains environment variables: the script ID, path to work directory, cache... And functions to factorize code.

Parameters (list):
    pyramid - <COMMON::PyramidRaster> or <COMMON::PyramidVector> - Pyramid to generate.
    functions - string - Configured functions, used in the script (mergeNtiff, wget...).

Example:
    (start code)
    (end code)

=cut
sub prepare {
    my $this = shift;
    my $pyramid = shift;
    my $functions = shift;

    # definition des variables d'environnement du script
    my $code = sprintf ("# Variables d'environnement\n");
    $code .= sprintf ("SCRIPT_ID=\"%s\"\n", $this->{id});
    $code .= sprintf ("COMMON_TMP_DIR=\"%s\"\n", $this->{commonTempDir});
    $code .= sprintf ("ROOT_TMP_DIR=\"%s\"\n", dirname($this->{tempDir}));
    $code .= sprintf ("TMP_DIR=\"%s\"\n", $this->{tempDir});
    if (ref ($pyramid) eq "COMMON::PyramidRaster") {
        $code .= sprintf ("MNT_CONF_DIR=\"%s\"\n", $this->{mntConfDir});
        $code .= sprintf ("DNT_CONF_DIR=\"%s\"\n", $this->{dntConfDir});
        $code .= sprintf ("ONT_CONF_DIR=\"%s\"\n", $this->{ontConfDir});
    }
    $code .= sprintf ("LIST_FILE=\"%s\"\n", $pyramid->getListFile() );

    if ($pyramid->getStorageType() eq "FILE") {
        $code .= sprintf ("PYR_DIR=\"%s\"\n", $pyramid->getDataDir() );
    }
    elsif ($pyramid->getStorageType() eq "CEPH") {
        $code .= sprintf ("PYR_POOL=\"%s\"\n", $pyramid->getDataPool() );
    }
    elsif ($pyramid->getStorageType() eq "S3") {
        $code .= sprintf ("PYR_BUCKET=\"%s\"\n", $pyramid->getDataBucket() );
    }
    elsif ($pyramid->getStorageType() eq "SWIFT") {
        $code .= sprintf ("PYR_CONTAINER=\"%s\"\n", $pyramid->getDataContainer() );

        if ($pyramid->keystoneConnection()) {
            $code .= "KEYSTONE_OPTION=\"-ks\"\n";
        } else {
            $code .= "KEYSTONE_OPTION=\"\"\n";
        }
    }
    else {
        ERROR("Storage type of new pyramid is not handled");
        return FALSE;
    }

    my $tmpListFile = File::Spec->catdir($this->{tempDir},"list_".$this->{id}.".txt");
    $code .= sprintf ("TMP_LIST_FILE=\"%s\"\n", $tmpListFile);
    $code .= "\n";
    
    $code .= "# Pour mémoriser les dalles supprimées\n";
    $code .= "declare -A RM_IMGS\n";

    $code .= "# Fonctions\n";
    $code .= "$functions\n";

    $code .= "# Création du dossier temporaire\n";
    $code .= "mkdir -p \${TMP_DIR}\n\n";

    $code .= "# Création de la liste temporaire\n";
    $code .= "if [ ! -f \"\${TMP_LIST_FILE}\" ] ; then touch \${TMP_LIST_FILE} ; fi\n\n";

    $this->write($code);

    return TRUE;
}

=begin nd
Function: write
Print text in the script's file, using the opened stream.

Parameters (list):
    text - string - Text to write in file.
    w - integer - Weight of the instructions to write
=cut
sub write {
    my $this = shift;
    my $text = shift;
    my $w = shift;
    
    my $stream = $this->{stream};
    printf $stream "%s", $text;
    
    if ($this->{weight} != 0 && defined $w) {
        my $oldpercent = int($this->{currentweight}/$this->{weight}*100);
        $this->{currentweight} += $w;
        my $newpercent = int($this->{currentweight}/$this->{weight}*100);
        if ($oldpercent != $newpercent) {
            print $stream "echo \"------------- Progression : $newpercent%\"\n";
        }
    }
}

# Function: close
sub close {
    my $this = shift;
    
    my $stream = $this->{stream};

    print $stream "echo \"------------- Progression : COMPLETE\"\n";
    printf $stream "\necho \"Theorical weight was : %s\"\n", $this->{weight};

    # On copie la liste temporaire de ce script vers le dossier commun si il existe
    printf $stream "if [ -f \"\${TMP_LIST_FILE}\" ];then\n";
    printf $stream "\t\necho \"Temporary files list is moving to the common directory\"\n";
    printf $stream "\tmv \${TMP_LIST_FILE} \${COMMON_TMP_DIR}\n";
    printf $stream "fi\n";

    if ($this->{executedAlone}) {
        printf $stream "\necho \"Temporary files lists (list_*.txt in the common directory) are added to the global files list, then removed\"\n";
        printf $stream "cat \${COMMON_TMP_DIR}/list_*.txt >>\${LIST_FILE}\n";
        printf $stream "rm -f \${COMMON_TMP_DIR}/list_*.txt\n";
        printf $stream "BackupListFile\n\n";
    }
    
    close $stream;
}

1;
__END__
