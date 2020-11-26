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

use strict;
use warnings;
use COMMON::ProxyStorage;

use Data::Dumper;

# Import du bundle de test Test2::Suite
use Test2::V0 -target => 'COMMON::ProxyStorage';



# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;
use constant INITIAL_ENV => %ENV;

sub override_env {
    my $hashref = shift;
    foreach my $key (keys %{$hashref}) {
        $ENV{$key} = $hashref->{$key};
    }
}
sub reset_env {
    %ENV = INITIAL_ENV;
}
        

subtest test_checkEnvironmentVariables => sub {
    subtest file_all_ok => sub {
        my $method_return = COMMON::ProxyStorage::checkEnvironmentVariables('FILE');
        is($method_return, TRUE, "checkEnvironmentVariables('FILE') returns TRUE");
    };
    subtest ceph_all_ok => sub {
        my %temp_env = (
            'ROK4_CEPH_CONFFILE' => '/etc/ceph/ceph.conf',
            'ROK4_CEPH_USERNAME' => 'client.admin',
            'ROK4_CEPH_CLUSTERNAME' => 'ceph'
        );
        override_env(\%temp_env);

        my $method_return = COMMON::ProxyStorage::checkEnvironmentVariables('CEPH');
        is($method_return, TRUE, "checkEnvironmentVariables('CEPH') returns TRUE");
        my %module_scope_vars = COMMON::ProxyStorage::getConfiguration(keys(%temp_env));
        is(%module_scope_vars, %temp_env, "Module scope variables affectation OK.");   

        reset_env();
    };
    # subtest s3_all_ok => sub {
    # };
    # subtest swift_all_ok => sub {
    #     subtest with_keystone => sub {
    #     };
    #     subtest without_keystone => sub {
    #     };
    # };  
};
done_testing;
