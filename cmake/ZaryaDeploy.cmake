install(TARGETS zarya
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin)

install(FILES README.md LICENSE THIRD_PARTY_NOTICES.md RELEASE_NOTES.md DESTINATION share/zarya OPTIONAL)

install(DIRECTORY DESTINATION share/zarya/cores/xray)
install(FILES packaging/windows/cores-xray-README.txt
        DESTINATION share/zarya/cores/xray
        RENAME README.txt
        OPTIONAL)

if(UNIX AND NOT APPLE)
    configure_file(
        ${CMAKE_SOURCE_DIR}/packaging/linux/zarya.desktop.in
        ${CMAKE_BINARY_DIR}/zarya.desktop
        @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/zarya.desktop DESTINATION share/applications)
endif()
