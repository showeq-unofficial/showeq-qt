cmake_minimum_required(VERSION 3.21)
project(showeq-qt CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets WebSockets)
find_package(Protobuf REQUIRED)

# --- protobuf (shared schema with showeq-daemon) ----------------------------
set(SEQ_PROTO_DIR "${CMAKE_SOURCE_DIR}/proto")
set(SEQ_PROTO_FILES
  "${SEQ_PROTO_DIR}/seq/v1/events.proto"
  "${SEQ_PROTO_DIR}/seq/v1/client.proto"
)

if(NOT EXISTS "${SEQ_PROTO_DIR}/seq/v1/events.proto")
  message(FATAL_ERROR
    "showeq-proto submodule is not checked out. Run:\n"
    "  git submodule update --init --recursive")
endif()

set(SEQ_PROTO_GEN_DIR "${CMAKE_BINARY_DIR}/gen-proto")
file(MAKE_DIRECTORY "${SEQ_PROTO_GEN_DIR}")

set(SEQ_PROTO_SRCS)
set(SEQ_PROTO_HDRS)
foreach(proto ${SEQ_PROTO_FILES})
  get_filename_component(proto_name "${proto}" NAME_WE)
  get_filename_component(proto_dir "${proto}" DIRECTORY)
  file(RELATIVE_PATH proto_rel "${SEQ_PROTO_DIR}" "${proto_dir}")
  set(cc "${SEQ_PROTO_GEN_DIR}/${proto_rel}/${proto_name}.pb.cc")
  set(hh "${SEQ_PROTO_GEN_DIR}/${proto_rel}/${proto_name}.pb.h")
  add_custom_command(
    OUTPUT "${cc}" "${hh}"
    COMMAND protobuf::protoc
            --cpp_out ${SEQ_PROTO_GEN_DIR}
            --proto_path ${SEQ_PROTO_DIR}
            ${proto}
    DEPENDS "${proto}" protobuf::protoc
    COMMENT "protoc ${proto_rel}/${proto_name}.proto"
    VERBATIM
  )
  list(APPEND SEQ_PROTO_SRCS "${cc}")
  list(APPEND SEQ_PROTO_HDRS "${hh}")
endforeach()

add_library(seq-proto STATIC ${SEQ_PROTO_SRCS} ${SEQ_PROTO_HDRS})
target_include_directories(seq-proto PUBLIC "${SEQ_PROTO_GEN_DIR}")
target_link_libraries(seq-proto PUBLIC protobuf::libprotobuf)
set_target_properties(seq-proto PROPERTIES AUTOMOC OFF AUTORCC OFF AUTOUIC OFF)

# --- application sources ----------------------------------------------------
set(SOURCES
  src/main.cpp
  src/app/MainWindow.cpp
  src/app/ConnectDialog.cpp
  src/app/Settings.cpp
  src/daemon/DaemonConnection.cpp
  src/daemon/SpawnModel.cpp
  src/daemon/ZoneState.cpp
  src/daemon/PlayerState.cpp
  src/widgets/SpawnListWidget.cpp
  src/widgets/MapWidget.cpp
  src/widgets/CompassWidget.cpp
  src/widgets/PlayerStatsWidget.cpp
  src/widgets/SpellListWidget.cpp
  src/widgets/MessageWindow.cpp
  src/widgets/NetworkDiagWidget.cpp
  resources/showeq-qt.qrc
)

add_executable(showeq-qt ${SOURCES})

target_include_directories(showeq-qt PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${SEQ_PROTO_GEN_DIR}"
)

target_link_libraries(showeq-qt PRIVATE
  seq-proto
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::WebSockets
)

# On Windows set the executable subsystem to Windows (no console window).
if(WIN32)
  set_target_properties(showeq-qt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# --- CPack ------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "showeq-qt")
set(CPACK_PACKAGE_VERSION "0.1.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ShowEQ Qt6 Client")
if(WIN32)
  set(CPACK_GENERATOR "NSIS")
elseif(UNIX)
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "showeq-unofficial")
endif()
include(CPack)
