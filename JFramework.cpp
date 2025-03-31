// JFramework.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <cassert>
#include "gtest/gtest.h"
#include "JFramework.h"

// Define an event type
class  MyEvent
{
public:
	int value;
	std::string message;
    MyEvent()
    {

    }
    MyEvent(int value, std::string message)
    {
        this->value = value;
        this->message = message;
    }
};

// 定义一个事件处理器
void MyEventHandler(MyEvent* e)
{
	std::cout << "Event received with value: " << e->value << std::endl;
}

int main(int argc, char** argv)
{
	//::testing::InitGoogleTest(&argc, argv);
	//return RUN_ALL_TESTS();

     // 获取全局的 TypeEventSystem 实例
    JFramework::TypeEventSystem& eventSystem = JFramework::TypeEventSystem::Global;

    // 注册事件处理器
    auto unregisterHandler = eventSystem.Register<MyEvent>(MyEventHandler);

    // 发送事件
    eventSystem.Send<MyEvent>();

    // 发送带有参数的事件
    MyEvent event(42, "Hello World");
    eventSystem.Send(&event);

    // 注销事件处理器
    unregisterHandler->UnRegister();

    // 再次发送事件，处理器不再被调用
    eventSystem.Send<MyEvent>();


	return 0;
}
