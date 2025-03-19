#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "LevelEditor.h"

// 전방 선언
class SLevelBasedTreeView;

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

    // 새로 추가된 함수들: 에디터 메뉴 확장 관련
    static TSharedRef<FExtender> OnExtendLevelEditorMenu(const TSharedRef<FUICommandList> CommandList, TArray<AActor*> SelectedActors);
    static void ShowSelectedActorMetadata();
    static void SelectTreeNodeForActor();
    static void InstallHooks();
    static void RemoveHooks();
    
private:
    // 새로 추가된 정적 멤버 변수들: 에디터 메뉴 확장 관련
    static FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors LevelEditorMenuExtenderDelegate;
    static FDelegateHandle LevelEditorExtenderDelegateHandle;
    
};