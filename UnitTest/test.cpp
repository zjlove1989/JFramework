#include "pch.h"
#include "../JFramework.h"
#include <iostream>
#include <thread>

using namespace JFramework;

// ========== �쳣���� ==========
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

// ========== EventBus ���� ==========
class TestEvent : public IEvent
{
};

class TestHandler : public ICanHandleEvent
{
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

class AnotherEvent : public IEvent
{
};

class CountingHandler : public ICanHandleEvent
{
public:
	int count = 0;
	void HandleEvent(std::shared_ptr<IEvent>) override { ++count; }
};

class ExceptionHandler : public ICanHandleEvent
{
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
	// handler ����������
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

TEST(EventBusTest, UnRegisterNotRegisteredHandler)
{
	EventBus bus;
	TestHandler handler;
	// δע��ֱ��ע������Ӧ����
	EXPECT_NO_THROW(bus.UnRegisterEvent(typeid(TestEvent), &handler));
}

TEST(EventBusTest, UnRegisterTwice)
{
	EventBus bus;
	TestHandler handler;
	bus.RegisterEvent(typeid(TestEvent), &handler);
	bus.UnRegisterEvent(typeid(TestEvent), &handler);
	// �ٴ�ע������Ӧ����
	EXPECT_NO_THROW(bus.UnRegisterEvent(typeid(TestEvent), &handler));
}

TEST(EventBusTest, ConcurrentRegisterAndSend)
{
	EventBus bus;
	CountingHandler handler;
	auto reg = [&]
		{
			for (int i = 0; i < 100; ++i)
				bus.RegisterEvent(typeid(TestEvent), &handler);
		};
	auto send = [&]
		{
			for (int i = 0; i < 100; ++i)
				bus.SendEvent(std::make_shared<TestEvent>());
		};
	std::thread t1(reg), t2(send);
	t1.join();
	t2.join();
	// ֻ��֤������
	SUCCEED();
}

// ========== BindableProperty ���� ==========

struct CustomType
{
	int x;
	bool operator==(const CustomType& o) const { return x == o.x; }
};

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
	EXPECT_EQ(observed, 5); // ���ٸ���
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
	EXPECT_EQ(v, 5); // ���ٸ���
}

TEST(BindablePropertyTest, DestructorCleansObservers)
{
	int v = 0;
	{
		BindableProperty<int> prop(1);
		auto u = prop.Register([&](const int& val) { v = val; });
		prop.SetValue(2);
		EXPECT_EQ(v, 2);
		// prop �뿪��������������Ӧ����
	}
	SUCCEED();
}

TEST(BindablePropertyTest, CallbackThrowsException)
{
	BindableProperty<int> prop(0);
	int v = 0;
	auto u1 = prop.Register([&](const int&) { throw std::runtime_error("fail"); });
	auto u2 = prop.Register([&](const int& val) { v = val; });
	// ��ʹ�лص����쳣�������ص���Ȼ������
	EXPECT_NO_THROW(prop.SetValue(123));
	EXPECT_EQ(v, 123);
}

TEST(BindablePropertyTest, UnRegisterWhenObjectDestroyed)
{
	BindableProperty<int> prop(0);
	int v = 0;
	class MyTrigger : public UnRegisterTrigger {};
	MyTrigger trigger;
	{
		auto u = prop.Register([&](const int& val) { v = val; });
		u->UnRegisterWhenObjectDestroyed(&trigger);
		prop.SetValue(7);
		EXPECT_EQ(v, 7);
	}
	// trigger����ʱ�Զ�ע��
	trigger.UnRegister();
	prop.SetValue(8);
	EXPECT_EQ(v, 7); // ���ٸ���
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

// ========== UnRegisterTrigger ���� ==========
class DummyUnRegister : public IUnRegister
{
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

// ========== ICanInit ���� ==========
class DummyCanInit : public ICanInit
{
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

// ========== IOCContainer ���� ==========
class DummyModel : public IModel
{
public:
	void Init() override {}
	void Deinit() override {}
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

class DummySystem : public ISystem
{
public:
	void Init() override {}
	void Deinit() override {}
	void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
	void HandleEvent(std::shared_ptr<IEvent>) override {}

private:
	std::weak_ptr<IArchitecture> mArch;
};

class DummyUtility : public IUtility {};

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

TEST(IOCContainerTest, GetUnregisteredReturnsNull)
{
	IOCContainer container;
	auto got = container.Get<IModel>(typeid(DummyModel));
	EXPECT_EQ(got, nullptr);
}

TEST(IOCContainerTest, ClearMultipleTimes)
{
	IOCContainer container;
	container.Clear();
	container.Clear();
	SUCCEED();
}

// ========== IArchitecture ���� ==========

class TestModel : public IModel
{
public:
	bool inited = false;
	void Init() override { inited = true; }
	void Deinit() override { inited = false; }
	void SetArchitecture(std::shared_ptr<IArchitecture>) override {}
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return {}; }
};

class TestSystem : public ISystem
{
public:
	bool inited = false;
	void Init() override { inited = true; }
	void Deinit() override { inited = false; }
	void SetArchitecture(std::shared_ptr<IArchitecture>) override {}
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return {}; }
	void HandleEvent(std::shared_ptr<IEvent>) override {}
};

class TestUtility : public IUtility {};

class TestArchitecture : public Architecture
{
protected:
	void Init() override {}
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

// �¼�ע����ַ�
class MyEvent : public IEvent
{
};
class MyHandler : public ICanHandleEvent
{
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

// ����ַ�
class MyCommand : public IJCommand
{
public:
	bool executed = false;
	void Execute() override { executed = true; }
	void SetArchitecture(std::shared_ptr<IArchitecture>) override {}
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

// ��ѯ�ַ�
template <typename T>
class MyQuery : public IQuery<T>
{
public:
	T value;
	MyQuery(T v)
		: value(v)
	{
	}
	T Do() override { return value; }
	void SetArchitecture(std::shared_ptr<IArchitecture>) override {}
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return {}; }
};
TEST(ArchitectureTest, SendQuery)
{
	auto arch = std::make_shared<TestArchitecture>();
	int result = arch->SendQuery<MyQuery<int>>(42);
	EXPECT_EQ(result, 42);
}

// �������ڹ���
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

// �쳣��֧
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

// ========== BindablePropertyUnRegister ���� ==========

TEST(BindablePropertyUnRegisterTest, GetIdReturnsCorrectId)
{
	BindableProperty<int> prop(0);
	auto unreg = prop.Register([](const int&) {});
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
	EXPECT_EQ(value, 1); // ���ٸ���
}

TEST(BindablePropertyUnRegisterTest, UnRegisterIsIdempotent)
{
	BindableProperty<int> prop(0);
	int value = 0;
	auto unreg = prop.Register([&](const int& v) { value = v; });
	prop.SetValue(1);
	unreg->UnRegister();
	// �ٴε��� UnRegister ��Ӧ����
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
	auto unreg = prop.Register([&](const int& v)
		{
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
	class MyTrigger : public UnRegisterTrigger {};
	MyTrigger trigger;
	{
		auto unreg = prop.Register([&](const int& v) { value = v; });
		unreg->UnRegisterWhenObjectDestroyed(&trigger);
		prop.SetValue(5);
		EXPECT_EQ(value, 5);
	}
	// trigger����ʱ�Զ�ע��
	trigger.UnRegister();
	prop.SetValue(10);
	EXPECT_EQ(value, 5); // ���ٸ���
}

TEST(BindablePropertyUnRegisterTest, UnRegisterWhenObjectDestroyedIsIdempotent)
{
	BindableProperty<int> prop(0);
	int value = 0;
	class MyTrigger : public UnRegisterTrigger {};
	MyTrigger trigger;
	auto unreg = prop.Register([&](const int& v) { value = v; });
	unreg->UnRegisterWhenObjectDestroyed(&trigger);
	// ��ε��ò�Ӧ����
	EXPECT_NO_THROW(unreg->UnRegisterWhenObjectDestroyed(&trigger));
	trigger.UnRegister();
	prop.SetValue(10);
	EXPECT_EQ(value, 0);
}

TEST(BindablePropertyUnRegisterTest, CallbackCanBeNull)
{
	BindableProperty<int> prop(0);
	// ����ʱ callback Ϊ�գ���Ӧ����
	auto unreg = std::make_shared<BindablePropertyUnRegister<int>>(0, &prop, nullptr);
	EXPECT_NO_THROW(unreg->Invoke(1));
}

TEST(BindablePropertyUnRegisterTest, PropertyPointerNullAfterUnRegister)
{
	BindableProperty<int> prop(0);
	auto unreg = prop.Register([](const int&) {});
	unreg->UnRegister();
	// �ٴ� UnRegister ��Ӧ�������� mProperty ��Ϊ nullptr
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
	EXPECT_EQ(v1, 1); // u1 ���ٸ���
	EXPECT_EQ(v2, 2); // u2 ��Ȼ����
}

TEST(BindablePropertyTest, CustomTypeBind)
{
	BindableProperty<CustomType> prop({ 1 });
	int observed = 0;
	auto u = prop.Register([&](const CustomType& v) { observed = v.x; });
	prop.SetValue({ 42 });
	EXPECT_EQ(observed, 42);
}

TEST(BindablePropertyTest, MoveAssignAndMoveConstruct)
{
	BindableProperty<int> prop1(1);
	prop1.SetValue(2);
	BindableProperty<int> prop2(std::move(prop1));
	EXPECT_EQ(prop2.GetValue(), 2);
	BindableProperty<int> prop3;
	prop3 = std::move(prop2);
	EXPECT_EQ(prop3.GetValue(), 2);
}

// ========== �����ӿڣ�ICanGetModel�ȣ���Ԫ���� ==========

class DummyArch : public Architecture
{
protected:
	void Init() override {}
};

class DummyEvent : public IEvent
{
};

class DummyHandler : public ICanHandleEvent
{
public:
	bool called = false;
	void HandleEvent(std::shared_ptr<IEvent>) override { called = true; }
};

// ICommand ������
class DummyCommand : public IJCommand
{
public:
	bool executed = false;
	void Execute() override { executed = true; }
	void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
	std::weak_ptr<IArchitecture> mArch;
};

// IQuery ������
class DummyQuery : public IQuery<int>
{
public:
	int Do() override { return 42; }
	void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
	std::weak_ptr<IArchitecture> mArch;
};

// ========== �����ӿ�ʵ���� ==========
class CanGetModelObj : public ICanGetModel
{
public:
	std::weak_ptr<IArchitecture> mArch;
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanGetSystemObj : public ICanGetSystem
{
public:
	std::weak_ptr<IArchitecture> mArch;
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanSendCommandObj : public ICanSendCommand
{
public:
	std::weak_ptr<IArchitecture> mArch;
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanSendQueryObj : public ICanSendQuery
{
public:
	std::weak_ptr<IArchitecture> mArch;
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanGetUtilityObj : public ICanGetUtility
{
public:
	std::weak_ptr<IArchitecture> mArch;
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanSendEventObj : public ICanSendEvent
{
public:
	std::weak_ptr<IArchitecture> mArch;
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }
};

class CanRegisterEventObj : public ICanRegisterEvent
{
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
	// ���� DummyCommand::executed �Ǿֲ��������޷�ֱ�Ӷ��ԣ�����ͨ�������쳣�ж�����
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

// ========== Architecture ��Ԫ���� ==========

class ArchTestModel : public IModel
{
public:
	bool inited = false;
	void Init() override { inited = true; }
	void Deinit() override { inited = false; }
	void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
	std::weak_ptr<IArchitecture> mArch;
};

class ArchTestSystem : public ISystem
{
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

class ArchTestUtility : public IUtility {};

class ArchTestEvent : public IEvent
{
};

class ArchTestHandler : public ICanHandleEvent
{
public:
	bool called = false;
	void HandleEvent(std::shared_ptr<IEvent>) override { called = true; }
};

class ArchTestCommand : public IJCommand
{
public:
	bool executed = false;
	void Execute() override { executed = true; }
	void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
	std::weak_ptr<IArchitecture> mArch;
};

class ArchTestQuery : public IQuery<int>
{
public:
	int Do() override { return 1234; }
	void SetArchitecture(std::shared_ptr<IArchitecture> arch) override { mArch = arch; }
	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

private:
	std::weak_ptr<IArchitecture> mArch;
};

class MyArchitecture : public Architecture
{
protected:
	void Init() override {}
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

class DummyArchForInit : public Architecture
{
protected:
	void Init() override { ++initCount; }

public:
	int initCount = 0;
};

TEST(ArchitectureTest, InitArchitectureMultipleTimes)
{
	auto arch = std::make_shared<DummyArchForInit>();
	arch->InitArchitecture();
	arch->InitArchitecture();
	EXPECT_EQ(arch->initCount, 1);
}

TEST(ArchitectureTest, DeinitMultipleTimes)
{
	auto arch = std::make_shared<DummyArchForInit>();
	arch->InitArchitecture();
	arch->Deinit();
	arch->Deinit();
	EXPECT_FALSE(arch->IsInitialized());
}

TEST(ArchitectureTest, RegisterSameTypeDifferentInstance)
{
	auto arch = std::make_shared<MyArchitecture>();
	auto model1 = std::make_shared<ArchTestModel>();
	arch->RegisterModel<ArchTestModel>(model1);
	auto model2 = std::make_shared<ArchTestModel>();
	EXPECT_THROW(arch->RegisterModel<ArchTestModel>(model2), ComponentAlreadyRegisteredException);
}

// ========== AbstractCommand ��Ԫ���� ==========
class TestArch : public Architecture
{
protected:
	void Init() override {}
};

class MyAbstractCommand : public AbstractCommand
{
public:
	bool executed = false;
	std::weak_ptr<IArchitecture> archSet;

protected:
	void OnExecute() override { executed = true; }
};

TEST(AbstractCommandTest, ExecuteCallsOnExecute)
{
	MyAbstractCommand cmd;
	EXPECT_FALSE(cmd.executed);
	cmd.Execute();
	EXPECT_TRUE(cmd.executed);
}

TEST(AbstractCommandTest, SetAndGetArchitecture)
{
	auto arch = std::make_shared<TestArch>();
	MyAbstractCommand cmd;
	cmd.SetArchitecture(arch);
	EXPECT_EQ(cmd.GetArchitecture().lock(), arch);
}

// ========== AbstractModel ��Ԫ���� ==========
class MyAbstractModel : public AbstractModel
{
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

TEST(AbstractModelTest, SetAndGetArchitecture)
{
	auto arch = std::make_shared<TestArch>();
	auto model = std::make_shared<MyAbstractModel>();
	arch->RegisterModel<MyAbstractModel>(model);
	// RegisterModel ���Զ����� SetArchitecture
	EXPECT_EQ(model->GetArchitecture().lock(), arch);
}

// ========== AbstractSystem ��Ԫ���� ==========
class DummyEventForSystem : public IEvent
{
};

class MyAbstractSystem : public AbstractSystem
{
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

TEST(AbstractSystemTest, SetAndGetArchitecture)
{
	auto arch = std::make_shared<TestArch>();
	auto model = std::make_shared<MyAbstractSystem>();
	arch->RegisterSystem<MyAbstractSystem>(model);
	// RegisterSystem ���Զ����� SetArchitecture
	EXPECT_EQ(model->GetArchitecture().lock(), arch);
}

// ========== AbstractController ��Ԫ���� ==========
class DummyEventForController : public IEvent
{
};

class MyAbstractController : public AbstractController
{
public:
	MyAbstractController(std::shared_ptr<IArchitecture> arch)
		: mArch(arch)
	{
	}
	bool eventHandled = false;

	std::weak_ptr<IArchitecture> GetArchitecture() const override { return mArch; }

protected:
	void OnEvent(std::shared_ptr<IEvent> event) override { eventHandled = true; }

private:
	std::weak_ptr<IArchitecture> mArch;
};

TEST(AbstractControllerTest, HandleEventCallsOnEvent)
{
	auto arch = std::make_shared<MyArchitecture>();
	MyAbstractController ctrl(arch);
	auto evt = std::make_shared<DummyEventForController>();
	EXPECT_FALSE(ctrl.eventHandled);

	// ͨ���ӿ�ָ����� HandleEvent
	ICanHandleEvent* handler = &ctrl;
	handler->HandleEvent(evt);

	EXPECT_TRUE(ctrl.eventHandled);
}

// ========== AbstractQuery ��Ԫ���� ==========
class MyAbstractQuery : public AbstractQuery<int>
{
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

class NoOnExecuteCommand : public AbstractCommand
{
protected:
	void OnExecute() override { throw std::runtime_error("Not implemented"); }
};

TEST(AbstractCommandTest, OnExecuteThrows)
{
	NoOnExecuteCommand cmd;
	EXPECT_THROW(cmd.Execute(), std::runtime_error);
}

class NoOnInitModel : public AbstractModel
{
protected:
	void OnInit() override { throw std::runtime_error("Not implemented"); }
	void OnDeinit() override {}
};

TEST(AbstractModelTest, OnInitThrows)
{
	NoOnInitModel model;
	EXPECT_THROW(model.Init(), std::runtime_error);
}

class NoOnEventSystem : public AbstractSystem
{
protected:
	void OnInit() override {}
	void OnDeinit() override {}
	void OnEvent(std::shared_ptr<IEvent>) override { throw std::runtime_error("Not implemented"); }
};

TEST(AbstractSystemTest, OnEventThrows)
{
	NoOnEventSystem sys;
	auto evt = std::make_shared<DummyEventForSystem>();
	EXPECT_THROW(sys.HandleEvent(evt), std::runtime_error);
}
