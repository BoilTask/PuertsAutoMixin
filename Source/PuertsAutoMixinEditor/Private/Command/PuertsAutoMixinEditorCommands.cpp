#include "PuertsAutoMixinEditorCommands.h"

#define LOCTEXT_NAMESPACE "FPuertsAutoMixinEditorCommands"

void FPuertsAutoMixinEditorCommands::RegisterCommands()
{
    UI_COMMAND(CreatePuertsTemplate, "Create Puerts Template", "Create Puerts template file", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(CopyAsRelativePath, "Copy as Relative Path", "Copy module name as relative path.", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(BindToPuerts, "Bind", "Implement PuertsInterface", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(UnbindFromPuerts, "Unbind", "Remove the implementation of PuertsInterface", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(RevealInExplorer, "Reveal in Explorer", "Reveal Puerts file in explorer", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
