#pragma once

#include "UObject/Interface.h"

#include "PuertsInterface.generated.h"

UINTERFACE()
class PUERTSAUTOBIND_API UPuertsInterface : public UInterface
{
	GENERATED_BODY()
};

class PUERTSAUTOBIND_API IPuertsInterface
{
	GENERATED_BODY()

public:
	/**
	 * 返回模块路径，如果没有export default，那么需要指定类名，比如 "./GameModule:MyClass"
	 */
	UFUNCTION(BlueprintNativeEvent)
	FString GetJavaScriptModule() const;
};
