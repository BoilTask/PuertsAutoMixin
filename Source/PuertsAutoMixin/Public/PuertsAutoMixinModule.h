// Copyright DiDaPiPa 2024

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class PUERTSAUTOBIND_API IPuertsAutoBindModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override = 0;
	virtual void ShutdownModule() override = 0;
};
