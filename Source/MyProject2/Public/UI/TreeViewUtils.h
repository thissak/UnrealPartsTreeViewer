// TreeViewUtils.h
// 파트 트리뷰를 위한 일반 유틸리티 함수 모음 헤더

#pragma once

#include "CoreMinimal.h"

// FPartTreeItem 구조체 전방 선언
struct FPartTreeItem;
class UTexture2D;
struct FSlateBrush;

/**
 * 이미지 매니저 클래스
 * 파트 이미지 관련 기능을 중앙화한 싱글톤 클래스입니다.
 */
class MYPROJECT2_API FPartImageManager
{
public:
    /** 싱글톤 인스턴스 가져오기 */
    static FPartImageManager& Get();

    /** 인스턴스 초기화 (모듈 시작 시 호출) */
    void Initialize();

    /** 이미지 존재 여부 캐싱 함수
     * @param PartNoToItemMap - 파트 번호별 항목 맵
     */
    void CacheImageExistence(const TMap<FString, TSharedPtr<FPartTreeItem>>& PartNoToItemMap);

    /** 이미지 있는 파트 번호 집합 가져오기 */
    const TSet<FString>& GetPartsWithImageSet() const { return PartsWithImageSet; }

    /** 파트 번호별 이미지 경로 맵 가져오기 */
    const TMap<FString, FString>& GetPartNoToImagePathMap() const { return PartNoToImagePathMap; }

    /** 이미지 존재 여부 확인 함수
     * @param PartNo - 파트 번호
     * @return 이미지가 있으면 true, 없으면 false
     */
    bool HasImage(const FString& PartNo) const;

    /** 파트 이미지 로드 함수
     * @param PartNo - 파트 번호
     * @return 로드된 텍스처, 실패 시 nullptr
     */
    UTexture2D* LoadPartImage(const FString& PartNo);

    /** 이미지 브러시 생성 함수
     * @param Texture - 텍스처 (nullptr일 경우 빈 브러시 생성)
     * @return 생성된 브러시
     */
    TSharedPtr<FSlateBrush> CreateImageBrush(UTexture2D* Texture = nullptr);

private:
    /** 생성자 (private) */
    FPartImageManager();

    /** 싱글톤 인스턴스 */
    static FPartImageManager* Instance;

    /** 이미지가 있는 파트 번호 집합 */
    TSet<FString> PartsWithImageSet;

    /** 파트 번호별 이미지 경로 맵 */
    TMap<FString, FString> PartNoToImagePathMap;

    /** 초기화 여부 */
    bool bIsInitialized;
};

/**
 * 트리뷰 유틸리티 클래스
 * CSV 파일 처리 및 일반 유틸리티 함수들을 제공합니다.
 */
class MYPROJECT2_API FTreeViewUtils
{
public:
    /**
     * CSV 파일 읽기 함수
     * @param FilePath - CSV 파일 경로
     * @param OutRows - 읽은 CSV 데이터를 저장할 배열
     * @return 파일 읽기 성공 여부
     */
    static bool ReadCSVFile(const FString& FilePath, TArray<TArray<FString>>& OutRows);
    
    /**
     * 안전한 문자열 반환 함수
     * @param InStr - 입력 문자열
     * @return 비어있으면 "N/A", 아니면 원래 문자열
     */
    static FString GetSafeString(const FString& InStr);
    
    /**
     * 항목이 검색어와 일치하는지 확인하는 함수
     * @param Item - 확인할 항목
     * @param InSearchText - 소문자로 변환된 검색어
     * @return 검색어가 포함된 항목이면 true, 아니면 false
     */
    static bool DoesItemMatchSearch(const TSharedPtr<FPartTreeItem>& Item, const FString& InSearchText);
    
    /**
     * 한 항목이 다른 항목의 자식인지 확인하는 함수
     * @param PotentialChild - 잠재적 자식 항목
     * @param PotentialParent - 잠재적 부모 항목
     * @return 자식 여부
     */
    static bool IsChildOf(const TSharedPtr<FPartTreeItem>& PotentialChild, const TSharedPtr<FPartTreeItem>& PotentialParent);
    
    /**
     * 항목의 부모 항목 찾기 함수
     * @param ChildItem - 자식 항목
     * @param PartNoToItemMap - 파트 번호별 항목 맵
     * @return 부모 항목
     */
    static TSharedPtr<FPartTreeItem> FindParentItem(const TSharedPtr<FPartTreeItem>& ChildItem, const TMap<FString, TSharedPtr<FPartTreeItem>>& PartNoToItemMap);

	/**
	 * 자식 중에 이미지가 있는 항목이 있는지 확인하는 함수
	 * @param Item - 확인할 항목
	 * @return 이미지가 있으면 true, 없으면 false
	 */
	static bool HasChildWithImage(TSharedPtr<FPartTreeItem> Item);
    
	/**
	 * 이미지 필터링 적용 시 항목 필터링 함수
	 * @param Item - 필터링할 항목
	 * @return 필터에 포함되면 true, 아니면 false
	 */
	static bool FilterItemsByImage(TSharedPtr<FPartTreeItem> Item);
};