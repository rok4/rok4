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
use constant LOG_METHODS => { # Méthodes à surcharger pour les bouchons sur les appels à Log::Log4perl
    TRACE => sub { return; },
    DEBUG => sub { return; },
    INFO => sub { return; },
    WARN => sub { return; },
    ERROR => sub { return; },
    FATAL => sub { return; },
    ALWAYS => sub { return; }
};

sub override_env {
    my $hashref = shift;
    foreach my $key (keys %{$hashref}) {
        $ENV{$key} = $hashref->{$key};
    }
}
sub reset_env {
    %ENV = INITIAL_ENV;
}

# Tested method : COMMON::ProxyStorage::checkEnvironmentVariables()
subtest test_checkEnvironmentVariables => sub {
    # Case : file storage, everything ok
    subtest file_all_ok => sub {
        # Environment for the test
        my $main_mock = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
        );
        $main_mock->override('_setConfigurationElement' => sub {});

        # Test return value : must be TRUE
        my $method_return = COMMON::ProxyStorage::checkEnvironmentVariables('FILE');
        is( $method_return, TRUE, "checkEnvironmentVariables('FILE') returns TRUE" );

        # Test calls to logger and module configuration setters : must have none
        my @logging_subs_called = keys( %{$main_mock->sub_tracking()} );
        is( scalar(@logging_subs_called), 0, "No call to logger." );

        # Reset environment
        $main_mock = undef;

        done_testing;
    };

    # Case : ceph object storage, everything ok
    subtest ceph_all_ok => sub {
        # Environment for the test
        my %temp_env = (
            'ROK4_CEPH_CONFFILE' => '/etc/ceph/ceph.conf',
            'ROK4_CEPH_USERNAME' => 'client.admin',
            'ROK4_CEPH_CLUSTERNAME' => 'ceph'
        );
        override_env(\%temp_env);
        my $override_log_subs = LOG_METHODS;
        my $main_mock = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => $override_log_subs
        );
        $main_mock->override('_setConfigurationElement' => sub {});


        # Test return value : must be TRUE
        my $method_return = COMMON::ProxyStorage::checkEnvironmentVariables('CEPH');
        is($method_return, TRUE, "checkEnvironmentVariables('CEPH') returns TRUE");

        # Test module scoped variables : must match environment
        my %module_scope_vars = COMMON::ProxyStorage::getConfiguration(keys(%temp_env));
        is(%module_scope_vars, %temp_env, "Module scope variables affectation OK.");

        # Test calls (namespace = $main)
        my @logging_subs_called = keys( %{$main_mock->sub_tracking()} );
        my $logger_subs = join('|', keys(%{$override_log_subs}));
        is( scalar(grep(/^($logger_subs)$/, @logging_subs_called)), 0, "No call to logger." );
        is( scalar(@{$main_mock->sub_tracking()->{_setConfigurationElement}}), 3, "Calls to setter." );

        # Reset environment
        reset_env();
        $main_mock = undef;

        done_testing;
    };


    # Case : s3 object storage, everything ok
    subtest s3_all_ok => sub {
        # Environment for the test
        my %temp_env = (
            'ROK4_S3_URL' => 'https://url.to.s3_service.com:985',
            'ROK4_S3_KEY' => 'ThisIsAkey',
            'ROK4_S3_SECRETKEY' => 'e0b2d012c4aeae33cbf753f3'
        );
        override_env(\%temp_env);
        my $override_log_subs = LOG_METHODS;
        my $main_mock = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
        );
        $main_mock->override('_setConfigurationElement' => sub {});
        my $UA_mock = mock 'LWP::UserAgent' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            override => {
                ssl_opts => sub {
                    my $self = shift;
                    $self->{ssl_opts_string} = "SSL options";
                },
                env_proxy => sub {
                    my $self = shift;
                    $self->{env_proxy_string} = "Proxy = system";
                }
            }
        );


        # Test return value : must be TRUE
        my $method_return = COMMON::ProxyStorage::checkEnvironmentVariables('S3');
        is($method_return, TRUE, "checkEnvironmentVariables('S3') returns TRUE");

        # Test module scoped variables : must match environment
        my %expected_vars = (%temp_env, (
            'ROK4_S3_ENDPOINT_HOST' => 'url.to.s3_service.com'
        ));
        my %module_scope_vars = COMMON::ProxyStorage::getConfiguration(keys(%expected_vars));
        is(%module_scope_vars, %expected_vars, "Module scope variables (except UA) affectation OK.");

        # Test calls (namespace = $main)
        my @logging_subs_called = keys( %{$main_mock->sub_tracking()} );
        my $logger_subs = join('|', keys(%{$override_log_subs}));
        is( scalar(grep(/^($logger_subs)$/, @logging_subs_called)), 0, "No call to logger." );
        is( scalar(@{$main_mock->sub_tracking()->{_setConfigurationElement}}), 5, "Calls to setter." );

        # Test calls (namespace = LWP::UserAgent)
        my $sub_calls_count = [
            scalar( @{$UA_mock->sub_tracking()->{new}} ),
            scalar( @{$UA_mock->sub_tracking()->{ssl_opts}} ),
            scalar( @{$UA_mock->sub_tracking()->{env_proxy}} )
        ];
        is( $sub_calls_count, [1, 1, 1], "User agent creation." );
        my %module_scope_UA_var = COMMON::ProxyStorage::getConfiguration('UA');
        ok( exists($module_scope_UA_var{'UA'}), "Module scope UA variable affectation OK." );


        # Reset environment
        COMMON::ProxyStorage::resetConfiguration();
        reset_env();
        $main_mock = undef;
        $UA_mock = undef;

        done_testing;
    };

    # Case : s3 object storage, everything ok
    subtest swift_all_ok => sub {

        # keystone authentication
        subtest with_keystone => sub {
            # Environment for the test
            my %temp_env = (
                'ROK4_SWIFT_AUTHURL' => 'https://auth.exemple.com:8080/swift/',
                'ROK4_SWIFT_USER' => 'swift_user_1',
                'ROK4_SWIFT_PASSWD' => 'swift_password_1',
                'ROK4_KEYSTONE_DOMAINID' => 'swift_test_domain',
                'ROK4_SWIFT_PUBLICURL' => 'https://cluster.swift.com:8081',
                'ROK4_KEYSTONE_PROJECTID' => 'kzty3tg85bypmtek1dgv2d61'
            );
            override_env(\%temp_env);
            my $override_log_subs = LOG_METHODS;
            my $main_mock = mock 'COMMON::ProxyStorage' => (
                track => TRUE,
                override => LOG_METHODS
            );
            $main_mock->override('_setConfigurationElement' => sub {});
            my $UA_mock = mock 'LWP::UserAgent' => (
                track => TRUE,
                override_constructor => {
                    new => 'hash'
                },
                override => {
                    ssl_opts => sub {
                        my $self = shift;
                        $self->{ssl_opts_string} = "SSL options";
                    },
                    env_proxy => sub {
                        my $self = shift;
                        $self->{env_proxy_string} = "Proxy = system";
                    }
                }
            );


            # Test return value : must be TRUE
            my $method_return = COMMON::ProxyStorage::checkEnvironmentVariables('SWIFT', TRUE);
            is($method_return, TRUE, "checkEnvironmentVariables('SWIFT', TRUE) returns TRUE");

            # Test module scoped variables : must match environment
            my %expected_vars = (%temp_env, (
                'ROK4_KEYSTONE_IS_USED' => TRUE
            ));
            my %module_scope_vars = COMMON::ProxyStorage::getConfiguration(keys(%expected_vars));
            is(%module_scope_vars, %expected_vars, "Module scope variables (except UA) affectation OK.");

            # Test calls (namespace = $main)
            my @logging_subs_called = keys( %{$main_mock->sub_tracking()} );
            my $logger_subs = join('|', keys(%{$override_log_subs}));
            is( scalar(grep(/^($logger_subs)$/, @logging_subs_called)), 0, "No call to logger." );
            is( scalar(@{$main_mock->sub_tracking()->{_setConfigurationElement}}), 8, "Calls to setter." );

            # Test calls to user agent
            my $sub_calls_count = [
                scalar( @{$UA_mock->sub_tracking()->{new}} ),
                scalar( @{$UA_mock->sub_tracking()->{ssl_opts}} ),
                scalar( @{$UA_mock->sub_tracking()->{env_proxy}} )
            ];
            is( $sub_calls_count, [1, 1, 1], "User agent creation." );
            my %module_scope_UA_var = COMMON::ProxyStorage::getConfiguration('UA');
            ok( exists($module_scope_UA_var{'UA'}), "Module scope UA variable affectation OK." );


            # Reset environment
            COMMON::ProxyStorage::resetConfiguration();
            reset_env();
            $main_mock = undef;
            $UA_mock = undef;

            done_testing;
        };

        # native authentication
        subtest without_keystone => sub {
            # Environment for the test
            my %temp_env = (
                'ROK4_SWIFT_AUTHURL' => 'https://auth.exemple.com:8080/swift/',
                'ROK4_SWIFT_USER' => 'swift_user_1',
                'ROK4_SWIFT_PASSWD' => 'swift_password_1',
                'ROK4_SWIFT_ACCOUNT' => 'swift_account_name'
            );
            override_env(\%temp_env);
            my $override_log_subs = LOG_METHODS;
            my $main_mock = mock 'COMMON::ProxyStorage' => (
                track => TRUE,
                override => LOG_METHODS
            );
            $main_mock->override('_setConfigurationElement' => sub {});
            my $UA_mock = mock 'LWP::UserAgent' => (
                track => TRUE,
                override_constructor => {
                    new => 'hash'
                },
                override => {
                    ssl_opts => sub {
                        my $self = shift;
                        $self->{ssl_opts_string} = "SSL options";
                    },
                    env_proxy => sub {
                        my $self = shift;
                        $self->{env_proxy_string} = "Proxy = system";
                    }
                }
            );


            # Test return value : must be TRUE
            my $method_return = COMMON::ProxyStorage::checkEnvironmentVariables('SWIFT', FALSE);
            is($method_return, TRUE, "checkEnvironmentVariables('SWIFT', FALSE) returns TRUE");

            # Test module scoped variables : must match environment
            my %expected_vars = (%temp_env, (
                'ROK4_KEYSTONE_IS_USED' => FALSE
            ));
            my %module_scope_vars = COMMON::ProxyStorage::getConfiguration(keys(%expected_vars));
            is(%module_scope_vars, %expected_vars, "Module scope variables (except UA) affectation OK.");

            # Test calls (namespace = $main)
            my @logging_subs_called = keys( %{$main_mock->sub_tracking()} );
            my $logger_subs = join('|', keys(%{$override_log_subs}));
            is( scalar(grep(/^($logger_subs)$/, @logging_subs_called)), 0, "No call to logger." );
            is( scalar(@{$main_mock->sub_tracking()->{_setConfigurationElement}}), 6, "Calls to setter." );

            # Test calls to user agent
            my $sub_calls_count = [
                scalar( @{$UA_mock->sub_tracking()->{new}} ),
                scalar( @{$UA_mock->sub_tracking()->{ssl_opts}} ),
                scalar( @{$UA_mock->sub_tracking()->{env_proxy}} )
            ];
            is( $sub_calls_count, [1, 1, 1], "User agent creation." );
            my %module_scope_UA_var = COMMON::ProxyStorage::getConfiguration('UA');
            ok( exists($module_scope_UA_var{'UA'}), "Module scope UA variable affectation OK." );


            # Reset environment
            COMMON::ProxyStorage::resetConfiguration();
            reset_env();
            $main_mock = undef;
            $UA_mock = undef;

            done_testing;
        };

        done_testing;
    };

    done_testing;
};



# Tested method : COMMON::ProxyStorage::getSwiftToken()
# subtest test_getSwiftToken => sub {
#     subtest ok_first_try_with_keystone => sub {

#         done_testing;
#     };

#     done_testing;
# };





done_testing;