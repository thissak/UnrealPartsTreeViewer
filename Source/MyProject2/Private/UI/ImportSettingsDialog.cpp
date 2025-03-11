#include "UI/ImportSettingsDialog.h"
#include "ImportSettings.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SImportSettingsDialog::Construct(const FArguments& InArgs)
{
    CurrentSettings = InArgs._CurrentSettings;
    OnSettingsConfirmedDelegate = InArgs._OnSettingsConfirmed;
    OnDialogCanceledDelegate = InArgs._OnDialogCanceled;

    // 재질 업데이트 정책 옵션 초기화
    MaterialUpdateOptions.Add(MakeShareable(new FString(TEXT("Always update"))));
    MaterialUpdateOptions.Add(MakeShareable(new FString(TEXT("Keep existing"))));
    MaterialUpdateOptions.Add(MakeShareable(new FString(TEXT("Always create new"))));

    // 체크박스 위젯 배열 초기화
    CheckboxWidgets.SetNum(3); // 4개의 체크박스 위젯을 위한 공간 확보

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(FMargin(8.0f))
        [
            SNew(SVerticalBox)

            // 제목
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("3DXML Import Settings")))
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
            ]
            
            // 제목 아래 구분선 추가
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 10)
            [
                SNew(SSeparator)
                .Thickness(1.0f)
            ]

            // 설정 항목들
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("투명재질 메쉬 제거")))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SAssignNew(CheckboxWidgets[0], SCheckBox)
                    .IsChecked(CurrentSettings.bRemoveTransparentMeshes ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SImportSettingsDialog::OnCheckboxStateChanged, FName("bRemoveTransparentMeshes"))
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("메쉬만 남기고 액터 정리")))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SAssignNew(CheckboxWidgets[1], SCheckBox)
                    .IsChecked(CurrentSettings.bCleanupNonStaticMeshActors ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SImportSettingsDialog::OnCheckboxStateChanged, FName("bCleanupNonStaticMeshActors"))
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("import 후 액터 선택")))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SAssignNew(CheckboxWidgets[2], SCheckBox)
                    .IsChecked(CurrentSettings.bSelectActorAfterImport ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SImportSettingsDialog::OnCheckboxStateChanged, FName("bSelectActorAfterImport"))
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10, 0, 5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Material update policy")))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .HAlign(HAlign_Right)
                .VAlign(VAlign_Center)
                [
                    SAssignNew(MaterialPolicyComboBox, SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&MaterialUpdateOptions)
                    .InitiallySelectedItem(MaterialUpdateOptions[CurrentSettings.MaterialUpdatePolicy])
                    .OnSelectionChanged(this, &SImportSettingsDialog::OnComboBoxSelectionChanged, FName("MaterialUpdatePolicy"))
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) {
                        return SNew(STextBlock).Text(FText::FromString(*Item.Get()));
                    })
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]() {
                            if (CurrentSettings.MaterialUpdatePolicy >= 0 && 
                                CurrentSettings.MaterialUpdatePolicy < MaterialUpdateOptions.Num())
                            {
                                return FText::FromString(*MaterialUpdateOptions[CurrentSettings.MaterialUpdatePolicy].Get());
                            }
                            return FText::FromString(TEXT("Always update"));
                        })
                    ]
                ]
            ]
            
            // 두 번째 구분선 추가
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 10)
            [
                SNew(SSeparator)
                .Thickness(1.0f)
            ]

            // 버튼 행
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 15, 0, 0)
            .HAlign(HAlign_Right)
            [
                SNew(SHorizontalBox)
                
                // 기본값으로 재설정 버튼
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0, 0, 10, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Reset to defaults")))
                    .OnClicked(this, &SImportSettingsDialog::OnResetToDefaultsClicked)
                ]
                
                // 확인 버튼 (위치 변경됨)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(10, 0, 0, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("OK")))
                    .HAlign(HAlign_Center)
                    .OnClicked(this, &SImportSettingsDialog::OnConfirmClicked)
                ]
                
                // 취소 버튼 (위치 변경됨)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(10, 0, 0, 0)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Cancel")))
                    .OnClicked(this, &SImportSettingsDialog::OnCancelClicked)
                ]
            ]
        ]
    ];
}

FReply SImportSettingsDialog::OnConfirmClicked()
{
    // 설정 확인 대리자 호출
    if (OnSettingsConfirmedDelegate.IsBound())
    {
        OnSettingsConfirmedDelegate.Execute();
    }
    return FReply::Handled();
}

FReply SImportSettingsDialog::OnCancelClicked()
{
    // 취소 대리자 호출
    if (OnDialogCanceledDelegate.IsBound())
    {
        OnDialogCanceledDelegate.Execute();
    }
    return FReply::Handled();
}

FReply SImportSettingsDialog::OnResetToDefaultsClicked()
{
    // 기본 설정으로 초기화
    CurrentSettings = FImportSettings();
    
    // 체크박스 UI 갱신
    for (int32 i = 0; i < CheckboxWidgets.Num(); ++i)
    {
        if (CheckboxWidgets[i].IsValid())
        {
            bool bIsChecked = false;
            
            // 각 체크박스에 해당하는 설정값 가져오기
            switch (i)
            {
                case 0: bIsChecked = CurrentSettings.bRemoveTransparentMeshes; break;
                case 1: bIsChecked = CurrentSettings.bCleanupNonStaticMeshActors; break;
                case 2: bIsChecked = CurrentSettings.bSelectActorAfterImport; break;
            }
            
            // 체크박스 상태 갱신
            CheckboxWidgets[i].Pin()->SetIsChecked(bIsChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
        }
    }
    
    // 콤보박스 UI 갱신
    if (MaterialPolicyComboBox.IsValid())
    {
        MaterialPolicyComboBox.Pin()->SetSelectedItem(MaterialUpdateOptions[CurrentSettings.MaterialUpdatePolicy]);
    }
    
    return FReply::Handled();
}

void SImportSettingsDialog::OnCheckboxStateChanged(ECheckBoxState NewState, FName PropertyName)
{
    bool bChecked = (NewState == ECheckBoxState::Checked);
    
    // 속성 이름에 따라 적절한 설정 값 업데이트
    if (PropertyName == "bRemoveTransparentMeshes")
    {
        CurrentSettings.bRemoveTransparentMeshes = bChecked;
    }
    else if (PropertyName == "bCleanupNonStaticMeshActors")
    {
        CurrentSettings.bCleanupNonStaticMeshActors = bChecked;
    }
    else if (PropertyName == "bSelectActorAfterImport")
    {
        CurrentSettings.bSelectActorAfterImport = bChecked;
    }
}

void SImportSettingsDialog::OnComboBoxSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo, FName PropertyName)
{
    if (PropertyName == "MaterialUpdatePolicy")
    {
        // 선택된 항목의 인덱스 찾기
        for (int32 i = 0; i < MaterialUpdateOptions.Num(); ++i)
        {
            if (MaterialUpdateOptions[i] == NewSelection)
            {
                CurrentSettings.MaterialUpdatePolicy = i;
                break;
            }
        }
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

bool ShowImportSettingsDialog(
    const TSharedPtr<SWindow>& ParentWindow,
    const FText& Title,
    const FImportSettings& CurrentSettings,
    FImportSettings& OutSettings)
{
    // 결과 플래그
    bool bResult = false;
    
    // 대화 상자 창 생성
    TSharedRef<SWindow> SettingsWindow = SNew(SWindow)
        .Title(Title)
        .SizingRule(ESizingRule::Autosized)
        .SupportsMaximize(false)
        .SupportsMinimize(false);
    
    // 대화 상자 위젯 생성
    TSharedPtr<SImportSettingsDialog> SettingsDialog;
    SettingsWindow->SetContent(
        SAssignNew(SettingsDialog, SImportSettingsDialog)
        .CurrentSettings(CurrentSettings)
        .OnSettingsConfirmed(FOnImportSettingsConfirmed::CreateLambda([&bResult, &SettingsWindow]() {
            bResult = true;
            SettingsWindow->RequestDestroyWindow();
        }))
        .OnDialogCanceled(FOnImportSettingsCanceled::CreateLambda([&SettingsWindow]() {
            SettingsWindow->RequestDestroyWindow();
        }))
    );
    
    // 모달 창으로 표시
    FSlateApplication::Get().AddModalWindow(SettingsWindow, ParentWindow);
    
    // 결과 설정 반환
    if (bResult && SettingsDialog.IsValid())
    {
        OutSettings = SettingsDialog->GetSettings();
    }
    
    return bResult;
}