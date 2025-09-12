// Copyright DiDaPiPa 2024

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class PUERTSAUTOMIXINEDITOR_API IPuertsAutoMixinEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override = 0;
	virtual void ShutdownModule() override = 0;
};
