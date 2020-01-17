# boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.70 REQUIRED COMPONENTS filesystem system date_time)
if (NOT Boost_FOUND)
    return()
endif()

# openssl
if(SUPPORT_SSL)
    find_package(OpenSSL REQUIRED)
    if(NOT OpenSSL_FOUND)
        message(WARNING "It was requested to build with SSL support but OpenSSL was not found.")
        set(SUPPORT_SSL OFF)
    endif()
endif()

# Find instrumented libraries
#[[ Note:
    Searching the same package but with another location doesn't 
    work well with 'find_package'. 
    Trying to copy variables & reset the cache just mess up 
    the whole link process.
    Hence the custom modules (FindLibc++, FindBoostCustom, FindOpenSSLCustom)
    are used now.
    If something can be done better, please let me know.
]]
if (BIN_MSAN AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # libcxx
    set(ENV{LIBCPP_ROOT} "${MSAN_LIBCXX_DIR}")
    set(LIBCPP_USE_STATIC ON)
    find_package(Libc++)
    if (NOT LIBCPP_FOUND)
        message(FATAL "Instrumented libc++ library not found.")
        return()
    endif()
    # boost
    set(ENV{BOOSTC_ROOT} "${MSAN_BOOST_DIR}")
    set(BOOSTC_USE_STATIC ON)
    find_package(BoostCustom 1.70 COMPONENTS filesystem system date_time)
    if (NOT BOOSTC_FOUND)
        message(FATAL "Instrumented Boost library not found.")
        return()
    endif()
    # openssl
    if(SUPPORT_SSL)
        set(ENV{OPENSSLC_ROOT} "${MSAN_OPENSSL_DIR}")
        set(OPENSSLC_USE_STATIC ON)
        find_package(OpenSSLCustom REQUIRED)
        if(NOT OPENSSLC_FOUND)
            message(WARNING "It was requested to build with SSL support but instrumented OpenSSL was not found.")
            set(SUPPORT_SSL OFF)
        endif()
    endif()
    set(INSTRUMENTED_FOUND TRUE)

endif()