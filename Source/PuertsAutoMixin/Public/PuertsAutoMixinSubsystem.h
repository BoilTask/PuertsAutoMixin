#pragma once

#include "JsEnv.h"
#include "SourceFileWatcher.h"

#include "PuertsAutoMixinSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FPuertsAutoMixinDelegate, const UClass*, Class, const FString&, Module);

struct FPuertsAutoMixinData
{
	FPuertsAutoMixinDelegate BindCallback;
	TSet<const UClass*> BindedClasses;
	TSet<FString> BindedModules;
	TMap<const UClass*, FString> ClassToModule;
};

UCLASS()
class PUERTSAUTOMIXIN_API UPuertsAutoMixinSubsystem : public UEngineSubsystem,
                                                      public FUObjectArray::FUObjectCreateListener,
                                                      public FUObjectArray::FUObjectDeleteListener
{
	GENERATED_BODY()

public:
	static UPuertsAutoMixinSubsystem& GetInstance()
	{
		return *GEngine->GetEngineSubsystem<UPuertsAutoMixinSubsystem>();
	}

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	void StartAutoBind();

	void StopBind();

	void StopListen();

	void OnPreBeginPIE(bool bIsSimulating);

	void OnPostPIEStarted(bool bIsSimulating);

	void OnEndPIE(bool bIsSimulating);

	void OnEndPlayMap();

	virtual void NotifyUObjectCreated(const UObjectBase* ObjectBase, int32 Index) override;

	virtual void NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index) override;

	virtual void OnUObjectArrayShutdown() override;

	UFUNCTION()
	void OnAsyncLoadingFlushUpdate();

	void TryBind(UObject* Object, FPuertsAutoMixinData* SpecificData = nullptr);
	void RegisterBindDelegate(const TSharedPtr<puerts::FJsEnv>& JsEnv, const FPuertsAutoMixinDelegate& Callback);
	void Reset();

private:
	inline void CallMixin(UClass* Class, const FString& Module, FPuertsAutoMixinData* SpecificData = nullptr);
	static inline void ExecuteMixin(FPuertsAutoMixinData& Data, UClass* Class, const FString& Module);

private:
	bool bActive = false;
	FDelegateHandle OnAsyncLoadingFlushUpdateHandle;
	FCriticalSection CandidatesLock;
	TArray<FWeakObjectPtr> Candidates; // binding candidates during async loading

public:
	UFUNCTION(BlueprintCallable)
	void BindMixin(const FPuertsAutoMixinDelegate& BindCallback);

private:
	void StartJavaScript();
	void HotReloadJavaScriptEnv(const FString& Path);

private:
	TMap<const TSharedPtr<puerts::FJsEnv>, FPuertsAutoMixinData> BindCallbacks;

private:
	TSharedPtr<puerts::FJsEnv> DefaultJsEnv;

#if WITH_EDITOR
	TSharedPtr<PUERTS_NAMESPACE::FSourceFileWatcher> SourceFileWatcher;
#endif
};
