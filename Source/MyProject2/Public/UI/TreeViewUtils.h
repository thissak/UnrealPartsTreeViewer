// TreeViewUtils.h
// 파트 트리뷰를 위한 일반 유틸리티 함수 모음 헤더

#pragma once

#include "CoreMinimal.h"

// FPartTreeItem 구조체 전방 선언
struct FPartTreeItem;

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
     * 필요한 열 인덱스 찾기 함수
     * @param HeaderRow - CSV 파일의 헤더 행
     * @param OutPartNoColIdx - 파트 번호 열 인덱스 (출력)
     * @param OutNextPartColIdx - 상위 파트 번호 열 인덱스 (출력)
     * @param OutLevelColIdx - 레벨 열 인덱스 (출력)
     * @param OutSNColIdx - 시리얼 번호 열 인덱스 (출력)
     * @param OutTypeColIdx - 유형 열 인덱스 (출력)
     * @param OutPartRevColIdx - 파트 리비전 열 인덱스 (출력)
     * @param OutPartStatusColIdx - 파트 상태 열 인덱스 (출력)
     * @param OutLatestColIdx - 최신 여부 열 인덱스 (출력)
     * @param OutNomenclatureColIdx - 명칭 열 인덱스 (출력)
     * @param OutInstanceIDTotalColIdx - 인스턴스 ID 총수량 열 인덱스 (출력)
     * @param OutQtyColIdx - 수량 열 인덱스 (출력)
     */
    static void FindColumnIndices(
        const TArray<FString>& HeaderRow,
        int32& OutPartNoColIdx,
        int32& OutNextPartColIdx,
        int32& OutLevelColIdx,
        int32& OutSNColIdx,
        int32& OutTypeColIdx,
        int32& OutPartRevColIdx,
        int32& OutPartStatusColIdx,
        int32& OutLatestColIdx,
        int32& OutNomenclatureColIdx,
        int32& OutInstanceIDTotalColIdx,
        int32& OutQtyColIdx);
};