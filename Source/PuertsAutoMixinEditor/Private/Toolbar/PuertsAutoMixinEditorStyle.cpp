#include "PuertsAutoMixinEditorStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_BRUSH(RelativePath, ...) FSlateBoxBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BORDER_BRUSH(RelativePath, ...) FSlateBorderBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define TTF_FONT(RelativePath, ...) FSlateFontInfo(RootToContentDir(RelativePath, TEXT(".ttf")), __VA_ARGS__)
#define OTF_FONT(RelativePath, ...) FSlateFontInfo(RootToContentDir(RelativePath, TEXT(".otf")), __VA_ARGS__)

TSharedPtr<ISlateStyle> FPuertsAutoMixinEditorStyle::Instance = nullptr;

FPuertsAutoMixinEditorStyle::FPuertsAutoMixinEditorStyle()
	: FSlateStyleSet("PuertsAutoMixinEditorStyle")
{
	const FVector2D Icon40x40(40.0f, 40.0f);

	SetContentRoot(IPluginManager::Get().FindPlugin("PuertsAutoMixin")->GetBaseDir() / TEXT("Resources"));

	Set("PuertsAutoMixinEditor.Status_NotBound", new IMAGE_BRUSH("Icons/icon_puerts_logo_40x", Icon40x40));
	Set("PuertsAutoMixinEditor.Status_Unknown", new IMAGE_BRUSH("Icons/icon_status_unknown_40x", Icon40x40));
	Set("PuertsAutoMixinEditor.Status_Bound", new IMAGE_BRUSH("Icons/icon_status_bound_40x", Icon40x40));
	Set("PuertsAutoMixinEditor.Status_BoundButInvalid"
	    , new IMAGE_BRUSH("Icons/icon_status_bound_but_invalid_40x", Icon40x40)
	);
	Set("PuertsAutoMixinEditor.RevealInExplorer", new IMAGE_BRUSH("Icons/icon_reveal_in_explorer_40x", Icon40x40));
	Set("PuertsAutoMixinEditor.CreatePuertsTemplate", new IMAGE_BRUSH("Icons/icon_create_template_40x", Icon40x40));
	Set("PuertsAutoMixinEditor.CopyAsRelativePath", new IMAGE_BRUSH("Icons/icon_copy_40x", Icon40x40));
	Set("PuertsAutoMixinEditor.BindToPuerts", new IMAGE_BRUSH("Icons/icon_bind_to_puerts_40x", Icon40x40));
	Set("PuertsAutoMixinEditor.UnbindFromPuerts", new IMAGE_BRUSH("Icons/icon_unbind_from_puerts_40x", Icon40x40));

	FSlateStyleRegistry::RegisterSlateStyle(*this);
}

FPuertsAutoMixinEditorStyle::~FPuertsAutoMixinEditorStyle()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*this);
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT
