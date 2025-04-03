#include <unordered_map>
#include <typeindex>
#include <functional>
#include <any>

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

		// ��ȡ�¼�
		template <typename T>
		typename std::enable_if<std::is_base_of<IEasyEvent, T>::value, T*>::type
			Get()
		{
			return mGlobalEvents->GetEvent<T>();
		}

		// ע���¼�
		template <typename T>
		typename std::enable_if<std::is_base_of<IEasyEvent, T>::value, T*>::type
			Register()
		{
			mGlobalEvents->AddEvent<T>();
		}

		// ����¼�(��Register��ͬ)
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


		// ��ȡ������¼�
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
		static EasyEvents* mGlobalEvents;
		std::unordered_map<std::type_index, std::unique_ptr<IEasyEvent>> mTypeEvents;
	};

	// ��ʼ����̬��Ա
	EasyEvents* EasyEvents::mGlobalEvents = new EasyEvents();


	class TypeEventSystem
	{
	public:
		static TypeEventSystem* Global;

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

	 TypeEventSystem* TypeEventSystem::Global = new TypeEventSystem();

	 class IOCContainer
	 {
	 private:
		 std::unordered_map<std::type_index, std::any> m_instances;

	 public:
		 // ע��ʵ��
		 template<typename T>
		 void Register(T* instance)
		 {
			 auto key = std::type_index(typeid(T));

			 if (m_instances.find(key) != m_instances.end())
			 {
				 m_instances[key] = instance;
			 }
			 else
			 {
				 m_instances.emplace(key, instance);
			 }
		 }

		 // ��ȡʵ��
		 template<typename T>
		 T* Get()
		 {
			 auto key = std::type_index(typeid(T));

			 auto it = m_instances.find(key);
			 if (it != m_instances.end())
			 {
				 try
				 {
					 return std::any_cast<T*>(it->second);
				 }
				 catch (const std::bad_any_cast&)
				 {
					 return nullptr;
				 }
			 }

			 return nullptr;
		 }

		 // ��ȡ���п�ת��ΪT���͵�ʵ��
		 template<typename T>
		 std::vector<T*> GetInstancesByType()
		 {
			 std::vector<T*> result;
			 T* casted = nullptr;

			 for (auto& pair : m_instances)
			 {
				 try
				 {
					 casted = std::any_cast<T*>(pair.second);
					 if (casted != nullptr)
					 {
						 result.push_back(casted);
					 }
				 }
				 catch (const std::bad_any_cast&)
				 {
					 // ���Ͳ�ƥ�䣬����
				 }
			 }

			 return result;
		 }

		 // �������
		 void Clear()
		 {
			 m_instances.clear();
		 }
	 };
}
