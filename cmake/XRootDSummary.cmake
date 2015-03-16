#-------------------------------------------------------------------------------
# Print the configuration summary
#-------------------------------------------------------------------------------
set( TRUE_VAR TRUE )
component_status( READLINE ENABLE_READLINE READLINE_FOUND )
component_status( FUSE     BUILD_FUSE      FUSE_FOUND )
component_status( CRYPTO   BUILD_CRYPTO    OPENSSL_FOUND )
component_status( KRB5     BUILD_KRB5      KERBEROS5_FOUND )
component_status( LIBEVENT BUILD_LIBEVENT  LIBEVENT_FOUND )
component_status( XRDCL    ENABLE_XRDCL    TRUE_VAR )
component_status( TESTS    BUILD_TESTS     CPPUNIT_FOUND )
component_status( HTTP     BUILD_HTTP      OPENSSL_FOUND )
component_status( CEPH     BUILD_CEPH      CEPH_FOUND )
component_status( PYTHON   BUILD_PYTHON    PYTHON_FOUND )

message( STATUS "----------------------------------------" )
message( STATUS "Installation path: " ${CMAKE_INSTALL_PREFIX} )
message( STATUS "C Compiler:        " ${CMAKE_C_COMPILER} )
message( STATUS "C++ Compiler:      " ${CMAKE_CXX_COMPILER} )
message( STATUS "Build type:        " ${CMAKE_BUILD_TYPE} )
message( STATUS "Plug-in version:   " ${PLUGIN_VERSION} )
message( STATUS "" )
message( STATUS "Readline support:  " ${STATUS_READLINE} )
message( STATUS "Fuse support:      " ${STATUS_FUSE} )
message( STATUS "Crypto support:    " ${STATUS_CRYPTO} )
message( STATUS "Kerberos5 support: " ${STATUS_KRB5} )
message( STATUS "XrdCl:             " ${STATUS_XRDCL} )
message( STATUS "LibEvent support:  " ${STATUS_LIBEVENT} )
message( STATUS "Tests:             " ${STATUS_TESTS} )
message( STATUS "HTTP support:      " ${STATUS_HTTP} )
message( STATUS "CEPH support:      " ${STATUS_CEPH} )
message( STATUS "Python support:    " ${STATUS_PYTHON} )
message( STATUS "----------------------------------------" )
