<img src="https://github.com/zjlove1989/JFramework/blob/master/LOGO.png" alt="LOGO" width="300" height="300" />

[中文](https://github.com/zjlove1989/JFramework/blob/master/README.md)

# JFramework C++ Framework

## Author：zjlove1989
## Email：2517917168@qq.com
## QQ Group：1048859619


JFramework is a general-purpose C++ application framework that provides core features such as Dependency Injection (DI), Event Bus, and Command Query Responsibility Segregation (CQRS). It is designed for building modular and extensible applications. The framework follows interface-oriented programming principles, supports dynamic component registration and management, and ensures stability through comprehensive unit testing.

## Core Features

### 1、Dependency Injection (IoC) Container
- Supports dynamic registration and retrieval of System, Model, and Utility components.
- Type-safe management based on std::type_index.
- Thread-safe component registration and retrieval for multi-threaded environments.

### 2、Event Bus
- Implements Publish-Subscribe pattern with support for multiple event handlers.
- Automatic event type matching, including inheritance-based event dispatching.
- Thread-safe event publishing and handling for high-concurrency scenarios.

### 3、Command and Query (CQRS)
- Command Pattern: Supports asynchronous command execution with chainable calls.
- Query Pattern: Supports queries with return values and parameter passing.
- Decouples components via Commands/Queries for better maintainability.

### 4、Property Binding
- Automatic notification for property changes (useful for UI data binding).
- Manages observer registration/unregistration to prevent memory leaks.
- Thread-safe property access and change notifications.
  
### 5、Component Lifecycle Management
- Supports component Initialization (Init) and Deinitialization (Deinit).
- Architecture-level lifecycle control to ensure ordered setup/cleanup.
- Supports late registration of components (dynamic addition post-initialization).

## Directory Structure

	JFramework/
	├─ include/                # Header files
	│  └─ JFramework.h         # Core framework definitions
	├─ src/                    # Source files (test-only)
	│  └─ JFramework.cpp       # Test entry point
	└─ test/                   # Unit tests
	   ├─ IOCContainerTest     # IoC Container tests
	   ├─ EventBusTest         # Event Bus tests
	   ├─ ArchitectureTest     # Architecture tests
	   └─ ...                  # Other component tests

## Usage Examples

### 1. Define Components
```cpp
// Model class
class TestModel : public JFramework::AbstractModel {
protected:
    void OnInit() override { /* Initialization logic */ }
    void OnDeinit() override { /* Cleanup logic */ }
};

// System class
class TestSystem : public JFramework::AbstractSystem {
protected:
    void OnInit() override { /* Initialization logic */ }
    void OnEvent(std::shared_ptr<JFramework::IEvent> event) override { /* Event handling */ }
};

// Command class
class TestCommand : public JFramework::AbstractCommand {
protected:
    void OnExecute() override { /* Command logic */ }
};
```
### 2. Register Components
```cpp
class AppArchitecture : public JFramework::Architecture {
protected:
    void Init() override {
        // Register a model
        RegisterModel<TestModel>(std::make_shared<TestModel>());
        // Register a system
        RegisterSystem<TestSystem>(std::make_shared<TestSystem>());
    }
};
```
### 3. Use Components
```cpp
int main() {
    auto arch = std::make_shared<AppArchitecture>();
    arch->InitArchitecture(); // Initialize the architecture

    // Get a model
    auto model = arch->GetModel<TestModel>();

    // Send a command
    arch->SendCommand(std::make_unique<TestCommand>());

    // Publish an event
    arch->SendEvent(std::make_shared<JFramework::TestEvent>());

    return 0;
}
```
## Unit Testing
### The framework includes comprehensive unit tests covering all core features:
- IOC Container Tests: Validate component registration, retrieval, and cleanup.
- Event Bus Tests: Verify publish-subscribe behavior, multi-handler support, and thread safety.
- Architecture Tests: Test component lifecycle, command/query execution, and event handling.
- Performance Tests: Measure throughput and latency under high concurrency.
- Memory Tests: Ensure proper cleanup of components and observers.

# Run Tests (Requires Google Test)
```bash
cd test
g++ -std=c++14 JFramework.cpp -lgtest -lgtest_main -pthread
./a.out
```

## Contribution Guidelines
### Ensure all unit tests pass before submitting code.
### New features must include corresponding headers and implementations.
### Follow C++14 standards and coding conventions.
### PRs must include a description of changes and test coverage details.

## License
MIT License. See LICENSE for details.
Project URL: https://github.com/zjlove1989/JFramework
