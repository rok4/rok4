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

use strict;
use warnings;

use FindBin qw($Bin);

use Test::More;
use Data::Dumper;

use Log::Log4perl qw(:easy);
# logger by default for unit tests
Log::Log4perl->easy_init({
    level => $WARN,
    layout => '%5p : %m (%M) %n'
});

# My tested class
use WMTSALAD::Pyramid;


my $valid_prop = 'be4/pyramide/tests/properties/WMTSalaD_valid_prop.conf';
my $valid_src = 'be4/pyramide/tests/sources/WMTSalaD_valid_src.txt';

my $prop_buffer; # buffer to store the properties file's content
my $prop_fh; # file handle for this file
my $temp_prop_file = 'be4/pyramide/tests/properties/WMTSalaD_temp_prop.conf'; # temporary properties file for tests
my $src_buffer; # buffer to store the datasources file's content
my $src_fh; # file handle for this file
my $temp_src_file = 'be4/pyramide/tests/sources/WMTSalaD_temp_src.txt'; # temporary datasources file for tests

open ($prop_fh, '<', $valid_prop) or die ("Unable to open properties file.");
$prop_buffer = do { local $/; <$prop_fh> };
close ($prop_fh);

####################### Good ######################

# Pyramid creation with all parameters defined
my $pyramid = WMTSALAD::Pyramid->new($valid_prop, $valid_src);
ok (defined $pyramid, "Pyramid created");

if (defined $pyramid) { print(sprintf "Pyramid object content : %s", $pyramid->dumpPyrHash()); }

undef $pyramid;


####################### Bad #######################

# Pyramid creation with no properties configuration file
my $errPyramid = WMTSALAD::Pyramid->new();
ok(! defined $errPyramid, "Pyramid with no properties configuration file");
undef $errPyramid;



####################### End #######################

sub writeTemp {
    my $content = shift;
    my $file = shift;

    open (my $handle, ">", $file) or return 0;
    $handle->print($content);
    close ($handle) or return 0;

    return 1;
}


done_testing();

