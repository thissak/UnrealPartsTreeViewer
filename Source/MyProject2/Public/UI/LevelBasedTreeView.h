// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Widgets/Images/SImage.h"
#include "Engine/Texture2D.h"
#include "AssetRegistry/AssetRegistryModule.h"

// 트리 아이템 데이터 구조체
struct FPartTreeItem
{
    // 기본 필드
    FString PartNo;
    FString NextPart;
    int32 Level;
    FString Type;
    
    // 추가 필드
    FString SN;
    FString PartRev;
    FString PartStatus;
    FString Latest;
    FString Nomenclature;
    FString InstanceIDTotalAllDB;
    FString Qty;
    
    // 자식 항목 배열
    TArray<TSharedPtr<FPartTreeItem>> Children;
    
    // 생성자
    FPartTreeItem(const FString& InPartNo, const FString& InNextPart, int32 InLevel, const FString& InType = TEXT(""))
        : PartNo(InPartNo)
        , NextPart(InNextPart)
        , Level(InLevel)
        , Type(InType)
        , SN(TEXT(""))
        , PartRev(TEXT(""))
        , PartStatus(TEXT(""))
        , Latest(TEXT(""))
        , Nomenclature(TEXT(""))
        , InstanceIDTotalAllDB(TEXT(""))
        , Qty(TEXT(""))

    {
    }
};

// 레벨 기반 트리뷰 위젯 클래스
class MYPROJECT2_API SLevelBasedTreeView : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLevelBasedTreeView)
        : _ExcelFilePath("")
    {}
        SLATE_ARGUMENT(FString, ExcelFilePath)
    SLATE_END_ARGS()

    // 위젯 생성
    void Construct(const FArguments& InArgs);
    
    // 엑셀 파일 로드 및 트리뷰 구성
    bool BuildTreeView(const FString& FilePath);

    // 위젯 반환 함수
    TSharedRef<SWidget> GetMetadataWidget();
    TSharedRef<SWidget> GetImageWidget();

    // 선택된 항목 메타데이터 텍스트 반환
    FText GetSelectedItemMetadata() const;

    // 선택 변경 이벤트 핸들러
    void OnSelectionChanged(TSharedPtr<FPartTreeItem> Item, ESelectInfo::Type SelectInfo);
    
    // 선택된 항목 이미지 업데이트
    void UpdateSelectedItemImage();

    void ExpandItemRecursively(TSharedPtr<FPartTreeItem> Item, bool bExpand);

private:
    // 트리뷰 위젯
    TSharedPtr<STreeView<TSharedPtr<FPartTreeItem>>> TreeView;
    
    // 트리 아이템 컬렉션
    TArray<TSharedPtr<FPartTreeItem>> AllRootItems;
    TMap<FString, TSharedPtr<FPartTreeItem>> PartNoToItemMap;
    TMap<int32, TArray<TSharedPtr<FPartTreeItem>>> LevelToItemsMap;
    int32 MaxLevel;
    
    // 이미지 관련 멤버
    TSharedPtr<FSlateBrush> CurrentImageBrush;
    TSharedPtr<SImage> ItemImageWidget;

    /** 컨텍스트 메뉴 생성 */
    TSharedPtr<SWidget> OnContextMenuOpening();

    // 이미지가 있는 파트 번호 집합
    TSet<FString> PartsWithImageSet;
    
    // 이미지 유무 캐싱
    void CacheImageExistence();
    
    // 빈 브러시 설정 헬퍼 함수
    void SetupEmptyBrush(FSlateBrush* Brush);
    
    // CSV 파일 읽기
    bool ReadCSVFile(const FString& FilePath, TArray<TArray<FString>>& OutRows);
    
    // 트리 구성 함수
    void CreateAndGroupItems(const TArray<TArray<FString>>& ExcelData, int32 PartNoColIdx, int32 NextPartColIdx, int32 LevelColIdx);
    void BuildTreeStructure();
    
    // 트리뷰 델리게이트
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPartTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
    void OnGetChildren(TSharedPtr<FPartTreeItem> Item, TArray<TSharedPtr<FPartTreeItem>>& OutChildren);

    // 유틸리티
    FString GetSafeString(const FString& InStr) const;
};

// 트리뷰 위젯 생성 헬퍼 함수
MYPROJECT2_API void CreateLevelBasedTreeView(TSharedPtr<SWidget>& OutTreeViewWidget
    , TSharedPtr<SWidget>& OutMetadataWidget, const FString& ExcelFilePath);