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
#include "TreeViewUtils.h"            // 명시적으로 추가
#include "Editor.h"                   // GEditor 및 UEditorEngine 참조를 위해 필요
#include "Editor/UnrealEdEngine.h"    // 추가 에디터 기능 참조를 위해 필요
#include "EngineUtils.h"
#include "ImportedNodeManager.h"      // 임포트된 노드 관리자
#include "Selection.h"
#include "Framework/Notifications/NotificationManager.h" // 알림 기능 사용을 위해 필요
#include "Widgets/Notifications/SNotificationList.h"     // 알림 위젯 사용을 위해 필요

#define LOCTEXT_NAMESPACE "FMyProject2EditorModule"

// 정적 멤버 변수 초기화
FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors FMyProject2EditorModule::LevelEditorMenuExtenderDelegate;
FDelegateHandle FMyProject2EditorModule::LevelEditorExtenderDelegateHandle;

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
        
    // 에디터 메뉴 확장 훅 설치
    InstallHooks();
}

void FMyProject2EditorModule::ShutdownModule()
{
    // 트리뷰 싱글톤 인스턴스 정리
    SLevelBasedTreeView::Shutdown();
    
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
    
    // 에디터 메뉴 확장 훅 제거
    RemoveHooks();
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
    
    // 로그 확인 - 싱글톤 인스턴스가 제대로 초기화되었는지 확인
    TSharedPtr<SLevelBasedTreeView> TreeViewInstance = SLevelBasedTreeView::Get();
    if (TreeViewInstance.IsValid())
    {
        UE_LOG(LogTemp, Display, TEXT("트리뷰 싱글톤 인스턴스가 성공적으로 초기화되었습니다."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("트리뷰 싱글톤 인스턴스가 초기화되지 않았습니다."));
    }

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

// 에디터 메뉴 확장 기능 설치
void FMyProject2EditorModule::InstallHooks()
{
    // 델리게이트 생성 및 등록
    LevelEditorMenuExtenderDelegate = FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateStatic(&FMyProject2EditorModule::OnExtendLevelEditorMenu);
    FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
    auto& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
    MenuExtenders.Add(LevelEditorMenuExtenderDelegate);
    LevelEditorExtenderDelegateHandle = MenuExtenders.Last().GetHandle();
    
    UE_LOG(LogTemp, Display, TEXT("FMyProject2EditorModule - 에디터 메뉴 확장 기능 설치 완료"));
}

// 에디터 메뉴 확장 기능 제거
void FMyProject2EditorModule::RemoveHooks()
{
    if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
    {
        FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
        LevelEditorModule.GetAllLevelViewportContextMenuExtenders().RemoveAll([](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& Delegate) {
            return Delegate.GetHandle() == LevelEditorExtenderDelegateHandle;
        });
        
        UE_LOG(LogTemp, Display, TEXT("FMyProject2EditorModule - 에디터 메뉴 확장 기능 제거 완료"));
    }
}

// 메뉴 확장 함수
TSharedRef<FExtender> FMyProject2EditorModule::OnExtendLevelEditorMenu(const TSharedRef<FUICommandList> CommandList, TArray<AActor*> SelectedActors)
{
    TSharedRef<FExtender> Extender(new FExtender());

    // 선택된 액터가 있을 때만 메뉴 추가
    if (SelectedActors.Num() > 0)
    {
        // "ActorTypeTools" 섹션 이후에 메뉴 항목 추가
        Extender->AddMenuExtension(
            "ActorTypeTools",  // 확장할 메뉴 지점
            EExtensionHook::After,  // 해당 지점 이후에 추가
            nullptr,  // 커맨드 리스트 (필요 없으면 nullptr)
            FMenuExtensionDelegate::CreateLambda([](FMenuBuilder& MenuBuilder) {
                // 메뉴 항목 생성
                MenuBuilder.BeginSection("FA50Tools", LOCTEXT("FA50ToolsHeading", "FA-50M Tools"));
                
                // 메타데이터 출력 메뉴 항목
                FUIAction Action_ShowMetadata(FExecuteAction::CreateStatic(&FMyProject2EditorModule::ShowSelectedActorMetadata));
                MenuBuilder.AddMenuEntry(
                    LOCTEXT("ShowActorMetadata", "로그에 액터 메타데이터 출력"),
                    LOCTEXT("ShowActorMetadata_Tooltip", "선택된 액터의 메타데이터를 로그에 출력합니다"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Details"),
                    Action_ShowMetadata);
                
                // 바운딩 박스 계산 메뉴 항목
                FUIAction Action_CalculateBounds(FExecuteAction::CreateLambda([]() {
                    FTreeViewUtils::CalculateSelectedActorMeshBounds();
                }));
                MenuBuilder.AddMenuEntry(
                    LOCTEXT("CalculateBounds", "메시 바운딩 박스 계산"),
                    LOCTEXT("CalculateBounds_Tooltip", "선택된 액터의 메시 바운딩 박스를 계산합니다"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.BoundingBox"),
                    Action_CalculateBounds);
                
                // 트리뷰에서 노드 선택 메뉴 항목
                FUIAction Action_SelectTreeNode(FExecuteAction::CreateStatic(&FMyProject2EditorModule::SelectTreeNodeForActor));
                MenuBuilder.AddMenuEntry(
                    LOCTEXT("SelectTreeNode", "트리뷰에서 노드 선택"),
                    LOCTEXT("SelectTreeNode_Tooltip", "선택된 액터에 해당하는 노드를 트리뷰에서 선택합니다"),
                    FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search"),
                    Action_SelectTreeNode);
                
                MenuBuilder.EndSection();
            }));
    }

    return Extender;
}

// 액터 메타데이터 출력 기능
void FMyProject2EditorModule::ShowSelectedActorMetadata()
{
    // 선택된 액터 가져오기
    USelection* SelectedActors = GEditor->GetSelectedActors();
    if (!SelectedActors || SelectedActors->Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("선택된 액터 없음"));
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("=== 선택된 액터의 메타데이터 출력 시작 ==="));
    
    for (FSelectionIterator It(*SelectedActors); It; ++It)
    {
        AActor* Actor = Cast<AActor>(*It);
        if (!Actor) continue;
        
        UE_LOG(LogTemp, Display, TEXT("액터: %s (클래스: %s)"), 
               *Actor->GetActorLabel(), *Actor->GetClass()->GetName());
        
        // 위치, 회전, 스케일 정보
        FVector Location = Actor->GetActorLocation();
        FRotator Rotation = Actor->GetActorRotation();
        FVector Scale = Actor->GetActorScale3D();
        
        UE_LOG(LogTemp, Display, TEXT("  위치: X=%.2f Y=%.2f Z=%.2f"), 
               Location.X, Location.Y, Location.Z);
        UE_LOG(LogTemp, Display, TEXT("  회전: P=%.2f Y=%.2f R=%.2f"), 
               Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
        UE_LOG(LogTemp, Display, TEXT("  스케일: X=%.2f Y=%.2f Z=%.2f"), 
               Scale.X, Scale.Y, Scale.Z);
        
        // 태그 정보
        if (Actor->Tags.Num() > 0)
        {
            UE_LOG(LogTemp, Display, TEXT("  태그 목록:"));
            for (const FName& Tag : Actor->Tags)
            {
                UE_LOG(LogTemp, Display, TEXT("    %s"), *Tag.ToString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("  태그 없음"));
        }
        
        // ImportedNodeManager에 등록되었는지 확인
        FString PartNo;
        // ImportedPart_ 태그에서 파트 번호 추출
        for (const FName& Tag : Actor->Tags)
        {
            FString TagStr = Tag.ToString();
            if (TagStr.StartsWith(TEXT("ImportedPart_")))
            {
                PartNo = TagStr.RightChop(13); // "ImportedPart_" 길이는 13
                break;
            }
        }
        
        if (!PartNo.IsEmpty())
        {
            bool bIsRegistered = FImportedNodeManager::Get().IsNodeImported(PartNo);
            UE_LOG(LogTemp, Display, TEXT("  파트 번호: %s (ImportedNodeManager 등록: %s)"), 
                   *PartNo, bIsRegistered ? TEXT("예") : TEXT("아니오"));
        }
        
        // 컴포넌트 정보
        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        
        UE_LOG(LogTemp, Display, TEXT("  컴포넌트 개수: %d"), Components.Num());
        for (UActorComponent* Component : Components)
        {
            if (!Component) continue;
            
            UE_LOG(LogTemp, Display, TEXT("    %s (클래스: %s)"), 
                   *Component->GetName(), *Component->GetClass()->GetName());
            
            // StaticMeshComponent인 경우 추가 정보
            if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Component))
            {
                if (UStaticMesh* Mesh = MeshComp->GetStaticMesh())
                {
                    UE_LOG(LogTemp, Display, TEXT("      메시: %s"), *Mesh->GetName());
                    
                    // 머티리얼 정보
                    int32 MaterialCount = MeshComp->GetNumMaterials();
                    UE_LOG(LogTemp, Display, TEXT("      머티리얼 개수: %d"), MaterialCount);
                    
                    for (int32 i = 0; i < MaterialCount; ++i)
                    {
                        if (UMaterialInterface* Material = MeshComp->GetMaterial(i))
                        {
                            UE_LOG(LogTemp, Display, TEXT("        머티리얼[%d]: %s"), 
                                   i, *Material->GetName());
                        }
                    }
                }
            }
        }
        
        UE_LOG(LogTemp, Display, TEXT("  --------------------------"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("=== 선택된 액터의 메타데이터 출력 종료 ==="));
    
    // 완료 알림 표시
    FNotificationInfo Info(LOCTEXT("MetadataLoggedNotif", "액터 메타데이터가 출력 로그에 기록되었습니다."));
    Info.ExpireDuration = 4.0f;
    Info.bUseLargeFont = false;
    FSlateNotificationManager::Get().AddNotification(Info);
}

// 트리뷰에서 노드 선택 기능
void FMyProject2EditorModule::SelectTreeNodeForActor()
{
    // 싱글톤 인스턴스 사용
    TSharedPtr<SLevelBasedTreeView> TreeViewInstance = SLevelBasedTreeView::Get();
    
    // 트리뷰 인스턴스 확인
    if (!TreeViewInstance.IsValid())
    {
        FNotificationInfo Info(LOCTEXT("NoTreeViewNotif", "트리뷰가 열려있지 않습니다. 먼저 Parts Tree 탭을 열어주세요."));
        Info.ExpireDuration = 4.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }
    
    // 선택된 액터 가져오기
    USelection* SelectedActors = GEditor->GetSelectedActors();
    if (!SelectedActors || SelectedActors->Num() == 0)
    {
        FNotificationInfo Info(LOCTEXT("NoActorSelectedNotif", "선택된 액터가 없습니다."));
        Info.ExpireDuration = 4.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }
    
    // 첫 번째 선택된 액터에 대해서만 처리
    AActor* Actor = Cast<AActor>(SelectedActors->GetSelectedObject(0));
    if (!Actor)
        return;
    
    // 임포트된 파트 번호 찾기
    FString PartNo;
    for (const FName& Tag : Actor->Tags)
    {
        FString TagStr = Tag.ToString();
        if (TagStr.StartsWith(TEXT("ImportedPart_")))
        {
            PartNo = TagStr.RightChop(13); // "ImportedPart_" 길이는 13
            break;
        }
    }
    
    if (PartNo.IsEmpty())
    {
        // 임포트된 파트 번호가 없는 경우 액터 이름으로 시도
        PartNo = Actor->GetName();
        
        FNotificationInfo Info(LOCTEXT("NoPartNoTagNotif", "임포트된 파트 태그가 없습니다. 액터 이름으로 시도합니다."));
        Info.ExpireDuration = 3.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    
    UE_LOG(LogTemp, Display, TEXT("액터 '%s'에 대한 파트 번호: %s"), *Actor->GetName(), *PartNo);
    
    // 트리뷰에서 노드 선택 시도
    bool bSuccess = TreeViewInstance->SelectNodeByPartNo(PartNo);
    
    // 결과 알림
    if (bSuccess)
    {
        FNotificationInfo Info(FText::Format(
            LOCTEXT("FoundNodeNotif", "파트 번호 '{0}'에 해당하는 노드를 트리뷰에서 선택했습니다."),
            FText::FromString(PartNo)));
        Info.ExpireDuration = 4.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    else
    {
        FNotificationInfo Info(FText::Format(
            LOCTEXT("NodeNotFoundNotif", "파트 번호 '{0}'에 해당하는 노드를 트리뷰에서 찾을 수 없습니다."),
            FText::FromString(PartNo)));
        Info.ExpireDuration = 4.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FMyProject2EditorModule, MyProject2Editor)