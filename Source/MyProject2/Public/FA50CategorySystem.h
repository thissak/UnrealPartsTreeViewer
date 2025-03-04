﻿// FA50MCategorySystem.h
// FA-50M 조종석 시스템 카테고리 유틸리티 및 열거형 정의

#pragma once

#include "CoreMinimal.h"

// FA-50M 시스템 주요 카테고리 열거형
UENUM(BlueprintType)
enum class EFA50MSystemCategory : uint8
{
    None = 0,
    FlightControl,        // 비행 제어 시스템
    PowerManagement,      // 전력 관리 시스템
    EngineFuel,           // 엔진 및 연료 시스템
    NavigationComm,       // 항법 및 통신 시스템
    WeaponsDefense,       // 무기 및 방어 시스템
    Lighting,             // 조명 시스템
    Emergency,            // 비상 시스템
    EnvironmentalControl, // 환경 제어 시스템
    DisplayRecording,     // 디스플레이 및 기록 시스템
    TestDiagnostics,      // 테스트 및 진단 시스템
    Avionics              // 항공전자 시스템 (FA-50M에 추가된 카테고리)
};

// 비행 제어 하위 시스템 열거형
UENUM(BlueprintType)
enum class EFlightControlSubsystem : uint8
{
    None = 0,
    FLCS,                 // 비행 제어 시스템
    ControlSurfaces,      // 제어 표면
    TrimControl,          // 트림 제어
    AutoPilot,            // 자동 조종 장치
    LandingGear,          // 착륙 장치
    SpeedBrake,           // 감속 브레이크
    AirBrake,             // 공기 브레이크
    StabilityAugmentation // 안정성 증강 시스템
};

// 항공전자 하위 시스템 열거형 (FA-50M 전용)
UENUM(BlueprintType)
enum class EAvionicsSubsystem : uint8
{
    None = 0,
    MFD,                  // 다기능 디스플레이
    HUD,                  // 헤드업 디스플레이
    HOTAS,                // 핸즈 온 스로틀 앤 스틱
    Radar,                // 레이더 시스템
    IFF,                  // 피아식별장치
    FLIR,                 // 전방 적외선 센서
    EW                    // 전자전 시스템
};

// FA-50M 파트 카테고리 구조체
struct FFA50MPartCategory
{
    EFA50MSystemCategory MainCategory;
    uint8 SubCategory;
    FString Notes;
    
    FFA50MPartCategory()
        : MainCategory(EFA50MSystemCategory::None)
        , SubCategory(0)
        , Notes(TEXT(""))
    {
    }
    
    FFA50MPartCategory(EFA50MSystemCategory InMainCategory, uint8 InSubCategory, const FString& InNotes = TEXT(""))
        : MainCategory(InMainCategory)
        , SubCategory(InSubCategory)
        , Notes(InNotes)
    {
    }
    
    bool operator==(const FFA50MPartCategory& Other) const
    {
        return MainCategory == Other.MainCategory && 
               SubCategory == Other.SubCategory && 
               Notes.Equals(Other.Notes);
    }
    
    bool operator!=(const FFA50MPartCategory& Other) const
    {
        return !(*this == Other);
    }
};

// FA-50M 카테고리 헬퍼 클래스
class FA50MCategoryHelpers
{
public:
    // 문자열에서 주 카테고리 열거형 값 반환
    static EFA50MSystemCategory GetCategoryFromString(const FString& CategoryStr)
    {
        if (CategoryStr.Equals(TEXT("FlightControl"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::FlightControl;
        else if (CategoryStr.Equals(TEXT("PowerManagement"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::PowerManagement;
        else if (CategoryStr.Equals(TEXT("EngineFuel"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::EngineFuel;
        else if (CategoryStr.Equals(TEXT("NavigationComm"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::NavigationComm;
        else if (CategoryStr.Equals(TEXT("WeaponsDefense"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::WeaponsDefense;
        else if (CategoryStr.Equals(TEXT("Lighting"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::Lighting;
        else if (CategoryStr.Equals(TEXT("Emergency"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::Emergency;
        else if (CategoryStr.Equals(TEXT("EnvironmentalControl"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::EnvironmentalControl;
        else if (CategoryStr.Equals(TEXT("DisplayRecording"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::DisplayRecording;
        else if (CategoryStr.Equals(TEXT("TestDiagnostics"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::TestDiagnostics;
        else if (CategoryStr.Equals(TEXT("Avionics"), ESearchCase::IgnoreCase))
            return EFA50MSystemCategory::Avionics;
        
        return EFA50MSystemCategory::None;
    }
    
    // 문자열에서 하위 카테고리 값 반환 (주 카테고리에 따라 다름)
    static uint8 GetSubCategoryFromString(EFA50MSystemCategory MainCategory, const FString& SubCategoryStr)
    {
        // 비행 제어 하위 카테고리
        if (MainCategory == EFA50MSystemCategory::FlightControl)
        {
            if (SubCategoryStr.Equals(TEXT("FLCS"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EFlightControlSubsystem::FLCS);
            else if (SubCategoryStr.Equals(TEXT("ControlSurfaces"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EFlightControlSubsystem::ControlSurfaces);
            else if (SubCategoryStr.Equals(TEXT("TrimControl"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EFlightControlSubsystem::TrimControl);
            else if (SubCategoryStr.Equals(TEXT("AutoPilot"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EFlightControlSubsystem::AutoPilot);
            else if (SubCategoryStr.Equals(TEXT("LandingGear"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EFlightControlSubsystem::LandingGear);
            else if (SubCategoryStr.Equals(TEXT("SpeedBrake"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EFlightControlSubsystem::SpeedBrake);
            else if (SubCategoryStr.Equals(TEXT("AirBrake"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EFlightControlSubsystem::AirBrake);
            else if (SubCategoryStr.Equals(TEXT("StabilityAugmentation"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EFlightControlSubsystem::StabilityAugmentation);
        }
        // 항공전자 하위 카테고리 (FA-50M 전용)
        else if (MainCategory == EFA50MSystemCategory::Avionics)
        {
            if (SubCategoryStr.Equals(TEXT("MFD"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EAvionicsSubsystem::MFD);
            else if (SubCategoryStr.Equals(TEXT("HUD"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EAvionicsSubsystem::HUD);
            else if (SubCategoryStr.Equals(TEXT("HOTAS"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EAvionicsSubsystem::HOTAS);
            else if (SubCategoryStr.Equals(TEXT("Radar"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EAvionicsSubsystem::Radar);
            else if (SubCategoryStr.Equals(TEXT("IFF"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EAvionicsSubsystem::IFF);
            else if (SubCategoryStr.Equals(TEXT("FLIR"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EAvionicsSubsystem::FLIR);
            else if (SubCategoryStr.Equals(TEXT("EW"), ESearchCase::IgnoreCase))
                return static_cast<uint8>(EAvionicsSubsystem::EW);
        }
        
        // 숫자형 문자열인 경우 직접 변환
        if (SubCategoryStr.IsNumeric())
        {
            return FCString::Atoi(*SubCategoryStr);
        }
        
        return 0;
    }
};

// FA-50M 카테고리 매퍼 클래스
class FFA50MCategoryMapper
{
public:
    FFA50MCategoryMapper();
    ~FFA50MCategoryMapper();
    
    // 매핑 파일 로드
    bool LoadMappingFile(const FString& FilePath);
    
    // 매핑 파일 저장
    bool SaveMappingFile(const FString& FilePath);
    
    // 파트 번호로 카테고리 조회
    bool GetCategoryForPart(const FString& PartNo, FFA50MPartCategory& OutCategory);
    
    // 파트 번호에 카테고리 설정
    void SetCategoryForPart(const FString& PartNo, const FFA50MPartCategory& Category);
    
    // 특정 카테고리에 속하는 모든 파트 번호 조회
    TArray<FString> GetPartsByCategory(EFA50MSystemCategory MainCategory, uint8 SubCategory = 0);
    
    // 모든 매핑 데이터 반환
    const TMap<FString, FFA50MPartCategory>& GetAllMappings() const;
    
    // 매핑 삭제
    void RemoveMapping(const FString& PartNo);
    
    // 저장되지 않은 변경사항 여부 확인
    bool HasUnsavedChanges() const;
    
private:
    // 파트 번호와 카테고리 매핑 맵
    TMap<FString, FFA50MPartCategory> PartCategoryMap;
    
    // 저장되지 않은 변경사항 플래그
    bool bHasUnsavedChanges;
};