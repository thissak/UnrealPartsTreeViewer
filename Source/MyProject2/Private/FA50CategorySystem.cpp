// FA50MCategorySystem.cpp
// FA-50M 조종석 시스템 카테고리 유틸리티 구현

#include "FA50CategorySystem.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"

FFA50MCategoryMapper::FFA50MCategoryMapper()
    : bHasUnsavedChanges(false)
{
}

FFA50MCategoryMapper::~FFA50MCategoryMapper()
{
}

bool FFA50MCategoryMapper::LoadMappingFile(const FString& FilePath)
{
    // 파일이 존재하는지 확인
    if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("매핑 파일을 찾을 수 없음: %s"), *FilePath);
        return false;
    }

    // 매핑 데이터 초기화
    PartCategoryMap.Empty();
    
    // CSV 파일 로드
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("매핑 파일 로드 실패: %s"), *FilePath);
        return false;
    }
    
    // 첫 번째 라인은 헤더로 간주
    bool bHasHeader = Lines.Num() > 0;
    int32 StartIndex = bHasHeader ? 1 : 0;
    
    // 각 라인 파싱
    for (int32 i = StartIndex; i < Lines.Num(); ++i)
    {
        TArray<FString> Columns;
        Lines[i].ParseIntoArray(Columns, TEXT(","), true);
        
        // 최소 3개 컬럼 필요: PartNo, MainCategory, SubCategory, [Notes]
        if (Columns.Num() < 3)
            continue;
            
        FString PartNo = Columns[0].TrimStartAndEnd();
        if (PartNo.IsEmpty())
            continue;
            
        // 카테고리 파싱
        FString MainCategoryStr = Columns[1].TrimStartAndEnd();
        FString SubCategoryStr = Columns[2].TrimStartAndEnd();
        
        EFA50mSystemCategory MainCategory = FA50MCategoryHelpers::GetCategoryFromString(MainCategoryStr);
        uint8 SubCategory = FA50MCategoryHelpers::GetSubCategoryFromString(MainCategory, SubCategoryStr);
        
        // 메모 필드 (선택사항)
        FString Notes;
        if (Columns.Num() > 3)
        {
            Notes = Columns[3].TrimStartAndEnd();
        }
        
        // 매핑 추가
        FFA50MPartCategory Category(MainCategory, SubCategory, Notes);
        PartCategoryMap.Add(PartNo, Category);
    }
    
    UE_LOG(LogTemp, Display, TEXT("매핑 파일 로드 완료: %s (총 %d개 항목)"), *FilePath, PartCategoryMap.Num());
    bHasUnsavedChanges = false;
    return true;
}

bool FFA50MCategoryMapper::SaveMappingFile(const FString& FilePath)
{
    // 저장할 문자열 배열 준비
    TArray<FString> Lines;
    
    // 헤더 추가
    Lines.Add(TEXT("PartNo,MainCategory,SubCategory,Notes"));
    
    // 모든 매핑 항목 추가
    for (const auto& Pair : PartCategoryMap)
    {
        const FString& PartNo = Pair.Key;
        const FFA50MPartCategory& Category = Pair.Value;
        
        // 카테고리를 문자열로 변환
        FString MainCategoryStr;
        FString SubCategoryStr;
        
        // 열거형 값을 문자열로 변환
        switch (Category.MainCategory)
        {
            case EFA50mSystemCategory::FlightControl: MainCategoryStr = TEXT("FlightControl"); break;
            case EFA50mSystemCategory::PowerManagement: MainCategoryStr = TEXT("PowerManagement"); break;
            case EFA50mSystemCategory::EngineFuel: MainCategoryStr = TEXT("EngineFuel"); break;
            case EFA50mSystemCategory::NavigationComm: MainCategoryStr = TEXT("NavigationComm"); break;
            case EFA50mSystemCategory::WeaponsDefense: MainCategoryStr = TEXT("WeaponsDefense"); break;
            case EFA50mSystemCategory::Lighting: MainCategoryStr = TEXT("Lighting"); break;
            case EFA50mSystemCategory::Emergency: MainCategoryStr = TEXT("Emergency"); break;
            case EFA50mSystemCategory::EnvironmentalControl: MainCategoryStr = TEXT("EnvironmentalControl"); break;
            case EFA50mSystemCategory::DisplayRecording: MainCategoryStr = TEXT("DisplayRecording"); break;
            case EFA50mSystemCategory::TestDiagnostics: MainCategoryStr = TEXT("TestDiagnostics"); break;
            case EFA50mSystemCategory::Avionics: MainCategoryStr = TEXT("Avionics"); break;
            default: MainCategoryStr = TEXT("None"); break;
        }
        
        // 하위 카테고리 문자열 생성 (주 카테고리에 따라 달라짐)
        if (Category.MainCategory == EFA50mSystemCategory::FlightControl)
        {
            switch (static_cast<EFlightControlSubsystem>(Category.SubCategory))
            {
                case EFlightControlSubsystem::FLCS: SubCategoryStr = TEXT("FLCS"); break;
                case EFlightControlSubsystem::ControlSurfaces: SubCategoryStr = TEXT("ControlSurfaces"); break;
                case EFlightControlSubsystem::TrimControl: SubCategoryStr = TEXT("TrimControl"); break;
                case EFlightControlSubsystem::AutoPilot: SubCategoryStr = TEXT("AutoPilot"); break;
                case EFlightControlSubsystem::LandingGear: SubCategoryStr = TEXT("LandingGear"); break;
                case EFlightControlSubsystem::SpeedBrake: SubCategoryStr = TEXT("SpeedBrake"); break;
                case EFlightControlSubsystem::AirBrake: SubCategoryStr = TEXT("AirBrake"); break;
                case EFlightControlSubsystem::StabilityAugmentation: SubCategoryStr = TEXT("StabilityAugmentation"); break;
                default: SubCategoryStr = TEXT("None"); break;
            }
        }
        else if (Category.MainCategory == EFA50mSystemCategory::Avionics)
        {
            switch (static_cast<EAvionicsSubsystem>(Category.SubCategory))
            {
                case EAvionicsSubsystem::MFD: SubCategoryStr = TEXT("MFD"); break;
                case EAvionicsSubsystem::HUD: SubCategoryStr = TEXT("HUD"); break;
                case EAvionicsSubsystem::HOTAS: SubCategoryStr = TEXT("HOTAS"); break;
                case EAvionicsSubsystem::Radar: SubCategoryStr = TEXT("Radar"); break;
                case EAvionicsSubsystem::IFF: SubCategoryStr = TEXT("IFF"); break;
                case EAvionicsSubsystem::FLIR: SubCategoryStr = TEXT("FLIR"); break;
                case EAvionicsSubsystem::EW: SubCategoryStr = TEXT("EW"); break;
                default: SubCategoryStr = TEXT("None"); break;
            }
        }
        // 다른 모든 카테고리에 대해서도 유사하게 구현
        // 여기서는 간결함을 위해 생략했지만, 실제로는 모든 카테고리에 대해 구현해야 함
        else
        {
            SubCategoryStr = FString::Printf(TEXT("%d"), Category.SubCategory);
        }
        
        // CSV 라인 생성
        FString Line = FString::Printf(TEXT("%s,%s,%s,%s"), 
            *PartNo, 
            *MainCategoryStr, 
            *SubCategoryStr, 
            *Category.Notes);
        
        Lines.Add(Line);
    }
    
    // 파일에 저장
    bool bSuccess = FFileHelper::SaveStringArrayToFile(Lines, *FilePath);
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Display, TEXT("매핑 파일 저장 완료: %s (총 %d개 항목)"), *FilePath, PartCategoryMap.Num());
        bHasUnsavedChanges = false;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("매핑 파일 저장 실패: %s"), *FilePath);
    }
    
    return bSuccess;
}

bool FFA50MCategoryMapper::GetCategoryForPart(const FString& PartNo, FFA50MPartCategory& OutCategory)
{
    const FFA50MPartCategory* FoundCategory = PartCategoryMap.Find(PartNo);
    
    if (FoundCategory)
    {
        OutCategory = *FoundCategory;
        return true;
    }
    
    // 찾지 못하면 기본값 설정
    OutCategory = FFA50MPartCategory();
    return false;
}

void FFA50MCategoryMapper::SetCategoryForPart(const FString& PartNo, const FFA50MPartCategory& Category)
{
    // 기존 값과 다른 경우에만 변경 사항으로 처리
    if (!PartCategoryMap.Contains(PartNo) || !(PartCategoryMap[PartNo] == Category))
    {
        PartCategoryMap.Add(PartNo, Category);
        bHasUnsavedChanges = true;
    }
}

TArray<FString> FFA50MCategoryMapper::GetPartsByCategory(EFA50mSystemCategory MainCategory, uint8 SubCategory)
{
    TArray<FString> Result;
    
    for (const auto& Pair : PartCategoryMap)
    {
        const FString& PartNo = Pair.Key;
        const FFA50MPartCategory& Category = Pair.Value;
        
        // 주 카테고리가 일치하고, 하위 카테고리가 0이거나 일치하면 추가
        if (Category.MainCategory == MainCategory && 
            (SubCategory == 0 || Category.SubCategory == SubCategory))
        {
            Result.Add(PartNo);
        }
    }
    
    return Result;
}

const TMap<FString, FFA50MPartCategory>& FFA50MCategoryMapper::GetAllMappings() const
{
    return PartCategoryMap;
}

void FFA50MCategoryMapper::RemoveMapping(const FString& PartNo)
{
    if (PartCategoryMap.Remove(PartNo) > 0)
    {
        bHasUnsavedChanges = true;
    }
}

bool FFA50MCategoryMapper::HasUnsavedChanges() const
{
    return bHasUnsavedChanges;
}