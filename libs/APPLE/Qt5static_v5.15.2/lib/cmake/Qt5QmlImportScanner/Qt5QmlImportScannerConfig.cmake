include(CMakeParseArguments)

function(QT5_IMPORT_QML_PLUGINS target)
    set(options)
    set(oneValueArgs "PATH_TO_SCAN")
    set(multiValueArgs)

    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT arg_PATH_TO_SCAN)
        set(arg_PATH_TO_SCAN "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # Find location of qmlimportscanner.
    find_package(Qt5 COMPONENTS Core)
    set(tool_path
        "${_qt5Core_install_prefix}/bin/qmlimportscanner")
    if(NOT EXISTS "${tool_path}" )
        message(FATAL_ERROR "The package \"Qt5QmlImportScannerConfig\" references the file
   \"${tool_path}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty.
")
    endif()

    # Find location of qml dir.
    set(qml_path "${_qt5Core_install_prefix}/qml/")

    # Small macro to avoid duplicating code in two different loops.
    macro(_qt5_QmlImportScanner_parse_entry)
        set(entry_name "qml_import_scanner_import_${idx}")
        cmake_parse_arguments("entry"
                              ""
                              "CLASSNAME;NAME;PATH;PLUGIN;RELATIVEPATH;TYPE;VERSION;" ""
                              ${${entry_name}})
    endmacro()

    # Macro used to populate the dependency link flags for a certain configuriation (debug vs
    # release) of a plugin.
    macro(_qt5_link_to_QmlImportScanner_library_dependencies Plugin Configuration PluginLocation
                                                             IsDebugAndRelease)

        set_property(TARGET "${Plugin}" APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})
        set(_imported_location "${PluginLocation}")
        _qt5_Core_check_file_exists("${_imported_location}")
        set_target_properties("${Plugin}" PROPERTIES
            "IMPORTED_LOCATION_${Configuration}" "${_imported_location}"
        )

        set(_static_deps
            ${_Qt5${entry_PLUGIN}_STATIC_${Configuration}_LIB_DEPENDENCIES}
        )

        if(NOT ${IsDebugAndRelease})
            set(_genex_condition "1")
        else()
            if(${Configuration} STREQUAL DEBUG)
                set(_genex_condition "$<CONFIG:Debug>")
            else()
                set(_genex_condition "$<NOT:$<CONFIG:Debug>>")
            endif()
        endif()
        if(_static_deps)
            set(_static_deps_genex "$<${_genex_condition}:${_static_deps}>")
            target_link_libraries(${imported_target} INTERFACE "${_static_deps_genex}")
        endif()

        set(_static_link_flags "${_Qt5${entry_PLUGIN}_STATIC_${Configuration}_LINK_FLAGS}")
        if(NOT CMAKE_VERSION VERSION_LESS "3.13" AND _static_link_flags)
            set(_static_link_flags_genex "$<${_genex_condition}:${_static_link_flags}>")
            target_link_options(${imported_target} INTERFACE "${_static_link_flags_genex}")
        endif()
    endmacro()

    # Run qmlimportscanner and include the generated cmake file.
    set(qml_imports_file_path
        "${CMAKE_CURRENT_BINARY_DIR}/Qt5_QmlPlugins_Imports_${target}.cmake")

    message(STATUS "Running qmlimportscanner to find used QML plugins. ")
    execute_process(COMMAND
                    "${tool_path}" "${arg_PATH_TO_SCAN}" -importPath "${qml_path}"
                    -cmake-output
                    OUTPUT_FILE "${qml_imports_file_path}")

    include("${qml_imports_file_path}" OPTIONAL RESULT_VARIABLE qml_imports_file_path_found)
    if(NOT qml_imports_file_path_found)
        message(FATAL_ERROR "Could not find ${qml_imports_file_path} which was supposed to be generated by qmlimportscanner.")
    endif()

    # Parse the generate cmake file.
    # It is possible for the scanner to find no usage of QML, in which case the import count is 0.
    if(qml_import_scanner_imports_count)
        set(added_plugins "")
        foreach(idx RANGE "${qml_import_scanner_imports_count}")
            _qt5_QmlImportScanner_parse_entry()
            if(entry_PATH AND entry_PLUGIN)
                # Sometimes a plugin appears multiple times with different versions.
                # Make sure to process it only once.
                list(FIND added_plugins "${entry_PLUGIN}" _index)
                if(NOT _index EQUAL -1)
                    continue()
                endif()
                list(APPEND added_plugins "${entry_PLUGIN}")

                # Add an imported target that will contain the link libraries and link options read
                # from one plugin prl file. This target will point to the actual plugin and contain
                # static dependency libraries and link flags.
                # By creating a target for each qml plugin, CMake will take care of link flag
                # deduplication.
                set(imported_target "${target}_QmlImport_${entry_PLUGIN}")
                add_library("${imported_target}" MODULE IMPORTED)
                target_link_libraries("${target}" PRIVATE "${imported_target}")

                # Read static library dependencies from the plugin .prl file.
                # And then set the link flags to the library dependencies extracted from the .prl
                # file.
                _qt5_Core_process_prl_file(
                    "${entry_PATH}/lib${entry_PLUGIN}.prl" RELEASE
                    _Qt5${entry_PLUGIN}_STATIC_RELEASE_LIB_DEPENDENCIES
                    _Qt5${entry_PLUGIN}_STATIC_RELEASE_LINK_FLAGS
                )
                _qt5_link_to_QmlImportScanner_library_dependencies(
                    "${imported_target}"
                    RELEASE
                    "${entry_PATH}/lib${entry_PLUGIN}.a"
                     FALSE)

            endif()
        endforeach()

        # Generate content for plugin initialization cpp file.
        set(added_imports "")
        set(qt5_qml_import_cpp_file_content "")
        foreach(idx RANGE "${qml_import_scanner_imports_count}")
            _qt5_QmlImportScanner_parse_entry()
            if(entry_PLUGIN)
                if(entry_CLASSNAME)
                    list(FIND added_imports "${entry_PLUGIN}" _index)
                    if(_index EQUAL -1)
                        string(APPEND qt5_qml_import_cpp_file_content
                               "Q_IMPORT_PLUGIN(${entry_CLASSNAME})\n")
                        list(APPEND added_imports "${entry_PLUGIN}")
                    endif()
                else()
                    message(FATAL_ERROR
                            "Plugin ${entry_PLUGIN} is missing a classname entry, please add one to the qmldir file.")
                endif()
            endif()
        endforeach()

        # Write to the generated file, and include it as a source for the given target.
        set(generated_import_cpp_path
            "${CMAKE_CURRENT_BINARY_DIR}/Qt5_QmlPlugins_Imports_${target}.cpp")
        configure_file("${Qt5QmlImportScanner_DIR}/Qt5QmlImportScannerTemplate.cpp.in"
                       "${generated_import_cpp_path}"
                       @ONLY)
        target_sources(${target} PRIVATE "${generated_import_cpp_path}")
    endif()
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_import_qml_plugins)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 5)
            qt5_import_qml_plugins(${ARGV})
        elseif(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_import_qml_plugins(${ARGV})
        endif()
    endfunction()
endif()
