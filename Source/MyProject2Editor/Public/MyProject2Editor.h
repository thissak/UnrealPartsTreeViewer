#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMyProject2EditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void RegisterMenu();
    void RegisterMenusCallback();

    void OnToolButtonClicked();
    
    // 도킹 탭 생성 함수
    TSharedRef<SDockTab> SpawnPartsTreeTab(const FSpawnTabArgs& SpawnTabArgs);
};