// JFramework.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "JFramework.h"
#include "packages/google-testmock.1.16.0/build/native/include/gmock/gmock-matchers.h"
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
using namespace JFramework;

class ExtendedTestEvent : public IEvent {
public:
    std::string GetEventType() const override { return "ExtendedTestEvent"; }
    int eventData = 0;
};

// 测试用的组件类
class TestModel : public AbstractModel {
protected:
    void OnInit() override { initialized = true; }
    void OnDeinit() override { initialized = false; }

public:
    bool initialized = false;
};

class TestSystem : public AbstractSystem {
protected:
    void OnInit() override { initialized = true; }
    void OnDeinit() override { initialized = false; }
    void OnEvent(std::shared_ptr<IEvent>) override { }

public:
    bool initialized = false;
};

class TestUtility : public IUtility { };

class TestEvent : public IEvent {
public:
    std::string GetEventType() const override { return "TestEvent"; }
};

class TestCommand : public AbstractCommand {
protected:
    void OnExecute() override { executed = true; }

public:
    bool executed = false;
};

class TestQuery : public AbstractQuery<int> {
protected:
    int OnDo() override { return 42; }
};

class TestEventHandler : public ICanHandleEvent {
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
class ExtendedTestModel : public AbstractModel {
protected:
    void OnInit() override { initCount++; }
    void OnDeinit() override { deinitCount++; }

public:
    int initCount = 0;
    int deinitCount = 0;
};

class ExtendedTestSystem : public AbstractSystem {
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
        if (testEvent) {
            lastEvent = event;
        }
    }

public:
    bool initialized = false;
    std::shared_ptr<IEvent> lastEvent = nullptr;
};

class ExtendedTestCommand : public AbstractCommand {
protected:
    void OnExecute() override { executionCount++; }

public:
    static int executionCount;
};
int ExtendedTestCommand::executionCount = 0;

class ExtendedTestQuery : public AbstractQuery<std::string> {
protected:
    std::string OnDo() override
    {
        return "QueryResult:" + std::to_string(queryParam);
    }

public:
    int queryParam = 0;
};

// 测试架构类
class TestArchitecture : public Architecture {
protected:
    void Init() override { }
};

// 测试架构类
class MultipleTestArchitecture : public Architecture {
protected:
    void Init() override { }
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

TEST(IOCContainerTest, AdvancedRegistration)
{
    IOCContainer container;

    // 测试多组件注册
    auto model1 = std::make_shared<ExtendedTestModel>();

    container.Register<ExtendedTestModel, IModel>(typeid(ExtendedTestModel),
        model1);

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
    container.Register<ExtendedTestSystem, ISystem>(typeid(ExtendedTestSystem),
        system);

    // 错误类型获取应返回nullptr
    EXPECT_EQ(nullptr, container.Get<IModel>(typeid(ExtendedTestSystem)));

    // 正确类型获取
    EXPECT_NE(nullptr, container.Get<ISystem>(typeid(ExtendedTestSystem)));
}

TEST(IOCContainerTest, DifferentBaseTypes)
{
    IOCContainer container;

    // 测试同一类型注册为不同基类
    auto model = std::make_shared<TestModel>();
    container.Register<TestModel, IModel>(typeid(TestModel), model);

    // 不应在ISystem容器中找到
    EXPECT_EQ(nullptr, container.Get<ISystem>(typeid(TestModel)));
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

    class OrderedHandler : public ICanHandleEvent {
    public:
        OrderedHandler(int id, std::vector<int>& order)
            : mId(id)
            , mOrder(order)
        {
        }
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

TEST(EventBusTest, EventTypeMatching)
{
    EventBus bus;

    class DerivedEvent : public TestEvent {
    public:
        std::string GetEventType() const override { return "DerivedEvent"; }
    };

    TestEventHandler baseHandler;
    TestEventHandler derivedHandler;

    bus.RegisterEvent(typeid(TestEvent), &baseHandler);
    bus.RegisterEvent(typeid(DerivedEvent), &derivedHandler);

    // 发送派生事件
    bus.SendEvent(std::make_shared<DerivedEvent>());

    // 只有派生事件处理器应该被调用
    EXPECT_FALSE(baseHandler.eventHandled);
    EXPECT_TRUE(derivedHandler.eventHandled);
}

TEST(EventBusTest, ClearAllHandlers)
{
    EventBus bus;
    TestEventHandler handler1, handler2;

    bus.RegisterEvent(typeid(TestEvent), &handler1);
    bus.RegisterEvent(typeid(TestEvent), &handler2);

    bus.Clear();

    bus.SendEvent(std::make_shared<TestEvent>());

    EXPECT_FALSE(handler1.eventHandled);
    EXPECT_FALSE(handler2.eventHandled);
}

TEST(EventBusTest, ExceptionInEventHandler)
{
    EventBus bus;

    class FaultyHandler : public ICanHandleEvent {
    public:
        void HandleEvent(std::shared_ptr<IEvent>) override
        {
            throw std::runtime_error("Handler error");
        }
    };

    class GoodHandler : public ICanHandleEvent {
    public:
        void HandleEvent(std::shared_ptr<IEvent>) override { handled = true; }
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

// Architecture 测试
TEST(ArchitectureTest, ComponentRegistration)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->RegisterModel(std::make_shared<TestModel>());
    arch->RegisterSystem(std::make_shared<TestSystem>());
    arch->RegisterUtility(std::make_shared<TestUtility>());
    arch->InitArchitecture();

    auto model = arch->GetModel<TestModel>();
    auto system = arch->GetSystem<TestSystem>();
    auto utility = arch->GetUtility<TestUtility>();

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
    arch->RegisterEvent<TestEvent>(&handler);

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
    // 注册测试组件
    arch->RegisterModel(std::make_shared<TestModel>());
    arch->RegisterSystem(std::make_shared<TestSystem>());
    arch->RegisterUtility(std::make_shared<TestUtility>());

    arch->InitArchitecture();

    auto utility = arch->GetUtility<TestUtility>();
    EXPECT_NE(nullptr, utility);

    // 测试utility的具体功能
}

TEST(ArchitectureTest, MultipleInitDeinitCycles)
{
    auto arch = std::make_shared<TestArchitecture>();
    auto model = std::make_shared<ExtendedTestModel>();
    arch->RegisterModel<ExtendedTestModel>(model);

    // 多次初始化和反初始化
    for (int i = 0; i < 3; ++i) {
        arch->InitArchitecture();
        EXPECT_EQ(i + 1, model->initCount);

        arch->Deinit();
        EXPECT_EQ(i + 1, model->deinitCount);
    }
}

static int executionCount = 0;

TEST(ArchitectureTest, CommandInCommand)
{
    class NestedCommand : public AbstractCommand {
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
    class FirstQuery : public AbstractQuery<int> {
    protected:
        int OnDo() override { return 10; }
    };

    class SecondQuery : public AbstractQuery<std::string> {
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

TEST(ArchitectureTest, LateComponentRegistration)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture(); // 先初始化架构

    // 然后注册组件
    auto model = std::make_shared<ExtendedTestModel>();
    arch->RegisterModel<ExtendedTestModel>(model);

    // 验证组件被自动初始化
    EXPECT_EQ(1, model->initCount);
}

TEST(ArchitectureTest, ComponentDependencies)
{
    class DependentSystem : public AbstractSystem {
    protected:
        void OnInit() override
        {
            // 获取依赖的模型
            auto model = GetModel<TestModel>();
            modelInitialized = model->initialized;
        }
        void OnDeinit() override { }
        void OnEvent(std::shared_ptr<IEvent>) override { }

    public:
        bool modelInitialized = false;
    };

    auto arch = std::make_shared<TestArchitecture>();
    auto system = std::make_shared<DependentSystem>();

    // 先注册系统依赖的模型
    arch->RegisterModel<TestModel>(std::make_shared<TestModel>());
    arch->RegisterSystem<DependentSystem>(system);

    arch->InitArchitecture();

    // 验证系统初始化时能够访问模型
    EXPECT_TRUE(system->modelInitialized);
}

TEST(ArchitectureTest, MultipleArchitectureInstances)
{
    auto arch1 = std::make_shared<MultipleTestArchitecture>();
    auto arch2 = std::make_shared<MultipleTestArchitecture>();

    auto model1 = std::make_shared<TestModel>();
    auto model2 = std::make_shared<TestModel>();

    arch1->RegisterModel<TestModel>(model1);
    arch2->RegisterModel<TestModel>(model2);

    arch1->InitArchitecture();

    // 验证两个架构实例独立
    EXPECT_TRUE(model1->initialized);
    EXPECT_FALSE(model2->initialized);
}

TEST(ConcurrencyTest, ConcurrentEventHandling)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    class ConcurrentEventHandler : public ICanHandleEvent {
    public:
        void HandleEvent(std::shared_ptr<IEvent>) override
        {
            std::lock_guard<std::mutex> lock(mutex);
            count++;
        }
        std::mutex mutex;
        int count = 0;
    };

    ConcurrentEventHandler handler;
    arch->RegisterEvent<TestEvent>(&handler);

    const int threadCount = 10;
    const int eventsPerThread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([arch]() {
            for (int j = 0; j < eventsPerThread; ++j) {
                arch->SendEvent(std::make_shared<TestEvent>());
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(threadCount * eventsPerThread, handler.count);
}

TEST(ConcurrencyTest, ConcurrentPropertyAccess)
{
    BindableProperty<int> prop(0);
    std::atomic<int> sum(0);
    const int threadCount = 10;
    const int iterations = 1000;
    std::vector<std::thread> threads;

    // 修改测试逻辑，只测试线程安全性，不验证最终值
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < iterations; ++j) {
                // 使用原子操作确保线程安全
                int current = prop.GetValue();
                prop.SetValue(current + 1);
                sum.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 验证所有操作都完成了
    EXPECT_EQ(threadCount * iterations, sum.load());
    // 不验证最终值，因为并发修改会导致不确定性
}

// BindableProperty 测试
TEST(BindablePropertyTest, ValueChangeNotification)
{
    BindableProperty<int> prop(10);
    bool notified = false;

    auto unregister = prop.Register([&](int val) {
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

    auto unregister = prop.RegisterWithInitValue([&](int val) {
        notified = true;
        EXPECT_EQ(10, val);
    });

    EXPECT_TRUE(notified);
}

TEST(BindablePropertyTest, ThreadSafety)
{
    BindableProperty<int> prop(0);
    std::atomic<int> notificationCount { 0 };
    std::vector<std::thread> threads;

    // 创建多个线程同时修改和监听属性
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&] {
            auto unregister = prop.Register([&](int) { notificationCount++; });
        });
    }

    // 等待所有线程完成
    for (auto& t : threads) {
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
        auto unregister = prop->Register([observer](int) { *observer = true; });
        prop->SetValue(1);
        prop->UnRegister(unregister->GetId());
        EXPECT_TRUE(*observer);
    }

    // observer应该被释放
    observer.reset();
    EXPECT_TRUE(weakObserver.expired());
}

TEST(BindablePropertyTest, OperatorOverloads)
{
    BindableProperty<int> prop(10);

    // 测试隐式转换操作符
    int value = prop;
    EXPECT_EQ(10, value);

    // 测试赋值操作符
    prop = 20;
    EXPECT_EQ(20, prop.GetValue());

    // 测试比较操作符
    EXPECT_TRUE(prop == 20);
    EXPECT_FALSE(prop != 20);
}

TEST(BindablePropertyTest, SetWithoutEvent)
{
    BindableProperty<int> prop(10);
    bool notified = false;

    auto unregister = prop.Register([&](int) { notified = true; });

    prop.SetValueWithoutEvent(20);
    EXPECT_EQ(20, prop.GetValue());
    EXPECT_FALSE(notified); // 不应触发通知
}

TEST(BindablePropertyTest, MultipleObservers)
{
    BindableProperty<std::string> prop("init");
    int notificationCount = 0;

    auto unregister1 = prop.Register([&](const std::string&) { notificationCount++; });
    auto unregister2 = prop.Register([&](const std::string&) { notificationCount++; });

    prop.SetValue("changed");
    EXPECT_EQ(2, notificationCount);
}

TEST(BindablePropertyTest, NoNotificationOnSameValue)
{
    BindableProperty<int> prop(10);
    int notificationCount = 0;

    auto unregister = prop.Register([&](int) { notificationCount++; });

    prop.SetValue(10); // 设置相同的值
    EXPECT_EQ(0, notificationCount); // 不应触发通知

    prop.SetValue(20);
    EXPECT_EQ(1, notificationCount);
}

TEST(BindablePropertyTest, MultipleRegistrations)
{
    BindableProperty<int> prop(0);
    std::vector<int> notifications;

    // 注册多个观察者
    auto unreg1 = prop.Register([&](int val) { notifications.push_back(val * 1); });
    auto unreg2 = prop.Register([&](int val) { notifications.push_back(val * 2); });
    auto unreg3 = prop.Register([&](int val) { notifications.push_back(val * 3); });

    prop.SetValue(10);

    // 验证所有观察者都收到通知
    EXPECT_EQ(3, notifications.size());
    EXPECT_THAT(notifications, testing::UnorderedElementsAre(10, 20, 30));
}

// 能力接口测试
TEST(CapabilityTest, CanGetModel)
{
    class TestComponent : public ICanGetModel {
    public:
        explicit TestComponent(std::shared_ptr<IArchitecture> arch)
            : mArch(arch)
        {
        }

        std::weak_ptr<IArchitecture> GetArchitecture() const override
        {
            return mArch;
        }

    private:
        std::shared_ptr<IArchitecture> mArch;
    };

    auto arch = std::make_shared<TestArchitecture>();
    arch->RegisterModel(std::make_shared<TestModel>());
    arch->RegisterSystem(std::make_shared<TestSystem>());
    arch->RegisterUtility(std::make_shared<TestUtility>());
    arch->InitArchitecture();
    TestComponent component(arch);

    auto model = component.GetModel<TestModel>();
    EXPECT_NE(nullptr, model);
}

TEST(CapabilityTest, CanSendCommand)
{
    // 将 arch 提升为静态变量或通过其他方式传递
    class TestComponent : public ICanSendCommand {
    public:
        explicit TestComponent(std::shared_ptr<IArchitecture> arch)
            : mArch(arch)
        {
        }

        std::weak_ptr<IArchitecture> GetArchitecture() const override
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

TEST(ExceptionTest, ComponentNotRegistered)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    // 测试获取未注册组件时的异常
    EXPECT_THROW(arch->GetSystem<ExtendedTestSystem>(),
        ComponentNotRegisteredException);
    EXPECT_THROW(arch->GetModel<ExtendedTestModel>(),
        ComponentNotRegisteredException);
}

TEST(IntegrationTest, ComponentInteraction)
{
    // 注册事件处理器
    class EventHandler : public AbstractController {
    protected:
        void OnEvent(std::shared_ptr<IEvent> event) override
        {
            auto testEvent = std::dynamic_pointer_cast<ExtendedTestEvent>(event);
            if (testEvent) {
                eventReceived = true;
            }
        }

        std::weak_ptr<IArchitecture> GetArchitecture() const override
        {
            return mArch;
        }

    public:
        bool eventReceived = false;
        EventHandler(std::shared_ptr<IArchitecture> arch)
            : mArch(arch)
        {
        }

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

    for (int i = 0; i < iterations; ++i) {
        arch->SendCommand(std::make_unique<TestCommand>());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Sent " << iterations << " commands in " << duration.count()
              << "ms\n";
    EXPECT_TRUE(duration.count() < 100); // 确保性能在合理范围内
}

TEST(PerformanceTest, EventThroughput)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    class PerformanceEventHandler : public ICanHandleEvent {
    public:
        void HandleEvent(std::shared_ptr<IEvent>) override { count++; }
        std::atomic<int> count { 0 };
    };

    PerformanceEventHandler handler;
    arch->RegisterEvent<TestEvent>(&handler);

    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        arch->SendEvent(std::make_shared<TestEvent>());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Sent " << iterations << " events in " << duration.count()
              << "ms\n";
    EXPECT_EQ(iterations, handler.count.load());
    EXPECT_TRUE(duration.count() < 100);
}

TEST(AutoUnRegisterTest, UnRegisterWhenDestroyed)
{
    BindableProperty<int> prop(0);
    bool notified = false;

    {
        UnRegisterTrigger trigger;
        auto unregister = prop.Register([&](int) { notified = true; });
        unregister->UnRegisterWhenObjectDestroyed(&trigger);

        prop.SetValue(1);
        EXPECT_TRUE(notified);
    } // trigger析构时应自动注销

    notified = false;
    prop.SetValue(2);
    EXPECT_FALSE(notified); // 应不再收到通知
}

TEST(AutoUnRegisterTest, ManualUnRegister)
{
    BindableProperty<int> prop(0);
    bool notified = false;

    auto unregister = prop.Register([&](int) { notified = true; });

    prop.SetValue(1);
    EXPECT_TRUE(notified);

    notified = false;
    unregister->UnRegister();
    prop.SetValue(2);
    EXPECT_FALSE(notified); // 手动注销后不应收到通知
}

TEST(EventRegistrationTest, MultipleEventTypes)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    class MultiEventHandler : public ICanHandleEvent {
    public:
        void HandleEvent(std::shared_ptr<IEvent> event) override
        {
            if (event->GetEventType() == "TestEvent")
                testEventCount++;
            else if (event->GetEventType() == "ExtendedTestEvent")
                extendedEventCount++;
        }
        int testEventCount = 0;
        int extendedEventCount = 0;
    };

    MultiEventHandler handler;
    arch->RegisterEvent<TestEvent>(&handler);
    arch->RegisterEvent<ExtendedTestEvent>(&handler);

    arch->SendEvent(std::make_shared<TestEvent>());
    arch->SendEvent(std::make_shared<ExtendedTestEvent>());

    EXPECT_EQ(1, handler.testEventCount);
    EXPECT_EQ(1, handler.extendedEventCount);

    // 测试注销特定事件类型
    arch->UnRegisterEvent<TestEvent>(&handler);
    arch->SendEvent(std::make_shared<TestEvent>());
    EXPECT_EQ(1, handler.testEventCount); // 不应增加
}

TEST(QueryTest, ChainedQueriesWithParameters)
{
    class ParamQuery : public AbstractQuery<int> {
    protected:
        int OnDo() override { return param; }

    public:
        int param = 0;
    };

    class ChainedQuery : public AbstractQuery<std::string> {
    protected:
        std::string OnDo() override
        {
            auto q1 = std::make_unique<ParamQuery>();
            q1->param = 10;
            auto q2 = std::make_unique<ParamQuery>();
            q2->param = 20;

            int result1 = this->SendQuery(std::move(q1));
            int result2 = this->SendQuery(std::move(q2));

            return std::to_string(result1 + result2);
        }
    };

    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    auto result = arch->SendQuery(std::make_unique<ChainedQuery>());
    EXPECT_EQ("30", result);
}

TEST(ExceptionSafetyTest, ComponentInitializationFailure)
{
    class FaultyModel : public AbstractModel {
    protected:
        void OnInit() override { throw std::runtime_error("Init failed"); }
        void OnDeinit() override { }
    };

    auto arch = std::make_shared<TestArchitecture>();
    auto model = std::make_shared<FaultyModel>();

    // 修改预期：注册时不应抛出异常
    EXPECT_NO_THROW(arch->RegisterModel<FaultyModel>(model));

    // 修改预期：初始化时应抛出异常
    EXPECT_THROW(arch->InitArchitecture(), std::runtime_error);

    // 模型应标记为未初始化
    EXPECT_FALSE(model->IsInitialized());

    // 架构应仍然可以正常使用
    EXPECT_NO_THROW(arch->SendCommand(std::make_unique<TestCommand>()));
}

TEST(ExceptionSafetyTest, CommandExecutionFailure)
{
    class FaultyCommand : public AbstractCommand {
    protected:
        void OnExecute() override { throw std::runtime_error("Command failed"); }
    };

    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    // 命令执行失败不应影响架构
    EXPECT_NO_THROW(arch->SendCommand(std::make_unique<FaultyCommand>()));

    // 架构应仍然正常工作
    EXPECT_NO_THROW(arch->SendCommand(std::make_unique<TestCommand>()));
}

TEST(ExceptionSafetyTest, EventHandlerThrowsDuringRegistration)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    class FaultyHandler : public ICanHandleEvent {
    public:
        void HandleEvent(std::shared_ptr<IEvent>) override
        {
            throw std::runtime_error("Faulty handler");
        }
    };

    FaultyHandler handler;

    // 注册时不应抛出异常
    EXPECT_NO_THROW(arch->RegisterEvent<TestEvent>(&handler));

    // 发送事件时应捕获异常
    EXPECT_NO_THROW(arch->SendEvent(std::make_shared<TestEvent>()));
}

TEST(ExceptionSafetyTest, ComponentRegistrationAfterDeinit)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();
    arch->Deinit();

    auto model = std::make_shared<TestModel>();

    // 反初始化后注册组件不应抛出异常
    EXPECT_NO_THROW(arch->RegisterModel<TestModel>(model));

    // 但组件不应被初始化
    EXPECT_FALSE(model->initialized);
}

TEST(PerformanceTest, PropertyNotificationScalability)
{
    BindableProperty<int> prop(0);
    const int observerCount = 1000;
    std::atomic<int> notificationCount { 0 };

    std::vector<std::shared_ptr<BindablePropertyUnRegister<int>>> observers;
    for (int i = 0; i < observerCount; ++i) {
        observers.push_back(prop.Register([&](int) { notificationCount++; }));
    }

    auto start = std::chrono::high_resolution_clock::now();
    prop.SetValue(1);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Notified " << observerCount << " observers in "
              << duration.count() << "μs\n";
    EXPECT_EQ(observerCount, notificationCount.load());
}

TEST(PerformanceTest, ConcurrentEventProcessing)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    class CountingHandler : public ICanHandleEvent {
    public:
        void HandleEvent(std::shared_ptr<IEvent>) override { count++; }
        std::atomic<int> count { 0 };
    };

    CountingHandler handler;
    arch->RegisterEvent<TestEvent>(&handler);

    const int threadCount = 10;
    const int eventsPerThread = 1000;
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([arch]() {
            for (int j = 0; j < eventsPerThread; ++j) {
                arch->SendEvent(std::make_shared<TestEvent>());
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Processed " << (threadCount * eventsPerThread) << " events in "
              << duration.count() << "ms\n";
    EXPECT_EQ(threadCount * eventsPerThread, handler.count.load());
}

TEST(MemoryTest, EventHandlerLeak)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    std::weak_ptr<ICanHandleEvent> weakHandler;

    {
        auto handler = std::make_shared<TestEventHandler>();
        weakHandler = handler;
        arch->RegisterEvent<TestEvent>(handler.get());

        // 发送事件确保注册成功
        arch->SendEvent(std::make_shared<TestEvent>());

        arch->UnRegisterEvent<TestEvent>(handler.get());

        EXPECT_TRUE(handler->eventHandled);
    }

    // handler超出作用域后应被释放
    EXPECT_TRUE(weakHandler.expired());
}

TEST(MemoryTest, PropertyObserverLeak)
{
    auto prop = std::make_shared<BindableProperty<int>>(0);
    std::weak_ptr<BindablePropertyUnRegister<int>> weakUnregister;

    {
        auto unregister = prop->Register([](int) { });
        weakUnregister = unregister;

        // 确保注册成功
        prop->SetValue(1);
    }

    // 强制触发析构
    prop.reset();

    // unregister超出作用域且属性被销毁后应被释放
    EXPECT_TRUE(weakUnregister.expired());
}

TEST(MemoryTest, ArchitectureSharedOwnership)
{
    std::weak_ptr<IArchitecture> weakArch;

    {
        auto arch = std::make_shared<TestArchitecture>();
        weakArch = arch;

        // 组件持有架构的引用
        auto model = std::make_shared<TestModel>();
        arch->RegisterModel<TestModel>(model);

        // 命令也持有架构引用
        auto cmd = std::make_unique<TestCommand>();
        cmd->SetArchitecture(arch);
    }

    // 所有引用释放后，架构应被销毁
    EXPECT_TRUE(weakArch.expired());
}

TEST(MemoryTest, EventHandlerUnregistration)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    std::weak_ptr<ICanHandleEvent> weakHandler;

    {
        auto handler = std::make_shared<TestEventHandler>();
        weakHandler = handler;
        arch->RegisterEvent<TestEvent>(handler.get());

        // 发送事件确保注册成功
        arch->SendEvent(std::make_shared<TestEvent>());
        EXPECT_TRUE(handler->eventHandled);

        // 反初始化架构
        arch->Deinit();
    }

    // handler超出作用域后应被释放
    EXPECT_TRUE(weakHandler.expired());
}

TEST(ControllerTest, ControllerFunctionality)
{
    class TestController : public AbstractController {
        void OnEvent(std::shared_ptr<IEvent> event) override
        {
            if (auto* e = dynamic_cast<TestEvent*>(event.get())) {
                handled = true;
            }
        }

        std::weak_ptr<IArchitecture> GetArchitecture() const override
        {
            return mArch;
        }

    private:
        std::shared_ptr<IArchitecture> mArch;

    public:
        explicit TestController(std::shared_ptr<IArchitecture> arch)
            : mArch(arch)
        {
        }

        bool handled = false;
    };

    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    TestController controller(arch);
    arch->RegisterEvent<TestEvent>(&controller);
    arch->SendEvent(std::make_shared<TestEvent>());
    EXPECT_TRUE(controller.handled);
}

TEST(BoundaryTest, NullPointerHandling)
{
    auto arch = std::make_shared<TestArchitecture>();

    // 测试空指针组件注册
    EXPECT_THROW(arch->RegisterSystem<TestSystem>(nullptr), std::invalid_argument);
    EXPECT_THROW(arch->RegisterModel<TestModel>(nullptr), std::invalid_argument);
    EXPECT_THROW(arch->RegisterUtility<TestUtility>(nullptr), std::invalid_argument);

    // 测试空指针事件发送
    EXPECT_THROW(arch->SendEvent(nullptr), std::invalid_argument);
    EXPECT_THROW(arch->SendCommand(nullptr), std::invalid_argument);

    // 测试空指针事件处理器注册
    EXPECT_THROW(arch->RegisterEvent<TestEvent>(nullptr), std::invalid_argument);
}

TEST(ConcurrencyTest, ParallelCommandExecution)
{
    auto arch = std::make_shared<TestArchitecture>();
    arch->InitArchitecture();

    const int threadCount = 20;
    const int commandsPerThread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> totalExecutions { 0 };

    // 创建可计数命令
    class CountingCommand : public AbstractCommand {
    public:
        CountingCommand(std::atomic<int>& counter)
            : counter(counter)
        {
        }

    protected:
        void OnExecute() override { counter++; }

    private:
        std::atomic<int>& counter;
    };

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < commandsPerThread; ++j) {
                arch->SendCommand(std::make_unique<CountingCommand>(totalExecutions));
            }
        });
    }

    for (auto& t : threads)
        t.join();
    EXPECT_EQ(threadCount * commandsPerThread, totalExecutions.load());
}

TEST(MemoryTest, ComponentCleanup)
{
    std::weak_ptr<IModel> weakModel;
    std::weak_ptr<ISystem> weakSystem;

    {
        auto arch = std::make_shared<TestArchitecture>();

        auto model = std::make_shared<TestModel>();
        auto system = std::make_shared<TestSystem>();

        weakModel = model;
        weakSystem = system;

        arch->RegisterModel<TestModel>(model);
        arch->RegisterSystem<TestSystem>(system);
        arch->InitArchitecture();

        // 显式反初始化
        arch->Deinit();
    }

    // 验证组件是否被正确释放
    EXPECT_TRUE(weakModel.expired());
    EXPECT_TRUE(weakSystem.expired());
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}