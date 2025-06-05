Here's a GitHub Markdown document summarizing the JFramework C++ framework and its unit tests:

# JFramework - A C++ Application Framework

## Overview
JFramework is a C++ framework that provides a structured architecture for building applications with components like Models, Systems, Commands, Queries, and Events. It follows an Inversion of Control (IoC) pattern and supports event-driven programming.

## Key Features

### Core Components
- **IOC Container**: Manages component lifecycle and dependencies
- **Event Bus**: Handles event publishing/subscribing
- **Command Pattern**: For executing operations
- **Query Pattern**: For data retrieval
- **Bindable Properties**: Observable properties with change notifications

### Component Types
- **Models**: Data containers
- **Systems**: Business logic processors
- **Controllers**: Mediators between models and views
- **Utilities**: Shared services

## Architecture

### Core Interfaces
```cpp
class IArchitecture {
    // Component registration
    virtual void RegisterSystem/Model/Utility(...);
    
    // Component access
    virtual std::shared_ptr<ISystem/IModel/IUtility> Get(...);
    
    // Command/Event/Query operations
    virtual void SendCommand/SendEvent(...);
};

class ICanInit {};       // Initialization interface
class IBelongToArchitecture {}; // Architecture ownership
class ICanSetArchitecture {};   // Architecture assignment

