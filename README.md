# Tank AI Project

This project is a C++ demonstration of game AI techniques in a simple 1v1 tank game. It features two distinct AI controllers—a classic Finite State Machine (FSM) and a flexible Behavior Tree (BT)—built on top of a modular, service-based architecture. The focus is on creating believable, debuggable, and distinct AI behaviors.

The original game framework (rendering, core tank physics) was provided, with the AI system and its supporting services being the core focus of this implementation.

## Features

- **Dual AI Architectures:**
  - **Finite State Machine (FSM):** A classic, easy-to-understand AI with explicit states and transitions (e.g., `Patrol`, `Engage`, `Flee`).
  - **Behavior Tree (BT):** A more modern, composable AI where complex behaviors emerge from a tree of simple nodes (`Selector`, `Sequence`, `Guard`, and custom actions).
- **Modular Service Layer:** Core functionalities like pathfinding, movement, sensing, and combat are encapsulated in decoupled services.
- **Sense-Think-Act Cycle:** A clean, logical update loop that ensures AI decisions are based on the most recent world state.
- **Comprehensive Debug Overlays:** A rich, real-time visualization suite to inspect pathfinding graphs, motion goals, sensor states, and the internal logic of both the FSM and BT controllers.

## Building and Running

The project uses CMake for building.

1.  **Prerequisites:** Ensure you have a C++ compiler (supporting C++17), CMake, and the necessary libraries for Raylib.
2.  **Configure:** Create a build directory and run CMake:
    ```bash
    mkdir build
    cd build
    cmake ..
    ```
3.  **Build:** Compile the project.
    ```bash
    cmake --build .
    ```
4.  **Run:** Execute the compiled binary from the `bin` directory inside your build folder.

## Architecture Overview

The AI system is designed around a clean separation of concerns, following a "Sense-Think-Act" pattern that executes every frame.

1.  **`AISubsystem`**: The central orchestrator. It owns all AI agents and services, and drives the main update tick.
2.  **`AIServiceGateway`**: A façade that provides a single, clean interface for AI controllers to interact with the world. It exposes queries (e.g., `Sense_VisibleEnemies()`) and intents (e.g., `MoveTo()`, `BeginFire()`) but hides the underlying service implementations.
3.  **Services**: A layer of decoupled modules that handle core tasks.
4.  **AI Controllers**: The "brains" of the AI that make decisions.

The per-frame data flow is as follows:
1.  **Sense:** The `SensingService` is ticked via the gateway to gather information about the world (visible enemies, sounds). This generates events like `onSpotted` or `onSoundHeard`.
2.  **Think:** The active AI controller (`FSMController` or `BehaviorTreeController`) is updated. It processes the events and queries the gateway to make decisions.
3.  **Act:** The `MotionService` and `CombatService` are ticked, executing the decisions made by the controller (e.g., following a path, aiming the turret, firing).

## AI Controllers

The project includes two distinct AI implementations that use the exact same `AIServiceGateway` API, demonstrating how different decision-making paradigms can be built on the same architectural foundation.

### Finite State Machine (FSM)

The FSM is a traditional, state-driven model. Its behavior is defined by a finite set of states and the explicit transitions between them.

-   **States:** `Idle`, `Patrol`, `LookAround`, `Engage`, `Search`, `Flee`.
-   **Logic:** Behavior is encapsulated within state classes (e.g., `EngageState`). Transitions are triggered by events (`onSpotted` moves to `Engage`) or internal logic (`EngageState` transitions to `Search` if the target is lost).
-   **Behavioral Profile:** The FSM is highly predictable and deterministic. It follows a rigid set of rules, resulting in a straightforward "see enemy, attack enemy, lose enemy, search for enemy" pattern.

### Behavior Tree (BT)

The BT is a more flexible, hierarchical model where complex behaviors emerge from the composition of simple nodes. Control flow is determined by the tree's structure, not by explicit state transitions.

-   **Structure:** The tree is composed of `Selector` (pick one), `Sequence` (do all), `Guard` (conditional), and `Action` nodes.
-   **Logic:** Priorities are encoded structurally. For example, the top-level `Selector` checks for low-HP defensive situations *before* attempting to engage, allowing for more nuanced tactics.
-   **Behavioral Profile:** The BT exhibits more sophisticated behavior. It has a distinct low-health mode (`GuardLowHP`) where it prioritizes fleeing and cautious movement. Its high-health behavior is also less rigid, using a `RandomSelector` to alternate between patrolling and looking around.

## Debugging

The project includes a powerful set of debug overlays, which are essential for understanding and verifying AI behavior. These are only compiled in debug builds and have no performance cost in release.

-   **`Enter`**: Toggle the AI controllers on or off globally.

The following keys toggle individual debug layers:
-   **`F1` - Pathfinding:** Visualizes the navigation graph, obstacles, and the path currently being planned.
-   **`F2` - Motion:** Shows the agent's current movement goal, lookahead vector, and arrival radius.
-   **`F3` - Sensing:** Displays the agent's Field of View (FOV), Line of Sight (LOS) checks, and memory of last-known enemy positions.
-   **`F4` - FSM:** Displays the current FSM state for each AI agent.
-   **`F5` - Behavior Tree:** Shows the currently active path in the tree, highlighting the leaf node being executed.

## Core Services

### Pathfinding Service
**Purpose:** Fast, modular path queries for tanks. Converts arbitrary world points into a safe polyline path over a centerline graph.
- **Key Components:** `GraphBuilder` (builds a nav graph), `A* Solver` (stateless search), and dynamic attachment for start/goal points.

### Motion Service
**Purpose:** To consume a path from the pathfinding service and steer a tank along it. It handles low-level vehicle dynamics, providing smooth movement and generating events for arrival or blocking.
- **Key Components:** A `PathFollower` using a pure-pursuit-style algorithm and stuck detection.

### Sensing Service
**Purpose:** Give the AI a believable picture of the world (vision & hearing) and a short-term memory.
- **Key Components:** Vision (`FOV` + `LOS` checks), a global `Audio::Bus` for event-based sound propagation, and a `Memory::Store` for decaying last-known positions.

### Combat Service
**Purpose:** Manages the weapon charging and firing lifecycle for a tank.
- **Key Components:** Tracks charge state and triggers a shot on release. Notifies the `SensingService` by emitting a sound event on firing.