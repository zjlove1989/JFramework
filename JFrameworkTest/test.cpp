
#include "pch.h"
#include "../JFramework.h"

namespace
{

	using namespace JFramework;

	class TypeEventSystemTest : public ::testing::Test
	{
	protected:
		TypeEventSystem system;
	};

	TEST_F(TypeEventSystemTest, RegisterAndTriggerEvent)
	{
		std::atomic_int counter{ 0 };

		// 注册事件处理器
		auto unregister = system.Register<int>([&](int* e)
			{
				counter.fetch_add(1);
			});

		// 发送事件
		int value = 42;
		system.Send(&value);

		EXPECT_EQ(counter.load(), 1);
	}

	TEST_F(TypeEventSystemTest, UnregisterStopsReceivingEvents)
	{
		std::atomic_int counter{ 0 };

		// 注册并立即注销
		{
			auto unregister = system.Register<int>([&](int* e)
				{
					counter.fetch_add(1);
				});
			unregister->UnRegister();
		} // 超出作用域自动注销

		int value = 42;
		system.Send(&value);

		EXPECT_EQ(counter.load(), 0);
	}

	TEST_F(TypeEventSystemTest, MultipleHandlersWork)
	{
		std::atomic_int counter{ 0 };

		auto unreg1 = system.Register<int>([&](int* e) { counter += 1; });
		auto unreg2 = system.Register<int>([&](int* e) { counter += 2; });

		int value = 42;
		system.Send(&value);

		EXPECT_EQ(counter.load(), 3);
	}

	TEST_F(TypeEventSystemTest, DefaultConstructibleEvent)
	{
		struct TestEvent { int value = 42; };

		std::atomic_int receivedValue{ 0 };
		auto unreg = system.Register<TestEvent>([&](TestEvent* e)
			{
				receivedValue = e->value;
			});

		system.Send<TestEvent>();  // 使用默认构造
		EXPECT_EQ(receivedValue.load(), 42);
	}


	TEST_F(TypeEventSystemTest, EventDataIntegrity)
	{
		struct CustomEvent
		{
			std::string message;
			int value;
		};

		std::string receivedMessage;
		int receivedValue = 0;

		auto unreg = system.Register<CustomEvent>([&](CustomEvent* e)
			{
				receivedMessage = e->message;
				receivedValue = e->value;
			});

		CustomEvent event;
		event.message = "Test Message";
		event.value = 12345;
		system.Send(&event);

		EXPECT_EQ(receivedMessage, "Test Message");
		EXPECT_EQ(receivedValue, 12345);
	}

} // namespace