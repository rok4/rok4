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
        my @main_subs_called = keys( %{$main_mock->sub_tracking()} );
        is( scalar(@main_subs_called), 0, "No call to logger." );

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
        my $main_mock = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
        );
        $main_mock->override('_setConfigurationElement' => sub {});


        # Test return value : must be TRUE
        my $method_return = COMMON::ProxyStorage::checkEnvironmentVariables('CEPH');
        is($method_return, TRUE, "checkEnvironmentVariables('CEPH') returns TRUE");

        # Test module scoped variables : must match environment
        my %module_scope_vars = COMMON::ProxyStorage::getConfiguration(keys(%temp_env));
        is(%module_scope_vars, %temp_env, "Module scope variables affectation OK.");

        # Test calls (namespace = $main)
        my @main_subs_called = keys( %{$main_mock->sub_tracking()} );
        my $logger_subs = join('|', @LOG_METHODS_NAME);
        is( scalar(grep(/^($logger_subs)$/, @main_subs_called)), 0, "No call to logger." );
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
        my @main_subs_called = keys( %{$main_mock->sub_tracking()} );
        my $logger_subs = join('|', @LOG_METHODS_NAME);
        is( scalar(grep(/^($logger_subs)$/, @main_subs_called)), 0, "No call to logger." );
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
            my @main_subs_called = keys( %{$main_mock->sub_tracking()} );
            my $logger_subs = join('|', @LOG_METHODS_NAME);
            is( scalar(grep(/^($logger_subs)$/, @main_subs_called)), 0, "No call to logger." );
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
            my @main_subs_called = keys( %{$main_mock->sub_tracking()} );
            my $logger_subs = join('|', @LOG_METHODS_NAME);
            is( scalar(grep(/^($logger_subs)$/, @main_subs_called)), 0, "No call to logger." );
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

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : $main
        $mocks_hash{'$main'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
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
        my @main_subs_called = keys( %{$mocks_hash{'$main'}->sub_tracking()} );
        my $logger_subs = join('|', @LOG_METHODS_NAME);
        is( scalar(grep(/^($logger_subs)$/, @main_subs_called)), 0, "No call to logger." );
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

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : $main
        $mocks_hash{'$main'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
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
        my @main_subs_called = keys( %{$mocks_hash{'$main'}->sub_tracking()} );
        my $logger_subs = join('|', @LOG_METHODS_NAME);
        is( scalar(grep(/^($logger_subs)$/, @main_subs_called)), 0, "No call to logger." );
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

        ### Namespace : $main
        $mocks_hash{'$main'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
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
        my @main_subs_called = keys( %{$mocks_hash{'$main'}->sub_tracking()} );
        my $logger_subs = join('|', @LOG_METHODS_NAME);
        is( scalar(grep(/^($logger_subs)$/, @main_subs_called)), 0, "No call to logger." );


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
        my $mock_main = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
        );
        $mock_main->override('_getConfigurationElement' => sub {
            return $swift_token;
        });


        # Test return value
        my $method_return = COMMON::ProxyStorage::returnSwiftToken();
        is($method_return, $swift_token, "Correct token returned");

        # Test calls (namespace = $main)
        my @main_subs_called = keys( %{$mock_main->sub_tracking()} );
        my $logger_subs = join('|', @LOG_METHODS_NAME);
        is( scalar(grep(/^($logger_subs)$/, @main_subs_called)), 1, "Call to logger." );
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
        my $mock_main = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
        );
        $mock_main->override('_getConfigurationElement' => sub {
            return undef;
        });


        # Test return value
        my $method_return = COMMON::ProxyStorage::returnSwiftToken();
        is($method_return, undef, "Undefined return.");

        # Test calls (namespace = $main)
        my @main_subs_called = keys( %{$mock_main->sub_tracking()} );
        my $logger_subs = join('|', @LOG_METHODS_NAME);
        is( scalar(grep(/^($logger_subs)$/, @main_subs_called)), 1, "Call to logger." );
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
            'source_type' => 'FILE',
            'source_path' => '/dir/to/source/s_file.pyr',
            'target_type' => 'FILE',
            'target_dir' => '/dir/to/target'
        );
        $variables{'target_path'} = "$variables{'target_dir'}/t_file.pyr";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
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
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels aux commandes système
        ok(exists($mocks_hash{'File::Path'}->sub_tracking()->{'make_path'}), "Directory arbroescence created.");
        is($mocks_hash{'File::Path'}->sub_tracking()->{'make_path'}[0]{'args'}[0], $variables{'target_dir'}, "Correct directory.");
        ok(exists($mocks_hash{'File::Copy'}->sub_tracking()->{'copy'}), "File copy.");
        is($mocks_hash{'File::Copy'}->sub_tracking()->{'copy'}[0]{'args'}, [$variables{'source_path'}, $variables{'target_path'}], "Correct source and target.");


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_file_to_ceph => sub {

        # Environment for the test
        my %variables = (
            'source_type' => 'FILE',
            'source_path' => '/dir/to/source/s_file.pyr',
            'target_type' => 'CEPH',
            'target_pool' => 't_pool',
            'target_object' =>'t_object'
        );
        $variables{'target_path'} = "$variables{'target_pool'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS
        );

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'system' => sub {
                    $? = 0;
                    return;
                }
            }
        );


        # Tests
        ## Valeur de retour
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels aux commandes système
        ok(exists($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'system'}), "Call to system command.");
        is($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'system'}[0]{'args'}[0], "rados", "Correct executable.");
        is($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'system'}[0]{'args'}[1], "-p $variables{'target_pool'}", "Correct pool.");
        is($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'system'}[0]{'args'}[2], "put $variables{'target_object'} $variables{'source_path'}", "Correct action.");


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_file_to_s3 => sub {

        # Environment for the test
        my %variables = (
            'source_type'           => 'FILE',
            'source_path'           => '/dir/to/source/s_file.pyr',
            'target_type'           => 'S3',
            'target_bucket'         => 't_bucket',
            'target_object'         => 't_object',
            'body_content'          => 'This is the body file content.',
            'date'                  => 'Tue, 08 Dec 2020 15:07:27 +0000',
            'ROK4_S3_URL'           => 'http://url_to_s3_cluster.net',
            'ROK4_S3_ENDPOINT_HOST' => 'http://url_to_s3_host.net/endpoint',
            'ROK4_S3_KEY'           => 'KeyToS3',
            'ROK4_S3_SECRETKEY'     => 'SecretKeyToS3'
        );
        $variables{'target_path'} = "$variables{'target_bucket'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                },
                'map_file' => sub {
                    $_[0] = $variables{'body_content'};
                }
            }
        );

        ### Namespace : HTTP::Request
        $mocks_hash{'HTTP::Request'} = mock 'HTTP::Request' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            add => {
                'header' => sub {
                    my $self = shift;
                    my $key = shift;
                    my $value = shift;
                    exists($self->{'headers'}->{$key});
                    $self->{'headers'}->{$key} = $value;
                    return $self->{'headers'}->{$key};
                },
                'content' => sub {
                    my $self = shift;
                    my $string = shift;
                    $self->{'body'} = $string;
                    return $self->{'body'}
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
        $variables{'UA'} = LWP::UserAgent->new();

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'readpipe' => sub {
                    $? = 0;
                    return $variables{'date'};
                }
            }
        );

        # Tests
        ## Valeur de retour
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels au shell
        ok(exists($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'readpipe'}), "qx// called");
        like($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'readpipe'}[0]{'args'}[0], qr/.*date.*/, "Call to shell 'date'");

        ## Appels liés aux requêtes
        ok(exists($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}), "Request sent.");
        is($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}[0]{'args'}[1]{'PUT'}, "$variables{'ROK4_S3_URL'}/$variables{'target_path'}", "Correct URL.");
        is($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}[0]{'args'}[1]{'body'}, $variables{'body_content'}, "Correct body.");
        my $expected_request_headers = {
            'Host' => $variables{'ROK4_S3_ENDPOINT_HOST'},
            'Date' => $variables{'date'},
            'Content-Type' => 'application/octet-stream',
            'Authorization' => qr/AWS $variables{'ROK4_S3_KEY'}:.+=/
        };
        like($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}[0]{'args'}[1]{'headers'}, $expected_request_headers, "Correct headers.");

        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_file_to_swift =>sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'FILE',
            'source_path'           => '/dir/to/source/s_file.pyr',
            'target_type'           => 'SWIFT',
            'target_container'      => 't_container',
            'target_object'         => 't_object',
            'body_content'          => 'This is the body file content.',
            'ROK4_SWIFT_PUBLICURL'  => 'https://cluster.swift.com:8081',
            'SWIFT_TOKEN' => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS'
        );
        $variables{'target_path'} = "$variables{'target_container'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                },
                'map_file' => sub {
                    $_[0] = $variables{'body_content'};
                },
                'sendSwiftRequest' => sub {
                    return HTTP::Response->new();
                }
            }
        );

        ### Namespace : HTTP::Request
        $mocks_hash{'HTTP::Request'} = mock 'HTTP::Request' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            add => {
                'content' => sub {
                    my $self = shift;
                    my $string = shift;
                    $self->{'body'} = $string;
                    return $self->{'body'}
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


        # Tests
        ## Valeur de retour
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels liés aux requêtes
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'sendSwiftRequest'}), "Request sent.");
        is($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'sendSwiftRequest'}[0]{'args'}[0]{'PUT'}, "$variables{'ROK4_SWIFT_PUBLICURL'}/$variables{'target_path'}", "Correct URL.");
        is($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'sendSwiftRequest'}[0]{'args'}[0]{'body'}, $variables{'body_content'}, "Correct body.");

        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }


        done_testing;
    };


    subtest ok_ceph_to_file => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'CEPH',
            'source_pool'           => 's_pool',
            'source_object'         => 's_object',
            'target_type'           => 'FILE',,
            'target_dir'           => '/dir/to/target'
        );
        $variables{'source_path'} = "$variables{'source_pool'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_dir'}/t_file.pyr";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                'getRealData' => sub {
                    my $type = shift;
                    my $path = shift;
                    return $path;
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

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'system' => sub {
                    $? = 0;
                    return;
                }
            }
        );


        # Tests
        ## Valeur de retour
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels système
        is($mocks_hash{'File::Path'}->sub_tracking()->{'make_path'}[0]{'args'}[0], $variables{'target_dir'}, "Target directory creation.");
        is($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'system'}[0]{'args'}, ["rados", "-p $variables{'source_pool'}", "get $variables{'source_object'} $variables{'target_path'}"], "Pulled from ceph.");


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_ceph_to_ceph => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'CEPH',
            'common_pool'           => 'c_pool',
            'source_object'         => 's_object',
            'target_type'           => 'CEPH',
            'target_object'         => 't_object'
        );
        $variables{'source_path'} = "$variables{'common_pool'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'common_pool'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                'getRealData' => sub {
                    my $type = shift;
                    my $path = shift;
                    return $path;
                }
            }
        );

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'system' => sub {
                    $? = 0;
                    return;
                }
            }
        );


        # Tests
        ## Valeur de retour
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels système
        is($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'system'}[0]{'args'}, ["rados", "-p $variables{'common_pool'}", "cp $variables{'source_object'} $variables{'target_object'}"], "Object copy inside ceph pool.");


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_ceph_to_s3 => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'CEPH',
            'source_pool'           => 's_pool',
            'source_object'         => 's_object',
            'target_type'           => 'S3',
            'target_bucket'         => 't_bucket',
            'target_object'         => 't_object',
            'body_content'          => 'This is the body file content.',
            'date'                  => 'Tue, 08 Dec 2020 15:07:27 +0000',
            'ROK4_S3_URL'           => 'http://url_to_s3_cluster.net',
            'ROK4_S3_ENDPOINT_HOST' => 'http://url_to_s3_host.net/endpoint',
            'ROK4_S3_KEY'           => 'KeyToS3',
            'ROK4_S3_SECRETKEY'     => 'SecretKeyToS3'
        );
        $variables{'target_path'} = "$variables{'target_bucket'}/$variables{'target_object'}";
        $variables{'source_path'} = "$variables{'source_pool'}/$variables{'source_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                'getRealData' => sub {
                    my $type = shift;
                    my $path = shift;
                    return $path;
                }
            }
        );

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'system' => sub {
                    $? = 0;
                    return;
                }
            }
        );

        todo 'not_implemented' => sub {
            # Tests
            ## Valeur de retour
            my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
            is($method_return, TRUE, "Returns TRUE.");

            ## Appels au logger
            foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
                ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
            }
            ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");
        };


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_ceph_to_swift => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'CEPH',
            'source_pool'           => 's_pool',
            'source_object'         => 's_object',
            'target_type'           => 'SWIFT',
            'target_container'      => 't_container',
            'target_object'         => 't_object',
            'body_content'          => 'This is the body file content.',
            'ROK4_SWIFT_PUBLICURL'  => 'https://cluster.swift.com:8081',
            'SWIFT_TOKEN'           => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS'
        );
        $variables{'source_path'} = "$variables{'source_pool'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_container'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                'getRealData' => sub {
                    my $type = shift;
                    my $path = shift;
                    return $path;
                }
            }
        );

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'system' => sub {
                    $? = 0;
                    return;
                }
            }
        );

        todo 'not_implemented' => sub {
            # Tests
            ## Valeur de retour
            my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
            is($method_return, TRUE, "Returns TRUE.");

            ## Appels au logger
            foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
                ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
            }
            ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");
        };


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_s3_to_file => sub {

        # Environment for the test
        my %variables = (
            'source_type'           => 'S3',
            'source_bucket'         => 's_bucket',
            'source_object'         => 's_object',

            'target_type'           => 'FILE',
            'target_dir'           => '/dir/to/source',

            'date'                  => 'Tue, 08 Dec 2020 15:07:27 +0000',
            'ROK4_S3_URL'           => 'http://url_to_s3_cluster.net',
            'ROK4_S3_ENDPOINT_HOST' => 'http://url_to_s3_host.net/endpoint',
            'ROK4_S3_KEY'           => 'KeyToS3',
            'ROK4_S3_SECRETKEY'     => 'SecretKeyToS3'
        );
        $variables{'source_path'} = "$variables{'source_bucket'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_dir'}/s_file.pyr";


        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                }
            }
        );

        ### Namespace : HTTP::Request
        $mocks_hash{'HTTP::Request'} = mock 'HTTP::Request' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            add => {
                'header' => sub {
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
        $variables{'UA'} = LWP::UserAgent->new();

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'readpipe' => sub {
                    $? = 0;
                    return $variables{'date'};
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
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels système
        ok(exists($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'readpipe'}), "qx// called");
        like($mocks_hash{'*CORE::GLOBAL'}->sub_tracking()->{'readpipe'}[0]{'args'}[0], qr/.*date.*/, "Call to shell 'date'");
        is($mocks_hash{'File::Path'}->sub_tracking()->{'make_path'}[0]{'args'}[0], $variables{'target_dir'}, "Target directory creation.");


        ## Appels liés aux requêtes
        ok(exists($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}), "Request sent.");
        is($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}[0]{'args'}[1]{'GET'}, "$variables{'ROK4_S3_URL'}/$variables{'source_path'}", "Correct URL.");
        my $expected_request_headers = {
            'Host' => $variables{'ROK4_S3_ENDPOINT_HOST'},
            'Date' => $variables{'date'},
            'Content-Type' => 'application/octet-stream',
            'Authorization' => qr/AWS $variables{'ROK4_S3_KEY'}:.+=/
        };
        like($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}[0]{'args'}[1]{'headers'}, $expected_request_headers, "Correct headers.");


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_s3_to_ceph => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'S3',
            'source_bucket'         => 's_bucket',
            'source_object'         => 's_object',
            'target_type'           => 'CEPH',
            'target_pool'           => 't_pool',
            'target_object'         => 't_object',
            'date'                  => 'Tue, 08 Dec 2020 15:07:27 +0000',
            'ROK4_S3_URL'           => 'http://url_to_s3_cluster.net',
            'ROK4_S3_ENDPOINT_HOST' => 'http://url_to_s3_host.net/endpoint',
            'ROK4_S3_KEY'           => 'KeyToS3',
            'ROK4_S3_SECRETKEY'     => 'SecretKeyToS3'
        );
        $variables{'source_path'} = "$variables{'source_bucket'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_pool'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                }
            }
        );

        ### Namespace : HTTP::Request
        $mocks_hash{'HTTP::Request'} = mock 'HTTP::Request' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            add => {
                'header' => sub {
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
        $variables{'UA'} = LWP::UserAgent->new();

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'system' => sub {
                    $? = 0;
                    return;
                },
                'readpipe' => sub {
                    $? = 0;
                    return $variables{'date'};
                }
            }
        );

        todo 'not_implemented' => sub {
            # Tests
            ## Valeur de retour
            my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
            is($method_return, TRUE, "Returns TRUE.");

            ## Appels au logger
            foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
                ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
            }
            ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");
        };


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_s3_to_s3 => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'S3',
            'source_bucket'         => 's_bucket',
            'source_object'         => 's_object',
            'target_type'           => 'S3',
            'target_bucket'         => 't_bucket',
            'target_object'         => 't_object',
            'date'                  => 'Tue, 08 Dec 2020 15:07:27 +0000',
            'ROK4_S3_URL'           => 'http://url_to_s3_cluster.net',
            'ROK4_S3_ENDPOINT_HOST' => 'http://url_to_s3_host.net/endpoint',
            'ROK4_S3_KEY'           => 'KeyToS3',
            'ROK4_S3_SECRETKEY'     => 'SecretKeyToS3'
        );
        $variables{'source_path'} = "$variables{'source_bucket'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_bucket'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                }
            }
        );

        ### Namespace : HTTP::Request
        $mocks_hash{'HTTP::Request'} = mock 'HTTP::Request' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            add => {
                'header' => sub {
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
        $variables{'UA'} = LWP::UserAgent->new();

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'readpipe' => sub {
                    $? = 0;
                    return $variables{'date'};
                }
            }
        );


        # Tests
        ## Valeur de retour
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels liés aux requêtes
        ok(exists($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}), "Request sent.");
        is($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}[0]{'args'}[1]{'PUT'}, "$variables{'ROK4_S3_URL'}/$variables{'target_path'}", "Correct URL.");
        my $expected_request_headers = {
            'Host' => $variables{'ROK4_S3_ENDPOINT_HOST'},
            'Date' => $variables{'date'},
            'Content-Type' => 'application/octet-stream',
            'Authorization' => qr/AWS $variables{'ROK4_S3_KEY'}:.+=/,
            'x-amz-copy-source' => "/$variables{'source_bucket'}/$variables{'source_object'}"
        };
        like($mocks_hash{'LWP::UserAgent'}->sub_tracking()->{'request'}[0]{'args'}[1]{'headers'}, $expected_request_headers, "Correct headers.");


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_s3_to_swift => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'S3',
            'source_bucket'         => 's_bucket',
            'source_object'         => 's_object',
            'date'                  => 'Tue, 08 Dec 2020 15:07:27 +0000',
            'ROK4_S3_URL'           => 'http://url_to_s3_cluster.net',
            'ROK4_S3_ENDPOINT_HOST' => 'http://url_to_s3_host.net/endpoint',
            'ROK4_S3_KEY'           => 'KeyToS3',
            'ROK4_S3_SECRETKEY'     => 'SecretKeyToS3',
            'target_type'           => 'SWIFT',
            'target_container'      => 't_container',
            'target_object'         => 't_object',
            'body_content'          => 'This is the body file content.',
            'ROK4_SWIFT_PUBLICURL'  => 'https://cluster.swift.com:8081',
            'SWIFT_TOKEN'           => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS'
        );
        $variables{'source_path'} = "$variables{'source_bucket'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_container'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                }
            }
        );

        ### Namespace : HTTP::Request
        $mocks_hash{'HTTP::Request'} = mock 'HTTP::Request' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            add => {
                'header' => sub {
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
        $variables{'UA'} = LWP::UserAgent->new();

        ### Namespace : *CORE::GLOBAL
        $mocks_hash{'*CORE::GLOBAL'} = mock '*CORE::GLOBAL' => (
            track => TRUE,
            set => {
                'system' => sub {
                    $? = 0;
                    return;
                },
                'readpipe' => sub {
                    $? = 0;
                    return $variables{'date'};
                }
            }
        );

        todo 'not_implemented' => sub {
            # Tests
            ## Valeur de retour
            my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
            is($method_return, TRUE, "Returns TRUE.");

            ## Appels au logger
            foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
                ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
            }
            ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");
        };


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_swift_to_file => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'SWIFT',
            'source_container'      => 's_container',
            'source_object'         => 's_object',
            'body_content'          => 'This is the body file content.',
            'ROK4_SWIFT_PUBLICURL'  => 'https://cluster.swift.com:8081',
            'SWIFT_TOKEN'           => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS',

            'target_type'           => 'FILE',
            'target_dir'            => '/dir/to/target'
        );
        $variables{'source_path'} = "$variables{'source_container'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_dir'}/t_file.pyr";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                },
                'sendSwiftRequest' => sub {
                    return HTTP::Response->new();
                }
            }
        );

        ### Namespace : HTTP::Request
        $mocks_hash{'HTTP::Request'} = mock 'HTTP::Request' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
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
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels système
        is($mocks_hash{'File::Path'}->sub_tracking()->{'make_path'}[0]{'args'}[0], $variables{'target_dir'}, "Target directory creation.");

        ## Appels liés aux requêtes
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'sendSwiftRequest'}), "Request sent.");
        is($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'sendSwiftRequest'}[0]{'args'}[0]{'GET'}, "$variables{'ROK4_SWIFT_PUBLICURL'}/$variables{'source_path'}", "Correct URL.");

        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }
        done_testing;
    };

    subtest ok_swift_to_ceph => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'SWIFT',
            'source_container'      => 's_container',
            'source_object'         => 's_object',
            'ROK4_SWIFT_PUBLICURL'  => 'https://cluster.swift.com:8081',
            'SWIFT_TOKEN'           => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS',

            'target_type'           => 'CEPH',
            'target_pool'           => 't_pool',
            'target_object'         => 't_object'

        );
        $variables{'source_path'} = "$variables{'source_container'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_pool'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                }
            }
        );

        todo 'not_implemented' => sub {
            # Tests
            ## Valeur de retour
            my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
            is($method_return, TRUE, "Returns TRUE.");

            ## Appels au logger
            foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
                ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
            }
            ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");
        };


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_swift_to_s3 => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'SWIFT',
            'source_container'      => 's_container',
            'source_object'         => 's_object',
            'ROK4_SWIFT_PUBLICURL'  => 'https://cluster.swift.com:8081',
            'SWIFT_TOKEN'           => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS',

            'target_type'           => 'S3',
            'target_bucket'         => 't_bucket',
            'target_object'         => 't_object',
            'date'                  => 'Tue, 08 Dec 2020 15:07:27 +0000',
            'ROK4_S3_URL'           => 'http://url_to_s3_cluster.net',
            'ROK4_S3_ENDPOINT_HOST' => 'http://url_to_s3_host.net/endpoint',
            'ROK4_S3_KEY'           => 'KeyToS3',
            'ROK4_S3_SECRETKEY'     => 'SecretKeyToS3'

        );
        $variables{'source_path'} = "$variables{'source_container'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_bucket'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                }
            }
        );

        todo 'not_implemented' => sub {
            # Tests
            ## Valeur de retour
            my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
            is($method_return, TRUE, "Returns TRUE.");

            ## Appels au logger
            foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
                ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
            }
            ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");
        };


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    subtest ok_swift_to_swift => sub {
        # Environment for the test
        my %variables = (
            'source_type'           => 'SWIFT',
            'source_container'      => 's_container',
            'source_object'         => 's_object',
            'ROK4_SWIFT_PUBLICURL'  => 'https://cluster.swift.com:8081',
            'SWIFT_TOKEN'           => 'f0GZyNcnf7_9SDJ31iShwUGzYlLAAlvLN7BQuWHK40YPpqjJ7O7f106ycPnCHYdRxtqQdU8GltNaoxlLk_3PZp4Wv-1r_CurUenWOLsEI-H6NeV65H6oZfPp4VhssTDzEjuk1PfWsVkwSSXBHt69pmPx9UwfMYz0eP7yIagNEz1VIl_uggBb2_PvprJTstQpS',

            'target_type'           => 'SWIFT',
            'target_container'      => 't_container',
            'target_object'         => 't_object'

        );
        $variables{'source_path'} = "$variables{'source_container'}/$variables{'source_object'}";
        $variables{'target_path'} = "$variables{'target_container'}/$variables{'target_object'}";

        ## Mocks
        my %mocks_hash = ();

        ### Namespace : COMMON::ProxyStorage
        $mocks_hash{'COMMON::ProxyStorage'} = mock 'COMMON::ProxyStorage' => (
            track => TRUE,
            override => LOG_METHODS,
            override => {
                '_getConfigurationElement' => sub {
                    my $key = shift;
                    return $variables{$key};
                },
                'sendSwiftRequest' => sub {
                    return HTTP::Response->new();
                }
            }
        );

        ### Namespace : HTTP::Request
        $mocks_hash{'HTTP::Request'} = mock 'HTTP::Request' => (
            track => TRUE,
            override_constructor => {
                new => 'hash'
            },
            add => {
                'header' => sub {
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


        # Tests
        ## Valeur de retour
        my $method_return = COMMON::ProxyStorage::copy($variables{'source_type'}, $variables{'source_path'}, $variables{'target_type'}, $variables{'target_path'});
        is($method_return, TRUE, "Returns TRUE.");

        ## Appels au logger
        foreach my $log_level ('WARN', 'FATAL', 'ERROR', 'INFO', 'TRACE') {
            ok(! exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{$log_level}), "No $log_level log entry.");
        }
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'DEBUG'}), "At least 1 DEBUG log entry.");

        ## Appels liés aux requêtes
        ok(exists($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'sendSwiftRequest'}), "Request sent.");
        is($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'sendSwiftRequest'}[0]{'args'}[0]{'COPY'}, "$variables{'ROK4_SWIFT_PUBLICURL'}/$variables{'source_path'}", "Correct URL.");
        is($mocks_hash{'COMMON::ProxyStorage'}->sub_tracking()->{'sendSwiftRequest'}[0]{'args'}[0]{'headers'}, {'Destination' => $variables{'target_path'}}, "Correct headers.");


        # Reset environment
        foreach my $mock (keys(%mocks_hash)) {
            $mocks_hash{$mock} = undef;
        }

        done_testing;
    };


    done_testing;
};

done_testing;
