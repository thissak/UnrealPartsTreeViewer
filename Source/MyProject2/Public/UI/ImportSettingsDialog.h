// Source/MyProject2/Public/UI/ImportSettingsDialog.h
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "ImportSettings.h"

// 델리게이트 타입 선언
DECLARE_DELEGATE(FOnImportSettingsConfirmed);
DECLARE_DELEGATE(FOnImportSettingsCanceled);

/** 3DXML 임포트 설정 대화 상자 위젯 클래스 */
class MYPROJECT2_API SImportSettingsDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SImportSettingsDialog)
        : _CurrentSettings(FImportSettings())
    {}
        /** 현재 설정 */
        SLATE_ARGUMENT(FImportSettings, CurrentSettings)
        
        /** 설정 확인 시 호출되는 대리자 */
        SLATE_EVENT(FOnImportSettingsConfirmed, OnSettingsConfirmed)
        
        /** 대화 상자 취소 시 호출되는 대리자 */
        SLATE_EVENT(FOnImportSettingsCanceled, OnDialogCanceled)
    SLATE_END_ARGS()

    /** 위젯 생성 함수 */
    void Construct(const FArguments& InArgs);

    /** 현재 설정 가져오기 */
    FImportSettings GetSettings() const { return CurrentSettings; }

private:
    /** 현재 설정 값 */
    FImportSettings CurrentSettings;

    /** 설정 확인 버튼 클릭 이벤트 */
    FReply OnConfirmClicked();

    /** 취소 버튼 클릭 이벤트 */
    FReply OnCancelClicked();

    /** 기본값으로 재설정 버튼 클릭 이벤트 */
    FReply OnResetToDefaultsClicked();

    /** 확인 버튼 클릭 대리자 */
    FOnImportSettingsConfirmed OnSettingsConfirmedDelegate;

    /** 취소 버튼 클릭 대리자 */
    FOnImportSettingsCanceled OnDialogCanceledDelegate;

    /** 체크박스 상태 변경 이벤트 핸들러 */
    void OnCheckboxStateChanged(ECheckBoxState NewState, FName PropertyName);

    /** 콤보박스 선택 변경 이벤트 핸들러 */
    void OnComboBoxSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo, FName PropertyName);

    /** 재질 업데이트 정책 옵션 */
    TArray<TSharedPtr<FString>> MaterialUpdateOptions;
};

/**
 * 임포트 설정 대화 상자를 표시하고 결과를 반환하는 함수
 * @param ParentWindow - 부모 창
 * @param Title - 대화 상자 제목
 * @param CurrentSettings - 현재 설정 값
 * @param OutSettings - 출력 설정 값
 * @return 사용자가 확인 버튼을 클릭했는지 여부
 */
bool MYPROJECT2_API ShowImportSettingsDialog(
    const TSharedPtr<SWindow>& ParentWindow,
    const FText& Title,
    const FImportSettings& CurrentSettings,
    FImportSettings& OutSettings);