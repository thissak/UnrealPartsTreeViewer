// TreeViewUtils.h
// 파트 트리뷰를 위한 일반 유틸리티 함수 모음 헤더

#pragma once

#include "CoreMinimal.h"

// FPartTreeItem 구조체 전방 선언
struct FPartTreeItem;

/**
 * 파일 일치 결과 구조체
 * 파일 검색 결과를 저장합니다.
 */
struct FFileMatchResult
{
    /** 일치하는 파일의 전체 경로 */
    FString FilePath;
    
    /** 일치하는 파일 이름 (경로 제외) */
    FString FileName;
    
    /** 파일이 발견되었는지 여부 */
    bool bFound;
    
    /** 오류 메시지 (발견되지 않은 경우) */
    FString ErrorMessage;
    
    /** 기본 생성자 */
    FFileMatchResult()
        : FilePath("")
        , FileName("")
        , bFound(false)
        , ErrorMessage("")
    {
    }
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
	static TSharedPtr<FPartTreeItem> FindParentItem(const TSharedPtr<FPartTreeItem>& ChildItem,
		const TMap<FString, TSharedPtr<FPartTreeItem>>& PartNoToItemMap);

	/**
	 * 항목의 메타데이터를 형식화된 텍스트로 반환하는 함수
	 * @param Item - 메타데이터를 가져올 항목
	 * @return 형식화된 메타데이터 텍스트
	 */
	static FText GetFormattedMetadata(const TSharedPtr<FPartTreeItem>& Item);

	/**
	 * 안전한 문자열 반환 함수
	 * @param InStr - 입력 문자열
	 * @return 비어있으면 "N/A", 아니면 원래 문자열
	 */
	static FString GetSafeString(const FString& InStr);

	/**
	 * 항목 생성 및 레벨별 그룹화 함수
	 * @param ExcelData - CSV 데이터
	 * @param PartNoColIdx - 파트 번호 컬럼 인덱스
	 * @param NextPartColIdx - 상위 파트 번호 컬럼 인덱스
	 * @param LevelColIdx - 레벨 컬럼 인덱스
	 * @param OutPartNoToItemMap - [출력] 파트 번호별 항목 맵
	 * @param OutLevelToItemsMap - [출력] 레벨별 항목 맵
	 * @param OutMaxLevel - [출력] 최대 레벨 깊이
	 * @param OutRootItems - [출력] 루트 항목 배열
	 * @return 생성된 유효 항목 수
	 */
	static int32 CreateAndGroupItems(
		const TArray<TArray<FString>>& ExcelData, 
		int32 PartNoColIdx, 
		int32 NextPartColIdx, 
		int32 LevelColIdx,
		TMap<FString, TSharedPtr<FPartTreeItem>>& OutPartNoToItemMap,
		TMap<int32, TArray<TSharedPtr<FPartTreeItem>>>& OutLevelToItemsMap,
		int32& OutMaxLevel,
		TArray<TSharedPtr<FPartTreeItem>>& OutRootItems);

    /**
     * 특정 디렉토리에서 PartNo와 일치하는 파일 찾기
     * @param DirectoryPath - 검색할 디렉토리 경로
     * @param FilePattern - 검색할 파일 패턴 (예: "*.3dxml")
     * @param PartNo - 일치시킬 파트 번호
     * @param PartIndexInFileName - 파일명을 언더바로 분리했을 때 파트번호가 위치한 인덱스
     * @return 파일 검색 결과
     */
    static FFileMatchResult FindMatchingFileForPartNo(
        const FString& DirectoryPath,
        const FString& FilePattern,
        const FString& PartNo,
        int32 PartIndexInFileName = 3);
	
	/**
	 * 언더바로 구분된 에셋 이름에서 파트 번호 추출
	 * @param AssetName - 에셋 이름
	 * @param PartIndex - 파트 번호가 있는 인덱스 위치 (기본값 3)
	 * @return 파트 번호, 찾지 못한 경우 빈 문자열
	 */
	static FString ExtractPartNoFromAssetName(const FString& AssetName, int32 PartIndex = 3);

	// 바운딩 박스 계산 함수 선언 추가
	/**
	 * 선택된 액터와 자식의 모든 스태틱 메시 컴포넌트의 바운딩 박스 계산
	 * @param ActorName - 선택적 액터 이름 필터 (비어있으면 모든 액터에 대해 계산)
	 * @return 성공 여부
	 */
	static bool CalculateSelectedActorMeshBounds(const FString& ActorName = TEXT(""));

	/**
	 * 선택된 액터의 피벗을 바운딩 박스 중심으로 이동
	 * @return 성공 여부
	 */
	static bool SetActorPivotToCenter();
};