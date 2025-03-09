// MyProject2Editor.cpp

#include "MyProject2Editor.h"
#include "LevelEditor.h"
#include "MyProject2/Public//UI/LevelBasedTreeView.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"
#include "UI/LevelBasedTreeView.h"
#include "ServiceLocator.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FMyProject2EditorModule"

// 도킹 탭 ID 정의
static const FName PartsTreeTabId = FName("PartsTree");

void FMyProject2EditorModule::StartupModule()
{
    RegisterMenu();
    
    // 이미지 매니저 인스턴스 생성 및 등록
    FPartImageManager* ImageManager = new FPartImageManager();
    ImageManager->Initialize();
    FServiceLocator::RegisterImageManager(ImageManager);

    // 탭 매니저에 도킹 탭 등록
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PartsTreeTabId, 
        FOnSpawnTab::CreateRaw(this, &FMyProject2EditorModule::SpawnPartsTreeTab))
        .SetDisplayName(LOCTEXT("PartsTreeTabTitle", "FA50m Parts Tree"))
        .SetTooltipText(LOCTEXT("PartsTreeTooltip", "FA-50M parts tree browser"))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.ViewOptions"));
}

void FMyProject2EditorModule::ShutdownModule()
{
    // 도구 메뉴 등록 해제
    UToolMenus::UnregisterOwner(this);
    
    // 도킹 탭 등록 해제
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PartsTreeTabId);

    // 정리 작업
    FPartImageManager* ImageManager = FServiceLocator::GetImageManager();
    if (ImageManager)
    {
        FServiceLocator::RegisterImageManager(nullptr);
        delete ImageManager;
    }
}

void FMyProject2EditorModule::RegisterMenu()
{
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMyProject2EditorModule::RegisterMenusCallback));
}

void FMyProject2EditorModule::RegisterMenusCallback()
{
    FToolMenuOwnerScoped OwnerScoped(this);

    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");

    if(ToolbarMenu)
    {
        FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PartsTreeSection");

        FToolMenuEntry Entry = FToolMenuEntry::InitToolBarButton(
            FName("OpenPartsTree"),
            FUIAction(FExecuteAction::CreateRaw(this, &FMyProject2EditorModule::OnToolButtonClicked)),
            FText::FromString("Parts Tree"),
            FText::FromString("Open the parts tree viewer"),
            FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.ViewOptions")
        );

        Section.AddEntry(Entry);
    }
}

void FMyProject2EditorModule::OnToolButtonClicked()
{
    // 도킹 탭 열기
    FGlobalTabmanager::Get()->TryInvokeTab(PartsTreeTabId);
}

TSharedRef<SDockTab> FMyProject2EditorModule::SpawnPartsTreeTab(const FSpawnTabArgs& SpawnTabArgs)
{
    // CSV 파일 경로 설정 (프로젝트의 Content/Data 폴더 내 CSV 파일)
    FString CSVFilePath = FPaths::ProjectContentDir() / TEXT("Data/data.csv");
    
    // 파일 존재 여부 확인
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*CSVFilePath))
    {
        return SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            [
                SNew(STextBlock)
                .Text(FText::FromString("CSV file not found: " + CSVFilePath))
            ];
    }
    
    // 트리뷰 위젯 생성
    TSharedPtr<SWidget> TreeViewWidget;
    TSharedPtr<SWidget> MetadataWidget;
    CreateLevelBasedTreeView(TreeViewWidget, MetadataWidget, CSVFilePath);

    TSharedRef<SWidget> ContentWidget = SNew(SSplitter)
        .Orientation(Orient_Horizontal)
        + SSplitter::Slot()
        .Value(0.5f)
        [
            TreeViewWidget.ToSharedRef()
        ]
        + SSplitter::Slot()
        .Value(0.5f)
        [
            MetadataWidget.ToSharedRef()
        ];
    
    // 도킹 탭 생성 및 반환
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            ContentWidget
        ];
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FMyProject2EditorModule, MyProject2Editor)