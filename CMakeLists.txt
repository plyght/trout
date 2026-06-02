cmake_minimum_required(VERSION 3.21)
project(trout LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(TROUT_WITH_LLAMA "Build with the llama.cpp inference backend" ON)
option(TROUT_BUILD_APP   "Build the macOS menu-bar app and input method (requires Apple)" ON)

# ---------------------------------------------------------------------------
# llama.cpp (optional inference backend)
# ---------------------------------------------------------------------------
if(TROUT_WITH_LLAMA)
    set(LLAMA_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_SERVER   OFF CACHE BOOL "" FORCE)
    set(LLAMA_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
    set(LLAMA_CURL           OFF CACHE BOOL "" FORCE)
    # The host CPU (-mcpu=native) is not always recognized by the active clang
    # (e.g. apple-m3 under some toolchains). Disable native tuning; arm64 still
    # gets NEON by default.
    set(GGML_NATIVE          OFF CACHE BOOL "" FORCE)
    if(APPLE)
        set(GGML_METAL ON CACHE BOOL "" FORCE)
        set(GGML_ACCELERATE ON CACHE BOOL "" FORCE)
    endif()
    add_subdirectory(third_party/llama.cpp EXCLUDE_FROM_ALL)
endif()

# ---------------------------------------------------------------------------
# trout-core (portable C++ library)
# ---------------------------------------------------------------------------
add_library(trout-core STATIC
    core/src/Log.cpp
    core/src/Paths.cpp
    core/src/Store.cpp
    core/src/Settings.cpp
    core/src/Models.cpp
    core/src/Compat.cpp
    core/src/Emoji.cpp
    core/src/Engine.cpp
    core/src/PromptRequestFactory.cpp
    core/src/SuggestionTextNormalizer.cpp
    core/src/TypoSuppression.cpp
    core/src/InferenceStub.cpp
)

target_include_directories(trout-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json
)

# System SQLite (ships with macOS).
find_library(SQLITE3_LIB sqlite3)
if(NOT SQLITE3_LIB)
    set(SQLITE3_LIB sqlite3)
endif()
target_link_libraries(trout-core PUBLIC ${SQLITE3_LIB})

if(TROUT_WITH_LLAMA)
    target_sources(trout-core PRIVATE core/src/LlamaBackend.cpp)
    target_compile_definitions(trout-core PUBLIC TROUT_WITH_LLAMA=1)
    target_link_libraries(trout-core PUBLIC llama)
endif()

# ---------------------------------------------------------------------------
# macOS application + input method
# ---------------------------------------------------------------------------
if(APPLE AND TROUT_BUILD_APP)
    add_subdirectory(platform/macos)
    add_subdirectory(inputmethod)
endif()

option(TROUT_BUILD_TESTS "Build trout unit tests" ON)
if(TROUT_BUILD_TESTS)
    enable_testing()
    add_executable(prompt-request-factory-tests tests/PromptRequestFactoryTests.cpp)
    target_link_libraries(prompt-request-factory-tests PRIVATE trout-core)
    add_test(NAME prompt-request-factory-tests COMMAND prompt-request-factory-tests)

    add_executable(suggestion-text-normalizer-tests tests/SuggestionTextNormalizerTests.cpp)
    target_link_libraries(suggestion-text-normalizer-tests PRIVATE trout-core)
    add_test(NAME suggestion-text-normalizer-tests COMMAND suggestion-text-normalizer-tests)

    add_executable(typo-suppression-tests tests/TypoSuppressionTests.cpp)
    target_link_libraries(typo-suppression-tests PRIVATE trout-core)
    add_test(NAME typo-suppression-tests COMMAND typo-suppression-tests)
endif()
