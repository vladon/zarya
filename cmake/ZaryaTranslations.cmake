get_filename_component(_ZARYA_QT_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)

set(_ZARYA_LINGUIST_HINT_DIRS "${_ZARYA_QT_BIN_DIR}")
if(DEFINED ENV{QT_HOST_BIN})
    list(APPEND _ZARYA_LINGUIST_HINT_DIRS "$ENV{QT_HOST_BIN}")
endif()
foreach(_qt_ver IN ITEMS 6.8.3 6.7.3 6.6.3)
    list(APPEND _ZARYA_LINGUIST_HINT_DIRS "C:/Qt/${_qt_ver}/msvc2022_64/bin")
endforeach()

find_program(ZARYA_LUPDATE NAMES lupdate lupdate-qt6 HINTS ${_ZARYA_LINGUIST_HINT_DIRS})
find_program(ZARYA_LRELEASE NAMES lrelease lrelease-qt6 HINTS ${_ZARYA_LINGUIST_HINT_DIRS})

if(NOT ZARYA_LUPDATE OR NOT ZARYA_LRELEASE)
    message(WARNING "Qt lupdate/lrelease not found; using prebuilt .qm from translations/ if present")
    set(ZARYA_USE_PREBUILT_QM TRUE)
else()
    set(ZARYA_USE_PREBUILT_QM FALSE)
endif()

set(ZARYA_TS_FILES
    ${CMAKE_SOURCE_DIR}/translations/zarya_en.ts
    ${CMAKE_SOURCE_DIR}/translations/zarya_ru.ts
)

set(ZARYA_QM_OUTPUT_DIR ${CMAKE_BINARY_DIR}/translations)
file(MAKE_DIRECTORY ${ZARYA_QM_OUTPUT_DIR})

set(ZARYA_QM_FILES
    ${ZARYA_QM_OUTPUT_DIR}/zarya_en.qm
    ${ZARYA_QM_OUTPUT_DIR}/zarya_ru.qm
)

file(GLOB_RECURSE ZARYA_LUPDATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

if(NOT ZARYA_USE_PREBUILT_QM)
    add_custom_target(zarya_lupdate
        COMMAND ${ZARYA_LUPDATE} -locations none -no-obsolete
            ${ZARYA_LUPDATE_SOURCES}
            -ts ${ZARYA_TS_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Update Zarya translation sources"
        VERBATIM
    )

    foreach(_ts_file ${ZARYA_TS_FILES})
        get_filename_component(_ts_name ${_ts_file} NAME_WE)
        set(_qm_file ${ZARYA_QM_OUTPUT_DIR}/${_ts_name}.qm)
        add_custom_command(
            OUTPUT ${_qm_file}
            COMMAND ${ZARYA_LRELEASE} ${_ts_file} -qm ${_qm_file}
            DEPENDS ${_ts_file}
            COMMENT "lrelease ${_ts_name}"
            VERBATIM
        )
    endforeach()

    add_custom_target(zarya_lrelease DEPENDS ${ZARYA_QM_FILES})
    add_dependencies(zarya zarya_lrelease)
else()
    foreach(_ts_file ${ZARYA_TS_FILES})
        get_filename_component(_ts_name ${_ts_file} NAME_WE)
        set(_prebuilt_qm ${CMAKE_SOURCE_DIR}/translations/${_ts_name}.qm)
        set(_qm_file ${ZARYA_QM_OUTPUT_DIR}/${_ts_name}.qm)
        if(EXISTS ${_prebuilt_qm})
            add_custom_command(
                OUTPUT ${_qm_file}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_prebuilt_qm} ${_qm_file}
                DEPENDS ${_prebuilt_qm}
                COMMENT "Copy prebuilt ${_ts_name}.qm"
                VERBATIM
            )
        endif()
    endforeach()
    if(ZARYA_QM_FILES)
        add_custom_target(zarya_lrelease DEPENDS ${ZARYA_QM_FILES})
        add_dependencies(zarya zarya_lrelease)
    endif()
endif()

add_custom_command(TARGET zarya POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:zarya>/translations
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ZARYA_QM_FILES}
        $<TARGET_FILE_DIR:zarya>/translations/
    COMMENT "Copy translation catalogs to runtime translations/"
)

install(FILES ${ZARYA_QM_FILES} DESTINATION share/zarya/translations OPTIONAL)
