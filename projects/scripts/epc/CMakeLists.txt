cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/*.sh)
install(FILES ${SCRIPTS} DESTINATION ${C15_INSTALL_PATH}/scripts
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_WRITE GROUP_EXECUTE
                WORLD_READ WORLD_WRITE WORLD_EXECUTE)

