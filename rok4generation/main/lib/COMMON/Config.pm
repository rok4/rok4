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
File: Config.pm

Class: COMMON::Config

(see ROK4GENERATION/libperlauto/COMMON_Config.png)

Using:
    (start code)
    # load properties from INI file
    my $cfgINI = COMMON::Config->new({
        'filepath' => "/mon/fichier/de/configuration.txt",
        'format' => "INI"
    });

    # load properties from INI file
    my $cfgJSON = COMMON::Config->new({
        'filepath' => "/mon/fichier/de/configuration.json",
        'format' => "JSON"
    });
    (end code)

Attributes:
    filePath - string - Path to the configuration file
    fileFormat - string - Configuration format : INI, JSON
    configuration - string hash - Configuration stored in string hash, with section and sub section, with orders
    rawConfiguration - string hash - Configuration stored in string hash, with section and sub section, without orders
    
Limitations:
    
=cut

################################################################################

package COMMON::Config;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use Scalar::Util qw/reftype/;
use JSON::Parse qw(assert_valid_json parse_json);
use Config::INI::Reader;

use COMMON::Array;

# inheritance
our @ISA;
use parent 'Exporter';

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constants
use constant TRUE  => 1;
use constant FALSE => 0;

# Constant: FILEFORMATS
# Define allowed values for configuration format
my @FILEFORMATS = ('INI', 'JSON');

################################################################################

BEGIN {}
INIT {}
END {}

################################################################################
#                             Group: Constructors                              #
################################################################################

=begin nd
Constructor: new

Config constructor. Bless an instance.

Parameters (hash):
    filepath - string - Configuration file path
    format - string - File format, INI or JSON

See also:
    <_loadINI>, <_loadJSON>
=cut
sub new {
    my $class = shift;
    my $params = shift;
    
    $class = ref($class) || $class;

    my $this = {
        file => undef,
        format => undef,
        configuration => {}
    };
    
    bless($this, $class);

    # filepath
    if (! exists $params->{'filepath'} || ! defined $params->{'filepath'}) {
        ERROR("Cannot use COMMON::Config->new whithout a 'filepath' parameter.");
        return undef;
    }
    
    if ( ! -e  $params->{'filepath'}) {
        ERROR(sprintf "Config file %s does not exists", $params->{'filepath'});
        return undef;
    }
    $this->{file} = $params->{'filepath'};

    # format
    if (! exists $params->{'format'} || ! defined $params->{'format'}) {
        ERROR("Cannot use COMMON::Config->new whithout a 'format' parameter.");
        return undef;
    }
    
    if ( ! defined COMMON::Array::isInArray(uc($params->{'format'}), @FILEFORMATS) ) {
        ERROR("Unknown file format: ".$params->{'format'});
        return undef;
    }

    $this->{format} = $params->{'format'};


    # load
    if ($this->{format} eq "INI") {
        $this->{configuration} = Config::INI::Reader->read_file($this->{file});
    }
    elsif ($this->{format} eq "JSON") {
        my $json_text = do {
            open(my $json_fh, "<", $this->{file}) or do {
                ERROR(sprintf "Cannot open JSON file : %s (%s)", $this->{file}, $! );
                return undef;
            };
            local $/;
            <$json_fh>
        };

        eval { assert_valid_json ($json_text); };
        if ($@) {
            ERROR(sprintf "File %s is not a valid JSON", $this->{file});
            ERROR($@);
            return undef;
        }

        $this->{configuration} = parse_json ($json_text);
    }

    return $this;
}



################################################################################
#                                Group: Tester                                 #
################################################################################

# isSection
# isSubSection
# isProperty
# getSection
# getSubSection
# getProperty
# setProperty
# getSections
# getSubSections
# getProperties
# getConfig
# getRawConfig

=begin_nd
Function: whatIs
=cut
sub whatIs {
    my $this = shift;
    my @path = @_;

    my $depth = scalar(@path);

    if ($depth == 0) {
        if (! defined $this->{configuration}) {return undef}
        if (! defined reftype($this->{configuration})) {return "SCALAR"}
        return reftype($this->{configuration})
    }

    my $sub = $this->{configuration};

    for (my $d = 0; $d < $depth; $d++) {

        if (! defined reftype($sub) || reftype($sub) ne "HASH") {return undef};

        my $p = $path[$d];

        if (! exists $sub->{$p}) {return undef};

        if ($d == $depth - 1) {
            # On lit le dernier niveau voulu
            if (! defined reftype($sub->{$p})) {return "SCALAR"}
            return reftype($this->{configuration})
        }

        $sub = $sub->{$p}
    }
}

################################################################################
#                           Group: Getters                                     #
################################################################################

=begin nd
Function: getConfigurationCopy
=cut
sub getConfigurationCopy {
    my $this = shift;

    return %{$this->{configuration}};
}


=begin nd
Function: getCopy
=cut
sub getConfigurationReference {
    my $this = shift;

    return $this->{configuration};
}

1;
__END__