// Source/MyProject2/Private/ImportSettings.cpp
#include "ImportSettings.h"

// 정적 멤버 초기화
UImportSettingsManager* UImportSettingsManager::Instance = nullptr;

UImportSettingsManager* UImportSettingsManager::Get()
{
	if (!Instance)
	{
		Instance = NewObject<UImportSettingsManager>();
		Instance->AddToRoot(); // 가비지 컬렉션 방지
        
		// 설정 로드 (UObject config 시스템 사용)
		if (Instance)
		{
			Instance->LoadConfig();
		}
	}
    
	return Instance;
}

void UImportSettingsManager::SaveSettings(const FImportSettings& NewSettings)
{
	CurrentSettings = NewSettings;
	SaveConfig();
}

void UImportSettingsManager::ResetToDefaults()
{
	CurrentSettings = FImportSettings();
	SaveConfig();
}