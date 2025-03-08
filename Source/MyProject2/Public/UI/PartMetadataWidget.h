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

private:
    /** 선택된 파트 항목 */
    TSharedPtr<FPartTreeItem> SelectedItem;

    /** 이미지 표시 위젯 */
    TSharedPtr<SImage> ItemImageWidget;

    /** 현재 이미지 브러시 */
    TSharedPtr<FSlateBrush> CurrentImageBrush;
};