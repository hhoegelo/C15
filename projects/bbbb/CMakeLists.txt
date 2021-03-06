cmake_minimum_required(VERSION 3.0)
project(bbbb)

include ( GNUInstallDirs )

set(LIBS giomm-2.4 libsoup-2.4)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

IF(NOT DEV_PC)
    file(GLOB_RECURSE UI_SOURCES src/ui/*.cpp)
    list(REMOVE_ITEM SOURCE_FILES EXCLUDE REGEX ${UI_SOURCES})
ENDIF(NOT DEV_PC)

IF(CONTROLS)
    ADD_DEFINITIONS(-D_WINDOW_CONTROLS)
ENDIF(CONTROLS)

IF(DEV_PC)
  ADD_DEFINITIONS(-D_DEVELOPMENT_PC -D_TESTS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wmaybe-uninitialized -Wuninitialized -Wdouble-promotion -Wreturn-type")
  set(LIBS ${LIBS} gtkmm-3.0 gdkmm-3.0 libpng)
ENDIF(DEV_PC)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wdouble-promotion")

foreach(lib ${LIBS})
  pkg_check_modules(${lib} REQUIRED ${lib})
  target_include_directories(${PROJECT_NAME} PUBLIC ${${lib}_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PUBLIC ${${lib}_LIBRARIES})
  link_directories(${${lib}_LIBRARY_DIRS})
endforeach(lib)

target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
target_link_libraries(${PROJECT_NAME} PUBLIC nltools)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../nltools/include/)

configure_file(${PROJECT_SOURCE_DIR}/systemd/${PROJECT_NAME}.service.in ${PROJECT_BINARY_DIR}/systemd/${PROJECT_NAME}.service @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/systemd/install-update-from-usb.service.in ${PROJECT_BINARY_DIR}/systemd/install-update-from-usb.service @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/systemd/splash-screen.service.in ${PROJECT_BINARY_DIR}/systemd/splash-screen.service @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/systemd/handle-mmc-device-appeared-twice.service.in ${PROJECT_BINARY_DIR}/systemd/handle-mmc-device-appeared-twice.service @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/systemd/gettimefromepc.service.in ${PROJECT_BINARY_DIR}/systemd/gettimefromepc.service @ONLY)

install(FILES ${PROJECT_BINARY_DIR}/systemd/${PROJECT_NAME}.service DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR} COMPONENT init)
install(FILES ${PROJECT_BINARY_DIR}/systemd/install-update-from-usb.service DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR} COMPONENT init)
install(FILES ${PROJECT_BINARY_DIR}/systemd/splash-screen.service DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR} COMPONENT init)
install(FILES ${PROJECT_BINARY_DIR}/systemd/handle-mmc-device-appeared-twice.service DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR} COMPONENT init)
install(FILES ${PROJECT_BINARY_DIR}/systemd/gettimefromepc.service DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR} COMPONENT init)

install(DIRECTORY DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR}/multi-user.target.wants)
install(DIRECTORY DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR}/sysinit.target.wants)

install(CODE "execute_process(
    COMMAND bash -c \"set -e
    mkdir -p $DESTDIR/${SYSTEMD_SERVICES_INSTALL_DIR}/multi-user.target.wants
    mkdir -p $DESTDIR/${SYSTEMD_SERVICES_INSTALL_DIR}/sysinit.target.wants
    ln -sf ../${PROJECT_NAME}.service $DESTDIR/${SYSTEMD_SERVICES_INSTALL_DIR}/multi-user.target.wants/${PROJECT_NAME}.service
    ln -sf ../install-update-from-usb.service $DESTDIR/${SYSTEMD_SERVICES_INSTALL_DIR}/multi-user.target.wants/install-update-from-usb.service
    ln -sf ../splash-screen.service $DESTDIR/${SYSTEMD_SERVICES_INSTALL_DIR}/sysinit.target.wants/splash-screen.service
    ln -sf ../handle-mmc-device-appeared-twice.service $DESTDIR/${SYSTEMD_SERVICES_INSTALL_DIR}/sysinit.target.wants/handle-mmc-device-appeared-twice.service
    ln -sf ../gettimefromepc.service $DESTDIR/${SYSTEMD_SERVICES_INSTALL_DIR}/sysinit.target.wants/gettimefromepc.service
    \")")

install(TARGETS ${PROJECT_NAME} DESTINATION ${C15_INSTALL_PATH}/${PROJECT_NAME})

