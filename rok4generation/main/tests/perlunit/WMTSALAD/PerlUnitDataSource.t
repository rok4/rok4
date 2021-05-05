# Copyright © (2011) Institut national de l'information
#                    géographique et forestière
#
# Géoportail SAV <contact.geoservices@ign.fr>
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

use Log::Log4perl qw(:easy);
# logger by default for unit tests
Log::Log4perl->easy_init({
    level => $WARN,
    layout => '%5p : %m (%M) %n'
});

# My tested class
use WMTSALAD::DataSource;

######################################################

# DataSource creation

my $dSrc = WMTSALAD::DataSource->new();
ok (defined $dSrc, "DataSource (empty) created");


# DataSource initilization
my $initSuccess = $dSrc->_init(    
    {
        type => "WMS",
        level => 7,
        order => 0,
    }
);
my $isInitialized = $initSuccess && ($dSrc->{type} eq "WMS") && ($dSrc->{level} == 7) && ($dSrc->{order} == 0);
ok ($isInitialized, "DataSource initialized ");

######################################################

# Bad parameters
my $errDSrc = WMTSALAD::DataSource->new();
my $error = $errDSrc->_init(    
    {
        type => "WFS",
        level => 7,
        order => 0,
    }
);
ok (! $error, "Wrong data source type");
undef $error;

$error = $errDSrc->_init(    
    {
        type => undef,
        level => 7,
        order => 0,
    }
);
ok (! $error, "Undef source type");
undef $error;

$error = $errDSrc->_init(    
    {
        type => "WMS",
        level => undef,
        order => 0,
    }
);
ok (! $error, "Level is undef.");
undef $error;

$error = $errDSrc->_init(    
    {
        type => "WMS",
        level => 7,
        order => 0.05,
    }
);
ok (! $error, "Order is not a positive integer");
undef $error;

$error = $errDSrc->_init(    
    {
        type => "WMS",
        level => 7,
        order => undef,
    }
);
ok (! $error, "Order is undef");
undef $error;

######################################################

done_testing();

