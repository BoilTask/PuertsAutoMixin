#pragma once

#include "CoreMinimal.h"
#include "JsEnv.h"
#include "Modules/ModuleInterface.h"

class FPuertsAutoMixinDelegate;

PUERTSAUTOMIXIN_API DECLARE_LOG_CATEGORY_EXTERN(LogPuertsAutoMixin, Log, All);

class PUERTSAUTOMIXIN_API IPuertsAutoMixinModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override = 0;
	virtual void ShutdownModule() override = 0;

	virtual void RegisterBindDelegate(const TSharedPtr<puerts::FJsEnv>& JsEnv, const FPuertsAutoMixinDelegate& Callback) = 0;
};
