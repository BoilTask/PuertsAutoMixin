#pragma once

#include "CoreMinimal.h"

#include "PuertsAutoMixinSetting.generated.h"

UCLASS(config = PuertsAutoMixin, defaultconfig, meta = (DisplayName = "PuertsAutoMixin"))
class UPuertsAutoMixinSetting : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(
        config
        , EditAnywhere
        , Category = "Default PuertsAutoMixin Environment"
        , meta = (DisplayName = "Enable JS Env In Editor", defaultValue = true)
    )
    bool bEnableEnvInEditor = true;
    UPROPERTY(
        config
        , EditAnywhere
        , Category = "Default PuertsAutoMixin Environment"
        , meta = (DisplayName = "Enable JS Env In Game", defaultValue = false)
    )
    bool bEnableEnvInGame = false;

    UPROPERTY(
        config
        , EditAnywhere
        , Category = "Default PuertsAutoMixin Environment"
        , meta = (DisplayName = "Debug Port", defaultValue = 8080)
    )
    int32 DebugPort = 8080;

    UPROPERTY(config
        , EditAnywhere
        , Category = "Default PuertsAutoMixin Environment"
        , meta = (DisplayName = "Wait Debugger", defaultValue = false)
    )
    bool WaitDebugger = false;

    UPROPERTY(config
        , EditAnywhere
        , Category = "Default PuertsAutoMixin Environment"
        , meta = (DisplayName = "Wait Debugger Timeout", defaultValue = 0)
    )
    double WaitDebuggerTimeout = 0;
};
