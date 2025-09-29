#include "PuertsAutoMixinEditorModule.h"

#include "PuertsAutoMixinModule.h"
#include "Command/PuertsAutoMixinEditorCommands.h"
#include "Toolbar/AnimationBlueprintToolbar.h"
#include "Toolbar/BlueprintToolbar.h"
#include "Toolbar/PuertsAutoMixinEditorStyle.h"

class FPuertsAutoMixinEditorModule : public IPuertsAutoMixinEditorModule
{
	virtual void StartupModule() override
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinEditorModule StartupModule"));

		Style = FPuertsAutoMixinEditorStyle::GetInstance();

		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FPuertsAutoMixinEditorModule::OnPostEngineInit);

		FPuertsAutoMixinEditorCommands::Register();

		BlueprintToolbar = MakeShareable(new FBlueprintToolbar);
		AnimationBlueprintToolbar = MakeShareable(new FAnimationBlueprintToolbar);
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinEditorModule ShutdownModule"));

		FPuertsAutoMixinEditorCommands::Unregister();
		FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	}

private:
	void OnPostEngineInit()
	{
		// 忽略非Editor的情况
		if (!GEditor)
		{
			return;
		}

		BlueprintToolbar->Initialize();
		AnimationBlueprintToolbar->Initialize();
	}

private:
	TSharedPtr<FBlueprintToolbar> BlueprintToolbar;
	TSharedPtr<FAnimationBlueprintToolbar> AnimationBlueprintToolbar;
	TSharedPtr<ISlateStyle> Style;
};

IMPLEMENT_MODULE(FPuertsAutoMixinEditorModule, PuertsAutoMixinEditor)
