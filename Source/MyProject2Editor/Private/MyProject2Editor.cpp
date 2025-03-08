// MyProject2Editor.cpp

#include "MyProject2Editor.h"
#include "LevelEditor.h"
#include "MyProject2/Public//UI/LevelBasedTreeView.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"
#include "UI/LevelBasedTreeView.h"
#include "ServiceLocator.h"

#define LOCTEXT_NAMESPACE "FMyProject2EditorModule"

void FMyProject2EditorModule::StartupModule()
{
    RegisterMenu();
    // 이미지 매니저 인스턴스 생성 및 등록
    FPartImageManager* ImageManager = new FPartImageManager();
    ImageManager->Initialize();
    FServiceLocator::RegisterImageManager(ImageManager);
}

void FMyProject2EditorModule::ShutdownModule()
{
    // 도구 메뉴 등록 해제
    UToolMenus::UnregisterOwner(this);

    // 윈도우 참조 해제
    if (TreeViewWindow.IsValid())
    {
        // 모듈 내 윈도우 참조 제거
        TreeViewWindow = nullptr;
    }

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

    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolbar.PlayToolBar");

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
    // CSV 파일 경로 설정 (프로젝트의 Content/Data 폴더 내 CSV 파일)
    FString CSVFilePath = FPaths::ProjectContentDir() / TEXT("Data/data.csv");
    
    // 파일 존재 여부 확인
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*CSVFilePath))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("CSV file not found: " + CSVFilePath));
        return;
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
    
    // 창 생성
    TreeViewWindow = SNew(SWindow)
        .Title(FText::FromString("FA50m Parts Tree Viewer"))
        .ClientSize(FVector2D(800, 600))
        .SizingRule(ESizingRule::UserSized)
        .bDragAnywhere(true)
        .SupportsMaximize(true)
        .SupportsMinimize(true)
        [
            ContentWidget
        ];
    
    // 창 표시
    FSlateApplication::Get().AddWindow(TreeViewWindow.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FMyProject2EditorModule, MyProject2Editor)