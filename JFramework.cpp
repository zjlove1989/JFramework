// JFramework.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <gtest/gtest.h>
#include "JFramework.h"
using namespace JFramework;


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
	bool eventHandled = false;
	void HandleEvent(std::shared_ptr<IEvent> event) override
	{
		eventHandled = true;
	}

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

	container.Register<TestModel,IModel>(typeid(TestModel), model1);
	container.Register<TestModel,IModel>(typeid(TestModel), model2);

	auto allModels = container.GetAll<IModel>();
	EXPECT_EQ(1, allModels.size());
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
	arch->RegisterEvent(typeid(TestEvent), & handler);
	
	auto event = std::make_shared<TestEvent>();
	arch->SendEvent(event);

	EXPECT_TRUE(handler.eventHandled);
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

	auto unregister = prop.Register([&](int) { notified = true; });
	prop.SetValue(20);
	EXPECT_TRUE(notified);

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

// 能力接口测试
TEST(CapabilityTest, CanGetModel)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	class TestComponent : public ICanGetModel
	{
	public:
		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return std::make_shared<TestArchitecture>();
		}
	};

	TestComponent component;
	EXPECT_THROW(component.GetModel<TestModel>(), ArchitectureNotSetException);
}

TEST(CapabilityTest, CanSendCommand)
{
	auto arch = std::make_shared<TestArchitecture>();
	arch->InitArchitecture();

	class TestComponent : public ICanSendCommand
	{
	public:
		bool commandSent = false;
		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return std::make_shared<TestArchitecture>();
		}
	};

	TestComponent component;
	EXPECT_THROW(component.SendCommand<TestCommand>(), ArchitectureNotSetException);
}


int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
