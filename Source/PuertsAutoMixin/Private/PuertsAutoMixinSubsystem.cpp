#include "PuertsAutoMixinSubsystem.h"

#include "PuertsInterface.h"

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
	TArray<const UClass*> ClassHierarchy;
	const UClass* CurrentClass = Class;
	while (CurrentClass && CurrentClass->IsChildOf<UObject>())
	{
		ClassHierarchy.Add(CurrentClass);
		CurrentClass = CurrentClass->GetSuperClass();
	}

	for (int32 i = ClassHierarchy.Num() - 1; i >= 0; --i)
	{
		const UClass* HierarchyClass = ClassHierarchy[i];
		if (!HierarchyClass->ImplementsInterface(InterfaceClass))
		{
			continue;
		}

		const UObject* CDO = HierarchyClass->GetDefaultObject();
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
	auto& Instance = GetInstance();
	FPuertsAutoMixinData Data;
	Data.BindCallback = Callback;
	FPuertsAutoMixinData& AddedData = Instance.BindCallbacks.Add(JsEnv, Data);

	// 处理在绑定前已经被创建的对象，只调用刚注册的Callback
	for (const auto Class : TObjectRange<UClass>())
	{
		TryBind(Class, &AddedData);
	}
}

void UPuertsAutoMixinSubsystem::Reset()
{
	auto& Instance = GetInstance();
	Instance.BindCallbacks.Empty();
}

void UPuertsAutoMixinSubsystem::CallMixin(const UClass* Class, const FString& Module, FPuertsAutoMixinData* SpecificData)
{
	auto& Instance = GetInstance();

	// 如果指定了特定的Data，只处理该Data
	if (SpecificData)
	{
		ExecuteMixin(*SpecificData, Class, Module);
		return;
	}

	// 否则遍历所有BindCallbacks
	for (auto& It : Instance.BindCallbacks)
	{
		const auto& JsEnv = It.Key;
		if (!JsEnv.IsValid())
		{
			Instance.BindCallbacks.Remove(It.Key);
			continue;
		}
		ExecuteMixin(It.Value, Class, Module);
	}
}

void UPuertsAutoMixinSubsystem::ExecuteMixin(FPuertsAutoMixinData& Data, const UClass* Class, const FString& Module)
{
	if (Data.BindedClasses.Contains(Class) || Data.BindedModules.Contains(Module))
	{
		return;
	}
	Data.BindedClasses.Emplace(Class);
	Data.BindedModules.Emplace(Module);
	if (Data.BindCallback.IsBound())
	{
		SCOPED_NAMED_EVENT(UPuertsAutoMixin_Mixin, FColor::Red);
		Data.BindCallback.Execute(Class, Module);
	}
}