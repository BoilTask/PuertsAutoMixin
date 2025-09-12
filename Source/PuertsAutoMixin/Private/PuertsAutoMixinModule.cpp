#include "PuertsAutoBindModule.h"

#include "PuertsAutoBindSubsystem.h"
#include "GameDelegates.h"
#include "PuertsInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogPuertsAutoBind, Log, All);

class FPuertsAutoBindModule : public IPuertsAutoBindModule,
                              public FUObjectArray::FUObjectCreateListener,
                              public FUObjectArray::FUObjectDeleteListener
{
	virtual void StartupModule() override
	{
		UE_LOG(LogPuertsAutoBind, Log, TEXT("PuertsAutoBindModule StartupModule"));

#if WITH_EDITOR
		if (!IsRunningGame())
		{
			FEditorDelegates::PreBeginPIE.AddRaw(this, &FPuertsAutoBindModule::OnPreBeginPIE);
			FEditorDelegates::PostPIEStarted.AddRaw(this, &FPuertsAutoBindModule::OnPostPIEStarted);
			FEditorDelegates::EndPIE.AddRaw(this, &FPuertsAutoBindModule::OnEndPIE);
			FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FPuertsAutoBindModule::OnEndPlayMap);
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
		UE_LOG(LogPuertsAutoBind, Log, TEXT("PuertsAutoBindModule ShutdownModule"));

		StopBind();
	}

	void StartAutoBind()
	{
		UE_LOG(LogTemp, Log, TEXT("PuertsAutoBindModule StartAutoBind"));

		if (bActive)
		{
			return;
		}

		GUObjectArray.AddUObjectCreateListener(this);
		GUObjectArray.AddUObjectDeleteListener(this);

		OnAsyncLoadingFlushUpdateHandle = FCoreDelegates::OnAsyncLoadingFlushUpdate.AddRaw(
			this
			, &FPuertsAutoBindModule::OnAsyncLoadingFlushUpdate
		);

		bActive = true;
	}

	void StopBind()
	{
		UE_LOG(LogTemp, Log, TEXT("PuertsAutoBindModule StopBind"));

		if (!bActive)
		{
			return;
		}

		StopListen();

		UPuertsAutoBindSubsystem::GetInstance().Reset();

		bActive = false;
	}

	void StopListen()
	{
		UE_LOG(LogTemp, Log, TEXT("PuertsAutoBindModule StopListen"));

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
		UE_LOG(LogTemp, Log, TEXT("PuertsAutoBindModule OnUObjectArrayShutdown"));

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

		UPuertsAutoBindSubsystem::GetInstance().CallMixin(Class, Module);
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

IMPLEMENT_MODULE(FPuertsAutoBindModule, PuertsAutoBind)
