#include "PuertsAutoMixinLibrary.h"

#include "PuertsInterface.h"

FString GetJavaScriptModule(const UObject* Object)
{
	const UObject* CDO;
	if (Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		CDO = Object;
	}
	else
	{
		const auto Class = Cast<UClass>(Object);
		CDO = Class ? Class->GetDefaultObject() : Object->GetClass()->GetDefaultObject();
	}
	if (CDO->HasAnyFlags(RF_NeedInitialization))
	{
		return "";
	}
	if (!CDO->GetClass()->ImplementsInterface(UPuertsInterface::StaticClass()))
	{
		return "";
	}
	return IPuertsInterface::Execute_GetJavaScriptModule(CDO);
}
