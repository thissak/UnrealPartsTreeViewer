// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/PartImageManager.h"

/**
 * 
 */
class MYPROJECT2_API FServiceLocator
{
public:
	// 이미지 매니저 관련
	static void RegisterImageManager(class FPartImageManager* Manager);
	static class FPartImageManager* GetImageManager();

	// 다른 서비스들도 필요하면 여기에 추가할 수 있음

private:
	static class FPartImageManager* ImageManagerInstance;
};