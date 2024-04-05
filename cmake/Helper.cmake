
function(handle_assets TargetName SRC_DIR DST_DIR)
    file(GLOB_RECURSE ALL_FILES ${SRC_DIR}/*.*)

    set(COMMANDS "")

    foreach(ASSET ${ALL_FILES})
        file(RELATIVE_PATH ASSET_DIR ${SRC_DIR} ${ASSET})
        get_filename_component(ASSET_DIR ${ASSET_DIR} DIRECTORY)
        get_filename_component(FILE_NAME ${ASSET} NAME)
        set(CURRENT_OUTPUT_PATH ${DST_DIR}/${ASSET_DIR}/${FILE_NAME})
        get_filename_component(CURRENT_OUTPUT_DIR ${CURRENT_OUTPUT_PATH} DIRECTORY)
        file(MAKE_DIRECTORY ${CURRENT_OUTPUT_DIR})

        add_custom_command(
                OUTPUT ${CURRENT_OUTPUT_PATH}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASSET} ${CURRENT_OUTPUT_PATH}
                DEPENDS ${ASSET}
                COMMENT "Copying ${ASSET} to ${CURRENT_OUTPUT_PATH}"
                VERBATIM
        )
        list(APPEND COMMANDS ${CURRENT_OUTPUT_PATH})
    endforeach()

    add_custom_target(
            ${TargetName}
            DEPENDS ${COMMANDS}
    )
endfunction()