#include <unordered_map>
#include <typeindex>
#include <functional>

namespace JFramework
{
	class IUnRegister
	{
	public: 
		virtual void UnRegister() = 0;
		virtual ~IUnRegister() = default;
	};

	class ICanAutoUnRegister
	{
	public:
		void UnRegisterWhenInstanceDestructed(std::shared_ptr<IUnRegister> unregister)
		{
			mUnRegisters.push_back(unregister);
		}

	protected:
		virtual ~ICanAutoUnRegister()
		{
			for (auto unRegister : mUnRegisters)
			{
				unRegister->UnRegister();
			}
		}

	protected:
		std::vector<std::shared_ptr<IUnRegister>> mUnRegisters;
	};

	class IEasyEvent
	{
	public:
		virtual std::shared_ptr<IUnRegister> Register(std::function<void()> onEvent) = 0;
		virtual ~IEasyEvent() = default;
	};

	class CustomUnRegister :public IUnRegister
	{
	public:
		 
		CustomUnRegister(std::function<void()> onUnRegister)
			: mOnUnRegister(std::move(onUnRegister))
		{
			
		}
		void UnRegister() override
		{
			if (mOnUnRegister != nullptr)
			{
				mOnUnRegister();
				mOnUnRegister = nullptr;
			}
		}
	private:
		std::function<void()> mOnUnRegister;
	};


	template <typename T>
	class EasyEvent :public IEasyEvent
	{
	private:
		struct CallbackEntry
		{
			size_t id;
			std::function<void(T*)> func;
		};
		std::vector<CallbackEntry> mOnEvent;
		size_t mNextId = 0;
	public:
		void Trigger(T* e)
		{
			std::vector<CallbackEntry> localCopy;
			{
				localCopy = mOnEvent;
			}
			for (const auto& entry : localCopy)
			{
				if (entry.func)
				{
					entry.func(e);
				}
			}
		}

		std::shared_ptr<IUnRegister> Register(std::function<void(T*)> onEvent)
		{
			const size_t id = mNextId++;
			mOnEvent.push_back({ id, std::move(onEvent) });

			return std::make_shared<CustomUnRegister>(
				[this, id]() { UnRegister(id); });
		}

		void UnRegister(size_t id)
		{
			mOnEvent.erase(
				std::remove_if(mOnEvent.begin(), mOnEvent.end(),
					[id](const CallbackEntry& entry) { return entry.id == id; }),
				mOnEvent.end());
		}
	private:
		std::shared_ptr<IUnRegister> Register(std::function<void() > onEvent) override
		{
			auto action = [onEvent = std::move(onEvent)](T*) { onEvent(); };
			return Register(std::move(action));
		}

	};

	class EasyEvents
	{
	public:
		EasyEvents(){
		}
		virtual ~EasyEvents() = default;

		// 获取事件
		template <typename T>
		typename std::enable_if<std::is_base_of<IEasyEvent, T>::value, T*>::type
			Get()
		{
			return mGlobalEvents.GetEvent<T>();
		}

		// 注册事件
		template <typename T>
		typename std::enable_if<std::is_base_of<IEasyEvent, T>::value, T*>::type
			Register()
		{
			mGlobalEvents.AddEvent<T>();
		}

		// 添加事件(与Register相同)
		template <typename T>
		typename std::enable_if<std::is_base_of<IEasyEvent, T>::value, T*>::type
			AddEvent()
		{
			mTypeEvents[typeid(T)] = std::make_unique<T>();
		}

		template <typename T>
		typename std::enable_if<std::is_base_of<IEasyEvent, T>::value, T*>::type
			GetEvent()
		{
			const auto& type = typeid(T);
			auto it = mTypeEvents.find(type);
			return it != mTypeEvents.end() ? static_cast<T*>(it->second.get()) : nullptr;
		}


		// 获取或添加事件
		template <typename T>
		typename std::enable_if<std::is_base_of<IEasyEvent, T>::value, T*>::type
			GetOrAddEvent()
		{
			const auto& type = typeid(T);
			auto it = mTypeEvents.find(type);

			if (it != mTypeEvents.end())
			{
				return static_cast<T*>(it->second.get());
			}

			auto event = std::make_unique<T>();
			auto* rawPtr = event.get();
			mTypeEvents.emplace(type, std::move(event));
			return rawPtr;
		}


	private:
		static EasyEvents mGlobalEvents;
		std::unordered_map<std::type_index, std::unique_ptr<IEasyEvent>> mTypeEvents;
	};

	// 初始化静态成员
	EasyEvents EasyEvents::mGlobalEvents;


	class TypeEventSystem
	{
	public:
		static TypeEventSystem Global;

		TypeEventSystem()
		{

		}
		virtual ~TypeEventSystem() = default;

		// Send event with default constructor
		template <typename T, typename = std::enable_if_t<std::is_default_constructible_v<T>>>
		void Send()
		{
			if (auto event = mEvents.GetEvent<EasyEvent<T>>())
			{
				T instance;
				event->Trigger(&instance);
			}
		}

		// Send event with parameter
		template <typename T>
		void Send(T* e)
		{
			if (auto event = mEvents.GetEvent<EasyEvent<T>>())
			{
				event->Trigger(e);
			}
		}

		// Register event handler
		template <typename T>
		std::shared_ptr<IUnRegister> Register(std::function<void(T*)> onEvent)
		{
			auto* event = mEvents.GetOrAddEvent<EasyEvent<T>>();
			return event->Register(std::move(onEvent));
		}

		// Unregister event handler
		template <typename T>
		void UnRegister(size_t id)
		{
			if (auto event = mEvents.GetEvent<EasyEvent<T>>())
			{
				event->UnRegister(id);
			}
		}

	private:
		EasyEvents mEvents;
	};

	 TypeEventSystem TypeEventSystem::Global;

}
