cmake_minimum_required(VERSION 3.20)
project(NexusCommanderMarkers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(NEXUSCOMMANDERMARKERS_CROSS_COMPILE "Cross-compile for Windows using MinGW" ON)

if(NEXUSCOMMANDERMARKERS_CROSS_COMPILE)
    if(NOT CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/mingw-w64-toolchain.cmake"
            CACHE FILEPATH "MinGW cross-compile toolchain")
    endif()
endif()

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.80
)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(nlohmann_json)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    )
    target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
endif()

file(GLOB TEXTURE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/*.png")
set(EMBEDDED_ASSETS_DIR "${CMAKE_BINARY_DIR}/generated")
set(EMBEDDED_TEXTURES_CPP "${EMBEDDED_ASSETS_DIR}/EmbeddedTextures.cpp")
set(EMBEDDED_TEXTURES_H "${EMBEDDED_ASSETS_DIR}/EmbeddedTextures.h")

add_custom_command(
    OUTPUT ${EMBEDDED_TEXTURES_CPP} ${EMBEDDED_TEXTURES_H}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EMBEDDED_ASSETS_DIR}"
    COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_assets.py"
            --textures-dir "${CMAKE_CURRENT_SOURCE_DIR}/assets/textures"
            --output-dir "${EMBEDDED_ASSETS_DIR}"
    DEPENDS ${TEXTURE_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_assets.py"
    COMMENT "Embedding Commander Markers PNG assets"
    VERBATIM
)

set(SRC_FILES
    src/entry.cpp
    src/core/AppState.cpp
    src/core/SettingsStore.cpp
    src/core/MumbleUtils.cpp
    src/data/MarkerSetJson.cpp
    src/data/MarkerSetClipboard.cpp
    src/data/MapDataCache.cpp
    src/data/StaticDataLoader.cpp
    src/data/CommunityMeta.cpp
    src/services/HttpClient.cpp
    src/services/Gw2MapApiClient.cpp
    src/services/MarkerListing.cpp
    src/services/CommunityMarkerService.cpp
    src/services/MarkerPlacementService.cpp
    src/services/MapWatchService.cpp
    src/services/RtApiService.cpp
    src/utils/MarkerBindHelper.cpp
    src/utils/Base64Codec.cpp
    src/utils/MarkerPlacementHelper.cpp
    src/utils/GameScanCode.cpp
    src/utils/InteractBindService.cpp
    src/utils/MumbleIdentity.cpp
    src/utils/WorldProjection.cpp
    src/ui/OverlayPanel.cpp
    src/ui/TextureService.cpp
    src/ui/DatAssetIconService.cpp
    src/ui/MarkersPanel.cpp
    src/ui/ScreenMapOverlay.cpp
    src/ui/BillboardRenderer.cpp
    src/ui/OptionsUiKit.cpp
    src/ui/OptionsPanel.cpp
    src/ui/SettingsWindow.cpp
    src/ui/LibraryEditorUi.cpp
    src/ui/MarkerSetEditorWindow.cpp
    src/ui/MapPickerUi.cpp
    src/ui/SquadMarkerIconPicker.cpp
    src/ui/CommunityLibraryUi.cpp
    src/ui/QuickAccessService.cpp
    src/ui/UiFontService.cpp
    ${EMBEDDED_TEXTURES_CPP}
)

add_custom_target(generate_embedded_assets DEPENDS ${EMBEDDED_TEXTURES_CPP} ${EMBEDDED_TEXTURES_H})

add_library(NexusCommanderMarkers SHARED ${SRC_FILES})
add_dependencies(NexusCommanderMarkers generate_embedded_assets)

target_include_directories(NexusCommanderMarkers PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/extern
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/nexus
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/mumble
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/rtapi
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(NexusCommanderMarkers PRIVATE
    imgui
    nlohmann_json::nlohmann_json
    winhttp
    ws2_32
    crypt32
    shell32
)

target_compile_definitions(NexusCommanderMarkers PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

if(CMAKE_CXX_COMPILER MATCHES "mingw" OR MINGW)
    target_link_options(NexusCommanderMarkers PRIVATE
        -static-libgcc
        -static-libstdc++
        -static
    )
endif()

set_target_properties(NexusCommanderMarkers PROPERTIES
    OUTPUT_NAME "NexusCommanderMarkers"
    PREFIX ""
    SUFFIX ".dll"
)
