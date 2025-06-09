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

## Usage Examples

### 1. Define Components
```cpp

// 1. 定义一个事件
class MyEvent : public IEvent
{
public:
	std::string msg;
	MyEvent(const std::string& m) : msg(m) {}
	std::string GetEventType() const override { return "MyEvent"; }
};

// 2. 定义一个 Model
class CounterModel : public AbstractModel
{
public:
	int value = 0;
protected:
	void OnInit() override { value = 0; }
	void OnDeinit() override {}
};

// 3. 定义一个 Utility
class LoggerUtility : public IUtility
{
public:
	void Log(const std::string& msg)
	{
		std::cout << "[Logger] " << msg << std::endl;
	}
};

// 4. 定义一个 Model，使用 Utility
class MyModel : public AbstractModel
{
protected:
	void OnInit() override
	{
		auto logger = GetUtility<LoggerUtility>();
		logger->Log("MyModel 初始化完成");
	}
	void OnDeinit() override {}
};

// 5. 定义一个 System，监听事件
class PrintSystem : public AbstractSystem
{
protected:
	void OnInit() override
	{
		RegisterEvent<MyEvent>(this);
	}
	void OnDeinit() override
	{
		UnRegisterEvent<MyEvent>(this);
	}
	void OnEvent(std::shared_ptr<IEvent> event) override
	{
		auto e = std::dynamic_pointer_cast<MyEvent>(event);
		if (e)
		{
			std::cout << "PrintSystem 收到事件: " << e->msg << std::endl;
		}
	}
};

// 6. 定义一个 Command
class AddCommand : public AbstractCommand
{
	int delta;
public:
	AddCommand(int d) : delta(d) {}
protected:
	void OnExecute() override
	{
		auto model = GetModel<CounterModel>();
		model->value += delta;
		SendEvent<MyEvent>("计数器已增加，当前值: " + std::to_string(model->value));
	}
};

// 7. 定义一个 Model，使用 Query
class TestQueryCounterModel : public AbstractModel
{
public:
	int value = 42;
protected:
	void OnInit() override { value = 42; }
	void OnDeinit() override {}
};

// 8. 定义一个 Command，使用 Utility
class PrintCommand : public AbstractCommand
{
	std::string mMsg;
public:
	PrintCommand(const std::string& msg) : mMsg(msg) {}
protected:
	void OnExecute() override
	{
		auto logger = GetUtility<LoggerUtility>();
		logger->Log("PrintCommand 执行: " + mMsg);
	}
};

// 9. 定义一个 Query，查询 CounterModel 的值
class GetCounterValueQuery : public AbstractQuery<int>
{
protected:
	int OnDo() override
	{
		// 通过基类接口获取 Model
		auto model = GetModel<TestQueryCounterModel>();
		return model->value;
	}
};

```
### 2. Register Components
```cpp
// 10. 定义架构实现
class MyAppArchitecture : public Architecture
{
protected:
	void Init() override
	{
		RegisterUtility(std::make_shared<LoggerUtility>());

		RegisterModel(std::make_shared<MyModel>());
		RegisterModel(std::make_shared<CounterModel>());
		RegisterModel(std::make_shared<TestQueryCounterModel>());

		RegisterSystem(std::make_shared<PrintSystem>());
	}
	void OnDeinit() override {}
};
```
### 3. Use Components
```cpp

int ArchitectureExample()
{
	// 创建架构实例
	auto arch = std::make_shared<MyAppArchitecture>();
	arch->InitArchitecture();

	// 发送命令
	arch->SendCommand<AddCommand>(5);
	arch->SendCommand<AddCommand>(3);

	// 获取 Model
	auto model = arch->GetModel<CounterModel>();
	std::cout << "最终计数值: " << model->value << std::endl;

	// 发送命令，命令内部会用到 Utility
	arch->SendCommand<PrintCommand>("Hello Utility!");

	// 通过架构发送 Query，获取 CounterModel 的值
	int result = arch->SendQuery<GetCounterValueQuery>();
	std::cout << "CounterModel value: " << result << std::endl; // 输出: 42

	arch->Deinit();
	return 0;
}

void BindablePropertyExample()
{
	// 1. 声明一个可绑定属性
	BindableProperty<int> counter(0);

	// 2. 注册观察者，监听属性变化
	auto unreg = counter.Register([](const int& value)
		{
			std::cout << "Counter changed to: " << value << std::endl;
		});

	// 3. 修改属性值，触发通知
	counter.SetValue(1); // 输出: Counter changed to: 1
	counter = 2;         // 输出: Counter changed to: 2

	// 4. 注销观察者（不再收到通知）
	unreg->UnRegister();
	counter.SetValue(3); // 无输出

	// 5. 支持带初始值通知的注册
	auto unreg2 = counter.RegisterWithInitValue([](const int& value)
		{
			std::cout << "Init observer, value: " << value << std::endl;
		});
	// 输出: Init observer, value: 3

	// 6. 赋值后再次触发
	counter = 10; // 输出: Init observer, value: 10

	BindableProperty<int> autoCounter(100);
	// 7. 观察者自动注销（可选，结合UnRegisterTrigger使用）
	{
		UnRegisterTrigger trigger;

		auto autoUnreg = autoCounter.Register([](const int& value)
			{
				std::cout << "Auto observer: " << value << std::endl;
			});
		// 绑定到 trigger，trigger 析构时自动注销
		autoUnreg->UnRegisterWhenObjectDestroyed(&trigger);

		autoCounter = 101; // 输出: Auto observer: 101

		// trigger 离开作用域时，autoUnreg 自动注销
	}
	// 这里 autoCounter 再次赋值不会有输出
}

int main(int argc, char** argv)
{
	BindablePropertyExample();

	ArchitectureExample();

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
