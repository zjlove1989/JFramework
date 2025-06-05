#ifndef JFRAMEWORK
#define JFRAMEWORK

#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>

namespace JFramework
{
	// ================ �쳣���� ================
	class FrameworkException : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class ArchitectureNotSetException : public FrameworkException
	{
	public:
		ArchitectureNotSetException(const std::string& typeName)
			: FrameworkException("Architecture not available: " + typeName)
		{
		}
	};

	class ComponentNotRegisteredException : public FrameworkException
	{
	public:
		explicit ComponentNotRegisteredException(const std::string& typeName)
			: FrameworkException("Component not registered: " + typeName)
		{
		}
	};

	// ================ ǰ������ ================
	class ISystem;
	class IModel;
	class ICommand;
	class IEvent;
	template <typename TResult>
	class IQuery;
	class IUtility;
	template <typename T>
	class BindableProperty;
	class ICanHandleEvent;

	// ================ ���ļܹ��ӿ� ================

	/// @brief �ܹ����Ľӿ�
	class IArchitecture
	{
	public:
		virtual ~IArchitecture() = default;

		// ע�����
		virtual void RegisterSystem(std::type_index typeId,
			std::shared_ptr<ISystem> system) = 0;
		virtual void RegisterModel(std::type_index typeId,
			std::shared_ptr<IModel> model) = 0;
		virtual void RegisterUtility(std::type_index typeId,
			std::shared_ptr<IUtility> utility) = 0;

		// ��ȡ���
		virtual std::shared_ptr<ISystem> GetSystem(std::type_index typeId) = 0;
		virtual std::shared_ptr<IModel> GetModel(std::type_index typeId) = 0;
		virtual std::shared_ptr<IUtility> GetUtility(std::type_index typeId) = 0;

		// �������
		virtual void SendCommand(std::unique_ptr<ICommand> command) = 0;

		// �¼�����
		virtual void SendEvent(std::shared_ptr<IEvent> event) = 0;
		virtual void RegisterEvent(std::type_index eventType,
			ICanHandleEvent* handler) = 0;
		virtual void UnRegisterEvent(std::type_index eventType,
			ICanHandleEvent* handler) = 0;

		virtual void Deinit() = 0;
	};

	// ================ �����ӿ� ================

	// ��ע���ӿ�
	class IUnRegister
	{
	public:
		virtual ~IUnRegister() = default;
		virtual void UnRegister() = 0;
	};

	class UnRegisterTrigger
	{
	public:
		virtual ~UnRegisterTrigger() { this->UnRegister(); }

		void AddUnRegister(std::shared_ptr<IUnRegister> unRegister)
		{
			std::lock_guard<std::mutex> lock(mMutex);
			mUnRegisters.push_back(std::move(unRegister));
		}

		void UnRegister()
		{
			std::lock_guard<std::mutex> lock(mMutex);
			for (auto& unRegister : mUnRegisters)
			{
				unRegister->UnRegister();
			}
			mUnRegisters.clear();
		}

	protected:
		std::mutex mMutex;
		std::vector<std::shared_ptr<IUnRegister>> mUnRegisters;
	};

	template <typename T>
	class AutoUnRegister : public IUnRegister,
		public std::enable_shared_from_this <AutoUnRegister<T>>
	{
	public:
		AutoUnRegister(int id, BindableProperty<T>* property, std::function<void(T)> callback)
			: mProperty(property),
			mCallback(std::move(callback)),
			mId(id)
		{
		}
		void UnRegisterWhenObjectDestroyed(UnRegisterTrigger* unRegisterTrigger)
		{
			unRegisterTrigger->AddUnRegister(this->shared_from_this());
		}
		int GetId() const { return mId; }

		void UnRegister() override
		{
			if (mProperty)
			{
				mProperty->UnRegister(mId);
				mProperty = nullptr; // ��ֹ�ظ�����
			}
		}

		void Invoke(T value)
		{
			if (mCallback)
			{
				mCallback(std::move(value));
			}
		}
	protected:
		int mId;
	private:
		BindableProperty<T>* mProperty;
		std::function<void(T)> mCallback;
	};

	// �ɹ۲�������
	template <typename T>
	class BindableProperty
	{
	public:
		BindableProperty() = default;

		explicit BindableProperty(const T& value) : mValue(std::move(value)) {}

		// ��ȡ��ǰֵ
		const T& GetValue() const { return mValue; }

		// ������ֵ
		void SetValue(const T& newValue)
		{
			std::lock_guard<std::mutex> lock(mMutex);
			if (mValue == newValue) return;
			mValue = newValue;
			Trigger();
		}

		// ������֪ͨ������
		void SetValueWithoutEvent(const T& newValue) { mValue = newValue; }

		// ע��۲��ߣ�����ʼֵ֪ͨ��
		std::shared_ptr<AutoUnRegister<T>> RegisterWithInitValue(
			std::function<void(const T&)> onValueChanged)
		{
			onValueChanged(mValue);
			return Register(std::move(onValueChanged));
		}

		// ע��۲���
		std::shared_ptr<AutoUnRegister<T>> Register(std::function<void(const T&)> onValueChanged)
		{
			std::lock_guard<std::mutex> lock(mMutex);
			auto unRegister = std::make_shared<AutoUnRegister<T>>(mNextId++, this, std::move(onValueChanged));
			mObservers.push_back(unRegister);
			return unRegister;
		}

		// ע���۲���
		void UnRegister(int id)
		{
			std::lock_guard<std::mutex> lock(mMutex);
			// ֱ�Ӵ�weak_ptr�б����Ƴ���Ӧid�Ĺ۲���
			mObservers.erase(
				std::remove_if(mObservers.begin(), mObservers.end(),
					[id](const std::weak_ptr<AutoUnRegister<T>>& weakObserver)
					{
						if (auto observer = weakObserver.lock())
						{
							return observer->GetId() == id;
						}
						return false; // ��ʧЧ�Ĺ۲�����Trigger����
					}),
				mObservers.end());
		}

		// ���������أ�����ʹ��
		operator T() const { return mValue; }
		BindableProperty<T>& operator=(const T& newValue)
		{
			SetValue(std::move(newValue));
			return *this;
		}

	private:
		// ֪ͨ���й۲���
		void Trigger()
		{
			// ֪ͨ��Ч�Ĺ۲���
			for (auto& observer : mObservers)
			{
				observer->Invoke(mValue);
			}
		}
		std::mutex mMutex;
		int mNextId = 0;
		T mValue;
		std::vector<std::shared_ptr<AutoUnRegister<T>>> mObservers;
	};

	/// @brief ��ʼ���ӿ�
	class ICanInit
	{
	public:
		bool IsInitialized() const { return mInitialized; }
		void SetInitialized(bool initialized) { mInitialized = initialized; }
		virtual ~ICanInit() = default;
		virtual void Init() = 0;
		virtual void Deinit() = 0;
	protected:
		bool mInitialized = false;
	};

	/// @brief �ܹ������ӿ�
	class IBelongToArchitecture
	{
	public:
		virtual ~IBelongToArchitecture() = default;
		virtual std::shared_ptr<IArchitecture> GetArchitecture() const = 0;
		template <typename T>
		std::shared_ptr<T> GetArchitectureAs() const
		{
			return std::dynamic_pointer_cast<T>(GetArchitecture());
		}
	};

	/// @brief �ܹ����ýӿ�
	class ICanSetArchitecture
	{
	public:
		virtual ~ICanSetArchitecture() = default;
		virtual void SetArchitecture(std::shared_ptr<IArchitecture> architecture) = 0;
	};

	// ================ ���ܽӿ� ================

	/// @brief ��ȡModel����
	class ICanGetModel : public IBelongToArchitecture
	{
	public:
		template <typename T>
		std::shared_ptr<T> GetModel()
		{
			auto arch = GetArchitecture();
			if (!arch)
			{
				throw ArchitectureNotSetException(typeid(T).name());
			}

			auto model = arch->GetModel(typeid(T));
			if (!model)
			{
				throw std::runtime_error("Model not registered: " +
					std::string(typeid(T).name()));
			}

			auto casted = std::dynamic_pointer_cast<T>(model);
			if (!casted)
			{
				throw std::bad_cast();
			}

			return casted;
		}
	};

	/// @brief ��ȡSystem����
	class ICanGetSystem : public IBelongToArchitecture
	{
	public:
		template <typename T>
		std::shared_ptr<T> GetSystem()
		{
			auto arch = GetArchitecture();
			if (!arch)
			{
				throw ArchitectureNotSetException(typeid(T).name());
			}

			auto system = arch->GetSystem(typeid(T));
			if (!system)
			{
				throw std::runtime_error("System not registered: " +
					std::string(typeid(T).name()));
			}

			auto casted = std::dynamic_pointer_cast<T>(system);
			if (!casted)
			{
				throw std::bad_cast();
			}

			return casted;
		}
	};

	/// @brief ����Command����
	class ICanSendCommand : public IBelongToArchitecture
	{
	public:
		template <typename T, typename... Args>
		void SendCommand(Args&&... args)
		{
			auto arch = GetArchitecture();
			if (!arch)
			{
				throw ArchitectureNotSetException(typeid(T).name());
			}
			arch->SendCommand(std::make_unique<T>(std::forward<Args>(args)...));
		}

		void SendCommand(std::unique_ptr<ICommand> command)
		{
			auto arch = GetArchitecture();
			if (!arch)
			{
				throw ArchitectureNotSetException(typeid(command).name());
			}
			arch->SendCommand(std::move(command));
		}
	};

	/// @brief ����Command����
	class ICanSendQuery : public IBelongToArchitecture
	{
	public:
		template <typename TQuery, typename... Args>
		auto SendQuery(Args&&... args) -> decltype(std::declval<TQuery>().Do())
		{
			static_assert(
				std::is_base_of_v<IQuery<decltype(std::declval<TQuery>().Do())>,
				TQuery>,
				"TQuery must inherit from IQuery");

			auto arch = GetArchitecture();
			if (!arch)
			{
				throw ArchitectureNotSetException(typeid(TQuery).name());
			}

			auto query = std::make_unique<TQuery>(std::forward<Args>(args)...);

			query->SetArchitecture(arch);
			return query->Do();
		}
	};

	class ICanGetUtility : public IBelongToArchitecture
	{
	public:
		template <typename T>
		std::shared_ptr<T> GetUtility()
		{
			auto arch = GetArchitecture();
			if (!arch)
			{
				throw ArchitectureNotSetException(typeid(T).name());
			}

			auto utility = arch->GetUtility(typeid(T));
			if (!utility)
			{
				throw std::runtime_error("Utility not registered: " +
					std::string(typeid(T).name()));
			}

			auto casted = std::dynamic_pointer_cast<T>(utility);
			if (!casted)
			{
				throw std::bad_cast();
			}

			return casted;
		}
	};

	// ================ �¼�ע�������ӿ� ================

	/// @brief ����Event����
	class ICanSendEvent : public IBelongToArchitecture
	{
	public:
		template <typename T, typename... Args>
		void SendEvent(Args&&... args)
		{
			auto arch = GetArchitecture();
			if (!arch)
			{
				throw ArchitectureNotSetException(typeid(T).name());
			}
			arch->SendEvent(std::make_shared<T>(std::forward<Args>(args)...));
		}
	};

	/// @brief ����Event����
	class ICanHandleEvent
	{
	public:
		virtual ~ICanHandleEvent() = default;
		virtual void HandleEvent(std::shared_ptr<IEvent> event) = 0;
	};

	/// @brief ע��/ע���¼���������
	class ICanRegisterEvent : public IBelongToArchitecture
	{
	public:
		virtual ~ICanRegisterEvent() = default;

		template <typename TEvent>
		void RegisterEvent(ICanHandleEvent* handler)
		{
			static_assert(std::is_base_of_v<IEvent, TEvent>,
				"TEvent must inherit from IEvent");

			auto arch = GetArchitecture();
			if (!arch) throw ArchitectureNotSetException(typeid(TEvent).name());

			arch->RegisterEvent(typeid(TEvent), handler);
		}

		template <typename TEvent>
		void UnRegisterEvent(ICanHandleEvent* handler)
		{
			static_assert(std::is_base_of_v<IEvent, TEvent>,
				"TEvent must inherit from IEvent");

			auto arch = GetArchitecture();
			if (!arch) throw ArchitectureNotSetException(typeid(TEvent).name());

			arch->UnRegisterEvent(typeid(TEvent), handler);
		}
	};

	// ================ ��������ӿ� ================

	/// @brief �¼��ӿ�
	class IEvent
	{
	public:
		virtual ~IEvent() = default;
		virtual std::string GetEventType() const = 0;
	};

	/// @brief ����ӿ�
	class ICommand : public ICanSetArchitecture,
		public ICanGetSystem,
		public ICanGetModel,
		public ICanSendCommand,
		public ICanSendEvent,
		public ICanSendQuery,
		public ICanGetUtility
	{
	public:
		virtual ~ICommand() override = default;
		virtual void Execute() = 0;
	};

	/// @brief Model�ӿ�
	class IModel : public ICanSetArchitecture,
		public ICanInit,
		public ICanSendEvent,
		public ICanSendQuery,
		public ICanGetUtility
	{
	};

	/// @brief System�ӿ�
	class ISystem : public ICanSetArchitecture,
		public ICanInit,
		public ICanGetModel,
		public ICanHandleEvent,
		public ICanGetSystem,
		public ICanRegisterEvent,
		public ICanSendEvent,
		public ICanSendQuery,
		public ICanGetUtility
	{
	};

	// @brief Controller�ӿ�
	class IController : public ICanGetSystem,
		public ICanGetModel,
		public ICanSendCommand,
		public ICanSendEvent,
		public ICanHandleEvent,
		public ICanRegisterEvent,
		public ICanSendQuery,
		public ICanGetUtility
	{
	};

	/// @brief Query�ӿ�
	template <typename TResult>
	class IQuery : public ICanSetArchitecture,
		public ICanGetModel,
		public ICanGetSystem,
		public ICanSendQuery
	{
	public:
		virtual ~IQuery() override = default;
		virtual TResult Do() = 0;
	};

	class IUtility {};

	// ================ ʵ���� ================

	/// @brief IOC����ʵ��
	class IOCContainer
	{
	public:
		template <typename T, typename TBase>
		void Register(std::type_index typeId, std::shared_ptr<TBase> component)
		{
			static_assert(std::is_base_of_v<TBase, T>, "T must inherit from TBase");
			auto& container = GetContainer<TBase>();
			std::lock_guard<std::mutex> lock(GetMutex<TBase>());
			container[typeId] = std::static_pointer_cast<TBase>(component);
		}

		template <typename TBase>
		std::shared_ptr<TBase> Get(std::type_index typeId)
		{
			auto& container = GetContainer<TBase>();
			std::lock_guard<std::mutex> lock(GetMutex<TBase>());
			auto it = container.find(typeId);
			return it != container.end() ? it->second : nullptr;
		}

		template <typename TBase>
		std::vector<std::shared_ptr<TBase>> GetAll()
		{
			auto& container = GetContainer<TBase>();
			std::lock_guard<std::mutex> lock(GetMutex<TBase>());
			std::vector<std::shared_ptr<TBase>> result;
			for (auto& pair : container)
			{
				result.push_back(pair.second);
			}
			return result;
		}

		void Clear()
		{
			std::lock_guard<std::mutex> lock1(mModelMutex);
			std::lock_guard<std::mutex> lock2(mSystemMutex);
			std::lock_guard<std::mutex> lock3(mUtilityMutex);
			mModels.clear();
			mSystems.clear();
			mUtilitys.clear();
		}

	private:
		template <typename T>
		std::unordered_map<std::type_index, std::shared_ptr<T>>& GetContainer();

		template <typename T>
		std::mutex& GetMutex();

		// ��ʽ�ػ�����
		template <>
		std::unordered_map<std::type_index, std::shared_ptr<IModel>>&
			GetContainer<IModel>()
		{
			return mModels;
		}

		template <>
		std::unordered_map<std::type_index, std::shared_ptr<ISystem>>&
			GetContainer<ISystem>()
		{
			return mSystems;
		}

		template <>
		std::unordered_map<std::type_index, std::shared_ptr<IUtility>>&
			GetContainer<IUtility>()
		{
			return mUtilitys;
		}

		template <>
		std::mutex& GetMutex<IModel>()
		{
			return mModelMutex;
		}

		template <>
		std::mutex& GetMutex<ISystem>()
		{
			return mSystemMutex;
		}

		template <>
		std::mutex& GetMutex<IUtility>()
		{
			return mUtilityMutex;
		}

		std::unordered_map<std::type_index, std::shared_ptr<IModel>> mModels;
		std::unordered_map<std::type_index, std::shared_ptr<ISystem>> mSystems;
		std::unordered_map<std::type_index, std::shared_ptr<IUtility>> mUtilitys;

		std::mutex mModelMutex;
		std::mutex mSystemMutex;
		std::mutex mUtilityMutex;
	};

	/// @brief �¼�����ʵ��
	class EventBus
	{
	public:
		void RegisterEvent(std::type_index eventType, ICanHandleEvent* handler)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_subscribers[eventType].push_back(handler);
		}

		void SendEvent(std::shared_ptr<IEvent> event)
		{
			std::vector<ICanHandleEvent*> subscribers;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				auto it = m_subscribers.find(typeid(*event));
				if (it != m_subscribers.end())
				{
					subscribers = it->second;
				}
			}

			for (auto& handler : subscribers)
			{
				handler->HandleEvent(event);
			}
		}

		void UnRegisterEvent(std::type_index eventType, ICanHandleEvent* handler)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto it = m_subscribers.find(eventType);
			if (it != m_subscribers.end())
			{
				auto& handlers = it->second;
				auto handlerIt = std::find(handlers.begin(), handlers.end(), handler);
				if (handlerIt != handlers.end())
				{
					handlers.erase(handlerIt);

					if (handlers.empty())
					{
						m_subscribers.erase(it);
					}
				}
			}
		}

		void Clear()
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_subscribers.clear();
		}

	private:
		std::mutex m_mutex;
		std::unordered_map<std::type_index, std::vector<ICanHandleEvent*>>
			m_subscribers;
	};

	/// @brief �ܹ�����ʵ��
	class Architecture : public IArchitecture,
		public std::enable_shared_from_this<Architecture>
	{
	public:
		void RegisterSystem(std::type_index typeId,
			std::shared_ptr<ISystem> system) override
		{
			if (!system)
			{
				throw std::invalid_argument("System cannot be null");
			}
			system->SetArchitecture(shared_from_this());
			mContainer->Register<ISystem>(typeId, system);
			if (mInitialized)
			{
				InitializeComponent(system);
			}
		}

		void RegisterModel(std::type_index typeId,
			std::shared_ptr<IModel> model) override
		{
			if (!model)
			{
				throw std::invalid_argument("Model cannot be null");
			}
			model->SetArchitecture(shared_from_this());
			mContainer->Register<IModel>(typeId, model);
			if (mInitialized)
			{
				InitializeComponent(model);
			}
		}

		void RegisterUtility(std::type_index typeId,
			std::shared_ptr<IUtility> utility) override
		{
			mContainer->Register<IUtility>(typeId, utility);
		}

		std::shared_ptr<ISystem> GetSystem(std::type_index typeId) override
		{
			return mContainer->Get<ISystem>(typeId);
		}

		std::shared_ptr<IModel> GetModel(std::type_index typeId) override
		{
			return mContainer->Get<IModel>(typeId);
		}

		std::shared_ptr<IUtility> GetUtility(std::type_index typeId) override
		{
			return mContainer->Get<IUtility>(typeId);
		}

		// ģ�巽��
		template <typename T>
		void RegisterSystem(std::shared_ptr<T> system)
		{
			static_assert(std::is_base_of_v<ISystem, T>, "T must inherit from ISystem");
			RegisterSystem(typeid(T), std::static_pointer_cast<ISystem>(system));
		}

		template <typename T>
		std::shared_ptr<T> GetSystem()
		{
			auto system = GetSystem(typeid(T));
			if (!system)
			{
				throw ComponentNotRegisteredException(typeid(T).name());
			}
			return std::dynamic_pointer_cast<T>(system);
		}

		// ģ�巽��
		template <typename T>
		void RegisterModel(std::shared_ptr<T> model)
		{
			static_assert(std::is_base_of_v<IModel, T>, "T must inherit from IModel");
			RegisterModel(typeid(T), std::static_pointer_cast<IModel>(model));
		}

		template <typename T>
		void RegisterEvent(ICanHandleEvent* handler)
		{
			static_assert(std::is_base_of_v<IEvent, T>, "T must inherit from IEvent");
			mEventBus->RegisterEvent(typeid(T), handler);
		}

		template <typename T>
		std::shared_ptr<T> GetModel()
		{
			auto model = GetModel(typeid(T));
			if (!model)
			{
				throw ComponentNotRegisteredException(typeid(T).name());
			}
			return std::dynamic_pointer_cast<T>(model);
		}

		void SendCommand(std::unique_ptr<ICommand> command) override
		{
			if (!command)
			{
				throw std::invalid_argument("Command cannot be null");
			}
			command->SetArchitecture(shared_from_this());
			command->Execute();
		}

		void SendEvent(std::shared_ptr<IEvent> event) override
		{
			if (!event)
			{
				throw std::invalid_argument("Event cannot be null");
			}
			mEventBus->SendEvent(event);
		}

		template <typename T, typename... Args>
		void SendEvent(Args&&... args)
		{
			this->SendEvent(std::make_shared<T>(std::forward<Args>(args)...));
		}

		template <typename TQuery>
		auto SendQuery(std::unique_ptr<TQuery> query) -> decltype(query->Do())
		{
			static_assert(std::is_base_of_v<IQuery<decltype(query->Do())>, TQuery>,
				"TQuery must inherit from IQuery");

			if (!query)
			{
				throw std::invalid_argument("Query cannot be null");
			}
			query->SetArchitecture(shared_from_this());
			return query->Do();
		}

		void RegisterEvent(std::type_index eventType,
			ICanHandleEvent* handler) override
		{
			mEventBus->RegisterEvent(eventType, handler);
		}

		void UnRegisterEvent(std::type_index eventType,
			ICanHandleEvent* handler) override
		{
			mEventBus->UnRegisterEvent(eventType, handler);
		}

		void Deinit() override
		{
			if (!mInitialized) return;

			mInitialized = false;

			this->OnDeinit();

			for (auto& model : mContainer->GetAll<IModel>())
			{
				UnInitializeComponent(model);
			}

			for (auto& system : mContainer->GetAll<ISystem>())
			{
				UnInitializeComponent(system);
			}

			mContainer->Clear();
			mEventBus->Clear();
		}

		virtual void InitArchitecture()
		{
			if (mInitialized) return;

			mInitialized = true;

			this->Init();

			for (auto& model : mContainer->GetAll<IModel>())
			{
				InitializeComponent(model);
			}

			for (auto& system : mContainer->GetAll<ISystem>())
			{
				InitializeComponent(system);
			}
		}

	protected:
		bool mInitialized;

		std::unique_ptr<IOCContainer> mContainer;
		std::unique_ptr<EventBus> mEventBus;
		// ˽�й��캯��
		Architecture()
		{
			mContainer = std::make_unique<IOCContainer>();
			mEventBus = std::make_unique<EventBus>();
			mInitialized = false;
		}

		virtual ~Architecture()
		{
		}

		virtual void Init() = 0;

		virtual void OnDeinit() {}

	private:
		template <typename TComponent>
		void InitializeComponent(std::shared_ptr<TComponent> component)
		{
			static_assert(std::is_base_of_v<ICanInit, TComponent>,
				"Component must implement ICanInit");

			if (!component->IsInitialized())
			{
				component->Init();
				component->SetInitialized(true);
			}
		}

		template <typename TComponent>
		void UnInitializeComponent(std::shared_ptr<TComponent> component)
		{
			static_assert(std::is_base_of_v<ICanInit, TComponent>,
				"Component must implement ICanInit");

			if (component->IsInitialized())
			{
				component->Deinit();
				component->SetInitialized(false);
			}
		}
	};

	// ================ ������� ================

	/// @brief ����Command
	class AbstractCommand : public ICommand
	{
	private:
		std::shared_ptr<IArchitecture> mArchitecture;

	public:
		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return mArchitecture;
		}

		void Execute() override { this->OnExecute(); }

	private:
		void SetArchitecture(std::shared_ptr<IArchitecture> architecture) override
		{
			mArchitecture = architecture;
		}

	protected:
		virtual void OnExecute() = 0;
	};

	class AbstractModel : public IModel
	{
	private:
		std::shared_ptr<IArchitecture> mArchitecture;

	public:
		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return mArchitecture;
		}

		virtual void Init() override { this->OnInit(); }

		void Deinit() override { OnDeinit(); }

	private:
		void SetArchitecture(std::shared_ptr<IArchitecture> architecture) override
		{
			mArchitecture = architecture;
		}

	protected:
		virtual void OnInit() = 0;
		virtual void OnDeinit() = 0;
	};

	class AbstractSystem : public ISystem
	{
	private:
		std::shared_ptr<IArchitecture> mArchitecture;

	public:
		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return mArchitecture;
		}

		virtual void Init() override { this->OnInit(); }

		void Deinit() override { OnDeinit(); }

		void HandleEvent(std::shared_ptr<IEvent> event) override { OnEvent(event); }

	private:
		void SetArchitecture(std::shared_ptr<IArchitecture> architecture) override
		{
			mArchitecture = architecture;
		}

	protected:
		virtual void OnInit() = 0;
		virtual void OnDeinit() = 0;
		virtual void OnEvent(std::shared_ptr<IEvent> event) = 0;
	};

	class AbstractController : public IController
	{
	private:
		void HandleEvent(std::shared_ptr<IEvent> event) override { OnEvent(event); }

	protected:
		virtual void OnEvent(std::shared_ptr<IEvent> event) = 0;
	};

	template <typename TResult>
	class AbstractQuery : public IQuery<TResult>
	{
	private:
		std::shared_ptr<IArchitecture> mArchitecture;

	public:
		std::shared_ptr<IArchitecture> GetArchitecture() const override
		{
			return mArchitecture;
		}

		TResult Do() override { return OnDo(); }

	public:
		void SetArchitecture(std::shared_ptr<IArchitecture> architecture) override
		{
			mArchitecture = architecture;
		}

	protected:
		virtual TResult OnDo() = 0;
	};
};  // namespace JFramework

#endif  // !_JFRAMEWORK_
