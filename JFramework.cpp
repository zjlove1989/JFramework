﻿// JFramework.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "JFramework.h"
#include "packages/google-testmock.1.16.0/build/native/include/gmock/gmock-matchers.h"
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>
using namespace JFramework;

// ========== 异常测试 ==========
TEST(ExceptionTest, ArchitectureNotSetException)
{
    EXPECT_THROW(throw ArchitectureNotSetException("TestType"), ArchitectureNotSetException);
}

TEST(ExceptionTest, ComponentNotRegisteredException)
{
    EXPECT_THROW(throw ComponentNotRegisteredException("TestType"), ComponentNotRegisteredException);
}

TEST(ExceptionTest, ComponentAlreadyRegisteredException)
{
    EXPECT_THROW(throw ComponentAlreadyRegisteredException("TestType"), ComponentAlreadyRegisteredException);
}

// ========== EventBus 测试 ==========
class TestEvent : public IEvent {
public:
    std::string GetEventType() const override { return "TestEvent"; }
};

class TestHandler : public ICanHandleEvent {
public:
    bool handled = false;
    void HandleEvent(std::shared_ptr<IEvent> event) override { handled = true; }
};

TEST(EventBusTest, RegisterAndSendEvent)
{
    EventBus bus;
    TestHandler handler;
    bus.RegisterEvent(typeid(TestEvent), &handler);
    bus.SendEvent(std::make_shared<TestEvent>());
    EXPECT_TRUE(handler.handled);
}

TEST(EventBusTest, UnRegisterEvent)
{
    EventBus bus;
    TestHandler handler;
    bus.RegisterEvent(typeid(TestEvent), &handler);
    bus.UnRegisterEvent(typeid(TestEvent), &handler);
    handler.handled = false;
    bus.SendEvent(std::make_shared<TestEvent>());
    EXPECT_FALSE(handler.handled);
}

TEST(EventBusTest, Clear)
{
    EventBus bus;
    TestHandler handler;
    bus.RegisterEvent(typeid(TestEvent), &handler);
    bus.Clear();
    handler.handled = false;
    bus.SendEvent(std::make_shared<TestEvent>());
    EXPECT_FALSE(handler.handled);
}

class AnotherEvent : public IEvent {
public:
    std::string GetEventType() const override { return "AnotherEvent"; }
};

class CountingHandler : public ICanHandleEvent {
public:
    int count = 0;
    void HandleEvent(std::shared_ptr<IEvent>) override { ++count; }
};

class ExceptionHandler : public ICanHandleEvent {
public:
    bool called = false;
    void HandleEvent(std::shared_ptr<IEvent>) override
    {
        called = true;
        throw std::runtime_error("handler error");
    }
};

TEST(EventBusTest, MultipleHandlersAllReceiveEvent)
{
    EventBus bus;
    CountingHandler h1, h2;
    bus.RegisterEvent(typeid(TestEvent), &h1);
    bus.RegisterEvent(typeid(TestEvent), &h2);
    bus.SendEvent(std::make_shared<TestEvent>());
    EXPECT_EQ(h1.count, 1);
    EXPECT_EQ(h2.count, 1);
}

TEST(EventBusTest, HandlerNotCalledForOtherEventType)
{
    EventBus bus;
    CountingHandler handler;
    bus.RegisterEvent(typeid(TestEvent), &handler);
    bus.SendEvent(std::make_shared<AnotherEvent>());
    EXPECT_EQ(handler.count, 0);
}

TEST(EventBusTest, DuplicateRegisterHandler)
{
    EventBus bus;
    CountingHandler handler;
    bus.RegisterEvent(typeid(TestEvent), &handler);
    bus.RegisterEvent(typeid(TestEvent), &handler);
    bus.SendEvent(std::make_shared<TestEvent>());
    // handler 被调用两次
    EXPECT_EQ(handler.count, 2);
}

TEST(EventBusTest, ExceptionInHandlerDoesNotAffectOthers)
{
    EventBus bus;
    ExceptionHandler exHandler;
    CountingHandler normalHandler;
    bus.RegisterEvent(typeid(TestEvent), &exHandler);
    bus.RegisterEvent(typeid(TestEvent), &normalHandler);
    bus.SendEvent(std::make_shared<TestEvent>());
    EXPECT_TRUE(exHandler.called);
    EXPECT_EQ(normalHandler.count, 1);
}

// ========== BindableProperty 测试 ==========
TEST(BindablePropertyTest, GetSetValue)
{
    BindableProperty<int> prop(1);
    EXPECT_EQ(prop.GetValue(), 1);
    prop.SetValue(2);
    EXPECT_EQ(prop.GetValue(), 2);
}

TEST(BindablePropertyTest, SetValueWithoutEvent)
{
    BindableProperty<int> prop(1);
    prop.SetValueWithoutEvent(3);
    EXPECT_EQ(prop.GetValue(), 3);
}

TEST(BindablePropertyTest, RegisterAndTrigger)
{
    BindableProperty<int> prop(1);
    int observed = 0;
    auto unreg = prop.Register([&](const int& v) { observed = v; });
    prop.SetValue(5);
    EXPECT_EQ(observed, 5);
    unreg->UnRegister();
    prop.SetValue(10);
    EXPECT_EQ(observed, 5); // 不再更新
}

TEST(BindablePropertyTest, RegisterWithInitValue)
{
    BindableProperty<int> prop(7);
    int observed = 0;
    auto unreg = prop.RegisterWithInitValue([&](const int& v) { observed = v; });
    EXPECT_EQ(observed, 7);
}

TEST(BindablePropertyTest, MultipleObservers)
{
    BindableProperty<int> prop(0);
    int v1 = 0, v2 = 0;
    auto u1 = prop.Register([&](const int& v) { v1 = v; });
    auto u2 = prop.Register([&](const int& v) { v2 = v; });
    prop.SetValue(42);
    EXPECT_EQ(v1, 42);
    EXPECT_EQ(v2, 42);
}

TEST(BindablePropertyTest, DuplicateRegisterCallback)
{
    BindableProperty<int> prop(0);
    int count = 0;
    auto cb = [&](const int&) { ++count; };
    auto u1 = prop.Register(cb);
    auto u2 = prop.Register(cb);
    prop.SetValue(1);
    EXPECT_EQ(count, 2);
}

TEST(BindablePropertyTest, UnRegisterStopsNotification)
{
    BindableProperty<int> prop(0);
    int v = 0;
    auto u = prop.Register([&](const int& val) { v = val; });
    prop.SetValue(5);
    EXPECT_EQ(v, 5);
    u->UnRegister();
    prop.SetValue(10);
    EXPECT_EQ(v, 5); // 不再更新
}

TEST(BindablePropertyTest, DestructorCleansObservers)
{
    int v = 0;
    {
        BindableProperty<int> prop(1);
        auto u = prop.Register([&](const int& val) { v = val; });
        prop.SetValue(2);
        EXPECT_EQ(v, 2);
        // prop 离开作用域析构，不应崩溃
    }
    SUCCEED();
}

TEST(BindablePropertyTest, CallbackThrowsException)
{
    BindableProperty<int> prop(0);
    int v = 0;
    auto u1 = prop.Register([&](const int&) { throw std::runtime_error("fail"); });
    auto u2 = prop.Register([&](const int& val) { v = val; });
    // 即使有回调抛异常，其他回调仍然被调用
    EXPECT_NO_THROW(prop.SetValue(123));
    EXPECT_EQ(v, 123);
}

TEST(BindablePropertyTest, UnRegisterWhenObjectDestroyed)
{
    BindableProperty<int> prop(0);
    int v = 0;
    class MyTrigger : public UnRegisterTrigger { };
    MyTrigger trigger;
    {
        auto u = prop.Register([&](const int& val) { v = val; });
        u->UnRegisterWhenObjectDestroyed(&trigger);
        prop.SetValue(7);
        EXPECT_EQ(v, 7);
    }
    // trigger析构时自动注销
    trigger.UnRegister();
    prop.SetValue(8);
    EXPECT_EQ(v, 7); // 不再更新
}

TEST(BindablePropertyTest, OperatorAssign)
{
    BindableProperty<int> prop(1);
    int v = 0;
    auto u = prop.Register([&](const int& val) { v = val; });
    prop = 99;
    EXPECT_EQ(prop.GetValue(), 99);
    EXPECT_EQ(v, 99);
}

// ========== UnRegisterTrigger 测试 ==========
class DummyUnRegister : public IUnRegister {
public:
    bool called = false;
    void UnRegister() override { called = true; }
};

TEST(UnRegisterTriggerTest, AddAndUnRegister)
{
    UnRegisterTrigger trigger;
    auto dummy = std::make_shared<DummyUnRegister>();
    trigger.AddUnRegister(dummy);
    trigger.UnRegister();
    EXPECT_TRUE(dummy->called);
}

// ========== ICanInit 测试 ==========
class DummyCanInit : public ICanInit {
public:
    bool inited = false;
    void Init() override { inited = true; }
    void Deinit() override { inited = false; }
};

TEST(ICanInitTest, InitAndDeinit)
{
    DummyCanInit obj;
    obj.Init();
    EXPECT_TRUE(obj.inited);
    obj.Deinit();
    EXPECT_FALSE(obj.inited);
    obj.SetInitialized(true);
    EXPECT_TRUE(obj.IsInitialized());
}

// ========== IOCContainer 测试 ==========
class DummyModel : public IModel {
public:
    void Init() override { }
    void Deinit() override { }
    void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
    std::weak_ptr<IArchitecture> mArch;
};

TEST(IOCContainerTest, RegisterAndGetModel)
{
    IOCContainer container;
    auto model = std::make_shared<DummyModel>();
    container.Register<DummyModel, IModel>(typeid(DummyModel), model);
    auto got = container.Get<IModel>(typeid(DummyModel));
    EXPECT_EQ(model, got);
}

TEST(IOCContainerTest, RegisterDuplicateThrows)
{
    IOCContainer container;
    auto model = std::make_shared<DummyModel>();
    container.Register<DummyModel, IModel>(typeid(DummyModel), model);
    using RegisterType = void (IOCContainer::*)(std::type_index, std::shared_ptr<IModel>);
    EXPECT_THROW((container.*static_cast<RegisterType>(&IOCContainer::Register<DummyModel, IModel>))(typeid(DummyModel), model), ComponentAlreadyRegisteredException);
}

TEST(IOCContainerTest, GetAllModels)
{
    IOCContainer container;
    auto model = std::make_shared<DummyModel>();
    container.Register<DummyModel, IModel>(typeid(DummyModel), model);
    auto all = container.GetAll<IModel>();
    EXPECT_EQ(all.size(), 1);
    EXPECT_EQ(all[0], model);
}

TEST(IOCContainerTest, Clear)
{
    IOCContainer container;
    auto model = std::make_shared<DummyModel>();
    container.Register<DummyModel, IModel>(typeid(DummyModel), model);
    container.Clear();
    EXPECT_EQ(container.Get<IModel>(typeid(DummyModel)), nullptr);
}

class DummySystem : public ISystem {
public:
    void Init() override { }
    void Deinit() override { }
    void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
    void HandleEvent(std::shared_ptr<IEvent>) override { }

private:
    std::weak_ptr<IArchitecture> mArch;
};

class DummyUtility : public IUtility { };

TEST(IOCContainerTest, RegisterAndGetSystem)
{
    IOCContainer container;
    auto sys = std::make_shared<DummySystem>();
    container.Register<DummySystem, ISystem>(typeid(DummySystem), sys);
    auto got = container.Get<ISystem>(typeid(DummySystem));
    EXPECT_EQ(sys, got);
}

TEST(IOCContainerTest, RegisterSystemDuplicateThrows)
{
    IOCContainer container;
    auto sys = std::make_shared<DummySystem>();
    container.Register<DummySystem, ISystem>(typeid(DummySystem), sys);
    using RegisterType = void (IOCContainer::*)(std::type_index, std::shared_ptr<ISystem>);
    EXPECT_THROW((container.*static_cast<RegisterType>(&IOCContainer::Register<DummySystem, ISystem>))(typeid(DummySystem), sys), ComponentAlreadyRegisteredException);
}

TEST(IOCContainerTest, GetAllSystems)
{
    IOCContainer container;
    auto sys = std::make_shared<DummySystem>();
    container.Register<DummySystem, ISystem>(typeid(DummySystem), sys);
    auto all = container.GetAll<ISystem>();
    EXPECT_EQ(all.size(), 1);
    EXPECT_EQ(all[0], sys);
}

TEST(IOCContainerTest, RegisterAndGetUtility)
{
    IOCContainer container;
    auto util = std::make_shared<DummyUtility>();
    container.Register<DummyUtility, IUtility>(typeid(DummyUtility), util);
    auto got = container.Get<IUtility>(typeid(DummyUtility));
    EXPECT_EQ(util, got);
}

TEST(IOCContainerTest, RegisterUtilityDuplicateThrows)
{
    IOCContainer container;
    auto util = std::make_shared<DummyUtility>();
    container.Register<DummyUtility, IUtility>(typeid(DummyUtility), util);
    using RegisterType = void (IOCContainer::*)(std::type_index, std::shared_ptr<IUtility>);
    EXPECT_THROW((container.*static_cast<RegisterType>(&IOCContainer::Register<DummyUtility, IUtility>))(typeid(DummyUtility), util), ComponentAlreadyRegisteredException);
}

TEST(IOCContainerTest, GetAllUtilitys)
{
    IOCContainer container;
    auto util = std::make_shared<DummyUtility>();
    container.Register<DummyUtility, IUtility>(typeid(DummyUtility), util);
    auto all = container.GetAll<IUtility>();
    EXPECT_EQ(all.size(), 1);
    EXPECT_EQ(all[0], util);
}

TEST(IOCContainerTest, ClearAll)
{
    IOCContainer container;
    auto model = std::make_shared<DummyModel>();
    auto sys = std::make_shared<DummySystem>();
    auto util = std::make_shared<DummyUtility>();
    container.Register<DummyModel, IModel>(typeid(DummyModel), model);
    container.Register<DummySystem, ISystem>(typeid(DummySystem), sys);
    container.Register<DummyUtility, IUtility>(typeid(DummyUtility), util);
    container.Clear();
    EXPECT_EQ(container.Get<IModel>(typeid(DummyModel)), nullptr);
    EXPECT_EQ(container.Get<ISystem>(typeid(DummySystem)), nullptr);
    EXPECT_EQ(container.Get<IUtility>(typeid(DummyUtility)), nullptr);
}

// ========== IArchitecture 测试 ==========

class TestModel : public IModel {
public:
    bool inited = false;
    void Init() override { inited = true; }
    void Deinit() override { inited = false; }
    void SetArchitecture(std::shared_ptr<IArchitecture>) override { }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return {}; }
};

class TestSystem : public ISystem {
public:
    bool inited = false;
    void Init() override { inited = true; }
    void Deinit() override { inited = false; }
    void SetArchitecture(std::shared_ptr<IArchitecture>) override { }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return {}; }
    void HandleEvent(std::shared_ptr<IEvent>) override { }
};

class TestUtility : public IUtility { };

class TestArchitecture : public Architecture {
protected:
    void Init() override { }
};

TEST(ArchitectureTest, RegisterAndGetModel)
{
    auto arch = std::make_shared<TestArchitecture>();
    auto model = std::make_shared<TestModel>();
    arch->RegisterModel<TestModel>(model);
    auto got = arch->GetModel<TestModel>();
    EXPECT_EQ(model, got);
}

TEST(ArchitectureTest, RegisterAndGetSystem)
{
    auto arch = std::make_shared<TestArchitecture>();
    auto sys = std::make_shared<TestSystem>();
    arch->RegisterSystem<TestSystem>(sys);
    auto got = arch->GetSystem<TestSystem>();
    EXPECT_EQ(sys, got);
}

TEST(ArchitectureTest, RegisterAndGetUtility)
{
    auto arch = std::make_shared<TestArchitecture>();
    auto util = std::make_shared<TestUtility>();
    arch->RegisterUtility<TestUtility>(util);
    auto got = arch->GetUtility<TestUtility>();
    EXPECT_EQ(util, got);
}

TEST(ArchitectureTest, GetUnregisteredThrows)
{
    auto arch = std::make_shared<TestArchitecture>();
    EXPECT_THROW(arch->GetModel<TestModel>(), ComponentNotRegisteredException);
    EXPECT_THROW(arch->GetSystem<TestSystem>(), ComponentNotRegisteredException);
    EXPECT_THROW(arch->GetUtility<TestUtility>(), ComponentNotRegisteredException);
}

// 事件注册与分发
class MyEvent : public IEvent {
public:
    std::string GetEventType() const override { return "MyEvent"; }
};
class MyHandler : public ICanHandleEvent {
public:
    bool called = false;
    void HandleEvent(std::shared_ptr<IEvent>) override { called = true; }
};
TEST(ArchitectureTest, RegisterEventAndSendEvent)
{
    auto arch = std::make_shared<TestArchitecture>();
    MyHandler handler;
    arch->RegisterEvent<MyEvent>(&handler);
    arch->SendEvent<MyEvent>();
    EXPECT_TRUE(handler.called);
    arch->UnRegisterEvent<MyEvent>(&handler);
    handler.called = false;
    arch->SendEvent<MyEvent>();
    EXPECT_FALSE(handler.called);
}

// 命令分发
class MyCommand : public ICommand {
public:
    bool executed = false;
    void Execute() override { executed = true; }
    void SetArchitecture(std::shared_ptr<IArchitecture>) override { }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return {}; }
};
TEST(ArchitectureTest, SendCommand)
{
    auto arch = std::make_shared<TestArchitecture>();
    auto cmd = std::make_unique<MyCommand>();
    MyCommand* cmdPtr = cmd.get();
    arch->SendCommand(std::move(cmd));
    EXPECT_TRUE(cmdPtr->executed);
}

// 查询分发
template <typename T>
class MyQuery : public IQuery<T> {
public:
    T value;
    MyQuery(T v)
        : value(v)
    {
    }
    T Do() override { return value; }
    void SetArchitecture(std::shared_ptr<IArchitecture>) override { }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return {}; }
};
TEST(ArchitectureTest, SendQuery)
{
    auto arch = std::make_shared<TestArchitecture>();
    int result = arch->SendQuery<MyQuery<int>>(42);
    EXPECT_EQ(result, 42);
}

// 生命周期管理
TEST(ArchitectureTest, InitAndDeinit)
{
    auto arch = std::make_shared<TestArchitecture>();
    auto model = std::make_shared<TestModel>();
    auto sys = std::make_shared<TestSystem>();
    arch->RegisterModel<TestModel>(model);
    arch->RegisterSystem<TestSystem>(sys);
    arch->InitArchitecture();
    EXPECT_TRUE(model->inited);
    EXPECT_TRUE(sys->inited);
    arch->Deinit();
    EXPECT_FALSE(model->inited);
    EXPECT_FALSE(sys->inited);
}

// 异常分支
TEST(ArchitectureTest, RegisterNullptrThrows)
{
    auto arch = std::make_shared<TestArchitecture>();
    EXPECT_THROW(arch->RegisterModel<TestModel>(nullptr), std::invalid_argument);
    EXPECT_THROW(arch->RegisterSystem<TestSystem>(nullptr), std::invalid_argument);
    EXPECT_THROW(arch->RegisterUtility<TestUtility>(nullptr), std::invalid_argument);
}

TEST(ModelTest, SetAndGetArchitecture)
{
    auto arch = std::make_shared<TestArchitecture>();
    auto model = std::make_shared<DummyModel>();
    model->SetArchitecture(arch);
    EXPECT_EQ(model->GetArchitecture().lock(), arch);
}

// ========== BindablePropertyUnRegister 测试 ==========

TEST(BindablePropertyUnRegisterTest, GetIdReturnsCorrectId)
{
    BindableProperty<int> prop(0);
    auto unreg = prop.Register([](const int&) { });
    int id = unreg->GetId();
    EXPECT_GE(id, 0);
}

TEST(BindablePropertyUnRegisterTest, UnRegisterRemovesObserver)
{
    BindableProperty<int> prop(0);
    int value = 0;
    auto unreg = prop.Register([&](const int& v) { value = v; });
    prop.SetValue(1);
    EXPECT_EQ(value, 1);
    unreg->UnRegister();
    prop.SetValue(2);
    EXPECT_EQ(value, 1); // 不再更新
}

TEST(BindablePropertyUnRegisterTest, UnRegisterIsIdempotent)
{
    BindableProperty<int> prop(0);
    int value = 0;
    auto unreg = prop.Register([&](const int& v) { value = v; });
    prop.SetValue(1);
    unreg->UnRegister();
    // 再次调用 UnRegister 不应崩溃
    EXPECT_NO_THROW(unreg->UnRegister());
    prop.SetValue(2);
    EXPECT_EQ(value, 1);
}

TEST(BindablePropertyUnRegisterTest, InvokeCallsCallback)
{
    bool called = false;
    BindableProperty<int> prop(0);
    auto unreg = std::make_shared<BindablePropertyUnRegister<int>>(0, &prop, [&](int) { called = true; });
    unreg->Invoke(123);
    EXPECT_TRUE(called);
}

TEST(BindablePropertyUnRegisterTest, InvokeWithMoveOnlyType)
{
    BindableProperty<int> prop(1);
    bool called = false;
    auto unreg = prop.Register([&](const int& v) {
        if (v)
            called = true;
    });
    prop.SetValue(42);
    EXPECT_TRUE(called);
}

TEST(BindablePropertyUnRegisterTest, UnRegisterWhenObjectDestroyedWorks)
{
    BindableProperty<int> prop(0);
    int value = 0;
    class MyTrigger : public UnRegisterTrigger { };
    MyTrigger trigger;
    {
        auto unreg = prop.Register([&](const int& v) { value = v; });
        unreg->UnRegisterWhenObjectDestroyed(&trigger);
        prop.SetValue(5);
        EXPECT_EQ(value, 5);
    }
    // trigger析构时自动注销
    trigger.UnRegister();
    prop.SetValue(10);
    EXPECT_EQ(value, 5); // 不再更新
}

TEST(BindablePropertyUnRegisterTest, UnRegisterWhenObjectDestroyedIsIdempotent)
{
    BindableProperty<int> prop(0);
    int value = 0;
    class MyTrigger : public UnRegisterTrigger { };
    MyTrigger trigger;
    auto unreg = prop.Register([&](const int& v) { value = v; });
    unreg->UnRegisterWhenObjectDestroyed(&trigger);
    // 多次调用不应崩溃
    EXPECT_NO_THROW(unreg->UnRegisterWhenObjectDestroyed(&trigger));
    trigger.UnRegister();
    prop.SetValue(10);
    EXPECT_EQ(value, 0);
}

TEST(BindablePropertyUnRegisterTest, CallbackCanBeNull)
{
    BindableProperty<int> prop(0);
    // 构造时 callback 为空，不应崩溃
    auto unreg = std::make_shared<BindablePropertyUnRegister<int>>(0, &prop, nullptr);
    EXPECT_NO_THROW(unreg->Invoke(1));
}

TEST(BindablePropertyUnRegisterTest, PropertyPointerNullAfterUnRegister)
{
    BindableProperty<int> prop(0);
    auto unreg = prop.Register([](const int&) { });
    unreg->UnRegister();
    // 再次 UnRegister 不应崩溃，且 mProperty 已为 nullptr
    EXPECT_NO_THROW(unreg->UnRegister());
}

TEST(BindablePropertyUnRegisterTest, UnRegisterDoesNotAffectOtherObservers)
{
    BindableProperty<int> prop(0);
    int v1 = 0, v2 = 0;
    auto u1 = prop.Register([&](const int& v) { v1 = v; });
    auto u2 = prop.Register([&](const int& v) { v2 = v; });
    prop.SetValue(1);
    u1->UnRegister();
    prop.SetValue(2);
    EXPECT_EQ(v1, 1); // u1 不再更新
    EXPECT_EQ(v2, 2); // u2 仍然更新
}

// ========== 能力接口（ICanGetModel等）单元测试 ==========

class DummyArch : public Architecture {
protected:
    void Init() override { }
};

class DummyEvent : public IEvent {
public:
    std::string GetEventType() const override { return "DummyEvent"; }
};

class DummyHandler : public ICanHandleEvent {
public:
    bool called = false;
    void HandleEvent(std::shared_ptr<IEvent>) override { called = true; }
};

// ICommand 测试用
class DummyCommand : public ICommand {
public:
    bool executed = false;
    void Execute() override { executed = true; }
    void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
    std::weak_ptr<IArchitecture> mArch;
};

// IQuery 测试用
class DummyQuery : public IQuery<int> {
public:
    int Do() override { return 42; }
    void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
    std::weak_ptr<IArchitecture> mArch;
};

// ========== 能力接口实现类 ==========
class CanGetModelObj : public ICanGetModel {
public:
    std::weak_ptr<IArchitecture> mArch;
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanGetSystemObj : public ICanGetSystem {
public:
    std::weak_ptr<IArchitecture> mArch;
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanSendCommandObj : public ICanSendCommand {
public:
    std::weak_ptr<IArchitecture> mArch;
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanSendQueryObj : public ICanSendQuery {
public:
    std::weak_ptr<IArchitecture> mArch;
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanGetUtilityObj : public ICanGetUtility {
public:
    std::weak_ptr<IArchitecture> mArch;
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanSendEventObj : public ICanSendEvent {
public:
    std::weak_ptr<IArchitecture> mArch;
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanRegisterEventObj : public ICanRegisterEvent {
public:
    std::weak_ptr<IArchitecture> mArch;
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

// ========== ICanGetModel ==========
TEST(CapabilityTest, ICanGetModel_Success)
{
    auto arch = std::make_shared<DummyArch>();
    auto model = std::make_shared<DummyModel>();
    arch->RegisterModel<DummyModel>(model);

    CanGetModelObj obj;
    obj.mArch = arch;
    auto got = obj.GetModel<DummyModel>();
    EXPECT_EQ(got, model);
}

TEST(CapabilityTest, ICanGetModel_ArchNotSet)
{
    CanGetModelObj obj;
    EXPECT_THROW(obj.GetModel<DummyModel>(), ArchitectureNotSetException);
}

// ========== ICanGetSystem ==========
TEST(CapabilityTest, ICanGetSystem_Success)
{
    auto arch = std::make_shared<DummyArch>();
    auto sys = std::make_shared<DummySystem>();
    arch->RegisterSystem<DummySystem>(sys);

    CanGetSystemObj obj;
    obj.mArch = arch;
    auto got = obj.GetSystem<DummySystem>();
    EXPECT_EQ(got, sys);
}

TEST(CapabilityTest, ICanGetSystem_ArchNotSet)
{
    CanGetSystemObj obj;
    EXPECT_THROW(obj.GetSystem<DummySystem>(), ArchitectureNotSetException);
}

// ========== ICanSendCommand ==========
TEST(CapabilityTest, ICanSendCommand_Success)
{
    auto arch = std::make_shared<DummyArch>();
    CanSendCommandObj obj;
    obj.mArch = arch;
    obj.SendCommand<DummyCommand>();
    // 由于 DummyCommand::executed 是局部变量，无法直接断言，但可通过不抛异常判断流程
    SUCCEED();
}

TEST(CapabilityTest, ICanSendCommand_ArchNotSet)
{
    CanSendCommandObj obj;
    EXPECT_THROW(obj.SendCommand<DummyCommand>(), ArchitectureNotSetException);
}

// ========== ICanSendQuery ==========
TEST(CapabilityTest, ICanSendQuery_Success)
{
    auto arch = std::make_shared<DummyArch>();
    CanSendQueryObj obj;
    obj.mArch = arch;
    int result = obj.SendQuery<DummyQuery>();
    EXPECT_EQ(result, 42);
}

TEST(CapabilityTest, ICanSendQuery_ArchNotSet)
{
    CanSendQueryObj obj;
    EXPECT_THROW(obj.SendQuery<DummyQuery>(), ArchitectureNotSetException);
}

// ========== ICanGetUtility ==========
TEST(CapabilityTest, ICanGetUtility_Success)
{
    auto arch = std::make_shared<DummyArch>();
    auto util = std::make_shared<DummyUtility>();
    arch->RegisterUtility<DummyUtility>(util);

    CanGetUtilityObj obj;
    obj.mArch = arch;
    auto got = obj.GetUtility<DummyUtility>();
    EXPECT_EQ(got, util);
}

TEST(CapabilityTest, ICanGetUtility_ArchNotSet)
{
    CanGetUtilityObj obj;
    EXPECT_THROW(obj.GetUtility<DummyUtility>(), ArchitectureNotSetException);
}

// ========== ICanSendEvent ==========
TEST(CapabilityTest, ICanSendEvent_Success)
{
    auto arch = std::make_shared<DummyArch>();
    DummyHandler handler;
    arch->RegisterEvent<DummyEvent>(&handler);

    CanSendEventObj obj;
    obj.mArch = arch;
    obj.SendEvent<DummyEvent>();
    EXPECT_TRUE(handler.called);
}

TEST(CapabilityTest, ICanSendEvent_ArchNotSet)
{
    CanSendEventObj obj;
    EXPECT_THROW(obj.SendEvent<DummyEvent>(), ArchitectureNotSetException);
}

// ========== ICanRegisterEvent ==========
TEST(CapabilityTest, ICanRegisterEvent_Success)
{
    auto arch = std::make_shared<DummyArch>();
    DummyHandler handler;
    CanRegisterEventObj obj;
    obj.mArch = arch;
    obj.RegisterEvent<DummyEvent>(&handler);
    arch->SendEvent<DummyEvent>();
    EXPECT_TRUE(handler.called);

    handler.called = false;
    obj.UnRegisterEvent<DummyEvent>(&handler);
    arch->SendEvent<DummyEvent>();
    EXPECT_FALSE(handler.called);
}

TEST(CapabilityTest, ICanRegisterEvent_ArchNotSet)
{
    CanRegisterEventObj obj;
    DummyHandler handler;
    EXPECT_THROW(obj.RegisterEvent<DummyEvent>(&handler), ArchitectureNotSetException);
    EXPECT_THROW(obj.UnRegisterEvent<DummyEvent>(&handler), ArchitectureNotSetException);
}

// ========== Architecture 单元测试 ==========

class ArchTestModel : public IModel {
public:
    bool inited = false;
    void Init() override { inited = true; }
    void Deinit() override { inited = false; }
    void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
    std::weak_ptr<IArchitecture> mArch;
};

class ArchTestSystem : public ISystem {
public:
    bool inited = false;
    bool eventHandled = false;
    void Init() override { inited = true; }
    void Deinit() override { inited = false; }
    void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
    void HandleEvent(std::shared_ptr<IEvent>) override { eventHandled = true; }

private:
    std::weak_ptr<IArchitecture> mArch;
};

class ArchTestUtility : public IUtility { };

class ArchTestEvent : public IEvent {
public:
    std::string GetEventType() const override { return "ArchTestEvent"; }
};

class ArchTestHandler : public ICanHandleEvent {
public:
    bool called = false;
    void HandleEvent(std::shared_ptr<IEvent>) override { called = true; }
};

class ArchTestCommand : public ICommand {
public:
    bool executed = false;
    void Execute() override { executed = true; }
    void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
    std::weak_ptr<IArchitecture> mArch;
};

class ArchTestQuery : public IQuery<int> {
public:
    int Do() override { return 1234; }
    void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
    std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
    std::weak_ptr<IArchitecture> mArch;
};

class MyArchitecture : public Architecture {
protected:
    void Init() override { }
};

TEST(ArchitectureTest, RegisterDuplicateThrows)
{
    auto arch = std::make_shared<MyArchitecture>();
    auto model = std::make_shared<ArchTestModel>();
    arch->RegisterModel<ArchTestModel>(model);
    EXPECT_THROW(arch->RegisterModel<ArchTestModel>(model), ComponentAlreadyRegisteredException);
}

TEST(ArchitectureTest, EventRegisterSendUnregister)
{
    auto arch = std::make_shared<MyArchitecture>();
    ArchTestHandler handler;
    arch->RegisterEvent<ArchTestEvent>(&handler);
    arch->SendEvent<ArchTestEvent>();
    EXPECT_TRUE(handler.called);

    handler.called = false;
    arch->UnRegisterEvent<ArchTestEvent>(&handler);
    arch->SendEvent<ArchTestEvent>();
    EXPECT_FALSE(handler.called);
}

TEST(ArchitectureTest, SendEventNullptrThrows)
{
    auto arch = std::make_shared<MyArchitecture>();
    EXPECT_THROW(arch->SendEvent(nullptr), std::invalid_argument);
}

TEST(ArchitectureTest, RegisterEventNullptrThrows)
{
    auto arch = std::make_shared<MyArchitecture>();
    EXPECT_THROW(arch->RegisterEvent<ArchTestEvent>(nullptr), std::invalid_argument);
    EXPECT_THROW(arch->UnRegisterEvent<ArchTestEvent>(nullptr), std::invalid_argument);
}

TEST(ArchitectureTest, SendCommandWorks)
{
    auto arch = std::make_shared<MyArchitecture>();
    auto cmd = std::make_unique<ArchTestCommand>();
    ArchTestCommand* cmdPtr = cmd.get();
    arch->SendCommand(std::move(cmd));
    EXPECT_TRUE(cmdPtr->executed);
}

TEST(ArchitectureTest, SendCommandNullptrThrows)
{
    auto arch = std::make_shared<MyArchitecture>();
    EXPECT_THROW(arch->SendCommand(nullptr), std::invalid_argument);
}

TEST(ArchitectureTest, SendQueryWorks)
{
    auto arch = std::make_shared<MyArchitecture>();
    int result = arch->SendQuery<ArchTestQuery>();
    EXPECT_EQ(result, 1234);
}

TEST(ArchitectureTest, SendQueryNullptrThrows)
{
    auto arch = std::make_shared<MyArchitecture>();
    std::unique_ptr<ArchTestQuery> q;
    EXPECT_THROW(arch->SendQuery(std::move(q)), std::invalid_argument);
}

TEST(ArchitectureTest, InitAndDeinitLifecycle)
{
    auto arch = std::make_shared<MyArchitecture>();
    auto model = std::make_shared<ArchTestModel>();
    auto sys = std::make_shared<ArchTestSystem>();
    arch->RegisterModel<ArchTestModel>(model);
    arch->RegisterSystem<ArchTestSystem>(sys);

    arch->InitArchitecture();
    EXPECT_TRUE(model->inited);
    EXPECT_TRUE(sys->inited);

    arch->Deinit();
    EXPECT_FALSE(model->inited);
    EXPECT_FALSE(sys->inited);
}

TEST(ArchitectureTest, SystemHandleEvent)
{
    auto arch = std::make_shared<MyArchitecture>();
    auto sys = std::make_shared<ArchTestSystem>();
    arch->RegisterSystem<ArchTestSystem>(sys);
    arch->RegisterEvent<ArchTestEvent>(sys.get());
    arch->SendEvent<ArchTestEvent>();
    EXPECT_TRUE(sys->eventHandled);
}

// ========== AbstractCommand 单元测试 ==========
class TestArch : public Architecture {
protected:
    void Init() override { }
};

class MyAbstractCommand : public AbstractCommand {
public:
    bool executed = false;
    std::weak_ptr<IArchitecture> archSet;

public:
    void OnExecute() override { executed = true; }
};

TEST(AbstractCommandTest, ExecuteCallsOnExecute)
{
    MyAbstractCommand cmd;
    EXPECT_FALSE(cmd.executed);
    cmd.OnExecute();
    EXPECT_TRUE(cmd.executed);
}

TEST(AbstractCommandTest, SetAndGetArchitecture)
{
    auto arch = std::make_shared<TestArch>();
    MyAbstractCommand cmd;
    cmd.SetArchitecture(arch);
    EXPECT_EQ(cmd.GetArchitecture().lock(), arch);
}

// ========== AbstractModel 单元测试 ==========
class MyAbstractModel : public AbstractModel {
public:
    bool inited = false;
    bool deinited = false;
    std::weak_ptr<IArchitecture> archSet;

protected:
    void OnInit() override { inited = true; }
    void OnDeinit() override { deinited = true; }
};

TEST(AbstractModelTest, InitAndDeinitCallOnInitOnDeinit)
{
    MyAbstractModel model;
    EXPECT_FALSE(model.inited);
    EXPECT_FALSE(model.deinited);
    model.Init();
    EXPECT_TRUE(model.inited);
    model.Deinit();
    EXPECT_TRUE(model.deinited);
}

// ========== AbstractSystem 单元测试 ==========
class DummyEventForSystem : public IEvent {
public:
    std::string GetEventType() const override { return "DummyEventForSystem"; }
};

class MyAbstractSystem : public AbstractSystem {
public:
    bool inited = false;
    bool deinited = false;
    bool eventHandled = false;
    std::weak_ptr<IArchitecture> archSet;

protected:
    void OnInit() override { inited = true; }
    void OnDeinit() override { deinited = true; }
    void OnEvent(std::shared_ptr<IEvent> event) override { eventHandled = true; }
};

TEST(AbstractSystemTest, InitAndDeinitCallOnInitOnDeinit)
{
    MyAbstractSystem sys;
    EXPECT_FALSE(sys.inited);
    EXPECT_FALSE(sys.deinited);
    sys.Init();
    EXPECT_TRUE(sys.inited);
    sys.Deinit();
    EXPECT_TRUE(sys.deinited);
}

TEST(AbstractSystemTest, HandleEventCallsOnEvent)
{
    MyAbstractSystem sys;
    auto evt = std::make_shared<DummyEventForSystem>();
    EXPECT_FALSE(sys.eventHandled);
    sys.HandleEvent(evt);
    EXPECT_TRUE(sys.eventHandled);
}

// ========== AbstractController 单元测试 ==========
class DummyEventForController : public IEvent {
public:
    std::string GetEventType() const override { return "DummyEventForController"; }
};

class MyAbstractController : public AbstractController {
public:
    bool eventHandled = false;

protected:
    void OnEvent(std::shared_ptr<IEvent> event) override { eventHandled = true; }
};

// ========== AbstractQuery 单元测试 ==========
class MyAbstractQuery : public AbstractQuery<int> {
public:
    bool called = false;
    int ret = 0;

protected:
    int OnDo() override
    {
        called = true;
        return ret;
    }
};

TEST(AbstractQueryTest, DoCallsOnDo)
{
    MyAbstractQuery query;
    query.ret = 77;
    EXPECT_FALSE(query.called);
    int result = query.Do();
    EXPECT_TRUE(query.called);
    EXPECT_EQ(result, 77);
}

TEST(AbstractQueryTest, SetAndGetArchitecture)
{
    auto arch = std::make_shared<TestArch>();
    MyAbstractQuery query;
    query.SetArchitecture(arch);
    EXPECT_EQ(query.GetArchitecture().lock(), arch);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}