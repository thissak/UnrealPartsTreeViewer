﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject2/Public/UI/LevelBasedTreeView.h"
#include "AssetRegistry/AssetRegistryModule.h"
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
    bFilteringImageNodes = false;
	bIsSearching = false;  // 검색 상태 초기화
	SearchText = "";       // 검색어 초기화

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
        if (bFilteringImageNodes)
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
TSharedPtr<SWidget> SLevelBasedTreeView::OnContextMenuOpening()
{
    FMenuBuilder MenuBuilder(true, nullptr);
    
    // 현재 선택된 항목 가져오기
    TArray<TSharedPtr<FPartTreeItem>> SelectedItems = TreeView->GetSelectedItems();
    TSharedPtr<FPartTreeItem> SelectedItem = (SelectedItems.Num() > 0) ? SelectedItems[0] : nullptr;
    
    MenuBuilder.BeginSection("TreeItemActions", FText::FromString(TEXT("메뉴")));
    {
        // 이미지 필터 활성화
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("이미지 있는 노드만 보기")),
            FText::FromString(TEXT("이미지가 있는 노드만 표시합니다")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    ToggleImageFiltering(true);
                }),
                FCanExecuteAction::CreateLambda([this]() { 
                    // 이미 필터링 중이면 비활성화
                    return !bFilteringImageNodes && FServiceLocator::GetImageManager()->GetPartsWithImageSet().Num() > 0; 
                }),
                FIsActionChecked::CreateLambda([this]() {
                    return bFilteringImageNodes;
                })
            ),
            NAME_None,
            EUserInterfaceActionType::Check
        );
        
        // 이미지 필터 해제
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("모든 노드 보기")),
            FText::FromString(TEXT("이미지 필터를 해제하고 모든 노드를 표시합니다")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    ToggleImageFiltering(false);
                }),
                FCanExecuteAction::CreateLambda([this]() { 
                    // 필터링 중일 때만 활성화
                    return bFilteringImageNodes; 
                })
            )
        );
        
        // 노드 이름 클립보드에 복사 (선택된 항목이 있을 때만 활성화)
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("노드 이름 복사")),
            FText::FromString(TEXT("선택한 항목의 이름을 클립보드에 복사합니다")),
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
                    
                        UE_LOG(LogTemp, Display, TEXT("노드 이름 클립보드에 복사됨: %d개 항목"), Items.Num());
                    }
                }),
                FCanExecuteAction::CreateLambda([this]() { 
                    // 하나 이상의 항목이 선택되었을 때만 활성화
                    return TreeView->GetSelectedItems().Num() > 0; 
                })
            )
        );
        
        // 상세 정보 보기 (선택된 항목이 있을 때만 활성화)
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("상세 정보 보기")),
            FText::FromString(TEXT("선택한 항목의 상세 정보를 봅니다")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    // 선택된 항목의 세부 정보를 로그로 출력
                    TArray<TSharedPtr<FPartTreeItem>> Items = TreeView->GetSelectedItems();
                    if (Items.Num() > 0)
                    {
                        TSharedPtr<FPartTreeItem> Item = Items[0];
                        UE_LOG(LogTemp, Display, TEXT("항목 상세 정보 - PartNo: %s, Level: %d"), 
                            *Item->PartNo, Item->Level);
                    }
                }),
                FCanExecuteAction::CreateLambda([SelectedItem]() { 
                    // 선택된 항목이 있을 때만 활성화
                    return SelectedItem.IsValid(); 
                })
            )
        );
        
        // 이미지 보기 (이미지가 있는 항목인 경우에만 활성화)
        bool bHasImage = SelectedItem.IsValid() && FServiceLocator::GetImageManager()->HasImage(SelectedItem->PartNo);
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("이미지 보기")),
            FText::FromString(TEXT("선택한 항목의 이미지를 봅니다")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    // 선택된 항목의 이미지 업데이트 (이미지가 있는 경우)
                    UpdateSelectedItemImage();
                }),
                FCanExecuteAction::CreateLambda([bHasImage]() { 
                    // 이미지가 있는 경우에만 활성화
                    return bHasImage; 
                })
            )
        );
        
        // 선택 노드 펼치기/접기 메뉴 항목들
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("하위 항목 모두 펼치기")),
            FText::FromString(TEXT("선택한 항목의 모든 하위 항목을 펼칩니다")),
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
        
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("하위 항목 모두 접기")),
            FText::FromString(TEXT("선택한 항목의 모든 하위 항목을 접습니다")),
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
    if (bFilteringImageNodes == bEnable)
    {
        return;
    }
        
    bFilteringImageNodes = bEnable;
    
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
    else if (bFilteringImageNodes)
    {
        // 이미지 필터링 (FTreeViewUtils 사용)
        for (const auto& Child : Item->Children)
        {
            if (FServiceLocator::GetImageManager()->HasImage(Child->PartNo) || FServiceLocator::GetImageManager()->HasChildWithImage(Child))
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