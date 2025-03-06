﻿// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PartMetadataWidget.h"
#include "UI/LevelBasedTreeView.h" // FPartTreeItem 구조체를 위해 필요
#include "SlateOptMacros.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "UI/PartMetadataWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SPartMetadataWidget::Construct(const FArguments& InArgs)
{
    // 기본 이미지 브러시 초기화
    CurrentImageBrush = CreateImageBrush(nullptr);

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

void SPartMetadataWidget::SetPartImagePathMap(const TMap<FString, FString>& InPartNoToImagePathMap)
{
    PartNoToImagePathMap = InPartNoToImagePathMap;
}

void SPartMetadataWidget::SetPartsWithImageSet(const TSet<FString>& InPartsWithImageSet)
{
    PartsWithImageSet = InPartsWithImageSet;
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
        .Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SPartMetadataWidget::GetSelectedItemMetadata)));
}

void SPartMetadataWidget::UpdateImage()
{
    if (!SelectedItem.IsValid() || !ItemImageWidget.IsValid())
    {
        return;
    }

    FString PartNoStr = SelectedItem->PartNo;
    
    // 이미지 로드 시도
    UTexture2D* Texture = LoadPartImage(PartNoStr);
    
    // 브러시 생성 및 설정
    CurrentImageBrush = CreateImageBrush(Texture);
    
    // 이미지 위젯에 새 브러시 설정
    ItemImageWidget->SetImage(CurrentImageBrush.Get());
}

FText SPartMetadataWidget::GetSelectedItemMetadata() const
{
    if (!SelectedItem.IsValid())
    {
        return FText::FromString("No item selected");
    }
    
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
        *GetSafeString(SelectedItem->SN),
        SelectedItem->Level,
        *GetSafeString(SelectedItem->Type),
        *GetSafeString(SelectedItem->PartNo),
        *GetSafeString(SelectedItem->PartRev),
        *GetSafeString(SelectedItem->PartStatus),
        *GetSafeString(SelectedItem->Latest),
        *GetSafeString(SelectedItem->Nomenclature),
        *GetSafeString(SelectedItem->InstanceIDTotalAllDB),
        *GetSafeString(SelectedItem->Qty),
        *GetSafeString(SelectedItem->NextPart)
    );
    
    return FText::FromString(MetadataText);
}

bool SPartMetadataWidget::HasImage(const FString& PartNo) const
{
    return PartsWithImageSet.Contains(PartNo);
}

UTexture2D* SPartMetadataWidget::LoadPartImage(const FString& PartNo)
{
    if (!HasImage(PartNo))
    {
        return nullptr;
    }
    
    FString* AssetPathPtr = PartNoToImagePathMap.Find(PartNo);
    FString AssetPath;
    
    if (AssetPathPtr)
    {
        AssetPath = *AssetPathPtr;
    }
    else
    {
        // 경로가 저장되지 않은 경우 기존 방식으로 경로 구성
        AssetPath = FString::Printf(TEXT("/Game/00_image/aaa_bbb_ccc_%s"), *PartNo);
    }
    
    // 에셋 로드 시도
    UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AssetPath);
    
    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("이미지 로드 실패: %s"), *AssetPath);
    }
    
    return Texture;
}

TSharedPtr<FSlateBrush> SPartMetadataWidget::CreateImageBrush(UTexture2D* Texture)
{
    FSlateBrush* NewBrush = new FSlateBrush();
    NewBrush->DrawAs = ESlateBrushDrawType::Image;
    NewBrush->Tiling = ESlateBrushTileType::NoTile;
    NewBrush->Mirroring = ESlateBrushMirrorType::NoMirror;
    
    if (Texture)
    {
        NewBrush->SetResourceObject(Texture);
        NewBrush->ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
    }
    else
    {
        NewBrush->DrawAs = ESlateBrushDrawType::NoDrawType;
        NewBrush->ImageSize = FVector2D(400, 300);
    }
    
    return MakeShareable(NewBrush);
}

FString SPartMetadataWidget::GetSafeString(const FString& InStr) const
{
    return InStr.IsEmpty() ? TEXT("N/A") : InStr;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION