if(PADOS_DOCS_ONLY)
    find_package(Doxygen REQUIRED OPTIONAL_COMPONENTS dot)
else()
    find_package(Doxygen OPTIONAL_COMPONENTS dot)
endif()

if(NOT DOXYGEN_FOUND)
    message(STATUS "Doxygen not found — 'docs' target unavailable.")
    return()
endif()

set(DOXYGEN_PROJECT_NAME        "PadOS")
set(DOXYGEN_PROJECT_VERSION     ${PROJECT_VERSION})
set(DOXYGEN_OUTPUT_DIRECTORY    ${CMAKE_CURRENT_BINARY_DIR}/docs)
set(DOXYGEN_RECURSIVE           YES)
set(DOXYGEN_EXTRACT_ALL         YES)
set(DOXYGEN_GENERATE_HTML       YES)
set(DOXYGEN_GENERATE_LATEX      NO)
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(DOXYGEN_STRIP_FROM_PATH     ${CMAKE_CURRENT_SOURCE_DIR})
set(DOXYGEN_EXCLUDE_PATTERNS    "*/ASF/*" "*/trash/*" "*/SDCard/*" "*ATSAM*" "*SAME*")
set(DOXYGEN_QUIET               YES)
set(DOXYGEN_WARN_FORMAT         "$file($line): $text")
set(DOXYGEN_ENABLE_PREPROCESSING YES)
set(DOXYGEN_MACRO_EXPANSION      YES)
set(DOXYGEN_EXPAND_ONLY_PREDEF   YES)
set(DOXYGEN_PREDEFINED           "__attribute__(x)=" "__WCHAR_MIN__=0" "__WCHAR_MAX__=0xffffffff")

if(DOXYGEN_DOT_FOUND)
    set(DOXYGEN_HAVE_DOT            YES)
    set(DOXYGEN_CLASS_GRAPH         YES)
    set(DOXYGEN_COLLABORATION_GRAPH YES)
    set(DOXYGEN_INCLUDE_GRAPH       YES)
    set(DOXYGEN_INCLUDED_BY_GRAPH   YES)
endif()

set(PADOS_DOCS_TOOLCHAIN_INPUTS "")
if(NOT PADOS_DOCS_ONLY AND IS_DIRECTORY "$ENV{PADOS_TOOLCHAIN_PATH}/arm-unknown-pados-eabi/include")
    set(DOXYGEN_INCLUDE_PATH "$ENV{PADOS_TOOLCHAIN_PATH}/arm-unknown-pados-eabi/include")
    file(GLOB PADOS_TOOLKIT_SYS_HEADERS "${DOXYGEN_INCLUDE_PATH}/sys/pados_*.h")
    list(APPEND PADOS_DOCS_TOOLCHAIN_INPUTS ${PADOS_TOOLKIT_SYS_HEADERS})
    if(IS_DIRECTORY "${DOXYGEN_INCLUDE_PATH}/PadOS")
        list(APPEND PADOS_DOCS_TOOLCHAIN_INPUTS "${DOXYGEN_INCLUDE_PATH}/PadOS")
    endif()
endif()

doxygen_add_docs(docs
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Kernel
    ${CMAKE_CURRENT_SOURCE_DIR}/System
    ${CMAKE_CURRENT_SOURCE_DIR}/Applications
    ${CMAKE_CURRENT_SOURCE_DIR}/ApplicationServer
    ${CMAKE_CURRENT_SOURCE_DIR}/SerialConsole
    ${CMAKE_CURRENT_SOURCE_DIR}/DataTranslation
    ${CMAKE_CURRENT_SOURCE_DIR}/DataTranslators
    ${PADOS_DOCS_TOOLCHAIN_INPUTS}
    COMMENT "Generating PadOS API documentation with Doxygen"
)

add_custom_command(TARGET docs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Documentation generated: ${CMAKE_CURRENT_BINARY_DIR}/docs/html/index.html"
    VERBATIM
)
