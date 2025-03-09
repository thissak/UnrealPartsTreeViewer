// Source/MyProject2/Public/UI/PartTreeItem.h
#pragma once

#include "CoreMinimal.h"

/**
 * 파트 트리 아이템 데이터 구조체
 * 트리뷰에 표시되는 각 항목을 표현합니다.
 */
struct FPartTreeItem
{
	// 기본 필드
	FString PartNo;               // 파트 번호
	FString NextPart;             // 상위 파트 번호
	int32 Level;                  // 파트 레벨 (계층 구조 깊이)
	FString Type;                 // 파트 유형
    
	// 추가 필드
	FString SN;                   // 시리얼 번호
	FString PartRev;              // 파트 리비전
	FString PartStatus;           // 파트 상태
	FString Latest;               // 최신 여부
	FString Nomenclature;         // 명칭
	FString InstanceIDTotalAllDB; // 총 인스턴스 ID 수
	FString Qty;                  // 수량
    
	// 자식 항목 배열
	TArray<TSharedPtr<FPartTreeItem>> Children;
    
	/**
	 * 생성자
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