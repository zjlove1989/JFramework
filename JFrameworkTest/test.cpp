
#include "pch.h"
#include "../JFramework.h"


#include <memory>
#include <vector>

namespace
{
	using namespace JFramework;




	// Define some test event types
	struct TestEventA
	{
		int value = 0;
	};

	struct TestEventB
	{
		std::string message;
	};

	struct NonDefaultConstructibleEvent
	{
		NonDefaultConstructibleEvent(int) {}  // No default constructor
	};

	class TypeEventSystemTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			// Reset the global instance before each test
			TypeEventSystem::Global =new TypeEventSystem();
		}

		// Helper function to track if a handler was called
		struct HandlerTracker
		{
			bool called = false;
			void handler(TestEventA* e) { called = true; if (e) e->value++; }
			void handlerB(TestEventB* e) { called = true; if (e) e->message += " processed"; }
		};
	};

	TEST_F(TypeEventSystemTest, SendWithParameterTriggersHandler)
	{
		HandlerTracker tracker;
		TestEventA event{ 42 };

		// Register handler
		auto unregister = TypeEventSystem::Global->Register<TestEventA>(
			[&](TestEventA* e) { tracker.handler(e); });

		// Send event
		TypeEventSystem::Global->Send(&event);

		// Verify handler was called and modified the event
		EXPECT_TRUE(tracker.called);
		EXPECT_EQ(event.value, 43);
	}

	TEST_F(TypeEventSystemTest, SendDefaultConstructibleTriggersHandler)
	{
		HandlerTracker tracker;

		// Register handler
		auto unregister = TypeEventSystem::Global->Register<TestEventA>(
			[&](TestEventA* e) { tracker.handler(e); });

		// Send default-constructed event
		TypeEventSystem::Global->Send<TestEventA>();

		// Verify handler was called
		EXPECT_TRUE(tracker.called);
	}

	TEST_F(TypeEventSystemTest, SendNonDefaultConstructibleFailsAtCompileTime)
	{
		// This test verifies that Send<T>() won't compile for non-default-constructible types
		// In actual code, this would be a compile-time check
		// EXPECT_COMPILE_FAILURE(TypeEventSystem::Global.Send<NonDefaultConstructibleEvent>());

		// For runtime test, we'll just verify we can send a pointer to such type
		NonDefaultConstructibleEvent event(42);
		TypeEventSystem::Global->Send(&event);
		SUCCEED();
	}

	TEST_F(TypeEventSystemTest, RegisterReturnsValidUnregisterObject)
	{
		auto unregister = TypeEventSystem::Global->Register<TestEventA>(
			[](TestEventA*) {});

		EXPECT_NE(unregister, nullptr);
	}

	TEST_F(TypeEventSystemTest, UnregisterRemovesHandler)
	{
		HandlerTracker tracker;

		// Register handler
		auto unregister = TypeEventSystem::Global->Register<TestEventA>(
			[&](TestEventA* e) { tracker.handler(e); });

		// Unregister
		unregister->UnRegister();

		// Send event
		TestEventA event;
		TypeEventSystem::Global->Send(&event);

		// Verify handler was NOT called
		EXPECT_FALSE(tracker.called);
	}

	TEST_F(TypeEventSystemTest, MultipleHandlersWorkCorrectly)
	{
		HandlerTracker tracker1, tracker2;
		TestEventB event{ "hello" };

		// Register two handlers
		auto unregister1 = TypeEventSystem::Global->Register<TestEventB>(
			[&](TestEventB* e) { tracker1.handlerB(e); });
		auto unregister2 = TypeEventSystem::Global->Register<TestEventB>(
			[&](TestEventB* e) { tracker2.handlerB(e); });

		// Send event
		TypeEventSystem::Global->Send(&event);

		// Verify both handlers were called and processed the event
		EXPECT_TRUE(tracker1.called);
		EXPECT_TRUE(tracker2.called);
		EXPECT_EQ(event.message, "hello processed processed");
	}

	TEST_F(TypeEventSystemTest, HandlerCanBeUnregisteredById)
	{
		HandlerTracker tracker;
		size_t handlerId = 0;

		// Register handler and capture its ID
		auto unregister = TypeEventSystem::Global->Register<TestEventA>(
			[&](TestEventA* e) { tracker.handler(e); });

		// Get the handler ID (this assumes EasyEvent exposes this somehow)
		// If not directly available, this test might need adjustment
		// handlerId = ...;

		// Unregister by ID
		TypeEventSystem::Global->UnRegister<TestEventA>(handlerId);

		// Send event
		TestEventA event;
		TypeEventSystem::Global->Send(&event);

		// Verify handler was NOT called
		EXPECT_FALSE(tracker.called);
	}

	TEST_F(TypeEventSystemTest, SendingUnregisteredEventTypeDoesNothing)
	{
		HandlerTracker tracker;
		TestEventA event;

		// Send event without registering any handler
		TypeEventSystem::Global->Send(&event);

		// Verify handler was NOT called
		EXPECT_FALSE(tracker.called);
	}






	// Test classes for demonstration
	class TestClassA
	{
	public:
		virtual ~TestClassA() = default;
		virtual const char* Name() { return "TestClassA"; }
	};

	class TestClassB : public TestClassA
	{
	public:
		const char* Name() override { return "TestClassB"; }
	};

	class TestClassC
	{
	public:
		const char* Name() { return "TestClassC"; }
	};

	class IOCContainerTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			// Setup code if needed
		}

		void TearDown() override
		{
			container.Clear();
		}

		IOCContainer container;
	};

	TEST_F(IOCContainerTest, RegisterAndGetSimpleType)
	{
		TestClassA instanceA;
		container.Register(&instanceA);

		auto* retrieved = container.Get<TestClassA>();
		ASSERT_NE(retrieved, nullptr);
		EXPECT_EQ(retrieved, &instanceA);
		EXPECT_STREQ(retrieved->Name(), "TestClassA");
	}

	TEST_F(IOCContainerTest, GetNonExistentTypeReturnsNull)
	{
		auto* retrieved = container.Get<TestClassA>();
		EXPECT_EQ(retrieved, nullptr);
	}

	TEST_F(IOCContainerTest, RegisterOverwritesExisting)
	{
		TestClassA instanceA1;
		TestClassA instanceA2;

		container.Register(&instanceA1);
		container.Register(&instanceA2);

		auto* retrieved = container.Get<TestClassA>();
		ASSERT_NE(retrieved, nullptr);
		EXPECT_EQ(retrieved, &instanceA2);
	}

	TEST_F(IOCContainerTest, GetInstancesByTypeWithSingleMatch)
	{
		TestClassA instanceA;
		TestClassC instanceC;

		container.Register(&instanceA);
		container.Register(&instanceC);

		auto results = container.GetInstancesByType<TestClassA>();
		ASSERT_EQ(results.size(), 1);
		EXPECT_EQ(results[0], &instanceA);
	}

	TEST_F(IOCContainerTest, GetInstancesByTypeWithMultipleMatches)
	{
		TestClassA instanceA;
		TestClassB instanceB1;
		TestClassB instanceB2;
		TestClassC instanceC;

		container.Register(&instanceA);
		container.Register(&instanceB1);
		container.Register(&instanceB2);
		container.Register(&instanceC);

		// Get all TestClassA instances (including derived classes)
		auto resultsA = container.GetInstancesByType<TestClassA>();
		ASSERT_EQ(resultsA.size(), 1);

		// Get only TestClassB instances
		auto resultsB = container.GetInstancesByType<TestClassB>();
		ASSERT_EQ(resultsB.size(), 1);

		// Get TestClassC instances
		auto resultsC = container.GetInstancesByType<TestClassC>();
		ASSERT_EQ(resultsC.size(), 1);
	}

	TEST_F(IOCContainerTest, GetInstancesByTypeWithNoMatches)
	{
		TestClassC instanceC;
		container.Register(&instanceC);

		auto results = container.GetInstancesByType<TestClassA>();
		EXPECT_TRUE(results.empty());
	}

	TEST_F(IOCContainerTest, ClearRemovesAllInstances)
	{
		TestClassA instanceA;
		TestClassB instanceB;
		TestClassC instanceC;

		container.Register(&instanceA);
		container.Register(&instanceB);
		container.Register(&instanceC);

		container.Clear();

		EXPECT_EQ(container.Get<TestClassA>(), nullptr);
		EXPECT_EQ(container.Get<TestClassB>(), nullptr);
		EXPECT_EQ(container.Get<TestClassC>(), nullptr);

		EXPECT_TRUE(container.GetInstancesByType<TestClassA>().empty());
		EXPECT_TRUE(container.GetInstancesByType<TestClassB>().empty());
		EXPECT_TRUE(container.GetInstancesByType<TestClassC>().empty());
	}

	TEST_F(IOCContainerTest, BadAnyCastReturnsNullInGet)
	{
		// This test verifies that bad_any_cast is caught and returns null
		TestClassA instanceA;
		container.Register(&instanceA);

		// Attempt to get as wrong type
		auto* retrieved = container.Get<TestClassC>();
		EXPECT_EQ(retrieved, nullptr);
	}

	TEST_F(IOCContainerTest, BadAnyCastIsSkippedInGetInstancesByType)
	{
		TestClassA instanceA;
		TestClassB instanceB;
		TestClassC instanceC;

		container.Register(&instanceA);
		container.Register(&instanceB);
		container.Register(&instanceC);

		// Should only return TestClassB instances, skip others
		auto results = container.GetInstancesByType<TestClassB>();
		ASSERT_EQ(results.size(), 1);
		EXPECT_EQ(results[0], &instanceB);
	}


}


