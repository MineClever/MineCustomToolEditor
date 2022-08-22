#include "ConfigIO/ConfigIO.h"
#include "ISettingsModule.h"
#include "Developer/Settings/Public/ISettingsContainer.h"

void FMineToolConfigLoader::OnStartupModule ()
{
    // Make based Config names
    BaseSetting.ContainerName = TEXT ("Project");
    BaseSetting.CategoryName = TEXT ("MineConfigs");
    BaseSetting.SectionName = TEXT ("MineConfigSettings");
    BaseSetting.DisplayName = FText::FromString (TEXT ("MineEditorSettings"));
    BaseSetting.DescriptionName = FText::FromString (TEXT ("Configure Settings"));

    // Register settings:
    {
        ISettingsModule *SettingsModule = FModuleManager::GetModulePtr<ISettingsModule> ("Settings");
        if (SettingsModule) {
            TSharedPtr<ISettingsContainer> const ProjectSettingsContainer = SettingsModule->GetContainer (BaseSetting.ContainerName);
            ProjectSettingsContainer->DescribeCategory (BaseSetting.CategoryName, BaseSetting.DisplayName, BaseSetting.DescriptionName);

            SettingsModule->RegisterSettings (BaseSetting.ContainerName, BaseSetting.CategoryName, BaseSetting.SectionName,
                BaseSetting.DisplayName,
                BaseSetting.DescriptionName,
                GetMutableDefault<UMineEditorConfigSettings> ()
            );
        }
    };

    // Fix Camera NearClip
    auto TimerToSetNearClip = [&]()
    {
        static auto const ConfigSettings = GetDefault<UMineEditorConfigSettings> ();

        if (ConfigSettings->bUseCustomDefaultCameraConfig)
        {
            // NOTE: Remove FarClip Limit Inside? FIX BUG !
            if (ConfigSettings->bForceClearViewportFarClipOverride)
            {
                auto AllViewPorts = GEditor->GetAllViewportClients ();
                for (FEditorViewportClient *ViewPort : AllViewPorts) {
                    ViewPort->OverrideFarClipPlane (-1.0);
                }
            }

            GNearClippingPlane = ConfigSettings->ConfigStartUpNearClip;
        }
    };

    TSharedPtr<FTimerHandle> const TimerHandlePtr = MakeShareable (new FTimerHandle);
    GEditor->GetTimerManager ()->SetTimer (*TimerHandlePtr, FTimerDelegate::CreateLambda (TimerToSetNearClip), 5.0, true, 5.0);

}

void FMineToolConfigLoader::OnShutdownModule ()
{
    // Unregistered settings
    ISettingsModule *SettingsModule = FModuleManager::GetModulePtr<ISettingsModule> ("Settings");
    if (SettingsModule) {
        SettingsModule->UnregisterSettings (BaseSetting.ContainerName, BaseSetting.CategoryName, BaseSetting.SectionName);
    }
}