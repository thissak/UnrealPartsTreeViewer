// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PartMetadataWidget.h"
#include "TreeViewUtils.h"
#include "ServiceLocator.h"
#include "UI/LevelBasedTreeView.h" // FPartTreeItem 구조체를 위해 필요
#include "UI/PartImageManager.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPartMetadataWidget::Construct(const FArguments& InArgs)
{
    // 기본 이미지 브러시 초기화
    CurrentImageBrush = FServiceLocator::GetImageManager()->CreateImageBrush(nullptr);

    // 위젯 구성
    ChildSlot
    [
        SNew(SVerticalBox)
        
        // 이미지 섹션
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Item Image"))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
        ]
        
        // 이미지 위젯
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
            .Padding(FMargin(4.0f))
            [
                GetImageWidget()
            ]
        ]
        
        // 분리선
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
                GetMetadataWidget()
            ]
        ]
    ];
}

void SPartMetadataWidget::SetSelectedItem(TSharedPtr<FPartTreeItem> InSelectedItem)
{
    SelectedItem = InSelectedItem;
    UpdateImage();
}

TSharedRef<SWidget> SPartMetadataWidget::GetImageWidget()
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

TSharedRef<SWidget> SPartMetadataWidget::GetMetadataWidget()
{
    // 메타데이터 표시를 위한 텍스트 블록 생성
    return SNew(STextBlock)
        .Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SPartMetadataWidget::GetSelectedItemMetadata)))
        .AutoWrapText(true);
}

void SPartMetadataWidget::UpdateImage()
{
    if (!SelectedItem.IsValid() || !ItemImageWidget.IsValid())
    {
        return;
    }

    FString PartNoStr = SelectedItem->PartNo;
    
    // 이미지 로드 시도 - 이미지 매니저에서 텍스처만 가져옴
    UTexture2D* Texture = FServiceLocator::GetImageManager()->LoadPartImage(PartNoStr);
    
    // 브러시 생성 및 설정 - UI 관련 처리는 여기서 수행
    CurrentImageBrush = FServiceLocator::GetImageManager()->CreateImageBrush(Texture);
    
    // 이미지 위젯에 새 브러시 설정
    ItemImageWidget->SetImage(CurrentImageBrush.Get());
}

FText SPartMetadataWidget::GetSelectedItemMetadata() const
{
    if (!SelectedItem.IsValid())
    {
        return FText::FromString("No item selected");
    }
    
    // 유틸 클래스의 함수 사용
    return FTreeViewUtils::GetFormattedMetadata(SelectedItem);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION