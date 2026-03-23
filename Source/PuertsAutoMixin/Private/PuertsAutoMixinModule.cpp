#include "PuertsAutoMixinModule.h"

#include "GameDelegates.h"
#include "PuertsAutoMixinLibrary.h"
#include "PuertsAutoMixinSetting.h"
#include "PuertsInterface.h"
#include "SourceFileWatcher.h"

DEFINE_LOG_CATEGORY(LogPuertsAutoMixin);

#if WITH_EDITOR
static FName SpecialFunctionName = FName(TEXT("__PuertsAutoMixinSucceeded"));
#endif

constexpr EInternalObjectFlags AsyncObjectFlags = EInternalObjectFlags_AsyncLoading | EInternalObjectFlags::Async;

struct FPuertsAutoMixinData
{
	FPuertsAutoMixinDelegate BindCallback;
	TSet<TWeakObjectPtr<UClass>> BindedClasses;
};

class PUERTSAUTOMIXIN_API FPuertsAutoMixinModule : public IPuertsAutoMixinModule,
                                                   public FUObjectArray::FUObjectCreateListener,
                                                   public FUObjectArray::FUObjectDeleteListener
{
	virtual void StartupModule() override
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem StartupModule"));

#if WITH_EDITOR
		bool bOnlyGame = IsRunningGame() || IsRunningDedicatedServer();
		const auto& Setting = GetMutableDefault<UPuertsAutoMixinSetting>();
		if (!bOnlyGame)
		{
			FEditorDelegates::PreBeginPIE.AddRaw(this, &FPuertsAutoMixinModule::OnPreBeginPIE);
			FEditorDelegates::EndPIE.AddRaw(this, &FPuertsAutoMixinModule::OnEndPIE);
			FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FPuertsAutoMixinModule::OnEndPlayMap);
		}
		if (Setting->bEnableEnvInEditor || bOnlyGame)
		{
			StartAutoBind();
		}
		if ((bOnlyGame && Setting->bEnableEnvInGame) || (!bOnlyGame && Setting->bEnableEnvInEditor))
		{
			StartJavaScript();
		}
#else
		StartAutoBind();
#endif
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem ShutdownModule"));

		StopBind();

		Reset();
	}

	void StartAutoBind()
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem StartAutoBind"));

		if (bActive)
		{
			return;
		}

		GUObjectArray.AddUObjectCreateListener(this);
		GUObjectArray.AddUObjectDeleteListener(this);

		OnAsyncLoadingFlushUpdateHandle = FCoreDelegates::OnAsyncLoadingFlushUpdate.AddRaw(
			this
			, &FPuertsAutoMixinModule::OnAsyncLoadingFlushUpdate
		);

		bActive = true;
	}

	void StopBind()
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem StopBind"));

		if (!bActive)
		{
			return;
		}

		StopListen();

		Reset();

		bActive = false;
	}

	void StopListen()
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem StopListen"));

		GUObjectArray.RemoveUObjectCreateListener(this);
		GUObjectArray.RemoveUObjectDeleteListener(this);

		FCoreDelegates::OnAsyncLoadingFlushUpdate.Remove(OnAsyncLoadingFlushUpdateHandle);
	}

	void OnPreBeginPIE(bool bIsSimulating)
	{
		StartAutoBind();

		auto Setting = GetMutableDefault<UPuertsAutoMixinSetting>();
		if (!Setting->bEnableEnvInGame)
		{
			Reset();
		}
	}

	void OnEndPIE(bool bIsSimulating)
	{
		const auto& Setting = GetMutableDefault<UPuertsAutoMixinSetting>();
		if (Setting->bEnableEnvInEditor && !Setting->bEnableEnvInGame)
		{
			StartJavaScript();
		}
	}

	void OnEndPlayMap()
	{
		const auto& Setting = GetMutableDefault<UPuertsAutoMixinSetting>();
		if (!Setting->bEnableEnvInEditor && !Setting->bEnableEnvInGame)
		{
			StopBind();
		}
	}

	void BindMixin(const FPuertsAutoMixinDelegate& BindCallback)
	{
		RegisterBindDelegate(DefaultJsEnv, BindCallback);
	}

	virtual void NotifyUObjectCreated(const UObjectBase* ObjectBase, int32 Index) override
	{
		UObject* Object = (UObject*)ObjectBase;
		// UE_LOG(LogPuertsAutoMixin, Display, TEXT("NotifyUObjectCreated:%s"), *Object->GetName());
		TryBind(Object, nullptr);
	}

	virtual void NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index) override
	{
		// UObject* Object = (UObject*)ObjectBase;
		// UE_LOG(LogPuertsAutoMixin, Display, TEXT("NotifyUObjectDeleted:%s"), *Object->GetName());
	}

	virtual void OnUObjectArrayShutdown() override
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem OnUObjectArrayShutdown"));

		if (!bActive)
		{
			return;
		}

		StopListen();

		bActive = false;
	}

	void OnAsyncLoadingFlushUpdate()
	{
		TArray<FWeakObjectPtr> CandidatesTemp;
		TArray<int> CandidatesRemovedIndexes;

		TArray<UObject*> LocalCandidates;
		{
			{
				FScopeLock Lock(&CandidatesLock);
				CandidatesTemp.Append(Candidates);
			}


			for (int32 i = CandidatesTemp.Num() - 1; i >= 0; --i)
			{
				FWeakObjectPtr ObjectPtr = CandidatesTemp[i];
				if (!ObjectPtr.IsValid())
				{
					// discard invalid objects
					CandidatesRemovedIndexes.Add(i);
					continue;
				}

				UObject* Object = ObjectPtr.Get();
				if (Object->HasAnyFlags(RF_NeedPostLoad)
					|| Object->HasAnyInternalFlags(AsyncObjectFlags)
					|| Object->GetClass()->HasAnyInternalFlags(AsyncObjectFlags))
				{
					// delay bind on next update
					continue;
				}

				LocalCandidates.Add(Object);
				CandidatesRemovedIndexes.Add(i);
			}
		}

		{
			FScopeLock Lock(&CandidatesLock);
			for (int32 j = 0; j < CandidatesRemovedIndexes.Num(); ++j)
			{
				Candidates.RemoveAt(CandidatesRemovedIndexes[j]);
			}
		}

		for (int32 i = 0; i < LocalCandidates.Num(); ++i)
		{
			UObject* Object = LocalCandidates[i];
			TryBind(Object, nullptr);
		}
	}

	void TryBind(UObject* Object, FPuertsAutoMixinData* SpecificData)
	{
		const auto Class = Object->IsA<UClass>() ? static_cast<UClass*>(Object) : Object->GetClass();
		if (Class->HasAnyClassFlags(CLASS_NewerVersionExists))
		{
			return;
		}

		static UClass* InterfaceClass = UPuertsInterface::StaticClass();
		const bool bImplPuertsInterface = Class->ImplementsInterface(InterfaceClass);

		if (IsInAsyncLoadingThread())
		{
			if (bImplPuertsInterface)
			{
				// 等加载完再绑定
				FScopeLock Lock(&CandidatesLock);
				Candidates.AddUnique(Object);
				return;
			}
		}

		if (!bImplPuertsInterface)
		{
			return;
		}

		if (Class->GetName().Contains(TEXT("SKEL_")))
		{
			return;
		}

		// 递归查找所有父类，从最顶层的UObject子类开始绑定
		TArray<UClass*> ClassHierarchy;
		UClass* CurrentClass = Class;
		while (CurrentClass && CurrentClass->IsChildOf<UObject>())
		{
			ClassHierarchy.Add(CurrentClass);
			CurrentClass = CurrentClass->GetSuperClass();
		}

#if WITH_EDITOR
		if (BindedClasses.Contains(Class))
		{
			// 如果发现特殊标记被删除，说明可能蓝图被Recompile，重新绑定相关模块
			if (!Class->FindFunctionByName(SpecialFunctionName, EIncludeSuperFlag::Type::ExcludeSuper))
			{
				for (int32 i = ClassHierarchy.Num() - 1; i >= 0; --i)
				{
					UClass* HierarchyClass = ClassHierarchy[i];
					for (auto& It : BindCallbacks)
					{
						auto& Data = It.Value;
						if (Data.BindedClasses.Contains(HierarchyClass))
						{
							Data.BindedClasses.Remove(HierarchyClass);
							Data.BindCallback.ExecuteIfBound(HierarchyClass, FString());
						}
					}
					BindedClasses.Remove(HierarchyClass);
				}
			}
		}
#endif

		for (int32 i = ClassHierarchy.Num() - 1; i >= 0; --i)
		{
			UClass* HierarchyClass = ClassHierarchy[i];
			if (!HierarchyClass->ImplementsInterface(InterfaceClass))
			{
				continue;
			}

			const UObject* CDO = HierarchyClass->GetDefaultObject(false);
			if (!CDO || CDO->HasAnyFlags(RF_NeedInitialization))
			{
				continue;
			}

			FString HierarchyModule = IPuertsInterface::Execute_GetJavaScriptModule(CDO);
			if (HierarchyModule.IsEmpty())
			{
				continue;
			}

			// 如果Module和Super一样，忽略，因为Super会去绑定
			UClass* SuperClass = HierarchyClass->GetSuperClass();
			if (SuperClass && SuperClass->ImplementsInterface(InterfaceClass))
			{
				const UObject* SuperCDO = SuperClass->GetDefaultObject(false);
				if (SuperCDO && !SuperCDO->HasAnyFlags(RF_NeedInitialization))
				{
					FString SuperModule = IPuertsInterface::Execute_GetJavaScriptModule(SuperCDO);
					if (HierarchyModule == SuperModule)
					{
						continue;
					}
				}
			}

#if WITH_EDITOR
			// 给绑定过的Class增加一个特殊的标记，用于判断是否绑定过，兼容重编译导致的重置
			if (!HierarchyClass->FindFunctionByName(SpecialFunctionName, EIncludeSuperFlag::Type::ExcludeSuper))
			{
				auto Func = NewObject<UFunction>(HierarchyClass, SpecialFunctionName);
				HierarchyClass->AddFunctionToFunctionMap(Func, SpecialFunctionName);
				BindedClasses.Emplace(HierarchyClass);
			}
#endif

			CallMixin(HierarchyClass, HierarchyModule, SpecificData);
		}
	}

	virtual void RegisterBindDelegate(const TSharedPtr<puerts::FJsEnv>& JsEnv
	                                  , const FPuertsAutoMixinDelegate& Callback) override
	{
		auto& AddedData = BindCallbacks.FindOrAdd(JsEnv);
		AddedData.BindCallback = Callback;

		// 处理在绑定前已经被创建的对象，只调用刚注册的Callback
		for (const auto Class : TObjectRange<UClass>())
		{
			TryBind(Class, &AddedData);
		}
	}

	void Reset()
	{
		BindCallbacks.Empty();

#if WITH_EDITOR
		BindedClasses.Empty();
#endif

		StopJavaScript();
	}

	void StartJavaScript()
	{
		UE_LOG(LogPuertsAutoMixin, Display, TEXT("PuertsAutoMixinSubsystem Start DefaultJavaScript"));

		UPuertsAutoMixinSetting* Setting = GetMutableDefault<UPuertsAutoMixinSetting>();
		if (!IsValid(Setting))
		{
			UE_LOG(LogPuertsAutoMixin, Error, TEXT("UPuertsAutoMixinSetting is invalid"));
			return;
		}
		std::function<void(const FString&)> SourceLoadedCallback = nullptr;
#if WITH_EDITOR
		SourceFileWatcher = MakeShared<PUERTS_NAMESPACE::FSourceFileWatcher>(
			[this](const FString& InPath)
			{
				HotReloadJavaScriptEnv(InPath);
			}
		);
		SourceLoadedCallback = [this](const FString& InPath)
		{
			UE_LOG(LogPuertsAutoMixin, Log, TEXT("Watch Path: %s"), *InPath);

			if (SourceFileWatcher.IsValid())
			{
				SourceFileWatcher->OnSourceLoaded(InPath);
			}
		};
#endif

		DefaultJsEnv = MakeShared<puerts::FJsEnv>(std::make_unique<puerts::DefaultJSModuleLoader>(
			                                          TEXT("JavaScript")
		                                          )
		                                          , std::make_shared<puerts::FDefaultLogger>()
		                                          , Setting->DebugPort
		                                          , SourceLoadedCallback

		);
		TArray<TPair<FString, UObject*>> Arguments;
		if (Setting->WaitDebugger)
		{
			DefaultJsEnv->WaitDebugger(Setting->WaitDebuggerTimeout);
		}
		DefaultJsEnv->Start(Setting->StartModule, Arguments);
	}

	void StopJavaScript()
	{
		UE_LOG(LogPuertsAutoMixin, Display, TEXT("PuertsAutoMixinSubsystem Stop DefaultJavaScript"));

		if (DefaultJsEnv.IsValid())
		{
			DefaultJsEnv.Reset();
		}
#if WITH_EDITOR
		if (SourceFileWatcher.IsValid())
		{
			SourceFileWatcher.Reset();
		}
#endif
	}

	void HotReloadJavaScriptEnv(const FString& Path)
	{
		if (DefaultJsEnv.IsValid())
		{
			TArray<uint8> Source;
			if (FFileHelper::LoadFileToArray(Source, *Path))
			{
				UE_LOG(LogPuertsAutoMixin, Log, TEXT("start ReloadSource %s"), *Path);
				DefaultJsEnv->ReloadSource(Path
				                           , puerts::PString(reinterpret_cast<const char*>(Source.GetData())
				                                             , Source.Num()
				                           )
				);
				UE_LOG(LogPuertsAutoMixin, Log, TEXT("end ReloadSource %s"), *Path);
			}
			else
			{
				UE_LOG(LogPuertsAutoMixin, Error, TEXT("read file fail for %s"), *Path);
			}
		}
	}

	void CallMixin(UClass* Class, const FString& Module, FPuertsAutoMixinData* SpecificData)
	{
		// 如果指定了特定的Data，只处理该Data
		if (SpecificData)
		{
			ExecuteMixin(*SpecificData, Class, Module);
			return;
		}

		// 否则遍历所有BindCallbacks
		for (auto& It : BindCallbacks)
		{
			const auto& JsEnv = It.Key;
			if (!JsEnv.IsValid())
			{
				BindCallbacks.Remove(It.Key);
				continue;
			}
			ExecuteMixin(It.Value, Class, Module);
		}
	}

	void ExecuteMixin(FPuertsAutoMixinData& Data, UClass* Class, const FString& Module)
	{
		if (Data.BindedClasses.Contains(Class))
		{
			return;
		}
		Data.BindedClasses.Emplace(Class);

		SCOPED_NAMED_EVENT(UPuertsAutoMixin_Mixin, FColor::Red);

		if (Data.BindCallback.IsBound())
		{
			Data.BindCallback.Execute(Class, Module);
		}
	}

private:
	bool bActive = false;
	FDelegateHandle OnAsyncLoadingFlushUpdateHandle;
	FCriticalSection CandidatesLock;
	TArray<FWeakObjectPtr> Candidates; // binding candidates during async loading

private:
	TMap<const TSharedPtr<puerts::FJsEnv>, FPuertsAutoMixinData> BindCallbacks;

private:
	TSharedPtr<puerts::FJsEnv> DefaultJsEnv;

#if WITH_EDITOR
	TSharedPtr<PUERTS_NAMESPACE::FSourceFileWatcher> SourceFileWatcher;
	TSet<TWeakObjectPtr<UClass>> BindedClasses;
#endif
};

IMPLEMENT_MODULE(FPuertsAutoMixinModule, PuertsAutoMixin)
