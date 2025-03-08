// Fill out your copyright notice in the Description page of Project Settings.


#include "ServiceLocator.h"
#include "UI/PartImageManager.h"

// 정적 멤버 초기화
FPartImageManager* FServiceLocator::ImageManagerInstance = nullptr;

void FServiceLocator::RegisterImageManager(FPartImageManager* Manager)
{
	ImageManagerInstance = Manager;
}

FPartImageManager* FServiceLocator::GetImageManager()
{
	// 등록된 인스턴스가 없으면 에러 로그
	if (!ImageManagerInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("이미지 매니저가 등록되지 않았습니다. RegisterImageManager를 먼저 호출해야 합니다."));
	}
	return ImageManagerInstance;
}
