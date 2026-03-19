#include "PuertsAutoMixinEditorToolbar.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "BlueprintEditor.h"
#include "PuertsAutoMixinModule.h"
#include "PuertsInterface.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Layout/Children.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ToolMenus.h"
#include "Command/PuertsAutoMixinEditorCommands.h"
#include "Library/PuertsAutoMixinEditorLibrary.h"
#include "Runtime/Slate/Private/Framework/Docking/SDockingTabWell.h"
#include "K2Node_FunctionResult.h"

#define LOCTEXT_NAMESPACE "FUnPuertsEditorModule"

FPuertsAutoMixinEditorToolbar::FPuertsAutoMixinEditorToolbar()
	: CommandList(new FUICommandList),
	  ContextObject(nullptr)
{
}

void FPuertsAutoMixinEditorToolbar::Initialize()
{
	BindCommands();
}

void FPuertsAutoMixinEditorToolbar::BindCommands()
{
	const auto& Commands = FPuertsAutoMixinEditorCommands::Get();
	CommandList->MapAction(Commands.CreatePuertsTemplate
	                       , FExecuteAction::CreateRaw(this, &FPuertsAutoMixinEditorToolbar::CreatePuertsTemplate_Executed)
	);
	CommandList->MapAction(Commands.CopyAsRelativePath
	                       , FExecuteAction::CreateRaw(this
	                                                   , &FPuertsAutoMixinEditorToolbar::CopyAsRelativePath_Executed
	                       )
	);
	CommandList->MapAction(Commands.BindToPuerts
	                       , FExecuteAction::CreateRaw(this, &FPuertsAutoMixinEditorToolbar::BindToPuerts_Executed)
	);
	CommandList->MapAction(Commands.UnbindFromPuerts
	                       , FExecuteAction::CreateRaw(this, &FPuertsAutoMixinEditorToolbar::UnbindFromPuerts_Executed)
	);
	CommandList->MapAction(Commands.RevealInExplorer
	                       , FExecuteAction::CreateRaw(this, &FPuertsAutoMixinEditorToolbar::RevealInExplorer_Executed)
	);
	CommandList->MapAction(Commands.DefaultModulePath
	                       , FExecuteAction::CreateRaw(this, &FPuertsAutoMixinEditorToolbar::DefaultModule_Executed)
	);
}

void FPuertsAutoMixinEditorToolbar::BuildToolbar(FToolBarBuilder& ToolbarBuilder, UObject* InContextObject)
{
	if (!InContextObject)
	{
		return;
	}

	ToolbarBuilder.BeginSection(NAME_None);

	const auto Blueprint = Cast<UBlueprint>(InContextObject);
	ToolbarBuilder.AddComboButton(
		FUIAction()
		, FOnGetContent::CreateLambda([&, Blueprint, InContextObject]()
			{
				ContextObject = InContextObject;
				const auto BindingStatus = GetBindingStatus(Blueprint);
				const FPuertsAutoMixinEditorCommands& Commands = FPuertsAutoMixinEditorCommands::Get();
				FMenuBuilder MenuBuilder(true, CommandList);
				if (BindingStatus == NotBound)
				{
					MenuBuilder.AddMenuEntry(Commands.BindToPuerts, NAME_None, LOCTEXT("Bind", "Bind"));
				}
				else
				{
					MenuBuilder.AddMenuEntry(Commands.CopyAsRelativePath
					                         , NAME_None
					                         , LOCTEXT("CopyAsRelativePath", "Copy as Relative Path")
					);
					MenuBuilder.AddMenuEntry(Commands.RevealInExplorer
					                         , NAME_None
					                         , LOCTEXT("RevealInExplorer", "Reveal in Explorer")
					);
					MenuBuilder.AddMenuEntry(Commands.CreatePuertsTemplate
					                         , NAME_None
					                         , LOCTEXT("CreatePuertsTemplate", "Create Puerts Template")
					);
					MenuBuilder.AddMenuEntry(Commands.DefaultModulePath
					                         , NAME_None
					                         , LOCTEXT("DefaultModulePath", "Default Module Path")
					);
					MenuBuilder.AddMenuEntry(Commands.UnbindFromPuerts, NAME_None, LOCTEXT("Unbind", "Unbind"));
				}
				return MenuBuilder.MakeWidget();
			}
		)
		, LOCTEXT("PuertsAutoMixin_Label", "PuertsAutoMixin")
		, LOCTEXT("PuertsAutoMixin_ToolTip", "PuertsAutoMixin")
		, TAttribute<FSlateIcon>::Create([Blueprint]
			{
				const auto BindingStatus = GetBindingStatus(Blueprint);
				FString InStyleName;
				switch (BindingStatus)
				{
				case Unknown:
					InStyleName = "PuertsAutoMixinEditor.Status_Unknown";
					break;
				case NotBound:
					InStyleName = "PuertsAutoMixinEditor.Status_NotBound";
					break;
				case Bound:
					InStyleName = "PuertsAutoMixinEditor.Status_Bound";
					break;
				case BoundButInvalid:
					InStyleName = "PuertsAutoMixinEditor.Status_BoundButInvalid";
					break;
				default:
					check(false);
				}

				return FSlateIcon("PuertsAutoMixinEditorStyle", *InStyleName);
			}
		)
	);

	ToolbarBuilder.EndSection();

	BuildNodeMenu();
}

void FPuertsAutoMixinEditorToolbar::BuildNodeMenu()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* BPMenu = UToolMenus::Get()->ExtendMenu("GraphEditor.GraphNodeContextMenu.K2Node_FunctionResult");
	BPMenu->AddDynamicSection("PuertsAutoMixin"
	                          , FNewToolMenuDelegate::CreateLambda([this](UToolMenu* ToolMenu)
		                          {
			                          UGraphNodeContextMenuContext* GraphNodeCtx = ToolMenu->FindContext<
				                          UGraphNodeContextMenuContext>();
			                          if (GraphNodeCtx && GraphNodeCtx->Graph)
			                          {
				                          if (GraphNodeCtx->Graph->GetName() == "GetJavaScriptModule")
				                          {
					                          FToolMenuSection& PuertsAutoMixinSection = ToolMenu->AddSection(
						                          "PuertsAutoMixin"
						                          , FText::FromString("PuertsAutoMixin")
					                          );
					                          PuertsAutoMixinSection.AddEntry(
						                          FToolMenuEntry::InitMenuEntryWithCommandList(
							                          FPuertsAutoMixinEditorCommands::Get().RevealInExplorer
							                          , CommandList
							                          , LOCTEXT("RevealInExplorer", "Reveal in Explorer")
						                          )
					                          );
				                          }
			                          }
		                          }
	                          )
	                          , FToolMenuInsert(NAME_None, EToolMenuInsertType::First)
	);
}

TSharedRef<FExtender> FPuertsAutoMixinEditorToolbar::GetExtender(UObject* InContextObject)
{
	TSharedRef<FExtender> ToolbarExtender(new FExtender());
	const auto ExtensionDelegate = FToolBarExtensionDelegate::CreateLambda(
		[this, InContextObject](FToolBarBuilder& ToolbarBuilder)
		{
			BuildToolbar(ToolbarBuilder, InContextObject);
		}
	);
	ToolbarExtender->AddToolBarExtension("Debugging", EExtensionHook::After, CommandList, ExtensionDelegate);
	return ToolbarExtender;
}

void FPuertsAutoMixinEditorToolbar::BindToPuerts_Executed() const
{
	const auto Blueprint = Cast<UBlueprint>(ContextObject);
	if (!IsValid(Blueprint))
	{
		return;
	}

	const auto TargetClass = Blueprint->GeneratedClass;
	if (!IsValid(TargetClass))
	{
		return;
	}

	if (TargetClass->ImplementsInterface(UPuertsInterface::StaticClass()))
	{
		return;
	}

#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION >= 1
	const auto Ok = FBlueprintEditorUtils::ImplementNewInterface(Blueprint, UPuertsInterface::StaticClass()->GetClassPathName());
#else
	const auto Ok = FBlueprintEditorUtils::ImplementNewInterface(Blueprint, FName("PuertsInterface"));
#endif

	if (!Ok)
	{
		return;
	}

	const auto Package = Blueprint->GetPackage();
	FString PuertsModuleName = Package->GetName();

	if (!PuertsModuleName.IsEmpty())
	{
		const auto InterfaceDesc = *Blueprint->ImplementedInterfaces.FindByPredicate(
			[](const FBPInterfaceDescription& Desc)
			{
				return Desc.Interface == UPuertsInterface::StaticClass();
			}
		);
		InterfaceDesc.Graphs[0]->Nodes[1]->Pins[1]->DefaultValue = PuertsModuleName;
	}

#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)

	const auto BlueprintEditors = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet").
		GetBlueprintEditors();
	for (auto BlueprintEditor : BlueprintEditors)
	{
		const auto MyBlueprintEditor = static_cast<FBlueprintEditor*>(&BlueprintEditors[0].Get());
		if (!MyBlueprintEditor || MyBlueprintEditor->GetBlueprintObj() != Blueprint)
		{
			continue;
		}
		MyBlueprintEditor->Compile();

		const auto Func = Blueprint->GeneratedClass->FindFunctionByName(FName("GetJavaScriptModule"));
		const auto GraphToOpen = FBlueprintEditorUtils::FindScopeGraph(Blueprint, Func);
		MyBlueprintEditor->OpenGraphAndBringToFront(GraphToOpen);
	}

#endif
}

void FPuertsAutoMixinEditorToolbar::UnbindFromPuerts_Executed() const
{
	const auto Blueprint = Cast<UBlueprint>(ContextObject);
	if (!IsValid(Blueprint))
	{
		return;
	}

	const auto TargetClass = Blueprint->GeneratedClass;
	if (!IsValid(TargetClass))
	{
		return;
	}

	if (!TargetClass->ImplementsInterface(UPuertsInterface::StaticClass()))
	{
		return;
	}

#if ENGINE_MAJOR_VERSION >=5 && ENGINE_MINOR_VERSION >= 1
	FBlueprintEditorUtils::RemoveInterface(Blueprint, UPuertsInterface::StaticClass()->GetClassPathName());
#else
	FBlueprintEditorUtils::RemoveInterface(Blueprint, FName("PuertsInterface"));
#endif

#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)

	const auto BlueprintEditors = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet").
		GetBlueprintEditors();
	for (auto BlueprintEditor : BlueprintEditors)
	{
		const auto MyBlueprintEditor = static_cast<FBlueprintEditor*>(&BlueprintEditors[0].Get());
		if (!MyBlueprintEditor || MyBlueprintEditor->GetBlueprintObj() != Blueprint)
		{
			continue;
		}
		MyBlueprintEditor->Compile();
		MyBlueprintEditor->RefreshEditors();
	}

#endif

	const auto ActiveTab = FGlobalTabmanager::Get()->GetActiveTab();
	if (!ActiveTab)
	{
		return;
	}

	const auto DockingTabWell = ActiveTab->GetParent();
	if (!DockingTabWell)
	{
		return;
	}

	const auto DockTabs = DockingTabWell->GetChildren(); // DockingTabWell->GetTabs();
	for (auto i = 0; i < DockTabs->Num(); i++)
	{
		const auto DockTab = StaticCastSharedRef<SDockTab>(DockTabs->GetChildAt(i));
		const auto Label = DockTab->GetTabLabel();
		if (Label.ToString().Equals("$$ Get Module Name $$"))
		{
			DockTab->RequestCloseTab();
		}
	}
}

void FPuertsAutoMixinEditorToolbar::CreatePuertsTemplate_Executed()
{
	const auto Blueprint = Cast<UBlueprint>(ContextObject);
	if (!IsValid(Blueprint))
	{
		return;
	}

	UClass* Class = Blueprint->GeneratedClass;

	const auto Func = Class->FindFunctionByName(FName("GetJavaScriptModule"));
	if (!IsValid(Func))
	{
		return;
	}

	FString ModuleName;
	Class->GetDefaultObject()->ProcessEvent(Func, &ModuleName);

	if (ModuleName.IsEmpty())
	{
		FNotificationInfo Info(LOCTEXT("ModuleNameRequired", "Please specify a module name first"));
		Info.ExpireDuration = 5;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	TArray<FString> ModuleNameParts;
	ModuleName.ParseIntoArray(ModuleNameParts, TEXT("."));
	const auto TemplateName = ModuleNameParts.Last();

	const auto FileName = GetScriptRealPath(ModuleName);

	if (FPaths::FileExists(FileName))
	{
		UE_LOG(LogPuertsAutoMixin
		       , Warning
		       , TEXT("%s")
		       , *FText::Format(LOCTEXT("FileAlreadyExists", "ts file ({0}) is already existed!"), FText::FromString(
			       TemplateName)).ToString()
		);
		return;
	}

	static FString BaseDir = IPluginManager::Get().FindPlugin(TEXT("PuertsAutoMixin"))->GetBaseDir();

	bool bFoundTemplate = false;
	for (auto TemplateClass = Class; TemplateClass; TemplateClass = TemplateClass->GetSuperClass())
	{
		auto TemplateClassName = TemplateClass->GetName().EndsWith("_C")
			                         ? TemplateClass->GetName().LeftChop(2)
			                         : TemplateClass->GetName();
		auto RelativeFilePath = "Config/TsTemplates" / TemplateClassName + ".ts";
		auto FullFilePath = FPaths::ProjectDir() / RelativeFilePath;
		if (!FPaths::FileExists(FullFilePath))
		{
			FullFilePath = BaseDir / RelativeFilePath;
		}

		if (!FPaths::FileExists(FullFilePath))
		{
			continue;
		}

		const auto Package = Blueprint->GetPackage();
		FString PuertsModuleName = Package->GetName();
		PuertsModuleName = PuertsModuleName.Replace(TEXT("/"), TEXT("."));
		PuertsModuleName = TEXT("UE") + PuertsModuleName;
		PuertsModuleName = PuertsModuleName + TEXT(".") + Class->GetName();
		FString Content;
		FFileHelper::LoadFileToString(Content, *FullFilePath);
		Content = Content.Replace(TEXT("ModuleName"), *Class->GetName());
		Content = Content.Replace(TEXT("ClassName"), *PuertsModuleName);

		FFileHelper::SaveStringToFile(Content, *FileName, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

		bFoundTemplate = true;
		break;
	}
	if (!bFoundTemplate)
	{
		UE_LOG(LogPuertsAutoMixin
		       , Error
		       , TEXT("No template found for class %s and its super classes!"), *Class->GetName()
		);
	}
}

void FPuertsAutoMixinEditorToolbar::RevealInExplorer_Executed()
{
	const auto Blueprint = Cast<UBlueprint>(ContextObject);
	if (!IsValid(Blueprint))
	{
		return;
	}

	const auto TargetClass = Blueprint->GeneratedClass;
	if (!IsValid(TargetClass))
	{
		return;
	}

	if (!TargetClass->ImplementsInterface(UPuertsInterface::StaticClass()))
	{
		return;
	}

	const auto Func = TargetClass->FindFunctionByName(FName("GetJavaScriptModule"));
	if (!IsValid(Func))
	{
		return;
	}

	FString ModuleName;
	const auto DefaultObject = TargetClass->GetDefaultObject();
	DefaultObject->UObject::ProcessEvent(Func, &ModuleName);

	const auto FileName = GetScriptRealPath(ModuleName);

	if (IFileManager::Get().FileExists(*FileName))
	{
		FPlatformProcess::ExploreFolder(*FileName);
	}
	else
	{
		FNotificationInfo NotificationInfo(FText::FromString("PuertsAutoMixin Notification"));
		NotificationInfo.Text = LOCTEXT("FileNotExist", "The file does not exist.");
		NotificationInfo.bFireAndForget = true;
		NotificationInfo.ExpireDuration = 100.0f;
		NotificationInfo.bUseThrobber = true;
		FSlateNotificationManager::Get().AddNotification(NotificationInfo);
	}
}

void FPuertsAutoMixinEditorToolbar::CopyAsRelativePath_Executed() const
{
	const auto Blueprint = Cast<UBlueprint>(ContextObject);
	if (!IsValid(Blueprint))
	{
		return;
	}

	const auto TargetClass = Blueprint->GeneratedClass;
	if (!IsValid(TargetClass))
	{
		return;
	}

	if (!TargetClass->ImplementsInterface(UPuertsInterface::StaticClass()))
	{
		return;
	}

	const auto Func = TargetClass->FindFunctionByName(FName("GetJavaScriptModule"));
	if (!IsValid(Func))
	{
		return;
	}

	FString ModuleName;
	const auto DefaultObject = TargetClass->GetDefaultObject();
	DefaultObject->UObject::ProcessEvent(Func, &ModuleName);

	const auto FileName = GetScriptRealPath(ModuleName);

	FPlatformApplicationMisc::ClipboardCopy(*FileName);
}

void FPuertsAutoMixinEditorToolbar::DefaultModule_Executed() const
{
	const auto Blueprint = Cast<UBlueprint>(ContextObject);
	if (!IsValid(Blueprint))
	{
		return;
	}

	const auto TargetClass = Blueprint->GeneratedClass;
	if (!IsValid(TargetClass))
	{
		return;
	}

	if (!TargetClass->ImplementsInterface(UPuertsInterface::StaticClass()))
	{
		return;
	}

	const auto Package = Blueprint->GetPackage();
	FString NewModuleName = Package->GetName();

	if (NewModuleName.IsEmpty())
	{
		return;
	}

	const auto Func = TargetClass->FindFunctionByName(FName("GetJavaScriptModule"));
	if (!IsValid(Func))
	{
		return;
	}

	FString OldModuleName;
	TargetClass->GetDefaultObject()->ProcessEvent(Func, &OldModuleName);

	UEdGraph* TargetGraph = FBlueprintEditorUtils::FindScopeGraph(Blueprint, Func);
	if (!TargetGraph)
	{
		return;
	}

	UK2Node_FunctionResult* ResultNode = nullptr;
	for (UEdGraphNode* Node : TargetGraph->Nodes)
	{
		ResultNode = Cast<UK2Node_FunctionResult>(Node);
		if (ResultNode)
		{
			break;
		}
	}

	if (!ResultNode)
	{
		return;
	}

	for (UEdGraphPin* Pin : ResultNode->Pins)
	{
		if (Pin->PinName != UEdGraphSchema_K2::PN_Execute && Pin->Direction == EGPD_Input)
		{
			Pin->DefaultValue = NewModuleName;
			break;
		}
	}

	if (!OldModuleName.IsEmpty() && OldModuleName != NewModuleName)
	{
		FString SuperModuleName;
		const auto SuperClass = TargetClass->GetSuperClass();
		if (SuperClass && SuperClass->ImplementsInterface(UPuertsInterface::StaticClass()))
		{
			const auto SuperFunc = SuperClass->FindFunctionByName(FName("GetJavaScriptModule"));
			if (SuperFunc)
			{
				SuperClass->GetDefaultObject()->ProcessEvent(SuperFunc, &SuperModuleName);
			}
		}

		if (OldModuleName != SuperModuleName)
		{
			const auto OldFilePath = GetScriptRealPath(OldModuleName);
			const auto NewFilePath = GetScriptRealPath(NewModuleName);

			if (FPaths::FileExists(OldFilePath) && !FPaths::FileExists(NewFilePath))
			{
				IFileManager::Get().Move(*NewFilePath, *OldFilePath);
			}
		}
	}

#if ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)

	const auto BlueprintEditors = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet").
		GetBlueprintEditors();
	for (auto BlueprintEditor : BlueprintEditors)
	{
		const auto MyBlueprintEditor = static_cast<FBlueprintEditor*>(&BlueprintEditors[0].Get());
		if (!MyBlueprintEditor || MyBlueprintEditor->GetBlueprintObj() != Blueprint)
		{
			continue;
		}
		MyBlueprintEditor->Compile();
		MyBlueprintEditor->OpenGraphAndBringToFront(TargetGraph);
	}

#endif
}

#undef LOCTEXT_NAMESPACE
