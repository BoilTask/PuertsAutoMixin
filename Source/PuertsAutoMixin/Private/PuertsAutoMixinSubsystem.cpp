#include "PuertsAutoMixinSubsystem.h"

#include "GameDelegates.h"

#include "PuertsAutoMixinModule.h"
#include "PuertsAutoMixinSetting.h"
#include "PuertsInterface.h"

constexpr EInternalObjectFlags AsyncObjectFlags = EInternalObjectFlags_AsyncLoading | EInternalObjectFlags::Async;

void UPuertsAutoMixinSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem StartupModule"));

	Super::Initialize(Collection);

#if WITH_EDITOR
	bool bOnlyGame = IsRunningGame() || IsRunningDedicatedServer();
	const auto& Setting = GetMutableDefault<UPuertsAutoMixinSetting>();
	if (!bOnlyGame)
	{
		FEditorDelegates::PreBeginPIE.AddUObject(this, &UPuertsAutoMixinSubsystem::OnPreBeginPIE);
		FEditorDelegates::EndPIE.AddUObject(this, &UPuertsAutoMixinSubsystem::OnEndPIE);
		FGameDelegates::Get().GetEndPlayMapDelegate().AddUObject(this, &UPuertsAutoMixinSubsystem::OnEndPlayMap);
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

void UPuertsAutoMixinSubsystem::Deinitialize()
{
	UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem ShutdownModule"));

	StopBind();

	StopJavaScript();

	Super::Deinitialize();
}

void UPuertsAutoMixinSubsystem::StartAutoBind()
{
	UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem StartAutoBind"));

	if (bActive)
	{
		return;
	}

	GUObjectArray.AddUObjectCreateListener(this);
	GUObjectArray.AddUObjectDeleteListener(this);

	OnAsyncLoadingFlushUpdateHandle = FCoreDelegates::OnAsyncLoadingFlushUpdate.AddUObject(
		this
		, &UPuertsAutoMixinSubsystem::OnAsyncLoadingFlushUpdate
	);

	bActive = true;
}

void UPuertsAutoMixinSubsystem::StopBind()
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

void UPuertsAutoMixinSubsystem::StopListen()
{
	UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem StopListen"));

	GUObjectArray.RemoveUObjectCreateListener(this);
	GUObjectArray.RemoveUObjectDeleteListener(this);

	FCoreDelegates::OnAsyncLoadingFlushUpdate.Remove(OnAsyncLoadingFlushUpdateHandle);
}

void UPuertsAutoMixinSubsystem::OnPreBeginPIE(bool bIsSimulating)
{
	StartAutoBind();

	auto Setting = GetMutableDefault<UPuertsAutoMixinSetting>();
	if (!Setting->bEnableEnvInGame)
	{
		StopJavaScript();
	}
}

void UPuertsAutoMixinSubsystem::OnEndPIE(bool bIsSimulating)
{
	const auto& Setting = GetMutableDefault<UPuertsAutoMixinSetting>();
	if (Setting->bEnableEnvInEditor && !Setting->bEnableEnvInGame)
	{
		StartJavaScript();
	}
}

void UPuertsAutoMixinSubsystem::OnEndPlayMap()
{
	const auto& Setting = GetMutableDefault<UPuertsAutoMixinSetting>();
	if (!Setting->bEnableEnvInEditor && !Setting->bEnableEnvInGame)
	{
		StopBind();
	}
}

void UPuertsAutoMixinSubsystem::BindMixin(const FPuertsAutoMixinDelegate& BindCallback)
{
	RegisterBindDelegate(DefaultJsEnv, BindCallback);
}

void UPuertsAutoMixinSubsystem::NotifyUObjectCreated(const UObjectBase* ObjectBase, int32 Index)
{
	UObject* Object = (UObject*)ObjectBase;
	// UE_LOG(LogPuertsAutoMixin, Display, TEXT("NotifyUObjectCreated:%s"), *Object->GetName());
	TryBind(Object);
}

void UPuertsAutoMixinSubsystem::NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index)
{
	// UObject* Object = (UObject*)ObjectBase;
	// UE_LOG(LogPuertsAutoMixin, Display, TEXT("NotifyUObjectDeleted:%s"), *Object->GetName());
}

void UPuertsAutoMixinSubsystem::OnUObjectArrayShutdown()
{
	UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinSubsystem OnUObjectArrayShutdown"));

	if (!bActive)
	{
		return;
	}

	StopListen();

	bActive = false;
}

void UPuertsAutoMixinSubsystem::OnAsyncLoadingFlushUpdate()
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
		TryBind(Object);
	}
}

void UPuertsAutoMixinSubsystem::TryBind(UObject* Object, FPuertsAutoMixinData* SpecificData)
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
			// FScopeLock Lock(&CandidatesLock);
			// Candidates.AddUnique(Object);
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

	for (int32 i = ClassHierarchy.Num() - 1; i >= 0; --i)
	{
		UClass* HierarchyClass = ClassHierarchy[i];
		if (!HierarchyClass->ImplementsInterface(InterfaceClass))
		{
			continue;
		}

		const UObject* CDO;
		if (Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			CDO = Object;
		}
		else
		{
			CDO = Class ? Class->GetDefaultObject() : Object->GetClass()->GetDefaultObject();
		}
		if (!CDO || CDO->HasAnyFlags(RF_NeedInitialization))
		{
			continue;
		}

		FString HierarchyModule = IPuertsInterface::Execute_GetJavaScriptModule(CDO);
		if (HierarchyModule.IsEmpty())
		{
			continue;
		}
		CallMixin(HierarchyClass, HierarchyModule, SpecificData);
	}
}

void UPuertsAutoMixinSubsystem::RegisterBindDelegate(const TSharedPtr<puerts::FJsEnv>& JsEnv
                                                     , const FPuertsAutoMixinDelegate& Callback)
{
	FPuertsAutoMixinData Data;
	Data.BindCallback = Callback;
	FPuertsAutoMixinData& AddedData = BindCallbacks.Add(JsEnv, Data);

	// 处理在绑定前已经被创建的对象，只调用刚注册的Callback
	for (const auto Class : TObjectRange<UClass>())
	{
		TryBind(Class, &AddedData);
	}
}

void UPuertsAutoMixinSubsystem::Reset()
{
	BindCallbacks.Empty();
}

void UPuertsAutoMixinSubsystem::StartJavaScript()
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
	Arguments.Add(TPair<FString, UObject*>(TEXT("JsHandler"), this));
	if (Setting->WaitDebugger)
	{
		DefaultJsEnv->WaitDebugger(Setting->WaitDebuggerTimeout);
	}
	DefaultJsEnv->Start(Setting->StartModule, Arguments);
}

void UPuertsAutoMixinSubsystem::StopJavaScript()
{
	UE_LOG(LogPuertsAutoMixin, Display, TEXT("PuertsAutoMixinSubsystem Stop DefaultJavaScript"));

	if (DefaultJsEnv.IsValid())
	{
		DefaultJsEnv.Reset();
	}
	if (SourceFileWatcher.IsValid())
	{
		SourceFileWatcher.Reset();
	}
}

void UPuertsAutoMixinSubsystem::HotReloadJavaScriptEnv(const FString& Path)
{
	if (DefaultJsEnv.IsValid())
	{
		TArray<uint8> Source;
		if (FFileHelper::LoadFileToArray(Source, *Path))
		{
			UE_LOG(LogPuertsAutoMixin, Log, TEXT("start ReloadSource %s"), *Path);
			DefaultJsEnv->ReloadSource(Path
			                           , puerts::PString(reinterpret_cast<const char*>(Source.GetData()), Source.Num())
			);
			UE_LOG(LogPuertsAutoMixin, Log, TEXT("end ReloadSource %s"), *Path);
		}
		else
		{
			UE_LOG(LogPuertsAutoMixin, Error, TEXT("read file fail for %s"), *Path);
		}
	}
}

void UPuertsAutoMixinSubsystem::CallMixin(UClass* Class, const FString& Module, FPuertsAutoMixinData* SpecificData)
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

void UPuertsAutoMixinSubsystem::ExecuteMixin(FPuertsAutoMixinData& Data, UClass* Class, const FString& Module)
{
#if WITH_EDITOR
	static FName SpecialFunctionName = FName(TEXT("__PuertsAutoMixinSucceeded"));
#endif

	if (Data.BindedClasses.Contains(Class))
	{
#if WITH_EDITOR
		// 兼容蓝图Recompile导致FuncMap被清空的情况
		if (Class->FindFunctionByName(SpecialFunctionName, EIncludeSuperFlag::Type::ExcludeSuper))
		{
			return;
		}
		Data.BindedModules.Remove(Module);
		if (Data.BindCallback.IsBound())
		{
			Data.BindCallback.Execute(Class, "");
		}
#else
		return;
#endif
	}
	if (Data.BindedModules.Contains(Module))
	{
		return;
	}
	Data.BindedClasses.Emplace(Class);
	Data.BindedModules.Emplace(Module);
	Data.ClassToModule.Add(Class, Module);

	SCOPED_NAMED_EVENT(UPuertsAutoMixin_Mixin, FColor::Red);

#if WITH_EDITOR
	// 给绑定过的Class增加一个特殊的标记，用于判断是否绑定过，兼容重编译导致的重置
	auto Func = NewObject<UFunction>(Class, SpecialFunctionName);
	Class->AddFunctionToFunctionMap(Func, SpecialFunctionName);
#endif

	if (Data.BindCallback.IsBound())
	{
		Data.BindCallback.Execute(Class, Module);
	}
}
