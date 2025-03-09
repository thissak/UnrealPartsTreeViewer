// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/LevelBasedTreeView.h"

#include "UI/PartMetadataWidget.h"
#include "UI/TreeViewUtils.h"
#include "ServiceLocator.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSearchBox.h"
#include "Windows/WindowsPlatformApplicationMisc.h"
#include "SlateOptMacros.h"

#include "DatasmithImportFactory.h"
#include "DatasmithImportOptions.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "DatasmithSceneManager.h"
#include "IAssetTools.h"
#include "Engine/StaticMeshActor.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#endif

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLevelBasedTreeView::Construct(const FArguments& InArgs)
{
    MaxLevel = 0;
	bIsSearching = false;  // 검색 상태 초기화
	SearchText = "";       // 검색어 초기화

	// 필터 관리자 초기화
	FilterManager = MakeShared<FPartTreeViewFilterManager>();
	
    // 메타데이터 위젯 참조 저장
    MetadataWidget = InArgs._MetadataWidget;
    
    // 이미지 브러시 초기화
	CurrentImageBrush = FServiceLocator::GetImageManager()->CreateImageBrush(nullptr);
    
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
void SLevelBasedTreeView::SelectActorByPartNo(const FString& PartNo)
{
#if WITH_EDITOR
    if (GEditor)
    {
        // 현재 선택 해제
        GEditor->SelectNone(true, true, false);
        
        // 월드의 모든 액터를 검색
        UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
        if (EditorWorld)
        {
            bool bFoundAnyActor = false;
            
            // 모든 액터 순회
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
                        bFoundAnyActor = true;
                    }
                }
            }
            
            if (!bFoundAnyActor)
            {
                UE_LOG(LogTemp, Warning, TEXT("일치하는 액터를 찾을 수 없음: %s"), *PartNo);
            }
        }
    }
#endif
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
            
			// 검색 입력창
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

// OnSearchTextChanged 함수 수정
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

// PerformSearch 함수 수정
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

void SLevelBasedTreeView::OnSelectionChanged(TSharedPtr<FPartTreeItem> Item, ESelectInfo::Type SelectInfo)
{
    // 선택 변경 시 메타데이터 위젯에 선택 항목 전달
    if (Item.IsValid() && MetadataWidget.IsValid())
    {
        MetadataWidget->SetSelectedItem(Item);
    }
}

void SLevelBasedTreeView::OnTreeItemDoubleClick(TSharedPtr<FPartTreeItem> Item)
{
    if (Item.IsValid())
    {
        // 더블클릭 시 SelectActorByPartNo 함수 호출
        SelectActorByPartNo(Item->PartNo);
    }
}

// 선택된 항목 이미지 업데이트
void SLevelBasedTreeView::UpdateSelectedItemImage()
{
	// 선택된 항목 가져오기
	TArray<TSharedPtr<FPartTreeItem>> SelectedItems = TreeView->GetSelectedItems();
    
	if (SelectedItems.Num() == 0 || !ItemImageWidget.IsValid())
	{
		return;
	}
    
	TSharedPtr<FPartTreeItem> SelectedItem = SelectedItems[0];
	FString PartNoStr = SelectedItem->PartNo;
    
	// 이미지 로드 시도 (이제 FPartImageManager 사용)
	UTexture2D* Texture = FServiceLocator::GetImageManager()->LoadPartImage(PartNoStr);
    
	// 브러시 생성 및 설정
	CurrentImageBrush = FServiceLocator::GetImageManager()->CreateImageBrush(Texture);
    
	// 이미지 위젯에 새 브러시 설정
	ItemImageWidget->SetImage(CurrentImageBrush.Get());
}

// 컨텍스트 메뉴 생성
// 컨텍스트 메뉴 생성
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
        MenuBuilder.AddMenuEntry(
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
        );
        
        // 상세 정보 보기
        MenuBuilder.AddMenuEntry(
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
        );
        
        // 이미지 보기 메뉴는 제거
        
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
					return TreeView->GetSelectedItems().Num() == 1; 
				})
			)
		);

    	// 자식 액터 제거 메뉴
    	MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Remove Child Actors")),
			FText::FromString(TEXT("Remove all child actors of the selected actor")),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateLambda([this]() {
					RemoveChildActorsExceptStaticMesh();
				}),
				FCanExecuteAction::CreateLambda([this]() { 
					return TreeView->GetSelectedItems().Num() == 1; 
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

// 이미지 위젯 반환 함수
TSharedRef<SWidget> SLevelBasedTreeView::GetImageWidget()
{
    // 이미지 박스 크기 설정
    const float ImageWidth = 300.0f;
    const float ImageHeight = 200.0f;

    // 이미지 위젯 생성
    TSharedRef<SBorder> ImageBorder = SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
        .Padding(0)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SAssignNew(ItemImageWidget, SImage)
            .DesiredSizeOverride(FVector2D(ImageWidth, ImageHeight))
            .Image(CurrentImageBrush.Get())
        ];

    return SNew(SBox)
        .WidthOverride(ImageWidth)
        .HeightOverride(ImageHeight)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SScaleBox)
            .Stretch(EStretch::ScaleToFit)
            .StretchDirection(EStretchDirection::Both)
            [
                ImageBorder
            ]
        ];
}

// ToggleImageFiltering 함수 구현
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

// 선택된 항목의 메타데이터를 텍스트로 반환하는 메서드 (계속)
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

// OnGenerateRow 함수 구현
TSharedRef<ITableRow> SLevelBasedTreeView::OnGenerateRow(TSharedPtr<FPartTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    // 기본 텍스트 색상과 폰트
    FSlateColor TextColor = FSlateColor(FLinearColor::White);
    FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 9);
    
    // 검색 중이고 검색어가 있는 경우
    if (bIsSearching && !SearchText.IsEmpty())
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
        ];
}

// OnGetChildren 함수 구현
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
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

// 트리뷰 위젯 생성 헬퍼 함수 - 모듈화된 버전
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

void SLevelBasedTreeView::ImportXMLToSelectedNode()
{
    // 선택된 노드 확인
    TArray<TSharedPtr<FPartTreeItem>> SelectedItems = TreeView->GetSelectedItems();
    if (SelectedItems.Num() != 1)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("3DXML 임포트를 위해 정확히 하나의 노드가 선택되어야 합니다.")));
        return;
    }
    
    TSharedPtr<FPartTreeItem> SelectedItem = SelectedItems[0];
    FString PartNo = SelectedItem->PartNo;
    
    // 언리얼 프로젝트 루트 디렉토리에 있는 3DXML 폴더 경로 설정 
    FString XMLDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("3DXML"));
    
    // 폴더 존재 확인 및 파일 찾기 코드 (변경 없음)
    if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*XMLDir))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("3DXML 폴더가 존재하지 않습니다: ") + XMLDir));
        return;
    }
    
    // 폴더 내 모든 3DXML 파일 찾기
    TArray<FString> FoundFiles;
    IFileManager::Get().FindFiles(FoundFiles, *(XMLDir / TEXT("*.3dxml")), true, false);
    
    if (FoundFiles.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("3DXML 폴더에 .3dxml 파일이 없습니다.")));
        return;
    }
    
    // 선택된 노드와 일치하는 파일 찾기
    FString MatchingFilePath;
    FString MatchingFileName;
    
    for (const FString& FileName : FoundFiles)
    {
        // 파일명만 추출 (경로 없이)
        FString Filename = FPaths::GetBaseFilename(FileName);
        
        // 언더바로 문자열 분리
        TArray<FString> Parts;
        Filename.ParseIntoArray(Parts, TEXT("_"));
        
        // 언더바로 구분된 부분이 4개 이상인지 확인하고, 3번 인덱스가 파트 번호인지 확인
        if (Parts.Num() >= 4 && Parts[3] == PartNo)
        {
            // 전체 경로 구성
            MatchingFilePath = FPaths::Combine(XMLDir, FileName);
            MatchingFileName = Filename;
            break;
        }
    }
    
    // 일치하는 파일을 찾지 못한 경우
    if (MatchingFilePath.IsEmpty())
    {
        FString MessageText = FString::Printf(TEXT("선택된 노드(%s)와 일치하는 3DXML 파일을 찾지 못했습니다."), *PartNo);
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(MessageText));
        return;
    }

#if WITH_EDITOR
    // 파일 임포트를 위한 전체 경로 작업
    FString FullFilePath = FPaths::ConvertRelativePathToFull(MatchingFilePath);
    
    UE_LOG(LogTemp, Display, TEXT("3DXML 파일 임포트 시작: %s"), *FullFilePath);
    
    // Datasmith 임포트 팩토리 및 옵션 생성/설정 (변경 없음)
    UDatasmithImportFactory* DatasmithFactory = NewObject<UDatasmithImportFactory>();
    if (!DatasmithFactory)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Datasmith 임포트 팩토리를 생성할 수 없습니다.")));
        return;
    }
    
    // 임포트 옵션 설정
    UDatasmithImportOptions* ImportOptions = NewObject<UDatasmithImportOptions>();
    if (ImportOptions)
    {
        // 기본 옵션 설정
        ImportOptions->BaseOptions.SceneHandling = EDatasmithImportScene::CurrentLevel;
        ImportOptions->BaseOptions.bIncludeGeometry = true;
        ImportOptions->BaseOptions.bIncludeMaterial = true;
        ImportOptions->BaseOptions.bIncludeLight = false;
        ImportOptions->BaseOptions.bIncludeCamera = false;
        ImportOptions->BaseOptions.bIncludeAnimation = false;
        
        // 충돌 정책 설정
        ImportOptions->MaterialConflictPolicy = EDatasmithImportAssetConflictPolicy::Update;
        ImportOptions->TextureConflictPolicy = EDatasmithImportAssetConflictPolicy::Update;
        ImportOptions->StaticMeshActorImportPolicy = EDatasmithImportActorPolicy::Update;
        ImportOptions->LightImportPolicy = EDatasmithImportActorPolicy::Update;
        ImportOptions->CameraImportPolicy = EDatasmithImportActorPolicy::Update;
        ImportOptions->OtherActorImportPolicy = EDatasmithImportActorPolicy::Update;
        ImportOptions->MaterialQuality = EDatasmithImportMaterialQuality::UseRealFresnelCurves;
        
        // 스태틱 메시 옵션 설정
        ImportOptions->BaseOptions.StaticMeshOptions.MinLightmapResolution = EDatasmithImportLightmapMin::LIGHTMAP_64;
        ImportOptions->BaseOptions.StaticMeshOptions.MaxLightmapResolution = EDatasmithImportLightmapMax::LIGHTMAP_1024;
        ImportOptions->BaseOptions.StaticMeshOptions.bGenerateLightmapUVs = true;
        ImportOptions->BaseOptions.StaticMeshOptions.bRemoveDegenerates = true;
        
        // 파일 정보 설정
        ImportOptions->FileName = FPaths::GetCleanFilename(FullFilePath);
        ImportOptions->FilePath = FullFilePath;
        ImportOptions->SourceUri = FullFilePath;
    }
    
    // 임포트 태스크 생성
    UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
    ImportTask->Filename = FullFilePath;
    ImportTask->DestinationPath = FString::Printf(TEXT("/Game/Datasmith/%s"), *PartNo);
    ImportTask->Options = ImportOptions;
    ImportTask->Factory = DatasmithFactory;
    ImportTask->bSave = true;
    ImportTask->bAutomated = true;
    ImportTask->bAsync = false;  // 동기 임포트로 설정
    
    // 임포트 태스크 준비
    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<UAssetImportTask*> ImportTasks;
    ImportTasks.Add(ImportTask);
    
    // 프로그레스 대화상자 표시
    GWarn->BeginSlowTask(FText::FromString(TEXT("3DXML 파일 임포트 중...")), true);
    
    // 임포트 수행 (동기적으로 실행)
    AssetToolsModule.Get().ImportAssetTasks(ImportTasks);
    
    // 프로그레스 대화상자 종료
    GWarn->EndSlowTask();
    
    UE_LOG(LogTemp, Display, TEXT("3DXML 파일 임포트 완료: %s"), *FullFilePath);
    
    // DatasmithSceneManager 생성 및 설정
    FDatasmithSceneManager SceneManager;
    
    // GetObjects() 함수를 사용하여 DatasmithScene 객체 찾기
    bool bSceneFound = false;
    const TArray<UObject*>& ImportedObjects = ImportTask->GetObjects();
    for (UObject* Object : ImportedObjects)
    {
        if (Object && Object->GetClass()->GetName().Contains(TEXT("DatasmithScene")))
        {
            if (SceneManager.SetDatasmithScene(Object))
            {
                UE_LOG(LogTemp, Display, TEXT("DatasmithScene 객체 찾음: %s"), *Object->GetName());
                bSceneFound = true;
                
                // SceneManager를 통해 액터 이름 변경 처리
                AActor* SceneActor = SceneManager.FindDatasmithSceneActor();
                if (SceneActor)
                {
                    // 자식 액터 찾기
                    AActor* TargetActor = SceneManager.GetFirstChildActor();
                    if (TargetActor)
                    {
                        // 액터 이름 변경
                        FString SafeActorName = PartNo;
                        SafeActorName.ReplaceInline(TEXT(" "), TEXT("_"));
                        SafeActorName.ReplaceInline(TEXT("-"), TEXT("_"));
                        
                        TargetActor->Rename(*SafeActorName);
                        TargetActor->SetActorLabel(*PartNo);
                        
                        // StaticMesh만 남기고 다른 자식 액터 제거
                        CleanupNonStaticMeshActors(TargetActor);
                        
                        // 액터 선택
                        GEditor->SelectNone(true, true, false);
                        GEditor->SelectActor(TargetActor, true, true, true);
                        
                        UE_LOG(LogTemp, Display, TEXT("액터 이름 변경 및 선택 완료: %s (원래 이름: %s)"), 
                               *SafeActorName, *PartNo);
                               
                        // DatasmithSceneActor 제거
                        UE_LOG(LogTemp, Display, TEXT("DatasmithSceneActor 제거: %s"), *SceneActor->GetName());
                        SceneActor->Destroy();
                        
                        // 임포트 성공 메시지 표시
                        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(
                            FString::Printf(TEXT("3DXML 파일 임포트 완료: %s\n액터 이름이 %s로 변경되었습니다."),
                                            *MatchingFileName, *PartNo)));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("DatasmithSceneActor에 자식 액터가 없습니다."));
                        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("DatasmithSceneActor에 자식 액터가 없습니다.")));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("DatasmithSceneActor를 찾을 수 없습니다."));
                    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("DatasmithSceneActor를 찾을 수 없습니다.")));
                }
                
                break;
            }
        }
    }
    
    if (!bSceneFound)
    {
        UE_LOG(LogTemp, Warning, TEXT("임포트된 DatasmithScene 객체를 찾을 수 없습니다."));
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("임포트된 DatasmithScene 객체를 찾을 수 없습니다.")));
    }
#else
    // 에디터가 아닌 환경에서는 임포트 불가
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("3DXML 파일 임포트는 에디터 모드에서만 가능합니다.")));
#endif
}

// StaticMesh 액터만 유지하고 나머지 자식 액터 제거하는 함수
void SLevelBasedTreeView::CleanupNonStaticMeshActors(AActor* RootActor)
{
    if (!RootActor)
        return;
        
    // 모든 StaticMesh 액터 찾기
    TArray<AStaticMeshActor*> StaticMeshActors;
    FindAllStaticMeshActors(RootActor, StaticMeshActors);
    
    UE_LOG(LogTemp, Display, TEXT("StaticMesh 액터 발견: %d개"), StaticMeshActors.Num());
    
    // 모든 StaticMesh 액터를 루트 액터에 직접 연결
    for (AStaticMeshActor* MeshActor : StaticMeshActors)
    {
        // 이미 직접 자식이면 건너뛰기
        AActor* CurrentParent = MeshActor->GetAttachParentActor();
        if (CurrentParent == RootActor)
            continue;
            
        // 월드 변환 저장
        FTransform OriginalTransform = MeshActor->GetActorTransform();
        
        // 현재 부모에서 분리
        MeshActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        
        // 루트 액터에 직접 연결
        MeshActor->AttachToActor(RootActor, FAttachmentTransformRules::KeepWorldTransform);
        
        UE_LOG(LogTemp, Verbose, TEXT("StaticMesh 액터 '%s'를 루트에 직접 연결"), *MeshActor->GetName());
    }
    
    // StaticMesh가 아닌 자식 액터 제거
    int32 RemovedCount = 0;
    RemoveNonStaticMeshChildren(RootActor, RemovedCount);
    
    UE_LOG(LogTemp, Display, TEXT("StaticMesh가 아닌 액터 제거 완료: %d개 제거됨"), RemovedCount);
}

// 액터의 모든 하위 구조에서 StaticMeshActor를 찾는 함수
void SLevelBasedTreeView::FindAllStaticMeshActors(AActor* RootActor, TArray<AStaticMeshActor*>& OutStaticMeshActors)
{
    if (!RootActor)
        return;
    
    // 자식 액터 목록 가져오기
    TArray<AActor*> ChildActors;
    RootActor->GetAttachedActors(ChildActors);
    
    // 각 자식 액터에 대해 처리
    for (AActor* ChildActor : ChildActors)
    {
        if (ChildActor)
        {
            // StaticMeshActor인 경우 결과 배열에 추가
            if (ChildActor->IsA(AStaticMeshActor::StaticClass()))
            {
                OutStaticMeshActors.Add(Cast<AStaticMeshActor>(ChildActor));
            }
            
            // 하위 액터들을 재귀적으로 검색
            FindAllStaticMeshActors(ChildActor, OutStaticMeshActors);
        }
    }
}

// StaticMeshActor를 제외한 자식 액터들을 재귀적으로 제거하는 함수
void SLevelBasedTreeView::RemoveNonStaticMeshChildren(AActor* Actor, int32& OutRemovedCount)
{
    if (!Actor)
        return;
    
    // 자식 액터 목록 가져오기
    TArray<AActor*> ChildActors;
    Actor->GetAttachedActors(ChildActors);
    
    // 각 자식 액터에 대해 처리
    for (AActor* ChildActor : ChildActors)
    {
        if (ChildActor)
        {
            // StaticMeshActor인 경우 스킵하고 보존
            if (ChildActor->IsA(AStaticMeshActor::StaticClass()))
            {
                UE_LOG(LogTemp, Verbose, TEXT("StaticMeshActor 유지: %s"), *ChildActor->GetName());
            }
            else
            {
                // 일반 Actor인 경우, 그 자식들을 먼저 재귀적으로 처리
                RemoveNonStaticMeshChildren(ChildActor, OutRemovedCount);
                
                // 그 다음 현재 Actor 제거
                UE_LOG(LogTemp, Verbose, TEXT("액터 제거: %s"), *ChildActor->GetName());
                ChildActor->Destroy();
                OutRemovedCount++;
            }
        }
    }
}

void SLevelBasedTreeView::RemoveChildActorsExceptStaticMesh()
{
#if WITH_EDITOR
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("에디터 월드를 찾을 수 없습니다."));
        return;
    }
    
    // 에디터에서 현재 선택된 액터들 가져오기
    TArray<AActor*> SelectedActors;
    GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);
    
    if (SelectedActors.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("선택된 액터가 없습니다. 먼저 액터를 선택해주세요.")));
        return;
    }
    
    // 모든 선택된 액터의 총 자식 수와 일반 Actor 수 계산
    int32 TotalChildCount = 0;
    int32 NonStaticMeshCount = 0;
    int32 StaticMeshCount = 0;
    
    for (AActor* Actor : SelectedActors)
    {
        TArray<AActor*> ChildActors;
        Actor->GetAttachedActors(ChildActors);
        TotalChildCount += ChildActors.Num();
        
        // StaticMeshActor가 아닌 액터 수 계산
        for (AActor* ChildActor : ChildActors)
        {
            if (ChildActor->IsA(AStaticMeshActor::StaticClass()))
            {
                StaticMeshCount++;
            }
            else
            {
                NonStaticMeshCount++;
            }
        }
    }
    
    if (TotalChildCount == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("선택한 액터들에 자식 액터가 없습니다.")));
        return;
    }
    
    if (NonStaticMeshCount == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("선택한 액터들에 일반 Actor 자식이 없고 StaticMeshActor만 있습니다.")));
        return;
    }
    
    // 사용자 확인 요청
    FString ConfirmMessage;
    if (SelectedActors.Num() == 1)
    {
        ConfirmMessage = FString::Printf(
            TEXT("선택한 액터(%s)의 자식 액터 중 StaticMeshActor를 제외한 %d개의 액터를 삭제하고,\n하위 모든 StaticMeshActor를 이 액터의 직접 자식으로 연결한 후\n액터 이름을 노드 이름으로 변경하시겠습니까?\n(총 자식 액터: %d개)"), 
            *SelectedActors[0]->GetName(), NonStaticMeshCount, TotalChildCount);
    }
    else
    {
        ConfirmMessage = FString::Printf(
            TEXT("선택한 %d개 액터의 자식 액터 중 StaticMeshActor를 제외한 %d개의 액터를 삭제하고,\n하위 모든 StaticMeshActor를 각 액터의 직접 자식으로 연결한 후\n액터 이름을 노드 이름으로 변경하시겠습니까?\n(총 자식 액터: %d개)"), 
            SelectedActors.Num(), NonStaticMeshCount, TotalChildCount);
    }
    
    EAppReturnType::Type ReturnType = FMessageDialog::Open(
        EAppMsgType::YesNo, 
        FText::FromString(ConfirmMessage)
    );
    
    if (ReturnType == EAppReturnType::Yes)
    {
        // 삭제된 액터 개수와 재연결된 StaticMeshActor 개수
        int32 RemovedCount = 0;
        int32 ReattachedCount = 0;
        
        // 각 선택된 액터에 대해 처리
        for (AActor* Actor : SelectedActors)
        {
            // 현재 선택된 트리뷰 노드의 이름 가져오기
            TArray<TSharedPtr<FPartTreeItem>> SelectedTreeItems = TreeView->GetSelectedItems();
            FString NewActorName;
            
            if (SelectedTreeItems.Num() > 0)
            {
                // 트리뷰에서 선택된 항목이 있으면 그 파트 번호를 사용
                NewActorName = SelectedTreeItems[0]->PartNo;
            }
            else
            {
                // 트리뷰에서 선택된 항목이 없으면 원래 액터 이름 사용
                NewActorName = Actor->GetName();
            }
            
            // 먼저 모든 StaticMeshActor를 찾아서 저장
            TArray<AStaticMeshActor*> StaticMeshActorsToReattach;
            FindAllStaticMeshActors(Actor, StaticMeshActorsToReattach);
            
            // StaticMeshActor를 선택된 액터에 직접 연결
            for (AStaticMeshActor* MeshActor : StaticMeshActorsToReattach)
            {
                // 이미 직접 자식이면 건너뛰기
                AActor* CurrentParent = MeshActor->GetAttachParentActor();
                if (CurrentParent == Actor)
                    continue;
                
                // 월드 변환 저장
                FTransform OriginalTransform = MeshActor->GetActorTransform();
                
                // 현재 부모에서 분리
                MeshActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
                
                // 선택된 액터에 직접 연결
                MeshActor->AttachToActor(Actor, FAttachmentTransformRules::KeepWorldTransform);
                
                UE_LOG(LogTemp, Display, TEXT("StaticMeshActor '%s'를 '%s'에 직접 연결했습니다."), 
                       *MeshActor->GetName(), *Actor->GetName());
                
                ReattachedCount++;
            }
            
            // 일반 액터 제거 (StaticMesh 제외)
            RemoveNonStaticMeshChildren(Actor, RemovedCount);
            
            // 액터 이름 변경
        	// 액터 이름 변경 (Name과 Label 모두 변경)
        	FString SafeActorName = NewActorName;
        	// 특수 문자나 공백 제거 또는 대체
        	SafeActorName.ReplaceInline(TEXT(" "), TEXT("_"));
        	SafeActorName.ReplaceInline(TEXT("-"), TEXT("_"));

        	// Name 변경
        	Actor->Rename(*SafeActorName);

        	// Label 변경 (UPROPERTY로 직접 접근할 수 없으므로 SetActorLabel 사용)
        	Actor->SetActorLabel(*NewActorName); // Label은 원래 이름 그대로 표시하기 위해 SafeActorName 대신 NewActorName 사용

        	UE_LOG(LogTemp, Display, TEXT("액터 이름 변경: %s -> %s (Label: %s)"), 
				   *Actor->GetName(), *SafeActorName, *NewActorName);
        }
        
        FString ResultMessage = FString::Printf(
            TEXT("총 %d개의 일반 Actor가 제거되었고, %d개의 StaticMeshActor가 선택된 액터의 직접 자식으로 연결되었습니다.\n액터 이름이 노드 이름으로 변경되었습니다."), 
            RemovedCount, ReattachedCount);
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ResultMessage));
        
        // 모든 액터 선택 해제
        GEditor->SelectNone(true, true, false);
    }
#else
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("자식 액터 제거는 에디터 모드에서만 가능합니다.")));
#endif
}
