﻿// JFramework.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "JFramework.h"
using namespace JFramework;

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

 
    // 注册事件处理器
    auto unregisterHandler = TypeEventSystem::Global->Register<MyEvent>(MyEventHandler);

    // 发送事件
    TypeEventSystem::Global->Send<MyEvent>();

    // 发送带有参数的事件
    MyEvent event(42, "Hello World");
    TypeEventSystem::Global->Send(&event);

    // 注销事件处理器
    unregisterHandler->UnRegister();

    // 再次发送事件，处理器不再被调用
    TypeEventSystem::Global->Send<MyEvent>();


	return 0;
}
