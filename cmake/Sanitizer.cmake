set(SANTIZERS "")
if(BIN_ASAN)
    set(SANTIZERS ${SANTIZERS} address)
endif()
if(BIN_UBSAN)
    set(SANTIZERS ${SANTIZERS} undefined)
endif()
if(BIN_TSAN)
    set(SANTIZERS ${SANTIZERS} thread)
endif()
# Memory sanitizer is extra, 
# since it needs to be linked with instrumented libraries