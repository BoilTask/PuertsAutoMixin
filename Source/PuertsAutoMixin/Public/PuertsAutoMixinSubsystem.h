#pragma once
#include "JsEnv.h"

#include "PuertsAutoMixinSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FPuertsAutoMixinDelegate, const UClass*, Class, const FString&, Module);

struct FPuertsAutoMixinData
{
	FPuertsAutoMixinDelegate BindCallback;
	TSet<const UClass*> BindedClasses;
};

UCLASS()
class PUERTSAUTOMIXIN_API UPuertsAutoMixinSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	static UPuertsAutoMixinSubsystem& GetInstance()
	{
		return *GEngine->GetEngineSubsystem<UPuertsAutoMixinSubsystem>();
	}

	void RegisterBindDelegate(const TSharedPtr<puerts::FJsEnv>& JsEnv, const FPuertsAutoMixinDelegate& Callback);
	void Reset();
	void CallMixin(const UClass* Class, const FString& Module);

private:
	TMap<const TSharedPtr<puerts::FJsEnv>, FPuertsAutoMixinData> BindCallbacks;
};
