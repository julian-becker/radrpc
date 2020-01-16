# Find instrumented libraries
if (BIN_MSAN)
    # libcxx
    set(ENV{LIBCPP_ROOT} "${MSAN_LIBCXX_DIR}")
    set(LIBCPP_USE_STATIC ON)
    find_package(Libc++)
    # boost - use custom module since it doesn't reset the cache..
    set(ENV{BOOSTC_ROOT} "${MSAN_BOOST_DIR}")
    set(BOOSTC_USE_STATIC ON)
    find_package(BoostCustom 1.70 COMPONENTS filesystem system date_time)
    # openssl
    if(SUPPORT_SSL)
        set(OPENSSL_ROOT_DIR_TMP "$ENV{OPENSSL_ROOT_DIR}")
        set(ENV{OPENSSL_ROOT_DIR} "${MSAN_OPENSSL_DIR}")
        set(OPENSSL_USE_STATIC_LIBS ON)
        find_package(OpenSSL REQUIRED)
        unset(OPENSSL_FOUND CACHE) 
        unset(OPENSSL_INCLUDE_DIR CACHE)
        unset(OPENSSL_CRYPTO_LIBRARY CACHE)
        unset(OPENSSL_CRYPTO_LIBRARIES CACHE)
        unset(OPENSSL_SSL_LIBRARY CACHE)
        unset(OPENSSL_SSL_LIBRARIES CACHE)
        unset(OPENSSL_LIBRARIES CACHE)
    endif()
endif()

# boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.70 REQUIRED COMPONENTS filesystem system date_time)
if (NOT Boost_FOUND)
    return()
endif()

# openssl
if(SUPPORT_SSL)
    if (BIN_MSAN)
        set(ENV{OPENSSL_ROOT_DIR} "${OPENSSL_ROOT_DIR_TMP}")
    endif()
    find_package(OpenSSL REQUIRED)
    if(NOT OpenSSL_FOUND)
        message(WARNING "It was requested to build with SSL support but OpenSSL was not found!")
        set(SUPPORT_SSL OFF)
    endif()
endif()





