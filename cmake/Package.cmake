# boost
set(Boost_USE_STATIC_LIBS ON)
if (IS_CI_BUILD AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    find_package(Boost 1.71 REQUIRED COMPONENTS filesystem system date_time)
else()
    find_package(Boost 1.72 REQUIRED COMPONENTS filesystem system date_time)
endif()
if (NOT Boost_FOUND)
    message(FATAL "Boost library not found.")
    return()
endif()

# openssl
if(SUPPORT_SSL)
    message("OpenSSL root: $ENV{OPENSSL_ROOT_DIR}")
    find_package(OpenSSL 1.1.1 REQUIRED)
    if(NOT OpenSSL_FOUND)
        message(FATAL "It was requested to build with SSL support but OpenSSL was not found.")
        return()
    endif()
endif()

# Find instrumented libraries
#[[ Note:
    Searching the same package but with another location
    doesn't seem to work well with 'find_package'. 
    Copying variables & reset the cache just mess up
    the whole link process.
    Hence the custom modules (FindLibc++, FindBoostCustom, FindOpenSSLCustom)
    are used now.
    If something can be done better, please let me know.
]]
if (BUILD_WITH_MSAN AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
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
    find_package(BoostCustom 1.72 COMPONENTS filesystem system date_time)
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

# llvm symbolizer
if (BUILD_TESTS OR BUILD_STRESS_TESTS)

    if (NOT IS_CI_BUILD)

        find_package(LLVMSymbolizer)
        if (NOT LLVM_SYMBOLIZER_FOUND)
            message(WARN "LLVM symbolizer not found.")
        endif()

    endif()

endif()