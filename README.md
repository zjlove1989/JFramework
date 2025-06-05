# JFramework C++ 框架

作者：zjlove1989
邮箱：2517917168@qq.com
QQ群：1048859619

JFramework 是一个基于 C++ 的通用应用框架，提供了依赖注入、事件总线、命令查询分离（CQRS）等核心功能，适用于构建模块化、可扩展的应用程序。框架设计遵循面向接口编程原则，支持组件的动态注册与管理，并通过单元测试确保稳定性。

## 核心特性

### 1、依赖注入（IOC）容器
- 支持系统（System）、模型（Model）、工具类（Utility）的动态注册与获取
- 基于 std::type_index 的类型管理，确保类型安全
- 线程安全的组件注册与获取，适用于多线程环境

### 2、事件总线（Event Bus）
- 支持事件的发布 - 订阅模式，可注册多个事件处理器
- 自动处理事件类型匹配，支持继承体系下的事件分发
- 线程安全的事件发送与处理，确保高并发场景下的稳定性

### 3、命令与查询（CQRS）
- 命令（Command）模式：支持异步执行命令，可链式调用
- 查询（Query）模式：支持带返回值的查询操作，支持参数传递
- 组件间通过命令 / 查询解耦，提升可维护性

### 4、命令与查询（CQRS）
- 支持属性变更的自动通知，适用于 UI 数据绑定场景
- 自动管理观察者注册与注销，避免内存泄漏
- 线程安全的属性访问与变更通知
  
### 5、组件生命周期管理
- 支持组件的初始化（Init）与反初始化（Deinit）
- 架构级生命周期控制，确保组件按顺序初始化 / 销毁
- 支持延迟注册组件，初始化后仍可动态添加

## 目录结构

    JFramework/
    ├─ include/                # 头文件
    │  └─ JFramework.h         # 核心框架定义
    ├─ src/                    # 源文件（仅包含测试代码）
    │  └─ JFramework.cpp       # 测试入口
    └─ test/                   # 单元测试
       ├─ IOCContainerTest     # IOC容器测试
       ├─ EventBusTest         # 事件总线测试
       ├─ ArchitectureTest     # 架构测试
       └─ ...                  # 其他组件测试

## 使用示例

### 1. 定义组件
```cpp
// 模型类
class TestModel : public JFramework::AbstractModel {
protected:
    void OnInit() override { /* 初始化逻辑 */ }
    void OnDeinit() override { /* 反初始化逻辑 */ }
};

// 系统类
class TestSystem : public JFramework::AbstractSystem {
protected:
    void OnInit() override { /* 初始化逻辑 */ }
    void OnEvent(std::shared_ptr<JFramework::IEvent> event) override { /* 事件处理 */ }
};

// 命令类
class TestCommand : public JFramework::AbstractCommand {
protected:
    void OnExecute() override { /* 命令逻辑 */ }
};
```
### 2. 注册组件到架构
```cpp
class AppArchitecture : public JFramework::Architecture {
protected:
    void Init() override {
        // 注册模型
        RegisterModel<TestModel>(std::make_shared<TestModel>());
        // 注册系统
        RegisterSystem<TestSystem>(std::make_shared<TestSystem>());
    }
};
```
### 3. 使用组件
```cpp
int main() {
    auto arch = std::make_shared<AppArchitecture>();
    arch->InitArchitecture(); // 初始化架构

    // 获取模型
    auto model = arch->GetModel<TestModel>();

    // 发送命令
    arch->SendCommand(std::make_unique<TestCommand>());

    // 发送事件
    arch->SendEvent(std::make_shared<JFramework::TestEvent>());

    return 0;
}
```
## 单元测试
### 框架包含完整的单元测试，覆盖核心功能的各个方面：
- IOC 容器测试：验证组件注册、获取、清除的正确性
- 事件总线测试：验证事件发布 - 订阅、多处理器、线程安全
- 架构测试：验证组件生命周期、命令 / 查询执行、事件处理
- 性能测试：验证高并发场景下的吞吐量与响应时间
- 内存测试：验证组件与观察者的内存释放逻辑

# 运行测试（需安装Google Test）
```bash
cd test
g++ -std=c++14 JFramework.cpp -lgtest -lgtest_main -pthread
./a.out
```

## 贡献指南
提交代码前请确保单元测试通过
新增功能需补充对应的头文件与实现
遵循 C++14 标准与代码规范
提交 PR 时需注明功能描述与测试覆盖情况

## 许可证
本框架采用 MIT 许可证，详情见 LICENSE 文件。
项目地址：https://github.com/zjlove1989/JFramework
