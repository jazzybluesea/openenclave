# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

## This function adds a CMake target for oedebugrt.dll which needs to be in same path as the host 
## executable in order for enclave applications to be debugguable via windbg
##

function(add_oedebugrt_target TARGET_NAME)

    if (NOT WIN32)
        message(WARNING "add_oedebugrt_target is only intended for WIN32 build environments. Check if this invocation is needed.")
    endif ()

    # Initialize the null list of dependencies for the target
    set(DEPENDENCIES "")

    # Define the DCAP provider path
    set(OE_DEBUGRT ${CMAKE_SOURCE_DIR}/../../../../bin/oedebugrt.dll)

    # No-op if the DCAP provider is not found
    if (NOT EXISTS ${OE_DEBUGRT})
        message (WARNING "oedebugrt not found, will not be able to debug enclave applications.")
    else ()
        # Add copy actions for each of the dependencies
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/oedebugrt.dll
            DEPENDS ${OE_DEBUGRT}
            COMMAND ${CMAKE_COMMAND} -E copy ${OE_DEBUGRT} ${CMAKE_CURRENT_BINARY_DIR})

        # Add the dependencies to the custom target list of dependencies
        list(APPEND DEPENDENCIES
            ${CMAKE_CURRENT_BINARY_DIR}/oedebugrt.dll)
    endif ()

    # Always create the requested target, which may have an empty dependency list
    add_custom_target(${TARGET_NAME} DEPENDS ${DEPENDENCIES})

endfunction(add_oedebugrt_target)
