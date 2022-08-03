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

        // register settings:
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
}

void FMineToolConfigLoader::OnShutdownModule ()
{
    // unregister settings
    ISettingsModule *SettingsModule = FModuleManager::GetModulePtr<ISettingsModule> ("Settings");
    if (SettingsModule) {
        SettingsModule->UnregisterSettings (BaseSetting.ContainerName, BaseSetting.CategoryName, BaseSetting.SectionName);
    }
}