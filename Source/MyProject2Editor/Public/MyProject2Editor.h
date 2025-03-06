#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FMyProject2EditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void RegisterMenu();
    void RegisterMenusCallback();

    void OnToolButtonClicked();

private:
    TSharedPtr<class SWindow> TreeViewWindow;

};
