#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMyProject2EditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void ResisterMenu();
    void ResisterMenusCallback();

    void OnToolButtonClicked();

private:
    TSharedPtr<class SWindow> TreeViewWindow;

};
