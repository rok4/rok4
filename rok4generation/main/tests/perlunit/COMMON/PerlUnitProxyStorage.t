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

# Note : environment variable 'ROK4_UNITEST_RUN' must be set to 'TRUE' before running this script for it to work.

use strict;
use warnings;

use Data::Dumper;
use JSON;

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

# Méthodes à surcharger pour les bouchons sur les appels à Log::Log4perl
my @LOG_METHODS_NAME = ('TRACE', 'DEBUG', 'INFO', 'WARN', 'ERROR', 'FATAL', 'ALWAYS');

# Import du bundle de test Test2::Suite
use Test2::V0;
use COMMON::ProxyStorage;

sub override_env {
    my $hashref = shift;
    foreach my $key (keys %{$hashref}) {
        $ENV{$key} = $hashref->{$key};
    }
};

sub reset_env {
    %ENV = INITIAL_ENV;
};

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
        $main_mock->override('_getConfigurationElement' => sub {
            my $key = shift;
            return $temp_env{$key};
        });
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

    # Case : swift object storage, everything ok
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
subtest test_getSwiftToken => sub {
    subtest ok_first_try_with_keystone => sub {
        # Environment for the test
        my %mocked_module_values = (
            'ROK4_KEYSTONE_IS_USED' => TRUE,
            'ROK4_SWIFT_AUTHURL' => 'https://auth.exemple.com:8080/swift/',
            'ROK4_SWIFT_USER' => 'swift_user_1',
            'ROK4_SWIFT_PASSWD' => 'swift_password_1',
            'ROK4_KEYSTONE_DOMAINID' => 'swift_test_domain',
            'ROK4_KEYSTONE_PROJECTID' => 'kzty3tg85bypmtek1dgv2d61',
            'UA' => undef,            
            'X-Subject-Token' => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS'
        );
        my $override_log_subs = LOG_METHODS;

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : $main
        $mocks_hash{'$main'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => $override_log_subs
        );
        $mocks_hash{'$main'}->override('_setConfigurationElement' => sub {
            my $key = shift;
            my $value = shift;
            $mocked_module_values{$key} = $value;
            return TRUE;
        });
        $mocks_hash{'$main'}->override('_getConfigurationElement' => sub {
            my $key = shift;
            return $mocked_module_values{$key};
        });

        ### Namespace : HTTP::Request::Common
        $mocks_hash{'HTTP::Request::Common'} = mock 'HTTP::Request::Common' => (
            track => TRUE,
            override => {
                POST => sub {
                    my $url = shift;
                    my %params = @_;
                    my %return_hash = (
                        'url' => $url,
                        %params
                    );
                    return \%return_hash;
                }
            }
        );

        ### Namespace : HTTP::Response
        $mocks_hash{'HTTP::Response'} = mock 'HTTP::Response' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            override => {
                is_success => sub {
                    my $self = shift;
                    return TRUE;
                }
            },
            add => {
                header => sub {
                    my $self = shift;
                    my $key = shift;
                    return $mocked_module_values{$key};
                }
            }
        );

        ### Namespace : LWP::UserAgent
        $mocks_hash{'LWP::UserAgent'} = mock 'LWP::UserAgent' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },            
            override => {
                request => sub {
                    my $self = shift;
                    return HTTP::Response->new();
                }
            }
        );
        $mocked_module_values{'UA'} = LWP::UserAgent->new();


        # Test return value : must be TRUE
        my $method_return = COMMON::ProxyStorage::getSwiftToken();
        is($method_return, TRUE, "getSwiftToken() with keystone returns TRUE");

        # Test calls (namespace  = LWP::UserAgent)
        is( scalar( @{$mocks_hash{'LWP::UserAgent'}->sub_tracking()->{request}} ), 1, "Request sent." );
        is( $mocks_hash{'LWP::UserAgent'}->sub_tracking()->{request}[0]{args}[1]{'url'}, $mocked_module_values{'ROK4_SWIFT_AUTHURL'}, "Request URL OK." );
        my $expected_body_obj = {
            "auth" => {
                "scope" => {
                    "project" => {
                        "id" => $mocked_module_values{'ROK4_KEYSTONE_PROJECTID'}
                    }
                },
                "identity" => {
                    "methods" => [
                        "password"
                    ],
                    "password" => {
                        "user" => {
                            "domain" => {
                                "id" => $mocked_module_values{'ROK4_KEYSTONE_DOMAINID'}
                            },
                            "name" => $mocked_module_values{'ROK4_SWIFT_USER'},
                            "password" => $mocked_module_values{'ROK4_SWIFT_PASSWD'}
                        }
                    }
                }
            }
        };
        is( JSON::from_json($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{request}[0]{args}[1]{'Content'}, {utf8 => 1}), $expected_body_obj, "Request body OK." );

        # Test calls (namespace = $main)
        my @logging_subs_called = keys( %{$mocks_hash{'$main'}->sub_tracking()} );
        my $logger_subs = join('|', keys(%{$override_log_subs}));
        is( scalar(grep(/^($logger_subs)$/, @logging_subs_called)), 0, "No call to logger." );
        is( scalar(@{$mocks_hash{'$main'}->sub_tracking()->{_setConfigurationElement}}), 1, "Calls to setter." );
        is( $mocks_hash{'$main'}->sub_tracking()->{_setConfigurationElement}[0]{args}, ['SWIFT_TOKEN', $mocked_module_values{'X-Subject-Token'}], "Swift token set as expected." );


        # Reset environment
        COMMON::ProxyStorage::resetConfiguration();
        reset_env();
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_first_try_without_keystone => sub {
        # Environment for the test
        my %mocked_module_values = (
            'ROK4_KEYSTONE_IS_USED' => FALSE,
            'ROK4_SWIFT_AUTHURL' => 'https://auth.exemple.com:8080/swift/',
            'ROK4_SWIFT_USER' => 'swift_user_1',
            'ROK4_SWIFT_PASSWD' => 'swift_password_1',
            'UA' => undef,            
            'X-Auth-Token' => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS',
            'X-Storage-Url' => 'https://cluster.swift.com:8081'
        );
        my $override_log_subs = LOG_METHODS;

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : $main
        $mocks_hash{'$main'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => $override_log_subs
        );
        $mocks_hash{'$main'}->override('_setConfigurationElement' => sub {
            my $key = shift;
            my $value = shift;
            $mocked_module_values{$key} = $value;
            return TRUE;
        });
        $mocks_hash{'$main'}->override('_getConfigurationElement' => sub {
            my $key = shift;
            return $mocked_module_values{$key};
        });

        ### Namespace : HTTP::Request::Common
        $mocks_hash{'HTTP::Request::Common'} = mock 'HTTP::Request::Common' => (
            track => TRUE,
            add_constructor => {
                new => 'hash'
            },
            add => {
                header => sub {
                    my $self = shift;
                    my $key = shift;
                    my $value = shift;
                    exists($self->{'headers'}->{$key});
                    $self->{'headers'}->{$key} = $value;
                    return $self->{'headers'}->{$key};
                }          
            },
            override => {
                GET => sub {
                    my $url = shift;
                    return HTTP::Request::Common->new('url' => $url);
                }
            }
        );

        ### Namespace : HTTP::Response
        $mocks_hash{'HTTP::Response'} = mock 'HTTP::Response' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            override => {
                is_success => sub {
                    my $self = shift;
                    return TRUE;
                }
            },
            add => {
                header => sub {
                    my $self = shift;
                    my $key = shift;
                    return $mocked_module_values{$key};
                }
            }
        );

        ### Namespace : LWP::UserAgent
        $mocks_hash{'LWP::UserAgent'} = mock 'LWP::UserAgent' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },            
            override => {
                request => sub {
                    my $self = shift;
                    return HTTP::Response->new();
                }
            }
        );
        $mocked_module_values{'UA'} = LWP::UserAgent->new();


        # Test return value : must be TRUE
        my $method_return = COMMON::ProxyStorage::getSwiftToken();
        is($method_return, TRUE, "getSwiftToken() without keystone returns TRUE");

        # Test calls (namespace  = LWP::UserAgent)
        is( scalar( @{$mocks_hash{'LWP::UserAgent'}->sub_tracking()->{request}} ), 1, "Request sent." );
        is( $mocks_hash{'LWP::UserAgent'}->sub_tracking()->{request}[0]{args}[1]{'url'}, $mocked_module_values{'ROK4_SWIFT_AUTHURL'}, "Request URL OK." );
        ok( !exists($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{request}[0]{args}[1]{'Content'}), "No request body." );

        # Test calls (namespace = $main)
        my @logging_subs_called = keys( %{$mocks_hash{'$main'}->sub_tracking()} );
        my $logger_subs = join('|', keys(%{$override_log_subs}));
        is( scalar(grep(/^($logger_subs)$/, @logging_subs_called)), 0, "No call to logger." );
        is( scalar(@{$mocks_hash{'$main'}->sub_tracking()->{_setConfigurationElement}}), 2, "Calls to setter." );
        is( $mocks_hash{'$main'}->sub_tracking()->{_setConfigurationElement}[0]{args}, ['SWIFT_TOKEN', $mocked_module_values{'X-Auth-Token'}], "Swift token set as expected." );
        is( $mocks_hash{'$main'}->sub_tracking()->{_setConfigurationElement}[1]{args}, ['ROK4_SWIFT_PUBLICURL', $mocked_module_values{'X-Storage-Url'}], "Swift public URL set as expected." );


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }
        COMMON::ProxyStorage::resetConfiguration();
        reset_env();

        done_testing;

    };

    done_testing;
};



# Tested method : COMMON::ProxyStorage::sendSwiftRequest()
subtest test_sendSwiftRequest => sub {
    subtest ok_first_try => sub {

        # Environment for the test
        my %mocked_module_values = (
            'UA' => undef,
            'SWIFT_TOKEN' => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS'
        );

        ## Mocks
        my %mocks_hash = ();
        my $override_log_subs = LOG_METHODS;

        ### Namespace : $main
        $mocks_hash{'$main'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => $override_log_subs
        );
        $mocks_hash{'$main'}->override('_getConfigurationElement' => sub {
            my $key = shift;
            return $mocked_module_values{$key};
        });

        ### Namespace : HTTP::Request
        $mocks_hash{'HTTP::Request'} = mock 'HTTP::Request' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            add => {
                header => sub {
                    my $self = shift;
                    my $key = shift;
                    my $value = shift;
                    exists($self->{'headers'}->{$key});
                    $self->{'headers'}->{$key} = $value;
                    return $self->{'headers'}->{$key};
                }          
            }
        );

        ### Namespace : HTTP::Response
        $mocks_hash{'HTTP::Response'} = mock 'HTTP::Response' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            override => {
                is_success => sub {
                    my $self = shift;
                    return TRUE;
                }
            }
        );

        ### Namespace : LWP::UserAgent
        $mocks_hash{'LWP::UserAgent'} = mock 'LWP::UserAgent' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },            
            override => {
                request => sub {
                    my $self = shift;
                    return HTTP::Response->new();
                }
            }
        );
        $mocked_module_values{'UA'} = LWP::UserAgent->new();


        # Test return value
        my $request = HTTP::Request->new('url' => 'https://cluster.swift.com:8081/kzty3tg85bypmtek1dgv2d61/pyramids');
        my $method_return = COMMON::ProxyStorage::sendSwiftRequest($request);
        ok(defined($method_return), 'COMMON::ProxyStorage::sendSwiftRequest($request) returns something.');
        ok($method_return->is_success(), 'Return is success.');

        # Test calls (namespace  = LWP::UserAgent)
        is( scalar( @{$mocks_hash{'LWP::UserAgent'}->sub_tracking()->{request}} ), 1, "Request sent." );
        is( $mocks_hash{'LWP::UserAgent'}->sub_tracking()->{request}[0]{args}[1]->{headers}, {'X-Auth-Token' => $mocked_module_values{'SWIFT_TOKEN'}}, "Request auth token OK." );

        # Test calls (namespace = $main)
        my @logging_subs_called = keys( %{$mocks_hash{'$main'}->sub_tracking()} );
        my $logger_subs = join('|', keys(%{$override_log_subs}));
        is( scalar(grep(/^($logger_subs)$/, @logging_subs_called)), 0, "No call to logger." );


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }
        COMMON::ProxyStorage::resetConfiguration();
        reset_env();

        done_testing;
    };

    done_testing;
};


# Tested method : COMMON::ProxyStorage::returnSwiftToken()
subtest test_returnSwiftToken => sub {
    subtest defined_token => sub {
        # Environment for the test
        my $swift_token = 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS';

        ## Mocks
        ### Namespace : $main
        my $override_log_subs = LOG_METHODS;
        my $mock_main = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => $override_log_subs
        );
        $mock_main->override('_getConfigurationElement' => sub {
            return $swift_token;
        });


        # Test return value
        my $method_return = COMMON::ProxyStorage::returnSwiftToken();
        is($method_return, $swift_token, "Correct token returned");

        # Test calls (namespace = $main)
        my @logging_subs_called = keys( %{$mock_main->sub_tracking()} );
        my $logger_subs = join('|', keys(%{$override_log_subs}));
        is( scalar(grep(/^($logger_subs)$/, @logging_subs_called)), 1, "Call to logger." );
        ok( exists($mock_main->sub_tracking()->{DEBUG}), "Call to DEBUG logger." );


        # Reset environment
        $mock_main = undef;
        COMMON::ProxyStorage::resetConfiguration();
    
        done_testing;
    };

    subtest undefined_token => sub {
        # Environment for the test
        ## Mocks
        ### Namespace : $main
        my $override_log_subs = LOG_METHODS;
        my $mock_main = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => $override_log_subs
        );
        $mock_main->override('_getConfigurationElement' => sub {
            return undef;
        });


        # Test return value
        my $method_return = COMMON::ProxyStorage::returnSwiftToken();
        is($method_return, undef, "Undefined return.");

        # Test calls (namespace = $main)
        my @logging_subs_called = keys( %{$mock_main->sub_tracking()} );
        my $logger_subs = join('|', keys(%{$override_log_subs}));
        is( scalar(grep(/^($logger_subs)$/, @logging_subs_called)), 1, "Call to logger." );
        ok( exists($mock_main->sub_tracking()->{DEBUG}), "Call to DEBUG logger." );


        # Reset environment
        $mock_main = undef;
        COMMON::ProxyStorage::resetConfiguration();
    
        done_testing;
    };

    done_testing;
};

# Tested method : COMMON::ProxyStorage::copy()
subtest test_copy => sub {
    # La fonction la plus conséquente.
    # 16 cas sans erreur : {fichier, ceph, s3, swift} -> {fichier, ceph, s3, swift}

    subtest ok_file_to_file => sub {
        # Environment for the test
        my %variables = (
            'fromType' => 'FILE',
            'fromPath' => '/dir/to/source/s_file.pyr',
            'toType' => 'FILE',
            'toPath' => '/dir/to/target/t_file.pyr',
            'dir' => ''
        );

        ## Mocks
        my %mocks_hash = ();
        my $override_log_subs = LOG_METHODS;

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => $override_log_subs
        );

        ### Namespace : File::Basename
        $mocks_hash{'File::Basename'} = mock 'File::Basename' => (
            track => TRUE,
            override => {
                dirname => sub {
                    return $variables{'dir'};
                }
            }
        );

        ### Namespace : File::Copy
        $mocks_hash{'File::Copy'} = mock 'File::Copy' => (
            track => TRUE,
            override => {
                copy => sub {
                    $? = 0;
                    return TRUE;
                }
            }
        );

        ### Namespace : File::Path
        $mocks_hash{'File::Path'} = mock 'File::Path' => (
            track => TRUE,
            override => {
                make_path => sub {
                    return TRUE;
                }
            }
        );

        # Tests
        ## Valeur de retour
        my $method_return = COMMON::ProxyStorage::copy($variables{'fromType'}, $variables{'fromPath'}, $variables{'toType'}, $variables{'toPath'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels aux commandes système
        ok(exists($mocks_hash{'File::Path'}->sub_tracking()->{'make_path'}), "Création d'une arborescence dossier.");
        is($mocks_hash{'File::Path'}->sub_tracking()->{'make_path'}[0]{'args'}[0], $variables{'dir'}, "Création du bon dossier.");
        ok(exists($mocks_hash{'File::Copy'}->sub_tracking()->{'copy'}), "Copie de fichier.");
        is($mocks_hash{'File::Copy'}->sub_tracking()->{'copy'}[0]{'args'}, [$variables{'fromPath'}, $variables{'toPath'}], "Bonne source et destination.");

        done_testing;
    };

    done_testing;
};

done_testing;
