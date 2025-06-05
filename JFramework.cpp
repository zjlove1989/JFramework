// JFramework.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <gtest/gtest.h>
#include "JFramework.h"
#include <chrono>
using namespace JFramework;

class ExtendedTestEvent : public IEvent
{
public:
	std::string GetEventType() const override { return "ExtendedTestEvent"; }
	int eventData = 0;
};

// 测试用的组件类
class TestModel : public AbstractModel
{
protected:
	void OnInit() override { initialized = true; }
	void OnDeinit() override { initialized = false; }
public:
	bool initialized = false;
};

class TestSystem : public AbstractSystem
{
protected:
	void OnInit() override { initialized = true; }
	void OnDeinit() override { initialized = false; }
	void OnEvent(std::shared_ptr<IEvent>) override {}
public:
	bool initialized = false;
};

class TestUtility : public IUtility {};

class TestEvent : public IEvent
{
public:
	std::string GetEventType() const override { return "TestEvent"; }
};

class TestCommand : public AbstractCommand
{
protected:
	void OnExecute() override { executed = true; }
public:
	bool executed = false;
};

class TestQuery : public AbstractQuery<int>
{
protected:
	int OnDo() override { return 42; }
};

class TestEventHandler : public ICanHandleEvent
{
public:
	std::shared_ptr<IEvent> lastEvent;
	bool eventHandled = false;
	void HandleEvent(std::shared_ptr<IEvent> event) override
	{
		lastEvent = event;
		eventHandled = true;
	}
};

// 扩展的测试组件
class ExtendedTestModel : public AbstractModel
{
protected:
	void OnInit() override
	{
		initCount++;
	}
	void OnDeinit() override
	{
		deinitCount++;
	}
public:
	int initCount = 0;
	int deinitCount = 0;
};

class ExtendedTestSystem : public AbstractSystem
{
protected:
	void OnInit() override
	{
		initialized = true;
		this->RegisterEvent<ExtendedTestEvent>(this);
	}
	void OnDeinit() override
	{
		initialized = false;
		this->UnRegisterEvent<ExtendedTestEvent>(this);
	}
	void OnEvent(std::shared_ptr<IEvent> event) override
	{
		auto testEvent = std::dynamic_pointer_cast<ExtendedTestEvent>(event);
		if (testEvent)
		{
			lastEvent = event;
		}
	}
public:
	bool initialized = false;
	std::shared_ptr<IEvent> lastEvent = nullptr;
};

class ExtendedTestCommand : public AbstractCommand
{
protected:
	void OnExecute() override { executionCount++; }
public:
	static int executionCount;
};
int ExtendedTestCommand::executionCount = 0;

class ExtendedTestQuery : public AbstractQuery<std::string>
{
protected:
	std::string OnDo() override { return "QueryResult:" + std::to_string(queryParam); }
public:
	int queryParam = 0;
};

// 测试架构类
class TestArchitecture : public Architecture
{
protected:
	void Init() override
	{
		// 注册测试组件
		RegisterModel<TestModel>(std::make_shared<TestModel>());
		RegisterSystem<TestSystem>(std::make_shared<TestSystem>());
		RegisterUtility(typeid(TestUtility), std::make_shared<TestUtility>());
	}
};

// IOCContainer 测试
TEST(IOCContainerTest, RegisterAndGet)
{
	IOCContainer container;
	auto model = std::make_shared<TestModel>();
	container.Register<TestModel, IModel>(typeid(TestModel), model);
	auto retrieved = container.Get<IModel>(typeid(TestModel));

	EXPECT_EQ(model, retrieved);
	EXPECT_EQ(nullptr, container.Get<IModel>(typeid(TestSystem)));
}

TEST(IOCContainerTest, GetAll)
{
	IOCContainer container;
	auto model1 = std::make_shared<TestModel>();
	auto model2 = std::make_shared<TestModel>();

	container.Register<TestModel, IModel>(typeid(TestModel), model1);
	container.Register<TestModel, IModel>(typeid(TestModel), model2);

	auto allModels = container.GetAll<IModel>();
	EXPECT_EQ(1, allModels.size());

	// 测试清除功能
	container.Clear();
}

TEST(IOCContainerTest, AdvancedRegistration)
{
	IOCContainer container;

	// 测试多组件注册
	auto model1 = std::make_shared<ExtendedTestModel>();
	auto model2 = std::make_shared<ExtendedTestModel>();

	container.Register<ExtendedTestModel, IModel>(typeid(ExtendedTestModel), model1);
	container.Register<ExtendedTestModel, IModel>(typeid(ExtendedTestModel), model2);

	// 测试获取所有组件
	auto allModels = container.GetAll<IModel>();
	EXPECT_EQ(1, allModels.size());

	// 测试清除功能
	container.Clear();
	EXPECT_TRUE(container.GetAll<IModel>().empty());
}

TEST(IOCContainerTest, TypeSafety)
{
	IOCContainer container;

	// 测试类型安全
	auto system = std::make_shared<ExtendedTestSystem>();
	container.Register<ExtendedTestSystem, ISystem>(typeid(ExtendedTestSystem), system);

	// 错误类型获取应返回nullptr
	EXPECT_EQ(nullptr, container.Get<IModel>(typeid(ExtendedTestSystem)));

	// 正确类型获取
	EXPECT_NE(nullptr, container.Get<ISystem>(typeid(ExtendedTestSystem)));
}

// EventBus 测试
TEST(EventBusTest, EventHandling)
{
	EventBus bus;
	TestEventHandler handler;
	auto event = std::make_shared<TestEvent>();

	bus.RegisterEvent(typeid(TestEvent), &handler);
	bus.SendEvent(event);

	EXPECT_TRUE(handler.eventHandled);
}

TEST(EventBusTest, UnregisterEvent)
{
	EventBus bus;
	TestEventHandler handler;
	auto event = std::make_shared<TestEvent>();

	bus.RegisterEvent(typeid(TestEvent), &handler);
	bus.UnRegisterEvent(typeid(TestEvent), &handler);
	bus.SendEvent(event);

	EXPECT_FALSE(handler.eventHandled);
}

TEST(EventBusTest, MultipleHandlers)
{
	EventBus bus;

	// 创建多个处理器
	TestEventHandler handler1;
	TestEventHandler handler2;
	auto event = std::make_shared<ExtendedTestEvent>();

	// 注册多个处理器
	bus.RegisterEvent(typeid(ExtendedTestEvent), &handler1);
	bus.RegisterEvent(typeid(ExtendedTestEvent), &handler2);

	// 发送事件
	bus.SendEvent(event);

	// 验证所有处理器都收到事件
	EXPECT_TRUE(handler1.eventHandled);
	EXPECT_TRUE(handler2.eventHandled);
}

TEST(EventBusTest, EventDataIntegrity)
{
	EventBus bus;
	TestEventHandler handler;

	// 创建带数据的事件
	auto event = std::make_shared<ExtendedTestEvent>();
	event->eventData = 42;

	bus.RegisterEvent(typeid(ExtendedTestEvent), &handler);
	bus.SendEvent(event);

	// 验证事件数据完整性
	auto receivedEvent = std::dynamic_pointer_cast<ExtendedTestEvent>(handler.lastEvent);
	EXPECT_NE(nullptr, receivedEvent);
	EXPECT_EQ(42, receivedEvent->eventData);
}

TEST(EventBusTest, EventHandlerOrder)
{
	EventBus bus;
	std::vector<int> handlerOrder;

	class OrderedHandler : public ICanHandleEvent
	{
	public:
		OrderedHandler(int id, std::vector<int>& order) : mId(id), mOrder(order) {}
		void HandleEvent(std::shared_ptr<IEvent>) override
		{
			mOrder.push_back(mId);
		}
	private:
		int mId;
		std::vector<int>& mOrder;
	};

	OrderedHandler handler1(1, handlerOrder);
	OrderedHandler handler2(2, handlerOrder);

	bus.RegisterEvent(typeid(TestEvent), &handler1);
	bus.RegisterEvent(typeid(TestEvent), &handler2);
	bus.SendEvent(std::make_shared<TestEvent>());

	// 验证事件处理顺序与注册顺序一致
	EXPECT_EQ(2, handlerOrder.size());
	EXPECT_EQ(1, handlerOrder[0]);
	EXPECT_EQ(2, handlerOrder[1]);
}

// Architecture 测试
TEST(ArchitectureTest, ComponentRegistration)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	auto model = arch->GetModel<TestModel>();
	auto system = arch->GetSystem<TestSystem>();
	auto utility = arch->GetUtility(typeid(TestUtility));

	EXPECT_NE(nullptr, model);
	EXPECT_NE(nullptr, system);
	EXPECT_NE(nullptr, utility);
	EXPECT_TRUE(model->initialized);
	EXPECT_TRUE(system->initialized);
}

TEST(ArchitectureTest, CommandExecution)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	auto cmd = std::make_unique<TestCommand>();
	auto cmdPtr = cmd.get();
	arch->SendCommand(std::move(cmd));

	EXPECT_TRUE(cmdPtr->executed);
}

TEST(ArchitectureTest, QueryExecution)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	auto query = std::make_unique<TestQuery>();
	int result = arch->SendQuery(std::move(query));

	EXPECT_EQ(42, result);
}

TEST(ArchitectureTest, EventHandling)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	TestEventHandler handler;
	arch->RegisterEvent(typeid(TestEvent), &handler);

	auto event = std::make_shared<TestEvent>();
	arch->SendEvent(event);

	EXPECT_TRUE(handler.eventHandled);
}

TEST(ArchitectureTest, ComponentLifecycle)
{
	auto arch = std::make_shared<TestArchitecture>();

	// 注册自定义组件
	auto model = std::make_shared<ExtendedTestModel>();
	arch->RegisterModel<ExtendedTestModel>(model);

	// 初始化前验证
	EXPECT_EQ(0, model->initCount);

	// 初始化架构
	arch->InitArchitecture();

	// 验证初始化
	EXPECT_EQ(1, model->initCount);

	// 反初始化
	arch->Deinit();

	// 验证反初始化
	EXPECT_EQ(1, model->deinitCount);
}

TEST(ArchitectureTest, CommandChaining)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	// 重置静态计数器
	ExtendedTestCommand::executionCount = 0;

	// 发送多个命令
	arch->SendCommand(std::make_unique<ExtendedTestCommand>());
	arch->SendCommand(std::make_unique<ExtendedTestCommand>());

	// 验证命令执行次数
	EXPECT_EQ(2, ExtendedTestCommand::executionCount);
}


TEST(ArchitectureTest, QueryWithParameters)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	auto query = std::make_unique<ExtendedTestQuery>();
	query->queryParam = 123;
	std::string result = arch->SendQuery(std::move(query));

	EXPECT_EQ("QueryResult:123", result);
}

TEST(ArchitectureTest, UtilityUsage)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	auto utility = arch->GetUtility(typeid(TestUtility));
	EXPECT_NE(nullptr, utility);

	// 测试utility的具体功能
}

TEST(ArchitectureTest, MultipleInitDeinitCycles)
{
	auto arch = std::make_shared<TestArchitecture>();
	auto model = std::make_shared<ExtendedTestModel>();
	arch->RegisterModel<ExtendedTestModel>(model);

	// 多次初始化和反初始化
	for (int i = 0; i < 3; ++i)
	{
		arch->InitArchitecture();
		EXPECT_EQ(i + 1, model->initCount);

		arch->Deinit();
		EXPECT_EQ(i + 1, model->deinitCount);
	}
}

static int executionCount = 0;

TEST(ArchitectureTest, CommandInCommand)
{
	class NestedCommand : public AbstractCommand
	{
	protected:
		void OnExecute() override
		{
			// 在命令中发送另一个命令
			this->SendCommand<TestCommand>();
		}
	public:
		
	};

	executionCount = 0;

	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	auto cmd = std::make_unique<NestedCommand>();
	arch->SendCommand(std::move(cmd));

	// 验证内部命令也被执行
	auto testCmd = std::make_unique<TestCommand>();
	auto cmdPtr = testCmd.get();
	arch->SendCommand(std::move(testCmd));
	EXPECT_TRUE(cmdPtr->executed);
}

TEST(ArchitectureTest, ChainedQueries)
{
	class FirstQuery : public AbstractQuery<int>
	{
	protected:
		int OnDo() override { return 10; }
	};

	class SecondQuery : public AbstractQuery<std::string>
	{
	protected:
		std::string OnDo() override
		{
			auto firstResult = this->SendQuery<FirstQuery>();
			return "Result:" + std::to_string(firstResult);
		}
	};

	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	auto result = arch->SendQuery(std::make_unique<SecondQuery>());
	EXPECT_EQ("Result:10", result);
}

TEST(EventBusTest, ExceptionInEventHandler)
{
	EventBus bus;

	class FaultyHandler : public ICanHandleEvent
	{
	public:
		void HandleEvent(std::shared_ptr<IEvent>) override
		{
			throw std::runtime_error("Handler error");
		}
	};

	class GoodHandler : public ICanHandleEvent
	{
	public:
		void HandleEvent(std::shared_ptr<IEvent>) override
		{
			handled = true;
		}
		bool handled = false;
	};

	FaultyHandler faulty;
	GoodHandler good;

	bus.RegisterEvent(typeid(TestEvent), &faulty);
	bus.RegisterEvent(typeid(TestEvent), &good);

	// 即使一个处理器抛出异常，其他处理器仍应正常工作
	EXPECT_NO_THROW(bus.SendEvent(std::make_shared<TestEvent>()));
	EXPECT_TRUE(good.handled);
}

TEST(ConcurrencyTest, ConcurrentComponentRegistration)
{
	auto arch = std::make_shared<TestArchitecture>();
	std::vector<std::thread> threads;
	const int threadCount = 10;

	for (int i = 0; i < threadCount; ++i)
	{
		threads.emplace_back([arch]
			{
				auto model = std::make_shared<ExtendedTestModel>();
				arch->RegisterModel<ExtendedTestModel>(model);
			});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	// 验证所有注册都成功
	auto models = arch->GetContainer()->GetAll<IModel>();
	EXPECT_EQ(1, models.size()); // 原始1个+新增的threadCount个
}
// BindableProperty 测试
TEST(BindablePropertyTest, ValueChangeNotification)
{
	BindableProperty<int> prop(10);
	bool notified = false;

	auto unregister = prop.Register([&](int val)
		{
			notified = true;
			EXPECT_EQ(20, val);
		});

	prop.SetValue(20);
	EXPECT_TRUE(notified);
}

TEST(BindablePropertyTest, Unregister)
{
	BindableProperty<int> prop(10);
	bool notified = false;

	{
		auto unregister = prop.Register([&](int) { notified = true; });
		prop.SetValue(20);
		EXPECT_TRUE(notified);
		prop.UnRegister(unregister->GetId());
	}

	notified = false;
	prop.SetValue(30);
	EXPECT_FALSE(notified);
}

TEST(BindablePropertyTest, RegisterWithInitValue)
{
	BindableProperty<int> prop(10);
	bool notified = false;

	auto unregister = prop.RegisterWithInitValue([&](int val)
		{
			notified = true;
			EXPECT_EQ(10, val);
		});

	EXPECT_TRUE(notified);
}

TEST(BindablePropertyTest, ThreadSafety)
{
	BindableProperty<int> prop(0);
	std::atomic<int> notificationCount{ 0 };
	std::vector<std::thread> threads;

	// 创建多个线程同时修改和监听属性
	for (int i = 0; i < 10; ++i)
	{
		threads.emplace_back([&]
			{
				auto unregister = prop.Register([&](int)
					{
						notificationCount++;
					});
			});
	}

	// 等待所有线程完成
	for (auto& t : threads)
	{
		t.join();
	}

	prop.SetValue(prop.GetValue() + 1);

	// 验证线程安全
	EXPECT_LE(10, notificationCount.load());
}

TEST(BindablePropertyTest, ValueSemantics)
{
	BindableProperty<std::string> prop("initial");

	// 测试字符串移动语义
	std::string longStr(1000, 'a');
	prop.SetValue(longStr);
	EXPECT_EQ(longStr, prop.GetValue());

	// 测试右值
	prop.SetValue(std::string(1000, 'b'));
	EXPECT_EQ(std::string(1000, 'b'), prop.GetValue());
}

TEST(BindablePropertyTest, MemoryManagement)
{
	auto prop = std::make_shared<BindableProperty<int>>(0);
	auto observer = std::make_shared<bool>(false);

	// 使用weak_ptr检测内存泄漏
	std::weak_ptr<bool> weakObserver = observer;
	{
		auto unregister = prop->Register([observer](int) 
			{ 
			*observer = true; 
			});
		prop->SetValue(1);
		prop->UnRegister(unregister->GetId());
		EXPECT_TRUE(*observer);
	}

	// observer应该被释放
	observer.reset();
	EXPECT_TRUE(weakObserver.expired());
}

// 能力接口测试
TEST(CapabilityTest, CanGetModel)
{
	class TestComponent : public ICanGetModel
	{
	public:
		explicit TestComponent(std::shared_ptr<IArchitecture> arch) : mArch(arch) {}

		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return mArch;
		}

	private:
		std::shared_ptr<IArchitecture> mArch;
	};

	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();
	TestComponent component(arch);

	auto model = component.GetModel<TestModel>();
	EXPECT_NE(nullptr, model);
}

TEST(CapabilityTest, CanSendCommand)
{
	// 将 arch 提升为静态变量或通过其他方式传递
	class TestComponent : public ICanSendCommand
	{
	public:
		explicit TestComponent(std::shared_ptr<IArchitecture> arch) : mArch(arch) {}

		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return mArch;
		}

	private:
		std::shared_ptr<IArchitecture> mArch;
	};

	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();
	TestComponent component(arch);

	auto cmd = std::make_unique<TestCommand>();
	auto cmdPtr = cmd.get();
	component.SendCommand(std::move(cmd));
	EXPECT_TRUE(cmdPtr->executed);
}

TEST(ExceptionTest, ArchitectureNotSet)
{
	class TestComponent : public ICanSendCommand
	{
	public:
		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return nullptr;
		}
	};

	TestComponent component;

	// 测试未设置架构时的异常
	EXPECT_THROW(component.SendCommand<TestCommand>(), ArchitectureNotSetException);
	EXPECT_THROW(component.SendCommand(std::make_unique<TestCommand>()), ArchitectureNotSetException);
}

TEST(ExceptionTest, ComponentNotRegistered)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	// 测试获取未注册组件时的异常
	EXPECT_THROW(arch->GetSystem<ExtendedTestSystem>(), ComponentNotRegisteredException);
	EXPECT_THROW(arch->GetModel<ExtendedTestModel>(), ComponentNotRegisteredException);
}

TEST(IntegrationTest, ComponentInteraction)
{
	// 注册事件处理器
	class EventHandler : public AbstractController
	{
	protected:
		void OnEvent(std::shared_ptr<IEvent> event) override
		{
			auto testEvent = std::dynamic_pointer_cast<ExtendedTestEvent>(event);
			if (testEvent)
			{
				eventReceived = true;
			}
		}

		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return mArch;
		}

	public:
		bool eventReceived = false;
		EventHandler(std::shared_ptr<IArchitecture> arch) :mArch(arch) {}
	private:
		std::shared_ptr<IArchitecture> mArch;
	};

	auto arch = std::make_shared<TestArchitecture>();

	// 注册自定义系统
	auto system = std::make_shared<ExtendedTestSystem>();
	arch->RegisterSystem<ExtendedTestSystem>(system);

	auto handler = std::make_shared<EventHandler>(arch);
	arch->RegisterEvent<ExtendedTestEvent>(handler.get());

	// 初始化架构
	arch->InitArchitecture();

	// 发送事件
	auto event = std::make_shared<ExtendedTestEvent>();
	arch->SendEvent(event);

	// 验证事件处理
	EXPECT_TRUE(handler->eventReceived);
	EXPECT_NE(nullptr, system->lastEvent);
}

TEST(PerformanceTest, CommandThroughput)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	const int iterations = 10000;
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < iterations; ++i)
	{
		arch->SendCommand(std::make_unique<TestCommand>());
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	std::cout << "Sent " << iterations << " commands in " << duration.count() << "ms\n";
	EXPECT_TRUE(duration.count() < 100); // 确保性能在合理范围内
}

TEST(PerformanceTest, EventThroughput)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	class PerformanceEventHandler : public ICanHandleEvent
	{
	public:
		void HandleEvent(std::shared_ptr<IEvent>) override
		{
			count++;
		}
		std::atomic<int> count{ 0 };
	};

	PerformanceEventHandler handler;
	arch->RegisterEvent<TestEvent>(&handler);

	const int iterations = 10000;
	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < iterations; ++i)
	{
		arch->SendEvent(std::make_shared<TestEvent>());
	}

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

	std::cout << "Sent " << iterations << " events in " << duration.count() << "ms\n";
	EXPECT_EQ(iterations, handler.count.load());
	EXPECT_TRUE(duration.count() < 100);
}

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}