#include "PuertsAutoMixinModule.h"

#include "PuertsAutoMixinSubsystem.h"

DEFINE_LOG_CATEGORY(LogPuertsAutoMixin);

class PUERTSAUTOMIXIN_API FPuertsAutoMixinModule : public IPuertsAutoMixinModule
{
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FPuertsAutoMixinModule, PuertsAutoMixin)
