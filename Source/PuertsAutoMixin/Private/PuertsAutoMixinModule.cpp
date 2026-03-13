#include "PuertsAutoMixinModule.h"

#include "PuertsAutoMixinSubsystem.h"
#include "GameDelegates.h"
#include "PuertsAutoMixinLibrary.h"
#include "PuertsInterface.h"

DEFINE_LOG_CATEGORY(LogPuertsAutoMixin);

class PUERTSAUTOMIXIN_API FPuertsAutoMixinModule : public IPuertsAutoMixinModule,
                              public FUObjectArray::FUObjectCreateListener,
                              public FUObjectArray::FUObjectDeleteListener
{
	virtual void StartupModule() override
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinModule StartupModule"));

#if WITH_EDITOR
		if (!IsRunningGame())
		{
			FEditorDelegates::PreBeginPIE.AddRaw(this, &FPuertsAutoMixinModule::OnPreBeginPIE);
			FEditorDelegates::PostPIEStarted.AddRaw(this, &FPuertsAutoMixinModule::OnPostPIEStarted);
			FEditorDelegates::EndPIE.AddRaw(this, &FPuertsAutoMixinModule::OnEndPIE);
			FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FPuertsAutoMixinModule::OnEndPlayMap);
		}
		if (IsRunningGame() || IsRunningDedicatedServer())
		{
			StartAutoBind();
		}
#else
		StartAutoBind();
#endif
	}

	virtual void ShutdownModule() override
	{
		UE_LOG(LogPuertsAutoMixin, Log, TEXT("PuertsAutoMixinModule ShutdownModule"));

		StopBind();
	}

	void StartAutoBind()
	{
		UE_LOG(LogTemp, Log, TEXT("PuertsAutoMixinModule StartAutoBind"));

		if (bActive)
		{
			return;
		}

		GUObjectArray.AddUObjectCreateListener(this);
		GUObjectArray.AddUObjectDeleteListener(this);

		OnAsyncLoadingFlushUpdateHandle = FCoreDelegates::OnAsyncLoadingFlushUpdate.AddRaw(
			this
			, &FPuertsAutoMixinModule::OnAsyncLoadingFlushUpdate
		);

		bActive = true;
	}

	void StopBind()
	{
		UE_LOG(LogTemp, Log, TEXT("PuertsAutoMixinModule StopBind"));

		if (!bActive)
		{
			return;
		}

		StopListen();

		UPuertsAutoMixinSubsystem::GetInstance().Reset();

		bActive = false;
	}

	void StopListen()
	{
		UE_LOG(LogTemp, Log, TEXT("PuertsAutoMixinModule StopListen"));

		GUObjectArray.RemoveUObjectCreateListener(this);
		GUObjectArray.RemoveUObjectDeleteListener(this);

		FCoreDelegates::OnAsyncLoadingFlushUpdate.Remove(OnAsyncLoadingFlushUpdateHandle);
	}

	void OnPreBeginPIE(bool bIsSimulating)
	{
		StartAutoBind();
	}

	void OnPostPIEStarted(bool bIsSimulating)
	{
	}

	void OnEndPIE(bool bIsSimulating)
	{
	}

	void OnEndPlayMap()
	{
		StopBind();
	}

	virtual void NotifyUObjectCreated(const UObjectBase* ObjectBase, int32 Index) override
	{
		UObject* Object = (UObject*)ObjectBase;
		UPuertsAutoMixinSubsystem::GetInstance().TryBind(Object);
	}

	virtual void NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index) override
	{
	}

	virtual void OnUObjectArrayShutdown() override
	{
		UE_LOG(LogTemp, Log, TEXT("PuertsAutoMixinModule OnUObjectArrayShutdown"));

		if (!bActive)
		{
			return;
		}

		StopListen();

		bActive = false;
	}

	void OnAsyncLoadingFlushUpdate()
	{
	}

private:
	bool bActive = false;
	FDelegateHandle OnAsyncLoadingFlushUpdateHandle;
};

IMPLEMENT_MODULE(FPuertsAutoMixinModule, PuertsAutoMixin)
