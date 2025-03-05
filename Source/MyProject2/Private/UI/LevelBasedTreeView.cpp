// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject2/Public/UI/LevelBasedTreeView.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Windows/WindowsPlatformApplicationMisc.h"

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
    
    // 이미지 브러시 초기화 - 완전한 설정으로 생성
    FSlateBrush* InitialBrush = new FSlateBrush();
    InitialBrush->DrawAs = ESlateBrushDrawType::NoDrawType;  // 초기 상태에서는 그리지 않음
    InitialBrush->Tiling = ESlateBrushTileType::NoTile;
    InitialBrush->Mirroring = ESlateBrushMirrorType::NoMirror;
    InitialBrush->ImageSize = FVector2D(400, 300);  // 초기 크기 설정
    
    CurrentImageBrush = MakeShareable(InitialBrush);
    
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
            .OnMouseButtonDoubleClick(this, &SLevelBasedTreeView::OnMouseButtonDoubleClick)
            .HeaderRow
            (
                SNew(SHeaderRow)
                + SHeaderRow::Column("PartNo")
                .DefaultLabel(FText::FromString("Part No"))
                .FillWidth(1.0f)
            )
    ];
    
    // 엑셀 파일 로드 및 트리뷰 구성
    if (!InArgs._ExcelFilePath.IsEmpty())
    {
        BuildTreeView(InArgs._ExcelFilePath);
    }
}

// 빈 브러시 설정을 위한 헬퍼 함수
void SLevelBasedTreeView::SetupEmptyBrush(FSlateBrush* Brush)
{
    Brush->SetResourceObject(nullptr);
    Brush->ImageSize = FVector2D(400, 300);
    Brush->DrawAs = ESlateBrushDrawType::NoDrawType;
    
    // 현재 브러시를 빈 브러시로 교체
    CurrentImageBrush = MakeShareable(Brush);
}

// 액터 선택 함수 추가
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
                        
                        // 선택된 액터 정보 로그 출력
                        UE_LOG(LogTemp, Display, TEXT("액터 선택됨: %s (PartNo: %s)"), *ActorName, *PartNo);
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

void SLevelBasedTreeView::OnMouseButtonDoubleClick(TSharedPtr<FPartTreeItem> Item)
{
    if (Item.IsValid())
    {
        // 더블클릭 시 SelectActorByPartNo 함수 호출
        SelectActorByPartNo(Item->PartNo);
        
        // 로그에 메시지 출력
        UE_LOG(LogTemp, Display, TEXT("더블클릭: '%s' 액터 선택됨"), *Item->PartNo);
    }
}

// CacheImageExistence 함수 수정 - 실제 에셋 경로를 저장
void SLevelBasedTreeView::CacheImageExistence()
{
    UE_LOG(LogTemp, Display, TEXT("CacheImageExistence 시작"));
    
    // Set 및 맵 초기화
    PartsWithImageSet.Empty();
    PartNoToImagePathMap.Empty(); // 파트 번호별 실제 이미지 경로를 저장할 맵 추가
    
    try
    {
        // 이미지 디렉토리 경로 (Game 폴더 상대 경로)
        const FString ImageDir = TEXT("/Game/00_image");
        UE_LOG(LogTemp, Display, TEXT("이미지 디렉토리 경로: %s"), *ImageDir);
        
        // 에셋 레지스트리 활용
        TArray<FAssetData> AssetList;
        FARFilter Filter;
        
        // ClassNames 대신 ClassPaths 사용
        Filter.ClassPaths.Add(UTexture2D::StaticClass()->GetClassPathName());
        Filter.PackagePaths.Add(*ImageDir);
        
        // 에셋 레지스트리에서 모든 텍스처 에셋 찾기
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        AssetRegistryModule.Get().GetAssets(Filter, AssetList);
        UE_LOG(LogTemp, Display, TEXT("GetAssets 호출 완료. 찾은 에셋 수: %d"), AssetList.Num());
        
        // 에셋 처리
        for (const FAssetData& Asset : AssetList)
        {
            FString AssetName = Asset.AssetName.ToString();
            FString AssetPath = Asset.ObjectPath.ToString();
            
            // 언더바로 문자열 분리
            TArray<FString> Parts;
            AssetName.ParseIntoArray(Parts, TEXT("_"));
            
            // 언더바로 구분된 부분이 4개 이상인지 확인
            if (Parts.Num() >= 4)
            {
                // 3번째 인덱스가 파트 번호
                FString PartNo = Parts[3];
                
                // PartsWithImageSet에 파트 번호 추가
                if(PartNoToItemMap.Contains(PartNo))
                {
                    PartsWithImageSet.Add(PartNo);
                    // 실제 에셋 경로 저장
                    PartNoToImagePathMap.Add(PartNo, AssetPath);
                    UE_LOG(LogTemp, Display, TEXT("파트 번호 '%s'에 대한 이미지 경로 저장: %s"), *PartNo, *AssetPath);
                }
            }
        }
    }
    catch (const std::exception&)
    {
        UE_LOG(LogTemp, Error, TEXT("에셋 레지스트리 처리 중 std::exception 에러"));
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("에셋 레지스트리 처리 중 알 수 없는 에러 발생"));
    }
    
    // 결과 로그 출력
    UE_LOG(LogTemp, Display, TEXT("이미지 있는 노드 갯수: %d / 전체 노드 갯수: %d"), 
           PartsWithImageSet.Num(), PartNoToItemMap.Num());
    
    UE_LOG(LogTemp, Display, TEXT("CacheImageExistence 완료"));
}

// UpdateSelectedItemImage 함수 수정 - 저장된 실제 이미지 경로 사용
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
    
    // 새 브러시 생성
    FSlateBrush* NewBrush = new FSlateBrush();
    NewBrush->DrawAs = ESlateBrushDrawType::Image;
    NewBrush->Tiling = ESlateBrushTileType::NoTile;
    NewBrush->Mirroring = ESlateBrushMirrorType::NoMirror;
    
    // Set을 확인하여 이미지가 있는지 미리 확인
    if (PartsWithImageSet.Contains(PartNoStr))
    {
        // 저장된 실제 이미지 경로 사용
        FString* AssetPathPtr = PartNoToImagePathMap.Find(PartNoStr);
        FString AssetPath;
        
        if (AssetPathPtr)
        {
            AssetPath = *AssetPathPtr;
            UE_LOG(LogTemp, Display, TEXT("저장된 이미지 경로 사용: %s"), *AssetPath);
        }
        else
        {
            // 경로가 저장되지 않은 경우 기존 방식으로 경로 구성
            AssetPath = FString::Printf(TEXT("/Game/00_image/aaa_bbb_ccc_%s"), *PartNoStr);
            UE_LOG(LogTemp, Warning, TEXT("저장된 이미지 경로가 없어 추정 경로 사용: %s"), *AssetPath);
        }
        
        // 에셋 로드 시도
        UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath);
        
        if (Texture)
        {
            // 새 브러시에 텍스처 설정
            NewBrush->SetResourceObject(Texture);
            NewBrush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
            
            // 현재 브러시를 새 브러시로 교체
            CurrentImageBrush = MakeShareable(NewBrush);
            
            UE_LOG(LogTemp, Display, TEXT("이미지 로드 성공: %s"), *AssetPath);
        }
        else
        {
            // 이미지가 로드되지 않음 - 빈 브러시 설정
            SetupEmptyBrush(NewBrush);
            UE_LOG(LogTemp, Error, TEXT("이미지 로드 실패: %s"), *AssetPath);
            
            // 파일 존재 여부 확인 (디버깅 목적)
            FString ContentDir = FPaths::ProjectContentDir();
            FString RelativePath = AssetPath.Replace(TEXT("/Game/"), TEXT(""));
            FString AbsolutePath = FPaths::Combine(ContentDir, RelativePath) + TEXT(".uasset");
            
            bool bFileExists = FPlatformFileManager::Get().GetPlatformFile().FileExists(*AbsolutePath);
            UE_LOG(LogTemp, Error, TEXT("파일 존재 여부: %s, 경로: %s"), 
                bFileExists ? TEXT("있음") : TEXT("없음"), *AbsolutePath);
        }
    }
    else
    {
        // 이미지가 없음 - 빈 브러시 설정
        SetupEmptyBrush(NewBrush);
        UE_LOG(LogTemp, Verbose, TEXT("파트 '%s'의 이미지가 없습니다"), *PartNoStr);
    }
    
    // 이미지 위젯에 새 브러시 설정
    ItemImageWidget->SetImage(CurrentImageBrush.Get());
}

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
                    return !bFilteringImageNodes && PartsWithImageSet.Num() > 0; 
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
                    
                        if (Items.Num() == 1)
                        {
                            UE_LOG(LogTemp, Display, TEXT("노드 이름 '%s'이(가) 클립보드에 복사되었습니다"), *Items[0]->PartNo);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Display, TEXT("%d개의 노드 이름이 클립보드에 복사되었습니다: %s"), Items.Num(), *CombinedNames);
                        }
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
                        FString DetailsInfo = FString::Printf(
                            TEXT("Part Details - PartNo: %s, Level: %d, Type: %s, Nomenclature: %s"),
                            *Item->PartNo, Item->Level, *Item->Type, *Item->Nomenclature
                        );
                        UE_LOG(LogTemp, Display, TEXT("%s"), *DetailsInfo);
                        
                        // 이미 메타데이터 위젯에 정보가 표시되므로 추가 작업은 필요 없음
                    }
                }),
                FCanExecuteAction::CreateLambda([SelectedItem]() { 
                    // 선택된 항목이 있을 때만 활성화
                    return SelectedItem.IsValid(); 
                })
            )
        );
        
        // 이미지 보기 (이미지가 있는 항목인 경우에만 활성화)
        bool bHasImage = SelectedItem.IsValid() && PartsWithImageSet.Contains(SelectedItem->PartNo);
        MenuBuilder.AddMenuEntry(
            FText::FromString(TEXT("이미지 보기")),
            FText::FromString(TEXT("선택한 항목의 이미지를 봅니다")),
            FSlateIcon(),
            FUIAction(
                FExecuteAction::CreateLambda([this]() {
                    // 선택된 항목의 이미지 업데이트 (이미지가 있는 경우)
                    UpdateSelectedItemImage();
                    UE_LOG(LogTemp, Display, TEXT("이미지 업데이트 완료"));
                }),
                FCanExecuteAction::CreateLambda([bHasImage]() { 
                    // 이미지가 있는 경우에만 활성화
                    return bHasImage; 
                })
            )
        );
        
        // 선택 노드 펼치기 (선택된 항목이 있을 때만 활성화)
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
                        UE_LOG(LogTemp, Display, TEXT("선택한 항목의 하위 항목이 모두 펼쳐졌습니다"));
                    }
                }),
                FCanExecuteAction::CreateLambda([SelectedItem]() { 
                    // 선택된 항목이 있고 자식이 있을 때만 활성화
                    return SelectedItem.IsValid() && SelectedItem->Children.Num() > 0; 
                })
            )
        );
        
        // 선택 노드 접기 (선택된 항목이 있을 때만 활성화)
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
                        UE_LOG(LogTemp, Display, TEXT("선택한 항목의 하위 항목이 모두 접혔습니다"));
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

bool SLevelBasedTreeView::ReadCSVFile(const FString& FilePath, TArray<TArray<FString>>& OutRows)
{
    // 파일을 줄 단위로 읽기
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
        return false;
    }
    
    for (const FString& Line : Lines)
    {
        if (Line.IsEmpty())
            continue;
            
        TArray<FString> Cells;
        
        // CSV 파싱 (쉼표로 구분된 셀)
        bool bInQuotes = false;
        FString CurrentCell;
        
        for (int32 i = 0; i < Line.Len(); i++)
        {
            TCHAR CurrentChar = Line[i];
            
            if (CurrentChar == TEXT('"'))
            {
                bInQuotes = !bInQuotes;
            }
            else if (CurrentChar == TEXT(',') && !bInQuotes)
            {
                Cells.Add(CurrentCell);
                CurrentCell.Empty();
            }
            else
            {
                CurrentCell.AppendChar(CurrentChar);
            }
        }
        
        // 마지막 셀 추가
        Cells.Add(CurrentCell);
        
        OutRows.Add(Cells);
    }
    
    return OutRows.Num() > 0;
}

bool SLevelBasedTreeView::BuildTreeView(const FString& FilePath)
{
    // 데이터 초기화
    AllRootItems.Empty();
    PartNoToItemMap.Empty();
    LevelToItemsMap.Empty();
    PartsWithImageSet.Empty();
    MaxLevel = 0;
    
    // CSV 파일 읽기
    TArray<TArray<FString>> ExcelData;
    if (!ReadCSVFile(FilePath, ExcelData))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read CSV file: %s"), *FilePath);
        return false;
    }
    
    if (ExcelData.Num() < 2) // 헤더 + 최소 1개 이상의 데이터 행 필요
    {
        UE_LOG(LogTemp, Error, TEXT("Not enough rows in data"));
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
    
    // 1단계: 모든 항목 생성 및 레벨별 그룹화
    CreateAndGroupItems(ExcelData, PartNoColIdx, NextPartColIdx, LevelColIdx);
    
    // 2단계: 트리 구조 구축
    BuildTreeStructure();
    
    // 이미지 존재 여부 캐싱 
    CacheImageExistence();
    
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

    // 노드 갯수 로그 출력 추가
    int32 TotalNodeCount = 0;
    for (const auto& LevelPair : LevelToItemsMap)
    {
        TotalNodeCount += LevelPair.Value.Num();
    }
    
    UE_LOG(LogTemp, Display, TEXT("트리 노드 총 갯수: %d"), TotalNodeCount);
    UE_LOG(LogTemp, Display, TEXT("루트 노드 갯수: %d"), AllRootItems.Num());
    
    // 각 레벨별 노드 갯수도 출력
    for (int32 Level = 0; Level <= MaxLevel; ++Level)
    {
        const TArray<TSharedPtr<FPartTreeItem>>* LevelItems = LevelToItemsMap.Find(Level);
        int32 LevelNodeCount = LevelItems ? LevelItems->Num() : 0;
        UE_LOG(LogTemp, Display, TEXT("레벨 %d 노드 갯수: %d"), Level, LevelNodeCount);
    }
    
    return AllRootItems.Num() > 0;
}

TSharedRef<SWidget> SLevelBasedTreeView::GetMetadataWidget()
{
    // 메타데이터 표시를 위한 텍스트 블록 생성
    TSharedRef<STextBlock> MetadataText = SNew(STextBlock)
        .Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SLevelBasedTreeView::GetSelectedItemMetadata)));
    
    return MetadataText;
}

// 이미지 위젯 반환 함수 구현
TSharedRef<SWidget> SLevelBasedTreeView::GetImageWidget()
{
    // 이미지 박스 크기 설정 (고정된 크기)
    const float ImageWidth = 300.0f;
    const float ImageHeight = 200.0f;

    // 이미지 위젯 생성 (SImage 대신 SBorder 사용)
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
    
    // 로컬 도우미 함수 정의
    auto SafeStr = [](const FString& Str) -> FString {
        return Str.IsEmpty() ? TEXT("N/A") : Str;
    };
    
    // 선택된 항목의 메타데이터 구성
    FString MetadataText = FString::Printf(
        TEXT("S/N: %s\n")
        TEXT("Level: %d\n")
        TEXT("Type: %s\n")
        TEXT("Part No: %s\n")
        TEXT("Part Rev: %s\n")
        TEXT("Part Status: %s\n")
        TEXT("Latest: %s\n")
        TEXT("Nomenclature: %s\n")
        TEXT("Instance ID 총수량(ALL DB): %s\n")
        TEXT("Qty: %s\n")
        TEXT("NextPart: %s"),
        *SafeStr(SelectedItem->SN),
        SelectedItem->Level,
        *SafeStr(SelectedItem->Type),
        *SafeStr(SelectedItem->PartNo),
        *SafeStr(SelectedItem->PartRev),
        *SafeStr(SelectedItem->PartStatus),
        *SafeStr(SelectedItem->Latest),
        *SafeStr(SelectedItem->Nomenclature),
        *SafeStr(SelectedItem->InstanceIDTotalAllDB),
        *SafeStr(SelectedItem->Qty),
        *SafeStr(SelectedItem->NextPart)
    );
    
    return FText::FromString(MetadataText);
}

// 안전한 문자열 반환 함수
FString SLevelBasedTreeView::GetSafeString(const FString& InStr) const
{
    return InStr.IsEmpty() ? TEXT("N/A") : InStr;
}

void SLevelBasedTreeView::OnSelectionChanged(TSharedPtr<FPartTreeItem> Item, ESelectInfo::Type SelectInfo)
{
    // 선택 변경 시 이미지 업데이트
    if (Item.IsValid())
    {
        UpdateSelectedItemImage();

    }
}

// ToggleImageFiltering 함수 구현
void SLevelBasedTreeView::ToggleImageFiltering(bool bEnable)
{
    UE_LOG(LogTemp, Display, TEXT("SLevelBasedTreeView::ToggleImageFiltering - 필터링 %s로 변경 시도"), 
        bEnable ? TEXT("활성화") : TEXT("비활성화"));
    
    // 이미 같은 상태면 아무것도 하지 않음
    if (bFilteringImageNodes == bEnable)
    {
        UE_LOG(LogTemp, Display, TEXT("SLevelBasedTreeView::ToggleImageFiltering - 이미 같은 상태, 변경 무시"));
        return;
    }
        
    bFilteringImageNodes = bEnable;
    
    UE_LOG(LogTemp, Display, TEXT("SLevelBasedTreeView::ToggleImageFiltering - PartsWithImageSet 크기: %d"), 
        PartsWithImageSet.Num());
    
    // 로깅을 위해 이미지가 있는 파트 번호 출력 (최대 5개까지만)
    int32 Count = 0;
    for (const FString& PartNo : PartsWithImageSet)
    {
        if (Count < 5)
        {
            UE_LOG(LogTemp, Display, TEXT("SLevelBasedTreeView::ToggleImageFiltering - 이미지 있는 파트: %s"), *PartNo);
        }
        Count++;
        
        if (Count == 5 && PartsWithImageSet.Num() > 5)
        {
            UE_LOG(LogTemp, Display, TEXT("SLevelBasedTreeView::ToggleImageFiltering - 외 %d개 더..."), 
                PartsWithImageSet.Num() - 5);
        }
    }
    
    // OnGetChildren 함수에서 필터링하도록 트리뷰 갱신만 요청
    if (TreeView.IsValid())
    {
        UE_LOG(LogTemp, Display, TEXT("SLevelBasedTreeView::ToggleImageFiltering - 트리뷰 갱신 요청"));
        TreeView->RequestTreeRefresh();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SLevelBasedTreeView::ToggleImageFiltering - 트리뷰가 유효하지 않음"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("SLevelBasedTreeView::ToggleImageFiltering - 완료, 현재 필터링 상태: %s"), 
        bFilteringImageNodes ? TEXT("활성화됨") : TEXT("비활성화됨"));
}

// LevelBasedTreeView.cpp에 함수 구현
bool SLevelBasedTreeView::HasChildWithImage(TSharedPtr<FPartTreeItem> Item)
{
    if (!Item.IsValid())
        return false;
    
    // 직접 이미지가 있는지 확인
    if (PartsWithImageSet.Contains(Item->PartNo))
        return true;
    
    // 자식 항목들도 확인
    for (const auto& Child : Item->Children)
    {
        if (HasChildWithImage(Child))
            return true;
    }
    
    return false;
}

// 이미지가 있는 항목만 재귀적으로 필터링하는 함수
bool SLevelBasedTreeView::FilterItemsByImage(TSharedPtr<FPartTreeItem> Item)
{
    // 현재 항목에 이미지가 있는지 확인
    bool bCurrentItemHasImage = PartsWithImageSet.Contains(Item->PartNo);
    
    // 하위 항목도 확인
    bool bAnyChildHasImage = false;
    
    for (auto& ChildItem : Item->Children)
    {
        if (FilterItemsByImage(ChildItem))
        {
            bAnyChildHasImage = true;
        }
    }
    
    // 현재 항목이나 하위 항목 중 하나라도 이미지가 있으면 true 반환
    return bCurrentItemHasImage || bAnyChildHasImage;
}

void SLevelBasedTreeView::CreateAndGroupItems(const TArray<TArray<FString>>& ExcelData, int32 PartNoColIdx, int32 NextPartColIdx, int32 LevelColIdx)
{
    // 필요한 열 인덱스 찾기
    const TArray<FString>& HeaderRow = ExcelData[0];
    
    int32 SNColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("S/N");
    });
    
    int32 TypeColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Type");
    });
    
    int32 PartRevColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Part Rev");
    });
    
    int32 PartStatusColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Part Status");
    });
    
    int32 LatestColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Latest");
    });
    
    int32 NomenclatureColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Nomenclature");
    });
    
    int32 InstanceIDTotalColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Instance ID 총수량(ALL DB)");
    });
    
    int32 QtyColIdx = HeaderRow.IndexOfByPredicate([](const FString& HeaderName) {
        return HeaderName == TEXT("Qty");
    });
    
    // 인덱스를 찾지 못했을 때 기본값 설정
    SNColIdx = (SNColIdx != INDEX_NONE) ? SNColIdx : 0;
    TypeColIdx = (TypeColIdx != INDEX_NONE) ? TypeColIdx : 2;
    PartRevColIdx = (PartRevColIdx != INDEX_NONE) ? PartRevColIdx : 4;
    PartStatusColIdx = (PartStatusColIdx != INDEX_NONE) ? PartStatusColIdx : 5;
    LatestColIdx = (LatestColIdx != INDEX_NONE) ? LatestColIdx : 6;
    NomenclatureColIdx = (NomenclatureColIdx != INDEX_NONE) ? NomenclatureColIdx : 7;
    InstanceIDTotalColIdx = (InstanceIDTotalColIdx != INDEX_NONE) ? InstanceIDTotalColIdx : 11;
    QtyColIdx = (QtyColIdx != INDEX_NONE) ? QtyColIdx : 12;
    
    // 모든 행 처리
    for (int32 i = 1; i < ExcelData.Num(); ++i)
    {
        const TArray<FString>& Row = ExcelData[i];
        if (Row.Num() <= FMath::Max3(PartNoColIdx, NextPartColIdx, LevelColIdx))
            continue;
            
        FString PartNo = Row[PartNoColIdx].TrimStartAndEnd();
        FString NextPart = Row[NextPartColIdx].TrimStartAndEnd();
        
        // 레벨 파싱
        FString LevelStr = Row[LevelColIdx].TrimStartAndEnd();
        int32 Level = 0;
        
        if (!LevelStr.IsEmpty() && !LevelStr.Equals(TEXT("nan"), ESearchCase::IgnoreCase))
        {
            Level = FCString::Atoi(*LevelStr);
        }
        
        if (!PartNo.IsEmpty())
        {
            // 파트 항목 생성
            TSharedPtr<FPartTreeItem> Item = MakeShared<FPartTreeItem>(PartNo, NextPart, Level);
            
            // 추가 필드 설정
            Item->Type = (TypeColIdx < Row.Num()) ? Row[TypeColIdx].TrimStartAndEnd() : TEXT("");
            Item->SN = (SNColIdx < Row.Num()) ? Row[SNColIdx].TrimStartAndEnd() : TEXT("");
            Item->PartRev = (PartRevColIdx < Row.Num()) ? Row[PartRevColIdx].TrimStartAndEnd() : TEXT("");
            Item->PartStatus = (PartStatusColIdx < Row.Num()) ? Row[PartStatusColIdx].TrimStartAndEnd() : TEXT("");
            Item->Latest = (LatestColIdx < Row.Num()) ? Row[LatestColIdx].TrimStartAndEnd() : TEXT("");
            Item->Nomenclature = (NomenclatureColIdx < Row.Num()) ? Row[NomenclatureColIdx].TrimStartAndEnd() : TEXT("");
            Item->InstanceIDTotalAllDB = (InstanceIDTotalColIdx < Row.Num()) ? Row[InstanceIDTotalColIdx].TrimStartAndEnd() : TEXT("");
            Item->Qty = (QtyColIdx < Row.Num()) ? Row[QtyColIdx].TrimStartAndEnd() : TEXT("");
            
            // 맵에 추가
            PartNoToItemMap.Add(PartNo, Item);
            
            // 레벨별 그룹에 추가
            if (!LevelToItemsMap.Contains(Level))
            {
                LevelToItemsMap.Add(Level, TArray<TSharedPtr<FPartTreeItem>>());
            }
            LevelToItemsMap[Level].Add(Item);
            
            // 최대 레벨 업데이트
            MaxLevel = FMath::Max(MaxLevel, Level);
            
            // NextPart가 없는 항목은 루트 후보
            if (NextPart.IsEmpty() || NextPart.Equals(TEXT("nan"), ESearchCase::IgnoreCase))
            {
                if (Level == 0) // 레벨 0의 항목만 실제 루트로 추가
                {
                    AllRootItems.Add(Item);
                }
            }
        }
    }
}

void SLevelBasedTreeView::BuildTreeStructure()
{
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
                }
            }
        }
    }
    
    // 루트 항목들이 설정되지 않았으면, 레벨 0의 항목들을 루트로 설정
    if (AllRootItems.Num() == 0 && LevelToItemsMap.Contains(0))
    {
        AllRootItems = LevelToItemsMap[0];
    }
}

TSharedRef<ITableRow> SLevelBasedTreeView::OnGenerateRow(TSharedPtr<FPartTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    // 직접 PartsWithImageSet에서 확인하여 색상 결정
    bool hasImage = PartsWithImageSet.Contains(Item->PartNo);
    
    // 이미지가 있는 노드는 빨간색으로 표시
    FSlateColor TextColor = hasImage ? 
        FSlateColor(FLinearColor::Red) : FSlateColor(FLinearColor::White);
        
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
                .ColorAndOpacity(TextColor) // 색상 적용
            ]
        ];
}

void SLevelBasedTreeView::OnGetChildren(TSharedPtr<FPartTreeItem> Item, TArray<TSharedPtr<FPartTreeItem>>& OutChildren)
{
    if (bFilteringImageNodes)
    {
        // 이미지가 있는 자식 항목만 필터링
        for (const auto& Child : Item->Children)
        {
            if (PartsWithImageSet.Contains(Child->PartNo) || HasChildWithImage(Child))
            {
                OutChildren.Add(Child);
                UE_LOG(LogTemp, Verbose, TEXT("노드 %s 추가됨 (이미지 있음)"), *Child->PartNo);
            }
        }
        
        UE_LOG(LogTemp, Display, TEXT("OnGetChildren - 총 %d개 자식 중 %d개 표시 (필터링 적용)"), 
            Item->Children.Num(), OutChildren.Num());
    }
    else
    {
        // 필터링 없이 모든 자식 항목 반환
        OutChildren = Item->Children;
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

// 트리뷰 위젯 생성 헬퍼 함수
void CreateLevelBasedTreeView(TSharedPtr<SWidget>& OutTreeViewWidget, TSharedPtr<SWidget>& OutMetadataWidget, const FString& ExcelFilePath)
{
    // SLevelBasedTreeView 인스턴스 생성
    TSharedPtr<SLevelBasedTreeView> TreeView = SNew(SLevelBasedTreeView)
        .ExcelFilePath(ExcelFilePath);
    
    OutTreeViewWidget = TreeView;
    
    // 메타데이터 위젯에 이미지 위젯 추가
    OutMetadataWidget = SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(FMargin(4.0f))
        [
            SNew(SVerticalBox)
            
            // 이미지 섹션 추가
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Item Image"))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
            ]
            
            // 이미지 위젯 추가
            + SVerticalBox::Slot()
            .AutoHeight()  // 또는 적절한 높이 지정
            .Padding(2)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
               .Padding(FMargin(4.0f))
               [
                   TreeView->GetImageWidget()
               ]
           ]

           // 분리선 추가
           + SVerticalBox::Slot()
           .AutoHeight()
           .Padding(FMargin(2, 28))
           [
               SNew(SSeparator)
               .Thickness(2.0f)
               .SeparatorImage(FAppStyle::GetBrush("Menu.Separator"))
           ]
           
           // 메타데이터 섹션 헤더
           + SVerticalBox::Slot()
           .AutoHeight()
           .Padding(2)
           [
               SNew(STextBlock)
               .Text(FText::FromString("Item Details"))
               .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
           ]
           
           // 메타데이터 내용
           + SVerticalBox::Slot()
           .FillHeight(1.0f)
           .Padding(2)
           [
               SNew(SScrollBox)
               + SScrollBox::Slot()
               [
                   // 메타데이터 내용 표시를 위한 기본 텍스트
                  TreeView->GetMetadataWidget()
              ]
          ]
      ];
}