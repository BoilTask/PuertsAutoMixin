#pragma once
#include "JsEnv.h"

#include "PuertsAutoBindSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FPuertsAutoBindDelegate, const UClass*, Class, const FString&, Module);

struct FPuertsAutoBindData
{
	FPuertsAutoBindDelegate BindCallback;
	TSet<const UClass*> BindedClasses;
};

UCLASS()
class PUERTSAUTOBIND_API UPuertsAutoBindSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	static UPuertsAutoBindSubsystem& GetInstance()
	{
		return *GEngine->GetEngineSubsystem<UPuertsAutoBindSubsystem>();
	}

	void RegisterBindDelegate(const TSharedPtr<puerts::FJsEnv>& JsEnv, const FPuertsAutoBindDelegate& Callback);
	void Reset();
	void CallMixin(const UClass* Class, const FString& Module);

private:
	TMap<const TSharedPtr<puerts::FJsEnv>, FPuertsAutoBindData> BindCallbacks;
};
