#pragma once

#include "Styling/SlateStyle.h"

class FPuertsAutoMixinEditorStyle : public FSlateStyleSet
{
public:
	FPuertsAutoMixinEditorStyle();
	virtual ~FPuertsAutoMixinEditorStyle() override;

	static TSharedPtr<ISlateStyle> GetInstance()
	{
		if (!Instance.IsValid())
		{
			Instance = MakeShared<FPuertsAutoMixinEditorStyle>();
		}
		return Instance;
	}

private:
	static TSharedPtr<ISlateStyle> Instance;
};
