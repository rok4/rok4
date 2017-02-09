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
File: PropertiesLoader.pm

Class: JOINCACHE::PropertiesLoader

Reads a configuration file for joinCache, respect the IniFiles format, but consider order. Possible sections are limited :
    - logger
    - pyramid
    - bboxes
    - composition
    - process

Using:
    (start code)
    use JOINCACHE::PropertiesLoader;

    my $objPropLoader = JOINCACHE::PropertiesLoader->new("/home/IGN/properties.txt");
    (end code)

Attributes:
    cfgFile - string - Configuration file path.
    
    logger - hash - Can be null
    pyramid - hash - Final pyramid's parameters
    extents - hash - Defines identifiants with associated extents (as OGR Geometry)
    composition - hash - Defines source pyramids for each level, extent, and order
|       level_id => [
|           { bbox => bbox_id1, source => descriptor_path1}
|           { bbox => bbox_id2, source => descriptor_path2}
|       ]
    sourcePyramids - string hash - Key is the descriptor's path. Just undefined values, to list used pyramids.
    process - hash - Generation parameters
=cut

################################################################################

package JOINCACHE::PropertiesLoader;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

use COMMON::Array;
use COMMON::ProxyGDAL;

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

=begin nd
Variable: SECTIONS

Define allowed sections
=cut
my @SECTIONS = ('pyramid','process','composition','logger','extents');

=begin nd
Variable: MERGEMETHODS

Define allowed merge methods
=cut
my @MERGEMETHODS = ('REPLACE','ALPHATOP','MULTIPLY','TOP');

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

# Function: new
sub new {
    my $class = shift;
    my $filepath = shift;

    $class  = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        cfgFile   => $filepath,
        
        logger => undef,
        pyramid => undef,
        extents => undef,
        composition => undef,
        process => undef,

        sourcePyramids => undef
    };

    bless($this, $class);

    # Load parameters
    return undef if (! $this->_init());
    return undef if (! $this->_load());
    return undef if (! $this->_check());

    return $this;
}

# Function: _init
sub _init {
    my $this = shift;

    my $file = $this->{cfgFile};
    
    if (! defined $file || $file eq "") {
        ERROR ("Filepath undefined");
        return FALSE;
    }
    
    if (! -f $file) {
        ERROR (sprintf "File properties '%s' doesn't exist !?", $file);
        return FALSE;
    }
    
    return TRUE;
}



=begin nd
Function: _load

Read line by line (order is important), no library is used.

See Also:
    <readCompositionLine>
=cut
sub _load {
    my $this = shift;

    if (! open CFGF, "<", $this->{cfgFile} ){
        ERROR(sprintf "Cannot open configurations' file %s.", $this->{cfgFile});
        return FALSE;
    }

    my $currentSection = undef;

    while( defined( my $l = <CFGF> ) ) {
        chomp $l;
        $l =~ s/\s+//g; # we remove all spaces
        $l =~ s/;\S*//; # we remove comments

        next if ($l eq '');

        if ($l =~ m/^\[(\w*)\]$/) {
            # Je lis une ligne [section]
            $l =~ s/[\[\]]//g;

            if (! defined COMMON::Array::isInArray($l, @SECTIONS)) {
                ERROR ("Invalid section's name");
                return FALSE;
            }
            $currentSection = $l;
            next;
        }

        if (! defined $currentSection) {
            ERROR (sprintf "A property must always be in a section (%s)",$l);
            return FALSE;
        }

        my @prop = split(/=/,$l,-1);
        if (scalar @prop != 2 || $prop[0] eq '' || $prop[1] eq '') {
            ERROR (sprintf "A line is invalid (%s). Must be prop = val",$l);
            return FALSE;
        }

        my $key = $prop[0];
        my $value = $prop[1];

        if ($currentSection eq 'extents') {
            # On lit une 'extent', on va directement la convertir en géométrie OGR
            if (exists $this->{$currentSection}->{$key}) {
                ERROR ("A property is defined twice in the configuration : section $currentSection, parameter $key");
                return FALSE;
            }

            my @limits = split (/,/, $value, -1);

            if (scalar @limits == 4) {
                # user supplied a BBOX
                $this->{extents}->{$key}->{extent} = COMMON::ProxyGDAL::geometryFromString("BBOX", $value);
                if (! defined $this->{extents}->{$key}->{extent}) {
                    ERROR("Cannot create a OGR geometry from the bbox $value");
                    ERROR("Cannot load extent with ID $key");
                    return FALSE ;
                }

            }
            else {
                # user supplied a file which contains bounding polygon
                $this->{extents}->{$key}->{extent} = COMMON::ProxyGDAL::geometryFromFile($value);
                if (! defined $this->{extents}->{$key}->{extent}) {
                    ERROR("Cannot create a OGR geometry from the file $value");
                    ERROR("Cannot load extent with ID $key");
                    return FALSE ;
                }
            }

            $this->{extents}->{$key}->{bboxes} = COMMON::ProxyGDAL::getBboxes($this->{extents}->{$key}->{extent});
            if (! defined $this->{extents}->{$key}->{bboxes}) {
                ERROR("Cannot calculate bboxes from the extent for level $key - $value");
                return FALSE;
            }


        }
        elsif ($currentSection eq 'composition') {
            if (! $this->readCompositionLine($key, $value)) {
                ERROR (sprintf "Cannot read a composition line !");
                return FALSE;
            }
        } else {
            if (exists $this->{$currentSection}->{$key}) {
                ERROR ("A property is defined twice in the configuration : section $currentSection, parameter $key");
                return FALSE;
            }
            $this->{$currentSection}->{$key} = $value;
        }

    }

    close CFGF;

    return TRUE;
}

=begin nd
Function: readComposition

Reads a *composition* section line. Determine sources by level and calculate priorities. We load source pyramids too.

Parameters (list):
    prop - string - Composition's name: levelId.extentId
    val - string - Composition's value: pyrPath1,pyrPath2,pyrPath3
=cut
sub readCompositionLine {
    my $this = shift;
    my $prop = shift;
    my $val = shift;

    my ($levelId,$extentId) = split(/\./,$prop,-1);

    if ($levelId eq '' || $extentId eq '') {
        ERROR ("Cannot define a level id and an extent id ($prop). Must be levelId.extentId");
        return FALSE;
    }

    my @pyrs = split(/,/,$val,-1);

    foreach my $pyr (@pyrs) {
        if ($pyr eq '') {
            ERROR ("Invalid list of pyramids ($val). Must be /path/pyr1.pyr,/path/pyr2.pyr");
            return FALSE;
        }

        my $objPyramid;

        if (! exists $this->{sourcePyramids}->{$pyr}) {
            # we have a new source pyramid, but not yet information about
            $objPyramid = COMMON::Pyramid->new("DESCRIPTOR", $pyr);
            if (! defined $objPyramid) {
                ERROR ("Cannot create the COMMON::Pyramid object from source pyramid's descriptor: $pyr ($levelId,$extentId)");
                return FALSE;
            }
            $this->{sourcePyramids}->{$pyr} = $objPyramid;

        } else {
            $objPyramid = $this->{sourcePyramids}->{$pyr};
        }

        if (! defined $objPyramid->getLevel($levelId)) {
            ERROR("No level $levelId in the source pyramid $pyr");
            return FALSE;
        }

        push(
            @{$this->{composition}->{$levelId}},
            {
                extent => $extentId,
                pyr => $pyr
            }
        );

    }

    return TRUE;
}


# Function: _check
sub _check {
    my $this = shift;

    # pyramid
    if (! defined $this->{pyramid}) {
        ERROR ("Section [pyramid] can not be empty !");
        return FALSE;
    }

    # composition
    if (! defined $this->{composition}) {
        ERROR ("Section [composition] can not be empty !");
        return FALSE;
    }

    # extents
    if (! defined $this->{extents}) {
        ERROR ("Section [extents] can not be empty !");
        return FALSE;
    }

    # process
    if (! defined $this->{process}) {
        ERROR ("Section [process] can not be empty !");
        return FALSE;
    }

    # merge method
    if (! defined $this->{process}->{merge_method} || ! defined COMMON::Array::isInArray($this->{process}->{merge_method}, @MERGEMETHODS)) {
        ERROR ("process.merge_method is not provided or is not allowed");
        return FALSE;
    }

    if (! defined $this->{process}->{job_number}) {
        ERROR ("process.job_number is not provided");
        return FALSE;
    }
    if (! defined $this->{process}->{path_temp}) {
        ERROR ("process.path_temp is not provided");
        return FALSE;
    }
    if (! defined $this->{process}->{path_temp_common}) {
        ERROR ("process.path_temp_common is not provided");
        return FALSE;
    }
    if (! defined $this->{process}->{job_number}) {
        ERROR ("process.job_number is not provided");
        return FALSE;
    }

    # Check extents' ids (have to be defined in the 'extents' section)

    while( my ($levelId,$sources) = each(%{$this->{composition}}) ) {

        foreach my $source (@{$sources}) {
            if (! exists $this->{extents}->{$source->{extent}}) {
                ERROR (sprintf "A extent ID (%s) from the composition is not define in the 'extents' section !", $source->{extent});
                return FALSE;
            }
            $source->{bboxes} = $this->{extents}->{$source->{extent}}->{bboxes};
            $source->{extent} = $this->{extents}->{$source->{extent}}->{extent};
            $source->{pyr} = $this->{sourcePyramids}->{$source->{pyr}};
        }
    }
    
    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getSourcePyramids
sub getSourcePyramids {
    my $this = shift;
    return $this->{sourcePyramids};
}

# Function: getPyramidSection
sub getPyramidSection {
    my $this = shift;
    return $this->{pyramid};
}

# Function: getLoggerSection
sub getLoggerSection {
    my $this = shift;
    return $this->{logger};
}

# Function: getCompositionSection
sub getCompositionSection {
    my $this = shift;
    return $this->{composition};
}

# Function: getBboxesSection
sub getExtentsSection {
    my $this = shift;
    return $this->{extents};
}

# Function: getProcessSection
sub getProcessSection {
    my $this = shift;
    return $this->{process};
}

1;
__END__
