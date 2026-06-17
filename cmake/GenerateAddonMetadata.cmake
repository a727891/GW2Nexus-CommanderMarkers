# Generate Version.h (each build) into ${CMAKE_BINARY_DIR}/generated.
#
# Usage (after add_library):
#   include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateAddonMetadata.cmake)
#   configure_addon_metadata(${TARGET_NAME} "<user-agent-prefix>")

function(configure_addon_metadata target user_agent_prefix)
    set(_generated_dir "${CMAKE_BINARY_DIR}/generated")
    set(_version_h "${_generated_dir}/Version.h")

    add_custom_command(
        OUTPUT "${_version_h}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_generated_dir}"
        COMMAND python3 "${CMAKE_SOURCE_DIR}/scripts/generate_addon_metadata.py"
                --user-agent-prefix "${user_agent_prefix}"
                --output-dir "${_generated_dir}"
        DEPENDS "${CMAKE_SOURCE_DIR}/scripts/generate_addon_metadata.py"
        COMMENT "Generating Version.h"
        VERBATIM
    )

    add_custom_target(generate_addon_metadata DEPENDS "${_version_h}")
    add_dependencies(${target} generate_addon_metadata)
    target_include_directories(${target} PRIVATE "${_generated_dir}")
endfunction()
