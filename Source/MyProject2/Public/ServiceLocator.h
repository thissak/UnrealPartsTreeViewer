// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/PartImageManager.h"
#include "UI/PartMetadataWidget.h"

/**
 * 
 */
class MYPROJECT2_API FServiceLocator
{
public:
	// 이미지 매니저 관련
	static void RegisterImageManager(class FPartImageManager* Manager);
	static class FPartImageManager* GetImageManager();

private:
	static class FPartImageManager* ImageManagerInstance;
};