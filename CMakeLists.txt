cmake_minimum_required(VERSION 3.16)
project(TankAI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(raylib REQUIRED)

# Source files in this repo
set(SRC
  Controllers/PlayerOneController.cpp
  Controllers/PlayerTwoController.cpp
  Controllers/PlayerThreeController.cpp
  Controllers/PlayerFourController.cpp
  CoreBullet/Bullet.cpp
  CoreTank/Tank.cpp
  Obstacles/Structures.cpp
  TankGame/MainGame.cpp
  compat/RaylibPlayMain.cpp
  Services/Pathfinding/Environment/Environment.cpp
  Services/Pathfinding/AStar/AStar.cpp
  Services/Pathfinding/Graph/GraphBuilder.cpp
  Services/Pathfinding/Primitives/MotionPrimitives.cpp
  Services/Pathfinding/PathfinderService.cpp
  AI/Gateway/AIServiceGateway.cpp
  Services/Motion/MotionService.cpp
  Services/Motion/Path/PathFollower.cpp
  AI/Controllers/DebugAIController.cpp
  Services/Sensing/Vision/LOS.cpp
  Services/Sensing/Vision/FOV.cpp
  Services/Sensing/Audio/Bus.cpp
  Services/Sensing/Memory/Store.cpp
  Services/Sensing/SensingService.cpp
  Helper/LineOfSight.cpp
  Services/Combat/CombatService.cpp
  AI/AISubsystem.cpp
  AI/Controllers/AIDecisionController.cpp
  AI/Controllers/FSM/FSMController.cpp
  AI/Controllers/FSM/States/IdleState.cpp
  AI/Controllers/FSM/States/PatrolState.cpp
  AI/Controllers/FSM/States/LookAroundState.cpp
  AI/Controllers/FSM/States/EngageState.cpp
  AI/Controllers/FSM/States/SearchState.cpp
  AI/Controllers/FSM/States/FleeState.cpp
  AI/Controllers/BT/BehaviorTreeController.cpp
  AI/Controllers/BT/Nodes/BTNode.cpp
)

add_executable(${PROJECT_NAME}
  ${SRC}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:DEBUG>:AI_DEBUG>
)

target_sources(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:DEBUG>:AI/Debug/AIDebugOverlay.cpp>
        $<$<CONFIG:DEBUG>:AI/Debug/Pathfinding/PathfindingDebugLayer.cpp>
        $<$<CONFIG:DEBUG>:AI/Debug/Motion/MotionDebugLayer.cpp>
        $<$<CONFIG:DEBUG>:AI/Debug/Sensing/SensingDebugLayer.cpp>
        $<$<CONFIG:DEBUG>:AI/Debug/FSM/FSMDebugLayer.cpp>
        $<$<CONFIG:DEBUG>:AI/Debug/BT/BTDebugLayer.cpp>
)

# Include directories for headers used across the project
# compat first so it shadows the original Windows-specific Play.h
target_include_directories(${PROJECT_NAME} PRIVATE
  compat
  ${CMAKE_CURRENT_SOURCE_DIR}
  Services
  Controllers
  CoreBullet
  CoreTank
  Obstacles
  TankGame
  Debug
  AI
)

# Link raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# Copy game data next to executable after build
set(RUNTIME_DATA_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Data)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${RUNTIME_DATA_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/TankGame/Data
          ${RUNTIME_DATA_DIR}
  COMMENT "Copying game Data/ assets next to executable"
)
