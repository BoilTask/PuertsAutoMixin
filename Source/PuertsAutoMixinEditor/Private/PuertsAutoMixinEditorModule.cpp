#include "PuertsAutoMixinEditorModule.h"

#include "PuertsAutoMixinSubsystem.h"

#include "ISettingsModule.h"
#include "PuertsAutoMixinModule.h"
#include "PuertsAutoMixinSetting.h"
#include "Command/PuertsAutoMixinEditorCommands.h"
#include "Toolbar/AnimationBlueprintToolbar.h"
#include "Toolbar/BlueprintToolbar.h"
#include "Toolbar/PuertsAutoMixinEditorStyle.h"

#define LOCTEXT_NAMESPACE "PuertsAutoMixinEditor"

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

		const auto SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
		if (SettingsModule)
		{
			SettingsModule->RegisterSettings("Project"
			                                 , "Plugins"
			                                 , "PuertsAutoMixin"
			                                 , LOCTEXT("PuertsAutoMixinEditorSetting_PuertsAutoMixin"
			                                           , "PuertsAutoMixin"
			                                 )
			                                 , LOCTEXT("PuertsAutoMixinEditorSetting_PuertsAutoMixin_Description"
			                                           , "configuration for PuertsAutoMixin."
			                                 )
			                                 , GetMutableDefault<UPuertsAutoMixinSetting>()
			);
		}
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
