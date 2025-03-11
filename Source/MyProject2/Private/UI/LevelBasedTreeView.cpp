// LevelBasedTreeView.cpp - FA-50M 파트 계층 구조 표시를 위한 트리 뷰 위젯 구현

#include "UI/LevelBasedTreeView.h"

#include "AssetViewUtils.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DatasmithSceneManager.h"
#include "Dialogs/Dialogs.h"
#include "Framework/Notifications/NotificationManager.h"
#include "ImportedNodeManager.h"
#include "ObjectTools.h"
#include "ServiceLocator.h"
#include "SlateOptMacros.h"
#include "UI/ImportSettingsDialog.h"
#include "UI/PartMetadataWidget.h"
#include "TreeViewUtils.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Notifications/SNotificationList.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#endif

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLevelBasedTreeView::Construct(const FArguments& InArgs)
{
    // 기본 변수 초기화
    MaxLevel = 0;
	bIsSearching = false;  // 검색 상태 초기화
	SearchText = "";       // 검색어 초기화

	// 필터 관리자 초기화
	FilterManager = MakeShared<FPartTreeViewFilterManager>();
	
    // 메타데이터 위젯 참조 저장
    MetadataWidget = InArgs._MetadataWidget;
    
    // 위젯 구성
    ChildSlot
    [
        SAssignNew(TreeView, STreeView<TSharedPtr<FPartTreeItem>>)
            .ItemHeight(24.0f)
            .TreeItemsSource(&AllRootItems)
            .OnGenerateRow(this, &SLevelBasedTreeView::OnGenerateRow)
            .OnGetChildren(this, &SLevelBasedTreeView::OnGetChildren)
            .OnSelectionChanged(this, &SLevelBasedTreeView::OnSelectionChanged)
            .OnContextMenuOpening(this, &SLevelBasedTreeView::OnContextMenuOpening)
            .OnMouseButtonDoubleClick(this, &SLevelBasedTreeView::OnTreeItemDoubleClick)
            .HeaderRow
            (
                SNew(SHeaderRow)
                + SHeaderRow::Column("PartNo")
                .DefaultLabel(FText::FromString("Part No"))
                .FillWidth(1.0f)
            )
    ];
    
    // 파일 로드 및 트리뷰 구성
    if (!InArgs._ExcelFilePath.IsEmpty())
    {
        BuildTreeView(InArgs._ExcelFilePath);
    }
}

// 액터 선택 함수
AActor* SLevelBasedTreeView::SelectActorByPartNo(const FString& PartNo)
{
    AActor* FoundActor = nullptr;
    
#if WITH_EDITOR
    if (GEditor)
    {
        // 현재 선택 해제
        GEditor->SelectNone(true, true, false);
        
        // 1. 먼저 ImportedNodeManager에서 해당 파트 번호로 등록된 액터 찾기
        if (AActor* RegisteredActor = FImportedNodeManager::Get().GetImportedActor(PartNo))
        {
            // ImportedNodeManager에 등록된 액터가 있으면 선택
            GEditor->SelectActor(RegisteredActor, true, true, true);
            UE_LOG(LogTemp, Display, TEXT("ImportedNodeManager에서 액터 찾음: %s"), *RegisteredActor->GetName());
            FoundActor = RegisteredActor;
            return FoundActor;
        }
        
        // 2. 월드의 모든 액터에서 태그로 검색
        if (UWorld* EditorWorld = GEditor->GetEditorWorldContext().World())
        {
            // ImportedPart_[PartNo] 태그 생성
            FName PartTag = FName(*FString::Printf(TEXT("ImportedPart_%s"), *PartNo));
            
            // 모든 액터 순회하며 태그 검색
            for (TActorIterator<AActor> ActorItr(EditorWorld); ActorItr; ++ActorItr)
            {
                AActor* CurrentActor = *ActorItr;
                if (CurrentActor && CurrentActor->ActorHasTag(PartTag))
                {
                    // 태그가 일치하는 액터 선택
                    GEditor->SelectActor(CurrentActor, true, true, true);
                    FoundActor = CurrentActor;
                    UE_LOG(LogTemp, Display, TEXT("태그로 액터 찾음: %s"), *CurrentActor->GetName());
                    break; // 첫 번째 일치 액터만 선택
                }
            }
            
            // 3. 태그에서도 찾지 못했으면 이름으로 검색
            if (!FoundActor)
            {
                for (TActorIterator<AActor> ActorItr(EditorWorld); ActorItr; ++ActorItr)
                {
                    AActor* CurrentActor = *ActorItr;
                    if (CurrentActor)
                    {
                        // 액터 이름과 PartNo 비교
                        FString ActorName = CurrentActor->GetName();
                        
                        // 정확히 일치하거나 부분 문자열로 포함되는지 검사
                        if (ActorName.Equals(PartNo, ESearchCase::IgnoreCase) || 
                            ActorName.Contains(PartNo, ESearchCase::IgnoreCase))
                        {
                            // 액터 선택
                            GEditor->SelectActor(CurrentActor, true, true, true);
                            FoundActor = CurrentActor;
                            UE_LOG(LogTemp, Display, TEXT("이름으로 액터 찾음: %s"), *CurrentActor->GetName());
                            break; // 첫 번째 일치 액터만 선택
                        }
                    }
                }
            }
            
            if (!FoundActor)
            {
                UE_LOG(LogTemp, Warning, TEXT("일치하는 액터를 찾을 수 없음: %s"), *PartNo);
            }
        }
    }
#endif

    return FoundActor;
}

// 검색 UI 위젯 반환 함수
TSharedRef<SWidget> SLevelBasedTreeView::GetSearchWidget()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(4.0f))
		[
			SNew(SVerticalBox)
            
			// 검색창 설명 텍스트
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Search Parts"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			]

		    // 검색 입력창과 설정 아이콘을 포함하는 가로 상자
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SSearchBox)
                    .HintText(FText::FromString("Enter text to search..."))
                    .OnTextChanged(this, &SLevelBasedTreeView::OnSearchTextChanged)
                    .OnTextCommitted(this, &SLevelBasedTreeView::OnSearchTextCommitted)
                ]
                
                // 설정 아이콘 버튼
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(4, 0, 0, 0)
                .VAlign(VAlign_Center)
                [
                    SNew(SButton)
                    .ButtonStyle(FAppStyle::Get(), "SimpleButton")
                    .ToolTipText(FText::FromString("Import Settings"))
                    .OnClicked(this, &SLevelBasedTreeView::OnSettingsButtonClicked)
                    .ContentPadding(FMargin(1, 0))
                    [
                        SNew(SImage)
                        .Image(FAppStyle::GetBrush("Icons.Settings"))
                        .ColorAndOpacity(FSlateColor::UseForeground())
                    ]
                ]
            ]
            
			// 검색 결과 정보
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(STextBlock)
				.Text_Lambda([this]() -> FText {
					if (bIsSearching && !SearchText.IsEmpty())
					{
						return FText::Format(FText::FromString("Found {0} matches for '{1}'"), 
							FText::AsNumber(SearchResults.Num()), FText::FromString(SearchText));
					}
					return FText::GetEmpty();
				})
				.Visibility_Lambda([this]() -> EVisibility {
					return (bIsSearching && !SearchText.IsEmpty()) ? EVisibility::Visible : EVisibility::Collapsed;
				})
			]
		];
}

// 검색어 변경 이벤트 핸들러
void SLevelBasedTreeView::OnSearchTextChanged(const FText& InText)
{
    // 새 검색어
    FString NewSearchText = InText.ToString();
    
    // 이전에 빈 상태에서 첫 글자 입력 시 트리 접기 실행
    if (SearchText.IsEmpty() && !NewSearchText.IsEmpty())
    {
        // 바로 실행하는 대신 다음 틱에 실행하도록 지연
        GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateLambda([this]() {
            FoldLevelZeroItems();
        }));
    }
    
    SearchText = NewSearchText;
    
    if (SearchText.IsEmpty())
    {
        // 검색어가 비었으면 검색 중지
        bIsSearching = false;
        
        // 모든 필터 해제 및 트리뷰 갱신
    	if (FilterManager->IsFilterEnabled("ImageFilter"))
        {
            ToggleImageFiltering(false);
        }
        else
        {
            TreeView->RequestTreeRefresh();
        }
    }
    else
    {
        // 검색 상태로 설정
        bIsSearching = true;
        
        // 검색 실행
        PerformSearch(SearchText);
    }
}

// 검색 실행 함수
void SLevelBasedTreeView::PerformSearch(const FString& InSearchText)
{
    // 검색 결과 초기화
    SearchResults.Empty();
    
    // 검색어를 소문자로 변환 (대소문자 구분 없는 검색)
    FString LowerSearchText = InSearchText.ToLower();
    UE_LOG(LogTemp, Display, TEXT("검색 시작: '%s'"), *InSearchText);
    
    // 모든 항목을 순회하며 검색
    for (const auto& Pair : PartNoToItemMap)
    {
        TSharedPtr<FPartTreeItem> Item = Pair.Value;
        if (FTreeViewUtils::DoesItemMatchSearch(Item, LowerSearchText))
        {
            SearchResults.Add(Item);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("검색 결과: %d개 항목 발견"), SearchResults.Num());
    
    // 트리가 접혀있고 검색 결과가 있는 경우에만 경로 펼치기 실행
    if (SearchResults.Num() > 0 && TreeView.IsValid())
    {
        // 검색 결과가 있으면 잠시 지연 후 결과로 경로 펼치기
        GEditor->GetTimerManager()->SetTimer(
            ExpandTimerHandle,
            FTimerDelegate::CreateLambda([this]() {
                // 첫 번째 검색 결과의 경로 펼치기
                if (SearchResults.Num() > 0)
                {
                    ExpandPathToItem(SearchResults[0]);
                    TreeView->SetItemSelection(SearchResults[0], true);
                    TreeView->RequestScrollIntoView(SearchResults[0]);
                }
            }),
            0.1f,  // 0.1초 지연 (접기 후 펼치기 위한 시간 간격)
            false  // 반복 없음
        );
    }
    
    // 트리뷰 갱신 (필터링 적용)
    TreeView->RequestTreeRefresh();
}

// 설정 핸들러
FReply SLevelBasedTreeView::OnSettingsButtonClicked()
{
    // 현재 저장된 임포트 설정 가져오기
    FImportSettings CurrentSettings = UImportSettingsManager::Get()->GetSettings();
    FImportSettings NewSettings = CurrentSettings;
    
    // 임포트 설정 대화상자 표시
    bool bConfirmed = ShowImportSettingsDialog(
        FSlateApplication::Get().GetActiveTopLevelWindow(),
        FText::FromString(TEXT("3DXML 임포트 설정")),
        CurrentSettings,
        NewSettings
    );
    
    if (bConfirmed)
    {
        // 새 설정 저장
        UImportSettingsManager::Get()->SaveSettings(NewSettings);
        UE_LOG(LogTemp, Display, TEXT("3DXML 임포트 설정이 업데이트되었습니다."));
    }
    
    return FReply::Handled();
}

// 레벨 0 항목들 접기 함수
void SLevelBasedTreeView::FoldLevelZeroItems()
{
    if (!TreeView.IsValid())
    {
        return;
    }

    int32 FoldedItemCount = 0;
    
    // 최상위 레벨 0 항목 접기
    if (LevelToItemsMap.Contains(0))
    {
        const TArray<TSharedPtr<FPartTreeItem>>& Level0Items = LevelToItemsMap[0];
        for (const auto& Level0Item : Level0Items)
        {
            // 최상위 레벨 0 항목만 접기
            TreeView->SetItemExpansion(Level0Item, false);
            FoldedItemCount++;
        }
        
        UE_LOG(LogTemp, Display, TEXT("레벨 0 항목 접기 완료: %d개 항목"), FoldedItemCount);
    }
    else
    {
        // LevelToItemsMap에 레벨 0 항목이 없으면 루트 항목 접기
        for (auto& RootItem : AllRootItems)
        {
            TreeView->SetItemExpansion(RootItem, false);
            FoldedItemCount++;
        }
        
        UE_LOG(LogTemp, Display, TEXT("루트 항목 접기 완료: %d개 항목"), FoldedItemCount);
    }
    
    // 트리뷰 갱신
    if (FoldedItemCount > 0)
    {
        TreeView->RequestTreeRefresh();
    }
}

// 검색 텍스트 확정 이벤트 핸들러
void SLevelBasedTreeView::OnSearchTextCommitted(const FText& InText, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		// Enter 키로 검색 실행
		SearchText = InText.ToString();
		if (!SearchText.IsEmpty())
		{
			PerformSearch(SearchText);
		}
	}
}

// 항목 선택 변경 이벤트 핸들러
void SLevelBasedTreeView::OnSelectionChanged(TSharedPtr<FPartTreeItem> Item, ESelectInfo::Type SelectInfo)
{
    // 선택 변경 시 메타데이터 위젯에 선택 항목 전달
    if (Item.IsValid() && MetadataWidget.IsValid())
    {
        MetadataWidget->SetSelectedItem(Item);
    }
}

// 트리뷰 항목 더블클릭 이벤트 핸들러
void SLevelBasedTreeView::OnTreeItemDoubleClick(TSharedPtr<FPartTreeItem> Item)
{
    if (Item.IsValid())
    {
        // 더블클릭 시 SelectActorByPartNo 함수 호출
        SelectActorByPartNo(Item->PartNo);
        
        // 선택된 액터에 카메라 초점 맞추기
#if WITH_EDITOR
        if (GEditor)
        {
            // 현재 선택된 액터 가져오기
            USelection* SelectedActors = GEditor->GetSelectedActors();
            if (SelectedActors && SelectedActors->Num() > 0)
            {
                // 첫 번째 선택된 액터를 가져옴
                AActor* SelectedActor = Cast<AActor>(SelectedActors->GetSelectedObject(0));
                if (SelectedActor)
                {
                    // 모든 뷰포트에 액터 표시 요청
                    GEditor->MoveViewportCamerasToActor(*SelectedActor, false);
                    
                    // 뷰포트 갱신
                    GEditor->RedrawLevelEditingViewports();
                    
                    UE_LOG(LogTemp, Display, TEXT("카메라를 선택된 액터로 이동: %s"), *SelectedActor->GetName());
                }
            }
        }
#endif
    }
}

// 컨텍스트 메뉴 생성 함수
TSharedPtr<SWidget> SLevelBasedTreeView::OnContextMenuOpening()
{
    FMenuBuilder MenuBuilder(true, nullptr);
    
    // 현재 선택된 항목 가져오기
    TArray<TSharedPtr<FPartTreeItem>> SelectedItems = TreeView->GetSelectedItems();
    TSharedPtr<FPartTreeItem> SelectedItem = (SelectedItems.Num() > 0) ? SelectedItems[0] : nullptr;
    
    MenuBuilder.BeginSection("TreeItemActions", FText::FromString(TEXT("Menu")));
    {
        // 이미지 필터 활성화
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("Show Only Nodes With Images")),
            FText::FromString(TEXT("Display only nodes that have images")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    ToggleImageFiltering(true);
                }),
                FCanExecuteAction::CreateLambda([this]() { 
                    return !FilterManager->IsFilterEnabled("ImageFilter") && 
                           FServiceLocator::GetImageManager()->GetPartsWithImageSet().Num() > 0; 
                }),
                FIsActionChecked::CreateLambda([this]() {
                    return FilterManager->IsFilterEnabled("ImageFilter");
                })
            ),
            NAME_None,
            EUserInterfaceActionType::Check
        );
        
        // 이미지 필터 해제
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("Show All Nodes")),
            FText::FromString(TEXT("Disable image filter and show all nodes")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    ToggleImageFiltering(false);
                }),
                FCanExecuteAction::CreateLambda([this]() { 
                    return FilterManager->IsFilterEnabled("ImageFilter"); 
                })
            )
        );
        
        // 노드 이름 클립보드에 복사
        /*MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("Copy Node Name")),
            FText::FromString(TEXT("Copy selected node name to clipboard")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    TArray<TSharedPtr<FPartTreeItem>> Items = TreeView->GetSelectedItems();
                    if (Items.Num() > 0)
                    {
                        FString CombinedNames;
                        // 모든 선택된 항목의 이름을 합치기
                        for (int32 i = 0; i < Items.Num(); i++)
                        {
                            if (i > 0)
                            {
                                // 첫 번째 항목이 아니면 구분자 추가
                                CombinedNames.Append(TEXT(", "));
                            }
                            CombinedNames.Append(Items[i]->PartNo);
                        }
                    
                        // 클립보드에 모든 선택된 항목의 이름 복사
                        FPlatformApplicationMisc::ClipboardCopy(*CombinedNames);
                    
                        UE_LOG(LogTemp, Display, TEXT("Node names copied to clipboard: %d items"), Items.Num());
                    }
                }),
                FCanExecuteAction::CreateLambda([this]() { 
                    // 하나 이상의 항목이 선택되었을 때만 활성화
                    return TreeView->GetSelectedItems().Num() > 0; 
                })
            )
        );*/
        
        // 상세 정보 보기
        /*MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("View Details")),
            FText::FromString(TEXT("View detailed information for the selected item")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    // 선택된 항목의 세부 정보를 로그로 출력
                    TArray<TSharedPtr<FPartTreeItem>> Items = TreeView->GetSelectedItems();
                    if (Items.Num() > 0)
                    {
                        TSharedPtr<FPartTreeItem> Item = Items[0];
                        UE_LOG(LogTemp, Display, TEXT("Item Details - PartNo: %s, Level: %d"), 
                            *Item->PartNo, Item->Level);
                    }
                }),
                FCanExecuteAction::CreateLambda([SelectedItem]() { 
                    // 선택된 항목이 있을 때만 활성화
                    return SelectedItem.IsValid(); 
                })
            )
        );*/
        
        // 선택 노드 펼치기 메뉴
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("Expand All Children")),
            FText::FromString(TEXT("Expand all child nodes of the selected item")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    TArray<TSharedPtr<FPartTreeItem>> Items = TreeView->GetSelectedItems();
                    if (Items.Num() > 0)
                    {
                        ExpandItemRecursively(Items[0], true);
                    }
                }),
                FCanExecuteAction::CreateLambda([SelectedItem]() { 
                    // 선택된 항목이 있고 자식이 있을 때만 활성화
                    return SelectedItem.IsValid() && SelectedItem->Children.Num() > 0; 
                })
            )
        );
        
        // 선택 노드 접기 메뉴
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("Collapse All Children")),
            FText::FromString(TEXT("Collapse all child nodes of the selected item")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    TArray<TSharedPtr<FPartTreeItem>> Items = TreeView->GetSelectedItems();
                    if (Items.Num() > 0)
                    {
                        ExpandItemRecursively(Items[0], false);
                    }
                }),
                FCanExecuteAction::CreateLambda([SelectedItem]() { 
                    // 선택된 항목이 있고 자식이 있을 때만 활성화
                    return SelectedItem.IsValid() && SelectedItem->Children.Num() > 0; 
                })
            )
        );
        
        // 분리선 추가
        MenuBuilder.AddMenuSeparator();

    	// 3DXML 파일 임포트 메뉴
    	MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Import 3DXML to Node")),
			FText::FromString(TEXT("Import 3DXML file to the selected node")),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this]() {
					ImportXMLToSelectedNode();
				}),
				FCanExecuteAction::CreateLambda([this]() { 
					// 하나의 항목이 선택되었을 때만 활성화
					return TreeView->GetSelectedItems().Num() > 0; 
				})
			)
		);

        // 바운딩 박스 계산 메뉴 추가
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("Calculate Mesh Bounds")),
            FText::FromString(TEXT("Calculate the bounding box of all meshes in the selected actor")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    // 선택된 항목 확인
                    TArray<TSharedPtr<FPartTreeItem>> Items = TreeView->GetSelectedItems();
                    if (Items.Num() > 0)
                    {
                        TSharedPtr<FPartTreeItem> Item = Items[0];
                        
                        // 선택된 노드의 Part No로 액터 찾기
                        SelectActorByPartNo(Item->PartNo);
                        
                        // 바운딩 박스 계산
                        FTreeViewUtils::CalculateSelectedActorMeshBounds();
                    }
                    else
                    {
                        // 선택된 항목이 없으면 현재 선택된 모든 액터에 대해 계산
                        FTreeViewUtils::CalculateSelectedActorMeshBounds();
                    }
                }),
                FCanExecuteAction::CreateLambda([]() { 
                    // 항상 활성화
                    return true; 
                })
            )
        );
    }
    MenuBuilder.EndSection();

    return MenuBuilder.MakeWidget();
}

// 트리 항목과 그 하위 항목들을 재귀적으로 펼치거나 접는 함수
void SLevelBasedTreeView::ExpandItemRecursively(TSharedPtr<FPartTreeItem> Item, bool bExpand)
{
    if (!Item.IsValid() || !TreeView.IsValid())
        return;
        
    // 현재 항목 펼치기/접기
    TreeView->SetItemExpansion(Item, bExpand);
    
    // 자식 항목들도 재귀적으로 펼치기/접기
    for (auto& Child : Item->Children)
    {
        ExpandItemRecursively(Child, bExpand);
    }
}

// 이미지 필터링 활성화/비활성화 함수
void SLevelBasedTreeView::ToggleImageFiltering(bool bEnable)
{
    // 이미 같은 상태면 아무것도 하지 않음
	if (FilterManager->IsFilterEnabled("ImageFilter") == bEnable) { return; }
        
	FilterManager->SetFilterEnabled("ImageFilter", bEnable);
    
    UE_LOG(LogTemp, Display, TEXT("이미지 필터링 %s: 이미지 있는 파트 %d개"), 
        bEnable ? TEXT("활성화") : TEXT("비활성화"), FServiceLocator::GetImageManager()->GetPartsWithImageSet().Num());
    
    // OnGetChildren 함수에서 필터링하도록 트리뷰 갱신만 요청
    if (TreeView.IsValid())
    {
        TreeView->RequestTreeRefresh();
    }
}

// 메타데이터 위젯 반환 함수
TSharedRef<SWidget> SLevelBasedTreeView::GetMetadataWidget()
{
    // 메타데이터 표시를 위한 텍스트 블록 생성
    TSharedRef<STextBlock> MetadataText = SNew(STextBlock)
        .Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SLevelBasedTreeView::GetSelectedItemMetadata)));
    
    return MetadataText;
}

// 선택된 항목의 메타데이터를 텍스트로 반환하는 메서드
FText SLevelBasedTreeView::GetSelectedItemMetadata() const
{
    // 선택된 항목 가져오기
    TArray<TSharedPtr<FPartTreeItem>> SelectedItems = TreeView->GetSelectedItems();
    
    if (SelectedItems.Num() == 0)
    {
        return FText::FromString("No item selected");
    }
    
    TSharedPtr<FPartTreeItem> SelectedItem = SelectedItems[0];
    
    return FTreeViewUtils::GetFormattedMetadata(SelectedItem);
}

// 트리뷰 구성 함수
bool SLevelBasedTreeView::BuildTreeView(const FString& FilePath)
{
    // 데이터 초기화
    AllRootItems.Empty();
    PartNoToItemMap.Empty();
    LevelToItemsMap.Empty();
    MaxLevel = 0;
    
    UE_LOG(LogTemp, Display, TEXT("트리뷰 구성 시작: %s"), *FilePath);
    
    // CSV 파일 읽기
    TArray<TArray<FString>> ExcelData;
    if (!FTreeViewUtils::ReadCSVFile(FilePath, ExcelData))
    {
        UE_LOG(LogTemp, Error, TEXT("CSV 파일 읽기 실패: %s"), *FilePath);
        return false;
    }
    
    if (ExcelData.Num() < 2) // 헤더 + 최소 1개 이상의 데이터 행 필요
    {
        UE_LOG(LogTemp, Error, TEXT("데이터 행이 부족합니다"));
        return false;
    }
    
    // 헤더 행
    const TArray<FString>& HeaderRow = ExcelData[0];
    
    // 필요한 열 인덱스 찾기
    int32 PartNoColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("PartNo") || HeaderName == TEXT("Part No");
    });
    
    int32 NextPartColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("NextPart");
    });
    
    int32 LevelColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Level");
    });
    
    // 인덱스를 찾지 못했으면 기본값 사용 (실제 엑셀 파일 구조 기준)
    PartNoColIdx = (PartNoColIdx != INDEX_NONE) ? PartNoColIdx : 3; // D열 (Part No)
    NextPartColIdx = (NextPartColIdx != INDEX_NONE) ? NextPartColIdx : 13; // N열 (NextPart)
    LevelColIdx = (LevelColIdx != INDEX_NONE) ? LevelColIdx : 1; // B열 (Level)
    
    UE_LOG(LogTemp, Display, TEXT("CSV 데이터 읽기 완료: %d개 행"), ExcelData.Num() - 1);
    
    
    // 1단계: 모든 항목 생성 및 레벨별 그룹화 (TreeViewUtils 사용)
    int32 ValidItemCount = FTreeViewUtils::CreateAndGroupItems(
        ExcelData, 
        PartNoColIdx, 
        NextPartColIdx, 
        LevelColIdx,
        PartNoToItemMap,
        LevelToItemsMap,
        MaxLevel,
        AllRootItems
    );
    
    // 2단계: 트리 구조 구축
    BuildTreeStructure();
    
    // 이미지 존재 여부 캐싱 (FPartImageManager 사용)
    FServiceLocator::GetImageManager()->CacheImageExistence(PartNoToItemMap);
    
    // 트리뷰 갱신
    if (TreeView.IsValid())
    {
        TreeView->RequestTreeRefresh();
        
        // 루트 항목 확장
        for (auto& RootItem : AllRootItems)
        {
            TreeView->SetItemExpansion(RootItem, true);
        }
    }

    // 트리뷰 구성 요약 로그 출력
    int32 TotalNodeCount = 0;
    for (const auto& LevelPair : LevelToItemsMap)
    {
        TotalNodeCount += LevelPair.Value.Num();
    }
    
    UE_LOG(LogTemp, Display, TEXT("트리뷰 구성 요약:"));
    UE_LOG(LogTemp, Display, TEXT("- 총 노드 수: %d개"), TotalNodeCount);
    UE_LOG(LogTemp, Display, TEXT("- 루트 노드 수: %d개"), AllRootItems.Num());
    UE_LOG(LogTemp, Display, TEXT("- 최대 레벨 깊이: %d"), MaxLevel);
    UE_LOG(LogTemp, Display, TEXT("- 이미지 있는 노드 수: %d개 (%.1f%%)"), 
           FServiceLocator::GetImageManager()->GetPartsWithImageSet().Num(), 
           (TotalNodeCount > 0) ? (float)FServiceLocator::GetImageManager()->GetPartsWithImageSet().Num() / TotalNodeCount * 100.0f : 0.0f);
    
    // 각 레벨별 노드 갯수 출력
    for (int32 Level = 0; Level <= MaxLevel; ++Level)
    {
        const TArray<TSharedPtr<FPartTreeItem>>* LevelItems = LevelToItemsMap.Find(Level);
        int32 LevelNodeCount = LevelItems ? LevelItems->Num() : 0;
        UE_LOG(LogTemp, Display, TEXT("- 레벨 %d 노드 수: %d개"), Level, LevelNodeCount);
    }
    
    return AllRootItems.Num() > 0;
}

// 트리 구조 구축 함수
void SLevelBasedTreeView::BuildTreeStructure()
{
    UE_LOG(LogTemp, Display, TEXT("트리 구조 구축 시작: 최대 레벨 %d"), MaxLevel);
    
    // 레벨 0부터 MaxLevel-1까지 순회하며 부모-자식 관계 설정
    for (int32 CurrentLevel = 0; CurrentLevel < MaxLevel; ++CurrentLevel)
    {
        // 현재 레벨의 항목이 있는지 확인
        if (!LevelToItemsMap.Contains(CurrentLevel))
            continue;
            
        // 다음 레벨의 항목이 있는지 확인
        if (!LevelToItemsMap.Contains(CurrentLevel + 1))
            continue;
            
        // 현재 레벨의 모든 항목
        const TArray<TSharedPtr<FPartTreeItem>>& CurrentLevelItems = LevelToItemsMap[CurrentLevel];
        
        // 다음 레벨의 모든 항목
        TArray<TSharedPtr<FPartTreeItem>>& NextLevelItems = LevelToItemsMap[CurrentLevel + 1];
        
        int32 ConnectionCount = 0;
        
        // 다음 레벨의 각 항목에 대해 부모 찾기
        for (const TSharedPtr<FPartTreeItem>& ChildItem : NextLevelItems)
        {
            // NextPart가 현재 레벨의 항목 중 하나와 일치하는지 확인
            if (!ChildItem->NextPart.IsEmpty() && !ChildItem->NextPart.Equals(TEXT("nan"), ESearchCase::IgnoreCase))
            {
                // 직접 맵을 사용하여 부모 항목 찾기 (더 효율적)
                TSharedPtr<FPartTreeItem>* ParentItemPtr = PartNoToItemMap.Find(ChildItem->NextPart);
                
                if (ParentItemPtr && (*ParentItemPtr)->Level == CurrentLevel)
                {
                    // 부모-자식 관계 설정
                    (*ParentItemPtr)->Children.Add(ChildItem);
                    ConnectionCount++;
                }
            }
        }
        
        UE_LOG(LogTemp, Verbose, TEXT("레벨 %d -> %d 부모-자식 연결: %d개 설정됨"), 
            CurrentLevel, CurrentLevel + 1, ConnectionCount);
    }
    
    // 루트 항목들이 설정되지 않았으면, 레벨 0의 항목들을 루트로 설정
    if (AllRootItems.Num() == 0 && LevelToItemsMap.Contains(0))
    {
        AllRootItems = LevelToItemsMap[0];
        UE_LOG(LogTemp, Display, TEXT("루트 항목 자동 설정: 레벨 0의 모든 항목(%d개)이 루트로 설정됨"), AllRootItems.Num());
    }
    
    UE_LOG(LogTemp, Display, TEXT("트리 구조 구축 완료: 루트 항목 %d개"), AllRootItems.Num());
}

// 트리뷰 행 생성 델리게이트
TSharedRef<ITableRow> SLevelBasedTreeView::OnGenerateRow(TSharedPtr<FPartTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    // 기본 텍스트 색상과 폰트
    FSlateColor TextColor = FSlateColor(FLinearColor::White);
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 9);
    bool bShowImportIcon = false;
    
    // 임포트된 노드 확인
    if (FImportedNodeManager::Get().IsNodeImported(Item->PartNo))
    {
        TextColor = FSlateColor(FLinearColor(0.0f, 0.8f, 0.4f)); // 밝은 녹색
        FontInfo = FCoreStyle::GetDefaultFontStyle("Bold", 9);
        bShowImportIcon = true;
    }
    // 검색 중이고 검색어가 있는 경우
    else if (bIsSearching && !SearchText.IsEmpty())
    {
        // 직접 검색어로 다시 확인
        FString LowerSearchText = SearchText.ToLower();
        FString LowerPartNo = Item->PartNo.ToLower();
        FString LowerNomenclature = Item->Nomenclature.ToLower();
        
        bool isSearchMatch = LowerPartNo.Contains(LowerSearchText) || 
                             LowerNomenclature.Contains(LowerSearchText);
        
        if (isSearchMatch)
        {
            // 검색 결과 항목: 녹색, 굵은 글씨
            TextColor = FSlateColor(FLinearColor(0.2f, 0.8f, 0.2f)); // 밝은 녹색
            FontInfo = FCoreStyle::GetDefaultFontStyle("Bold", 9);
        }
    }
    else if (!bIsSearching)
    {
        // 검색 중이 아닌 경우 이미지 있는 항목은 빨간색
        if (FServiceLocator::GetImageManager()->HasImage(Item->PartNo))
        {
            TextColor = FSlateColor(FLinearColor::Red);
        }
    }
    
    // 각 트리 항목에 대한 행 위젯 생성
    return SNew(STableRow<TSharedPtr<FPartTreeItem>>, OwnerTable)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(FMargin(4, 0))
            [
                SNew(STextBlock)
                .Text(FText::FromString(Item->PartNo))
                .ColorAndOpacity(TextColor)
                .Font(FontInfo)
            ]
            // 임포트된 노드에 아이콘 추가
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(FMargin(2, 0))
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("Icons.Import"))
                .Visibility(bShowImportIcon ? EVisibility::Visible : EVisibility::Collapsed)
            ]
        ];
}

// 트리뷰 자식 항목 반환 델리게이트
void SLevelBasedTreeView::OnGetChildren(TSharedPtr<FPartTreeItem> Item, TArray<TSharedPtr<FPartTreeItem>>& OutChildren)
{
	if (bIsSearching && !SearchText.IsEmpty())
	{
		// 검색 중인 경우: 검색 결과에 있는 자식 항목이나 
		// 검색 결과의 부모 경로에 있는 항목만 표시
		for (const auto& Child : Item->Children)
		{
			if (SearchResults.Contains(Child))
			{
				// 직접 검색 결과인 경우
				OutChildren.Add(Child);
			}
			else
			{
				// 자식 항목 중에 검색 결과가 있는지 확인
				bool bHasSearchResultChild = false;
				for (const auto& Result : SearchResults)
				{
					if (FTreeViewUtils::IsChildOf(Result, Child))
					{
						bHasSearchResultChild = true;
						break;
					}
				}
                
				if (bHasSearchResultChild)
				{
					OutChildren.Add(Child);
				}
			}
		}
	}
	else if (FilterManager->IsFilterEnabled("ImageFilter"))
	{
		// 이미지 필터링
		for (const auto& Child : Item->Children)
		{
			// 필터 관리자의 PassesAllFilters 함수를 사용하여 확인
			if (FilterManager->PassesAllFilters(Child))
			{
				OutChildren.Add(Child);
			}
		}
	}
	else
	{
		// 필터링 없이 모든 자식 항목 표시
		OutChildren = Item->Children;
	}
}

// 항목의 경로를 펼치는 함수
void SLevelBasedTreeView::ExpandPathToItem(const TSharedPtr<FPartTreeItem>& Item)
{
	// 부모 항목을 찾아 재귀적으로 경로 펼치기
	TSharedPtr<FPartTreeItem> ParentItem = FTreeViewUtils::FindParentItem(Item, PartNoToItemMap);
	if (ParentItem.IsValid())
	{
		ExpandPathToItem(ParentItem);
		TreeView->SetItemExpansion(ParentItem, true);
	}
}

// 3DXML 파일 임포트 함수
void SLevelBasedTreeView::ImportXMLToSelectedNode()
{
    // 선택된 노드 확인
    TArray<TSharedPtr<FPartTreeItem>> SelectedItems = TreeView->GetSelectedItems();
    if (SelectedItems.Num() == 0)
    {
        FNotificationInfo Info(FText::FromString(TEXT("3DXML 임포트를 위해 하나 이상의 노드를 선택해주세요.")));
        Info.ExpireDuration = 4.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
    }
    
    // 현재 저장된 임포트 설정 가져오기
    FImportSettings ImportSettings = UImportSettingsManager::Get()->GetSettings();

    // 언리얼 프로젝트 루트 디렉토리에 있는 3DXML 폴더 경로 설정 
    FString XMLDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("3DXML"));
    
    int32 SuccessCount = 0;
    int32 FailCount = 0;
    int32 AlreadyImportedCount = 0;
    TArray<FString> FailedParts;
    TArray<FString> ImportedParts;
    TArray<FString> AlreadyImportedParts;
    
    // 전체 항목 수 저장
    int32 TotalItems = SelectedItems.Num();
    
    // 선택된 모든 항목에 대해 처리
    for (int32 ItemIndex = 0; ItemIndex < SelectedItems.Num(); ItemIndex++)
    {
        TSharedPtr<FPartTreeItem> SelectedItem = SelectedItems[ItemIndex];
        FString PartNo = SelectedItem->PartNo;
        
        // FTreeViewUtils를 사용하여 일치하는 파일 찾기
        FFileMatchResult FileResult = FTreeViewUtils::FindMatchingFileForPartNo(
            XMLDir,           // 디렉토리 경로
            TEXT("*.3dxml"),  // 파일 패턴
            PartNo            // 파트 번호
        );
        
        // 파일을 찾지 못한 경우 다음 항목으로 넘어감
        if (!FileResult.bFound)
        {
            FailCount++;
            FailedParts.Add(PartNo);
            UE_LOG(LogTemp, Warning, TEXT("파일을 찾을 수 없음: %s - %s"), *PartNo, *FileResult.ErrorMessage);
            continue;
        }

#if WITH_EDITOR
        // DatasmithSceneManager를 사용하여 임포트 및 처리
        FDatasmithSceneManager SceneManager;
        
        // ImportSettings 적용
        SceneManager.SetImportSettings(ImportSettings);

        // 이미 임포트된 노드인지 확인
        if (SceneManager.IsAlreadyImportedInLevel(PartNo))
        {
            UE_LOG(LogTemp, Warning, TEXT("이미 임포트된 액터가 있음: %s"), *PartNo);
            AlreadyImportedCount++;
            AlreadyImportedParts.Add(PartNo);
            continue; // 이미 임포트된 경우 건너뛰기
        }
        
        // 현재 인덱스(1부터 시작)와 전체 갯수를 전달하여 진행 상황 표시
        AActor* ResultActor = SceneManager.ImportAndProcessDatasmith(
            FileResult.FilePath,     // 임포트할 파일 경로
            PartNo,                  // 파트 번호
            ItemIndex + 1,           // 현재 처리 중인 인덱스(1부터 시작)
            TotalItems               // 전체 항목 수
        );
        
        if (ResultActor)
        {
            SuccessCount++;
            ImportedParts.Add(PartNo);
            
            // 설정에 따라 마지막으로 처리된 액터 선택
            if (ImportSettings.bSelectActorAfterImport && SelectedItem == SelectedItems.Last())
            {
                GEditor->SelectNone(true, true, false);
                GEditor->SelectActor(ResultActor, true, true, true);
            }
        }
        else
        {
            FailCount++;
            FailedParts.Add(PartNo);
        }
#else
        // 에디터가 아닌 환경에서는 임포트 불가
        FNotificationInfo Info(FText::FromString(TEXT("3DXML 파일 임포트는 에디터 모드에서만 가능합니다.")));
        Info.ExpireDuration = 4.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
        return;
#endif
    }
    
    // 트리뷰 갱신
    if (SuccessCount > 0 && TreeView.IsValid())
    {
        // 트리뷰 강제 갱신
        TreeView->RebuildList();
        
        // 임포트된 마지막 항목을 화면에 표시
        if (ImportedParts.Num() > 0 && ImportSettings.bSelectActorAfterImport)
        {
            FString LastImportedPart = ImportedParts.Last();
            TSharedPtr<FPartTreeItem>* ItemPtr = PartNoToItemMap.Find(LastImportedPart);
            if (ItemPtr && ItemPtr->IsValid())
            {
                TreeView->SetItemSelection(*ItemPtr, true, ESelectInfo::OnMouseClick);
                TreeView->RequestScrollIntoView(*ItemPtr);
            }
        }
    }
    
    // 이미 임포트된 노드가 있는 경우 알림
    if (AlreadyImportedCount > 0)
    {
        FString SkippedMessage;
    
        if (AlreadyImportedCount == 1)
        {
            // 단일 노드인 경우
            SkippedMessage = FString::Printf(TEXT("이미 임포트된 노드가 있습니다: %s"), 
                *AlreadyImportedParts[0]);
        }
        else
        {
            // 여러 노드인 경우
            SkippedMessage = FString::Printf(TEXT("이미 임포트된 %d개 노드는 건너뜁니다: %s"), 
                AlreadyImportedCount, *FString::Join(AlreadyImportedParts, TEXT(", ")));
        }
    
        FNotificationInfo Info(FText::FromString(SkippedMessage));
        Info.ExpireDuration = 4.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    }
    
    // 처리 결과 알림 표시
    if (SuccessCount > 0 && FailCount == 0)
    {
        FString ResultMessage = FString::Printf(TEXT("%d개 노드에 3DXML 파일 임포트가 완료되었습니다."), SuccessCount);
        
        FNotificationInfo Info(FText::FromString(ResultMessage));
        Info.bUseLargeFont = false;
        Info.FadeInDuration = 0.5f;
        Info.FadeOutDuration = 0.5f;
        Info.ExpireDuration = 4.0f;
        Info.bUseSuccessFailIcons = true;
        
        TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
        if (NotificationItem.IsValid())
        {
            NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
        }
    }
    else if (SuccessCount > 0 && FailCount > 0)
    {
        FString ResultMessage = FString::Printf(TEXT("%d개 성공, %d개 실패\n실패한 노드: %s"), 
            SuccessCount, FailCount, *FString::Join(FailedParts, TEXT(", ")));
        
        FNotificationInfo Info(FText::FromString(ResultMessage));
        Info.bUseLargeFont = false;
        Info.FadeInDuration = 0.5f;
        Info.FadeOutDuration = 0.5f;
        Info.ExpireDuration = 6.0f;  // 좀 더 긴 시간 표시
        Info.bUseSuccessFailIcons = true;
        
        TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
        if (NotificationItem.IsValid())
        {
            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
        }
    }
    else if (SuccessCount == 0 && FailCount > 0)
    {
        FString ResultMessage = TEXT("모든 3DXML 파일 임포트에 실패했습니다.");
        
        FNotificationInfo Info(FText::FromString(ResultMessage));
        Info.bUseLargeFont = false;
        Info.FadeInDuration = 0.5f;
        Info.FadeOutDuration = 0.5f;
        Info.ExpireDuration = 4.0f;
        Info.bUseSuccessFailIcons = true;
        
        TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
        if (NotificationItem.IsValid())
        {
            NotificationItem->SetCompletionState(SNotificationItem::CS_Fail);
        }
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

// 트리뷰 위젯 생성 헬퍼 함수
void CreateLevelBasedTreeView(TSharedPtr<SWidget>& OutTreeViewWidget, TSharedPtr<SWidget>& OutMetadataWidget, const FString& ExcelFilePath)
{
    UE_LOG(LogTemp, Display, TEXT("트리뷰 위젯 생성 시작: %s"), *ExcelFilePath);
    
    // 메타데이터 위젯 생성
    TSharedPtr<SPartMetadataWidget> MetadataWidget = SNew(SPartMetadataWidget);
    
    // SLevelBasedTreeView 인스턴스 생성
    TSharedPtr<SLevelBasedTreeView> TreeView = SNew(SLevelBasedTreeView)
        .ExcelFilePath(ExcelFilePath)
        .MetadataWidget(MetadataWidget);
    
    OutTreeViewWidget = SNew(SVerticalBox)
        // 검색 위젯 추가
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            TreeView->GetSearchWidget()
        ]
        // 트리뷰 위젯
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        [
            TreeView.ToSharedRef()
        ];
    
    // 메타데이터 위젯 설정
    OutMetadataWidget = MetadataWidget.ToSharedRef();
    
    UE_LOG(LogTemp, Display, TEXT("트리뷰 위젯 생성 완료"));
}
