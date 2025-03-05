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

/**
 * 파트 트리 아이템 데이터 구조체
 * 트리뷰에 표시되는 각 항목을 나타냅니다.
 */
struct FPartTreeItem
{
    // 기본 필드
    FString PartNo;              ///< 파트 번호
    FString NextPart;            ///< 상위 파트 번호
    int32 Level;                 ///< 파트 레벨 (계층 구조 깊이)
    FString Type;                ///< 파트 유형
    
    // 추가 필드
    FString SN;                  ///< 시리얼 번호
    FString PartRev;             ///< 파트 리비전
    FString PartStatus;          ///< 파트 상태
    FString Latest;              ///< 최신 여부
    FString Nomenclature;        ///< 명칭
    FString InstanceIDTotalAllDB; ///< 총 인스턴스 ID 수
    FString Qty;                 ///< 수량
    
    // 자식 항목 배열
    TArray<TSharedPtr<FPartTreeItem>> Children;
    
    /**
     * 생성자
     * @param InPartNo - 파트 번호
     * @param InNextPart - 상위 파트 번호
     * @param InLevel - 파트 레벨
     * @param InType - 파트 유형 (선택적)
     */
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

/**
 * 레벨 기반 트리뷰 위젯 클래스
 * CSV 파일에서 로드한 파트 정보를 계층 구조로 표시하는 트리뷰 위젯입니다.
 */
class MYPROJECT2_API SLevelBasedTreeView : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SLevelBasedTreeView)
        : _ExcelFilePath("")
    {}
        SLATE_ARGUMENT(FString, ExcelFilePath)  ///< CSV 파일 경로
    SLATE_END_ARGS()

    //===== 초기화 및 기본 설정 =====//
    
    /**
     * 위젯 생성 함수
     * @param InArgs - 위젯 생성 인자
     */
    void Construct(const FArguments& InArgs);
    
    /**
     * CSV 파일 로드 및 트리뷰 구성 함수
     * @param FilePath - CSV 파일 경로
     * @return 로드 및 구성 성공 여부
     */
    bool BuildTreeView(const FString& FilePath);

    //===== 트리뷰 조작 기능 =====//
    
    /**
     * 이미지 필터링 적용/해제
     * @param bEnable - 필터링 활성화 여부
     */
    void ToggleImageFiltering(bool bEnable);
    
    /**
     * 선택한 항목 및 하위 항목을 재귀적으로 펼치거나 접기
     * @param Item - 펼치거나 접을 항목
     * @param bExpand - true: 펼치기, false: 접기
     */
    void ExpandItemRecursively(TSharedPtr<FPartTreeItem> Item, bool bExpand);
    
    /**
     * 특정 파트 번호와 이름이 일치하는 액터 선택
     * @param PartNo - 선택할 액터의 파트 번호
     */
    void SelectActorByPartNo(const FString& PartNo);

	//===== 검색 UI 위젯 반환 함수 =====//
	TSharedRef<SWidget> GetSearchWidget();
    
	//===== 검색 이벤트 핸들러 =====//
	void OnSearchTextChanged(const FText& InText);
	void OnSearchTextCommitted(const FText& InText, ETextCommit::Type CommitType);

    //===== 위젯 및 UI 관련 =====//
    
    /**
     * 메타데이터 위젯 반환 함수
     * @return 메타데이터 표시 위젯
     */
    TSharedRef<SWidget> GetMetadataWidget();
    
    /**
     * 이미지 위젯 반환 함수
     * @return 이미지 표시 위젯
     */
    TSharedRef<SWidget> GetImageWidget();
    
    /**
     * 선택된 항목 이미지 업데이트
     */
    void UpdateSelectedItemImage();
    
    /**
     * 선택된 항목 메타데이터 텍스트 반환
     * @return 메타데이터 텍스트
     */
    FText GetSelectedItemMetadata() const;

private:
	 //===== 검색 관련 변수 =====//
	 FString SearchText;
	 bool bIsSearching;
	 TArray<TSharedPtr<FPartTreeItem>> SearchResults;
	
    //===== 트리뷰 및 데이터 관련 변수 =====//
    
    /** 트리뷰 위젯 */
    TSharedPtr<STreeView<TSharedPtr<FPartTreeItem>>> TreeView;
    
    /** 모든 루트 항목 컬렉션 */
    TArray<TSharedPtr<FPartTreeItem>> AllRootItems;
    
    /** 파트 번호에 따른 항목 맵 */
    TMap<FString, TSharedPtr<FPartTreeItem>> PartNoToItemMap;
    
    /** 레벨별 항목 맵 */
    TMap<int32, TArray<TSharedPtr<FPartTreeItem>>> LevelToItemsMap;
    
    /** 최대 레벨 깊이 */
    int32 MaxLevel;
    
    //===== 이미지 관련 변수 =====//
    
    /** 현재 이미지 브러시 */
    TSharedPtr<FSlateBrush> CurrentImageBrush;
    
    /** 이미지 표시 위젯 */
    TSharedPtr<SImage> ItemImageWidget;
    
    /** 이미지가 있는 파트 번호 집합 */
    TSet<FString> PartsWithImageSet;
    
    /** 파트 번호별 이미지 경로 맵 */
    TMap<FString, FString> PartNoToImagePathMap;
    
    //===== 필터링 관련 변수 =====//
    
    /** 이미지 필터링 상태 */
    bool bFilteringImageNodes;
    
    /** 필터링된 루트 항목 목록 */
    TArray<TSharedPtr<FPartTreeItem>> FilteredRootItems;

    //===== 이벤트 핸들러 =====//
    
    /**
     * 트리뷰 항목 선택 변경 이벤트 핸들러
     * @param Item - 선택된 항목
     * @param SelectInfo - 선택 정보
     */
    void OnSelectionChanged(TSharedPtr<FPartTreeItem> Item, ESelectInfo::Type SelectInfo);
    
    /**
     * 트리뷰 항목 더블클릭 이벤트 핸들러
     * @param Item - 더블클릭된 항목
     */
    void OnMouseButtonDoubleClick(TSharedPtr<FPartTreeItem> Item);
    
    /**
     * 컨텍스트 메뉴 생성 함수
     * @return 생성된 컨텍스트 메뉴 위젯
     */
    TSharedPtr<SWidget> OnContextMenuOpening();

    //===== 트리뷰 델리게이트 =====//
    
    /**
     * 트리뷰 행 생성 델리게이트
     * @param Item - 표시할 항목
     * @param OwnerTable - 소유자 테이블
     * @return 생성된 테이블 행
     */
    TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FPartTreeItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
    
    /**
     * 트리뷰 자식 항목 반환 델리게이트
     * @param Item - 부모 항목
     * @param OutChildren - 자식 항목들을 저장할 배열
     */
    void OnGetChildren(TSharedPtr<FPartTreeItem> Item, TArray<TSharedPtr<FPartTreeItem>>& OutChildren);

	//===== 검색 관련 함수 =====//
	void PerformSearch(const FString& InSearchText);
	bool DoesItemMatchSearch(const TSharedPtr<FPartTreeItem>& Item, const FString& InSearchText);
	void ExpandPathToItem(const TSharedPtr<FPartTreeItem>& Item);
	TSharedPtr<FPartTreeItem> FindParentItem(const TSharedPtr<FPartTreeItem>& ChildItem);
	bool IsChildOf(const TSharedPtr<FPartTreeItem>& PotentialChild, const TSharedPtr<FPartTreeItem>& PotentialParent);

    //===== 유틸리티 함수 =====//
    
    /**
     * 이미지 유무 캐싱 함수
     * 모든 파트의 이미지 존재 여부를 미리 확인하고 캐싱합니다.
     */
    void CacheImageExistence();
    
    /**
     * 빈 브러시 설정 헬퍼 함수
     * @param Brush - 설정할 브러시
     */
    void SetupEmptyBrush(FSlateBrush* Brush);
    
    /**
     * CSV 파일 읽기 함수
     * @param FilePath - CSV 파일 경로
     * @param OutRows - 읽은 CSV 데이터를 저장할 배열
     * @return 파일 읽기 성공 여부
     */
    bool ReadCSVFile(const FString& FilePath, TArray<TArray<FString>>& OutRows);
    
    /**
     * 항목 생성 및 레벨별 그룹화 함수
     * @param ExcelData - CSV 데이터
     * @param PartNoColIdx - 파트 번호 컬럼 인덱스
     * @param NextPartColIdx - 상위 파트 번호 컬럼 인덱스
     * @param LevelColIdx - 레벨 컬럼 인덱스
     */
    void CreateAndGroupItems(const TArray<TArray<FString>>& ExcelData, int32 PartNoColIdx, int32 NextPartColIdx, int32 LevelColIdx);
    
    /**
     * 트리 구조 구축 함수
     * 항목들의 부모-자식 관계를 설정합니다.
     */
    void BuildTreeStructure();
    
    /**
     * 자식 중에 이미지가 있는 항목이 있는지 확인하는 함수
     * @param Item - 확인할 항목
     * @return 이미지가 있으면 true, 없으면 false
     */
    bool HasChildWithImage(TSharedPtr<FPartTreeItem> Item);
    
    /**
     * 이미지 필터링 적용 시 항목 필터링 함수
     * @param Item - 필터링할 항목
     * @return 필터에 포함되면 true, 아니면 false
     */
    bool FilterItemsByImage(TSharedPtr<FPartTreeItem> Item);
    
    /**
     * 안전한 문자열 반환 함수
     * @param InStr - 입력 문자열
     * @return 비어있으면 "N/A", 아니면 원래 문자열
     */
    FString GetSafeString(const FString& InStr) const;
};

/**
 * 트리뷰 위젯 생성 헬퍼 함수
 * @param OutTreeViewWidget - 출력 트리뷰 위젯
 * @param OutMetadataWidget - 출력 메타데이터 위젯
 * @param ExcelFilePath - CSV 파일 경로
 */
MYPROJECT2_API void CreateLevelBasedTreeView(TSharedPtr<SWidget>& OutTreeViewWidget,
    TSharedPtr<SWidget>& OutMetadataWidget, const FString& ExcelFilePath);