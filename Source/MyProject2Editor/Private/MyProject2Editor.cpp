// MyProject2Editor.cpp

#include "MyProject2Editor.h"
#include "LevelEditor.h"
#include "MyProject2/Public//UI/LevelBasedTreeView.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"
#include "UI/LevelBasedTreeView.h"

#define LOCTEXT_NAMESPACE "FMyProject2EditorModule"

void FMyProject2EditorModule::StartupModule()
{
    ResisterMenu();
}

void FMyProject2EditorModule::ShutdownModule()
{
    if (TreeViewWindow.IsValid())
    {
        TreeViewWindow->RequestDestroyWindow();
        TreeViewWindow.Reset();
    }

    UToolMenus::UnregisterOwner(this);
}

void FMyProject2EditorModule::ResisterMenu()
{
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMyProject2EditorModule::ResisterMenusCallback));
}

void FMyProject2EditorModule::ResisterMenusCallback()
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