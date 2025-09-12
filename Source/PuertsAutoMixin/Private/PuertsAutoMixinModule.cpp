#include "PuertsAutoMixinModule.h"

#include "PuertsAutoMixinSubsystem.h"
#include "GameDelegates.h"
#include "PuertsInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogPuertsAutoMixin, Log, All);

class FPuertsAutoMixinModule : public IPuertsAutoMixinModule,
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
		TryBind(Object);
	}

	virtual void NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index) override
	{
		UnBind(ObjectBase);
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

	void TryBind(UObject* Object)
	{
		const auto Class = Object->IsA<UClass>() ? static_cast<UClass*>(Object) : Object->GetClass();
		if (Class->HasAnyClassFlags(CLASS_NewerVersionExists))
		{
			return;
		}

		static UClass* InterfaceClass = UPuertsInterface::StaticClass();
		const bool bImplPuertsInterface = Class->ImplementsInterface(InterfaceClass);

		if (IsInAsyncLoadingThread())
		{
			if (bImplPuertsInterface)
			{
				// 等加载完再绑定
				// FScopeLock Lock(&CandidatesLock);
				// Candidates.AddUnique(Object);
				return;
			}
		}

		if (!bImplPuertsInterface)
		{
			return;
		}

		if (Class->GetName().Contains(TEXT("SKEL_")))
		{
			return;
		}

		const auto& Module = GetJavaScriptModule(Object);
		if (Module.IsEmpty())
		{
			return;
		}

		UPuertsAutoMixinSubsystem::GetInstance().CallMixin(Class, Module);
	}

	void UnBind(const UObjectBase* Object)
	{
	}

	void OnAsyncLoadingFlushUpdate()
	{
	}

protected:
	FString GetJavaScriptModule(const UObject* Object)
	{
		const UObject* CDO;
		if (Object->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			CDO = Object;
		}
		else
		{
			const auto Class = Cast<UClass>(Object);
			CDO = Class ? Class->GetDefaultObject() : Object->GetClass()->GetDefaultObject();
		}
		if (CDO->HasAnyFlags(RF_NeedInitialization))
		{
			return "";
		}
		if (!CDO->GetClass()->ImplementsInterface(UPuertsInterface::StaticClass()))
		{
			return "";
		}
		return IPuertsInterface::Execute_GetJavaScriptModule(CDO);
	}

private:
	bool bActive = false;
	FDelegateHandle OnAsyncLoadingFlushUpdateHandle;
};

IMPLEMENT_MODULE(FPuertsAutoMixinModule, PuertsAutoMixin)
