#include "PuertsAutoMixinSubsystem.h"

void UPuertsAutoMixinSubsystem::RegisterBindDelegate(const TSharedPtr<puerts::FJsEnv>& JsEnv
                                                    , const FPuertsAutoMixinDelegate& Callback)
{
	auto& Instance = GetInstance();
	FPuertsAutoMixinData Data;
	Data.BindCallback = Callback;
	Instance.BindCallbacks.Add(JsEnv, Data);

	// TODO处理在绑定前已经被创建的对象
}

void UPuertsAutoMixinSubsystem::Reset()
{
	auto& Instance = GetInstance();
	Instance.BindCallbacks.Empty();
}

void UPuertsAutoMixinSubsystem::CallMixin(const UClass* Class, const FString& Module)
{
	auto& Instance = GetInstance();
	for (auto& It : Instance.BindCallbacks)
	{
		const auto& JsEnv = It.Key;
		if (!JsEnv.IsValid())
		{
			Instance.BindCallbacks.Remove(It.Key);
			continue;
		}
		auto& Data = It.Value;
		if (Data.BindedClasses.Contains(Class))
		{
			continue;
		}
		Data.BindedClasses.Emplace(Class);
		if (Data.BindCallback.IsBound())
		{
			Data.BindCallback.Execute(Class, Module);
		}
	}
}
