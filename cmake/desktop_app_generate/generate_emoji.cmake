# Zarya wrapper around desktop-app lib_ui emoji generation.
# Prefixes codegen with a PATH launcher so shared Qt / OpenSSL DLLs resolve on Windows CI.

function(generate_emoji target_name emoji_map suggestions_json)
    set(gen_dst ${CMAKE_CURRENT_BINARY_DIR}/gen)
    file(MAKE_DIRECTORY ${gen_dst})

    set(gen_timestamp ${gen_dst}/emoji.timestamp)
    set(gen_files
        ${gen_dst}/emoji.cpp
        ${gen_dst}/emoji.h
        ${gen_dst}/emoji_suggestions_data.cpp
        ${gen_dst}/emoji_suggestions_data.h
    )

    set(gen_src
        ${CMAKE_CURRENT_SOURCE_DIR}/${emoji_map}
        ${CMAKE_CURRENT_SOURCE_DIR}/${suggestions_json}
    )

    set(_zarya_codegen_cmd)
    if(ZARYA_CODEGEN_LAUNCH AND EXISTS "${ZARYA_CODEGEN_LAUNCH}")
        list(APPEND _zarya_codegen_cmd "${ZARYA_CODEGEN_LAUNCH}")
    endif()
    list(APPEND _zarya_codegen_cmd "$<TARGET_FILE:codegen_emoji>")

    add_custom_command(
    OUTPUT
        ${gen_timestamp}
    BYPRODUCTS
        ${gen_files}
    COMMAND
        ${_zarya_codegen_cmd}
        -o${gen_dst}
        ${gen_src}
    COMMENT "Generating emoji (${target_name})"
    DEPENDS
        codegen_emoji
        ${gen_src}
    )
    generate_target(${target_name} emoji ${gen_timestamp} "${gen_files}" ${gen_dst})
endfunction()
