#pragma once

#include "Framework/Commands/Commands.h"

class FPuertsAutoMixinEditorCommands : public TCommands<FPuertsAutoMixinEditorCommands>
{
public:
	FPuertsAutoMixinEditorCommands()
		: TCommands<FPuertsAutoMixinEditorCommands>(TEXT("PuertsAutoMixinEditor")
		                                            , NSLOCTEXT("Contexts"
		                                                        , "PuertsAutoMixinEditor"
		                                                        , "PuertsAutoMixin Editor"
		                                            )
		                                            , NAME_None
		                                            , "PuertsAutoMixinEditorStyle"
		)
	{
	}

    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> CreatePuertsTemplate;
    TSharedPtr<FUICommandInfo> CopyAsRelativePath;
    TSharedPtr<FUICommandInfo> BindToPuerts;
    TSharedPtr<FUICommandInfo> UnbindFromPuerts;
    TSharedPtr<FUICommandInfo> RevealInExplorer;
};
