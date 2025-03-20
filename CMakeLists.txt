cmake_minimum_required (VERSION 3.22)

project(stat-terminal
  LANGUAGES CXX
  VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/arthursonzogni/ftxui.git
  GIT_TAG        v5.0.0
  GIT_PROGRESS   TRUE
  GIT_SHALLOW    TRUE
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(ftxui)
# ------------------------------------------------------------------------------

# --- SQLite -------------------------------------------------------------------
find_package(SQLite3 REQUIRED)

# --- Executable ---------------------------------------------------------------
add_executable(stat-terminal src/main.cpp)
target_include_directories(stat-terminal PRIVATE src)

target_link_libraries(stat-terminal
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component  # Not needed for this example.
  PRIVATE ${SQLite3_LIBRARIES}
)
