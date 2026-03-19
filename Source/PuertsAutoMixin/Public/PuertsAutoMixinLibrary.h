#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "PuertsAutoMixinLibrary.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FPuertsAutoMixinDelegate, const UClass*, Class, const FString&, Module);

UCLASS()
class PUERTSAUTOMIXIN_API UPuertsAutoMixinLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static FString GetJavaScriptModule(const UObject* Object);
};
