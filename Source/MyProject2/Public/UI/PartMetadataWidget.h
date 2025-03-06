// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Images/SImage.h"
#include "Engine/Texture2D.h"

// 전방 선언
struct FPartTreeItem;

/**
 * 파트 메타데이터 위젯 클래스
 * 선택된 파트의 상세 정보와 이미지를 표시하는 UI 위젯입니다.
 */
class MYPROJECT2_API SPartMetadataWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPartMetadataWidget)
    {}
    SLATE_END_ARGS()

    /**
     * 위젯 생성 함수
     * @param InArgs - 위젯 생성 인자
     */
    void Construct(const FArguments& InArgs);

    /**
     * 선택된 파트 항목 설정
     * @param InSelectedItem - 선택된 파트 트리 항목
     */
    void SetSelectedItem(TSharedPtr<FPartTreeItem> InSelectedItem);

    /**
     * 파트 이미지 경로 맵 설정
     * @param InPartNoToImagePathMap - 파트 번호별 이미지 경로 맵
     */
    void SetPartImagePathMap(const TMap<FString, FString>& InPartNoToImagePathMap);

    /**
     * 이미지가 있는 파트 번호 집합 설정
     * @param InPartsWithImageSet - 이미지가 있는 파트 번호 집합
     */
    void SetPartsWithImageSet(const TSet<FString>& InPartsWithImageSet);

    /**
     * 이미지 위젯 반환 함수
     * @return 이미지 표시 위젯
     */
    TSharedRef<SWidget> GetImageWidget();

    /**
     * 메타데이터 위젯 반환 함수
     * @return 메타데이터 표시 위젯
     */
    TSharedRef<SWidget> GetMetadataWidget();

private:
    /**
     * 선택된 항목 이미지 업데이트
     */
    void UpdateImage();

    /**
     * 선택된 항목 메타데이터 텍스트 반환
     * @return 메타데이터 텍스트
     */
    FText GetSelectedItemMetadata() const;

    /**
     * 이미지 존재 여부 확인 함수
     * @param PartNo - 파트 번호
     * @return 이미지가 있으면 true, 없으면 false
     */
    bool HasImage(const FString& PartNo) const;

    /**
     * 파트 이미지 로드 함수
     * @param PartNo - 파트 번호
     * @return 로드된 텍스처, 실패 시 nullptr
     */
    UTexture2D* LoadPartImage(const FString& PartNo);

    /**
     * 이미지 브러시 생성 함수
     * @param Texture - 텍스처 (nullptr일 경우 빈 브러시 생성)
     * @return 생성된 브러시
     */
    TSharedPtr<FSlateBrush> CreateImageBrush(UTexture2D* Texture = nullptr);

    /**
     * 안전한 문자열 반환 함수
     * @param InStr - 입력 문자열
     * @return 비어있으면 "N/A", 아니면 원래 문자열
     */
    FString GetSafeString(const FString& InStr) const;

private:
    /** 선택된 파트 항목 */
    TSharedPtr<FPartTreeItem> SelectedItem;

    /** 이미지 표시 위젯 */
    TSharedPtr<SImage> ItemImageWidget;

    /** 현재 이미지 브러시 */
    TSharedPtr<FSlateBrush> CurrentImageBrush;

    /** 파트 번호별 이미지 경로 맵 */
    TMap<FString, FString> PartNoToImagePathMap;

    /** 이미지가 있는 파트 번호 집합 */
    TSet<FString> PartsWithImageSet;
};