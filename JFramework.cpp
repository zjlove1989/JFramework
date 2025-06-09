// JFramework.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "JFramework.h"
#include <iostream>
using namespace JFramework;

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

	return 0;
}