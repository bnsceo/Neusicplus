cmake_minimum_required(VERSION 3.22)

project(Neusicplus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.13
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(Neusicplus
    PRODUCT_NAME "Neusicplus"
    COMPANY_NAME "NeusicWave"
    BUNDLE_ID "com.neusicwave.neusicplus"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Neusicplus needs microphone access to record audio."
)

juce_generate_juce_header(Neusicplus)

target_sources(Neusicplus PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/AudioEngine.cpp
    Source/AudioEngine.h
)

target_compile_definitions(Neusicplus PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Neusicplus,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
)

if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(Neusicplus PROPERTIES
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.neusicwave.neusicplus"
        XCODE_ATTRIBUTE_SUPPORTS_MACCATALYST "NO"
    )
endif()

target_link_libraries(Neusicplus PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
