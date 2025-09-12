#include "PuertsAutoMixinEditorLibrary.h"

#include "PuertsAutoMixinLibrary.h"
#include "PuertsInterface.h"

EPuertsBindingStatus GetBindingStatus(const UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return EPuertsBindingStatus::NotBound;
	}

	if (Blueprint->Status == EBlueprintStatus::BS_Dirty)
	{
		return EPuertsBindingStatus::Unknown;
	}

	const auto Target = Blueprint->GeneratedClass;

	if (!IsValid(Target))
	{
		return EPuertsBindingStatus::NotBound;
	}

	if (!Target->ImplementsInterface(UPuertsInterface::StaticClass()))
	{
		return EPuertsBindingStatus::NotBound;
	}

	const auto ModuleName = GetJavaScriptModule(Target);
	if (ModuleName.IsEmpty())
	{
		return EPuertsBindingStatus::Unknown;
	}
	if (!FPaths::FileExists(GetScriptRealPath(ModuleName)))
	{
		return EPuertsBindingStatus::BoundButInvalid;
	}
	return EPuertsBindingStatus::Bound;
}

FString GetScriptRealPath(FString Module)
{
	const auto& RootPath = TEXT("TypeScript");
	const auto ScriptPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() + RootPath);
	// 提取ModuleName最后一个:前的文本
	const auto ModulePath = Module.Contains(TEXT(":"))
									  ? Module.LeftChop(Module.Find(TEXT(":")))
									  : Module;
	const auto RelativePath = ModulePath + TEXT(".ts");
	return ScriptPath + "/" + RelativePath;
}
