#include "AssetMenuTools/FTextureAssetActionListener.h"
#include "AssetMenuTools/FAssetsProcessorFormSelection.hpp"

#define LOCTEXT_NAMESPACE "FTextureAssetActionListener"

/* Implementation */

class FUTextureAssetProcessor_AutoSetTexFormat :public TAssetsProcessorFormSelection_Builder<UTexture>
{
protected:
    static bool bTagRuleExist = false;
    static TSet<FString> TagRule_SRGB;
    static TSet<FString> TagRule_Normal;

public:
    FUTextureAssetProcessor_AutoSetTexFormat()
    {
        if (!bTagRuleExist)
        {
            UpdateTagRules ();
        }
    }

    virtual void ProcessAssets (TArray<UTexture *> &Assets) override
    {
        for (auto TexIt = Assets.CreateConstIterator(); TexIt; ++TexIt)
        {
            UTexture * const Texture = *TexIt;
            UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Current Target Texture is : %s"),*Texture->GetPathName());
            ConvertProcessor (Texture);
        }
        
    }

    static void UpdateTagRules ()
    {
        TagRule_SRGB.Append (CreateRuleFStringArray (TEXT ("srgb")));
        TagRule_Normal.Append (CreateRuleFStringArray (TEXT ("normal")));
        bTagRuleExist = true;
    }

    static TArray<FString>& CreateRuleFStringArray (const FName& RuleName)
    {
        /*
         * TODO:
         * Read rule form xml,ini...
         */
        TArray<FString> LTempStringArray;
        switch (RuleName)
        {
            default:
            case FName(TEXT("srgb")):
            {
                FString &&TempString = TEXT("srgb,fx,col,color");
                TempString.ParseIntoArray (LTempStringArray, TEXT (","), true);
                break;
            };
            case FName(TEXT("normal")):
            {
                break;
            }
        };
        return LTempStringArray;
    }

    static void ConvertProcessor (UTexture* PTexObj)
    {
    #pragma region TextureObjectProperties
        bool bSRGB = false;
        bool bNorm = false;
        bool bMask = false;
        bool bForceLinear = false;
        bool bSmallSize = false;
        bool bVirtual = false;
    #pragma endregion TextureObjectProperties

        PTexObj->Modify ();
        // DeferCompression is more easy to change properties
        PTexObj->DeferCompression = true;

        FString &&LTexName = PTexObj->GetName();
        TArray<FString> LTagsArray;
        TSet<FString> LTagsSet;

        // "T_Tex.Name.1001"-->"T_Tex_Name_1001"
        LTexName = LTexName.Replace (TEXT("."),TEXT("_"),ESearchCase::IgnoreCase);
        LTexName.ParseIntoArray (LTagsArray, TEXT ("_"), true);
        LTagsSet.Append (LTagsArray);

        /* Test SRGB */
        const TSet<FString>& LFoundTag_SRGB = TagRule_SRGB.Intersect(LTagsSet);
        if (LFoundTag_SRGB.Num()>0) bSRGB = true;
        

        /* Test Normal map */
        const TSet<FString>& LFoundTag_Normal = TagRule_Normal.Intersect(LTagsSet);
        if (LFoundTag_Normal.Num()>0) bNorm = true;

        /* SRGB Setting */
        switch (true)
        {
            case bForceLinear:
            case bNorm:
            case bMask:
            case (bSRGB == false):
            case (bSRGB && bForceLinear):
            {
                PTexObj->SRGB = false;
                break;
            }
            case bSRGB:
            default:
            {
                PTexObj->SRGB = true;
                break;
            }
        } //End Switch

        /* Format setting */
        TextureCompressionSettings LTempCompressionSettings;
        switch (true)
        {
            case (bSmallSize):
            {
                LTempCompressionSettings = TextureCompressionSettings::TC_Displacementmap;
                break;
            }
            case (bForceLinear && !bNorm):
            {
                LTempCompressionSettings = TextureCompressionSettings::TC_Displacementmap;
            }
            case (bForceLinear && bSRGB):
            {
                LTempCompressionSettings = TextureCompressionSettings::TC_BC7;
            }
            case (bForceLinear && bMask):
            {
                LTempCompressionSettings = TextureCompressionSettings::TC_Grayscale;
            }
            case bForceLinear:
            {
                break;
            }
            case (bNorm && bForceLinear):
            {
                LTempCompressionSettings = TextureCompressionSettings::TC_Displacementmap;
                break;
            }
            case (bNorm):
            {
                LTempCompressionSettings = TextureCompressionSettings::TC_Normalmap;
                break;
            }

            default:
            {
                LTempCompressionSettings = TextureCompressionSettings::TC_Default;
                break;
            }
        }
        PTexObj->CompressionSettings = LTempCompressionSettings;

        /* Process Virtual texture property */
        bool bVirtualTex = PTexObj->VirtualTextureStreaming;
    }

};


/* Menu Commands */

namespace  TextureAssetActionListenerInterior
{
    class MineAssetCtxMenuCommands final : public TCommands<MineAssetCtxMenuCommands>
    {
    public:

        /* INIT */
        static FName MenuCtxName;
        MineAssetCtxMenuCommands ()
            : TCommands<MineAssetCtxMenuCommands> (
                MenuCtxName, // Context name for fast lookup
                FText::FromString (MenuCtxName.ToString()), // Context name for displaying
                NAME_None,   // No parent context
                FEditorStyle::GetStyleSetName () // Icon Style Set
                )
        {
        }

        /* Register all commands in this class */
        virtual void RegisterCommands () override
        {

            UI_COMMAND (MenuCommand1,
                "Auto set Tex Format",
                "Auto set format for selected texture assets.",
                EUserInterfaceActionType::Button, FInputGesture ()
            );

        }

    public:
        /* Command Action Objects */
        TSharedPtr<FUICommandInfo> MenuCommand1;

    };
    FName MineAssetCtxMenuCommands::MenuCtxName = TEXT ("MineTexAssetCtxMenu");

    /* Extension to menu */
    class FMineContentBrowserExtensions_SelectedAssets
    {
    public:

        static void ExecuteProcessor (
            TSharedPtr<FAssetsProcessorFormSelection_Base> const Processor
        )
        {
            Processor->Execute ();
        }

        static TSharedRef<FExtender>
        OnExtendContentBrowserAssetSelectionMenu (
            const TArray<FAssetData> &SelectedAssets
        )
        {
            static TSharedPtr<FUICommandList> CommandList;

            if (!CommandList.IsValid ())
                CommandList = MakeShareable (new FUICommandList);
            TSharedRef<FExtender> Extender (new FExtender ());

            MappingCommand (CommandList, SelectedAssets);

            if (CheckSelectedTypeTarget<UTexture>(SelectedAssets)) {
                // Add the Static actions sub-menu extender
                Extender->AddMenuExtension (
                    "GetAssetActions",
                    EExtensionHook::After,
                    CommandList,
                    FMenuExtensionDelegate::CreateStatic (
                        &CreateActionsSubMenu)
                );
            }
            return Extender;
        };

        static void CreateActionsSubMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            const FSlateIcon BaseMenuIcon = FSlateIcon ();
            const FText BaseMenuName = LOCTEXT ("ActionsSubMenuLabel", "Mine Texture Asset Actions");
            const FText BaseMenuTip = LOCTEXT ("ActionsSubMenuToolTip", "Type-related actions for Texture Asset.");

            MenuBuilder.AddSubMenu (
                BaseMenuName,
                BaseMenuTip,
                FNewMenuDelegate::CreateStatic (
                    &PopulateActionsMenu),
                false,
                BaseMenuIcon,
                true,
                FName (TEXT ("MineTexAssetsActions"))
            );

        }

        static void PopulateActionsMenu (
            FMenuBuilder &MenuBuilder
        )
        {
            // Add to Menu
            static const MineAssetCtxMenuCommands &ToolCommands = MineAssetCtxMenuCommands::Get ();
            MenuBuilder.AddMenuEntry (ToolCommands.MenuCommand1);
        }


        /**
         * @brief : Check if target type in current selections
         * @tparam : Should derived from UObject
         * @param  SelectedAssets : Current Selections
         * @return : return true if target type in current selections
         */
        template<typename T>
        static bool CheckSelectedTypeTarget (const TArray<FAssetData> &SelectedAssets)
        {
            bool bCurrentType = false;
            for (auto AssetIt = SelectedAssets.CreateConstIterator (); AssetIt; ++AssetIt) {
                const FAssetData &Asset = *AssetIt;
                bCurrentType = bCurrentType || (Asset.AssetClass == T::StaticClass ()->GetFName ());
            }
            return bCurrentType;
        }

        //static FAssetsProcessorFormSelection_Base * &
        template<typename P>
        static TSharedPtr<FAssetsProcessorFormSelection_Base>
            CreateProcessorPtr (const TArray<FAssetData> &SelectedAssets)
        {
            static_assert (
                std::is_base_of_v<FAssetsProcessorFormSelection_Base, P>,
                "Must be derived from FAssetsProcessorFormSelection_Base"
                );

            TSharedPtr<P> Processor = MakeShareable (new P);//On Heap
            Processor->SelectedAssets = SelectedAssets;
            return StaticCastSharedPtr<FAssetsProcessorFormSelection_Base> (Processor);
        }


        static void MappingCommand (
            const TSharedPtr<FUICommandList> &CommandList,
            const TArray<FAssetData> &SelectedAssets
        )
        {
            static const MineAssetCtxMenuCommands &ToolCommands = MineAssetCtxMenuCommands::Get ();
            CommandList->MapAction (
                ToolCommands.MenuCommand1,
                FExecuteAction::CreateStatic (
                    &ExecuteProcessor,
                    CreateProcessorPtr<FUTextureAssetProcessor_AutoSetTexFormat> (SelectedAssets)
                ),
                FCanExecuteAction ()
            );
        }
    };

};



/* Load to module */

void FTextureAssetActionListener::InstallHooks ()
{
    UE_LOG (LogMineCustomToolEditor, Warning, TEXT ("Install Common Asset Menu Hook"));
    // register commands
    TextureAssetActionListenerInterior::MineAssetCtxMenuCommands::Register ();

    // Declare Delegate 
    ContentBrowserExtenderDelegate =
        FContentBrowserMenuExtender_SelectedAssets::CreateStatic (
            &TextureAssetActionListenerInterior::
            FMineContentBrowserExtensions_SelectedAssets::
            OnExtendContentBrowserAssetSelectionMenu
        );

    // Get all content module delegates
    TArray<FContentBrowserMenuExtender_SelectedAssets>
        &CBMenuExtenderDelegates = GetExtenderDelegates ();

    // Add The delegate of mine
    CBMenuExtenderDelegates.Add (ContentBrowserExtenderDelegate);
    ContentBrowserExtenderDelegateHandle =
        CBMenuExtenderDelegates.Last ().GetHandle ();

}

void FTextureAssetActionListener::RemoveHooks ()
{
    TArray<FContentBrowserMenuExtender_SelectedAssets>
        &CBMenuExtenderDelegates = GetExtenderDelegates ();

    CBMenuExtenderDelegates.RemoveAll (
        [&](const FContentBrowserMenuExtender_SelectedAssets &Delegate)->bool {
            return Delegate.GetHandle () == ContentBrowserExtenderDelegateHandle;
        }
    );
}

TArray<FContentBrowserMenuExtender_SelectedAssets> &
FTextureAssetActionListener::GetExtenderDelegates ()
{
    /////////////////////////////
    ///Get ContentBrowser Module
    /////////////////////////////
    FContentBrowserModule &ContentBrowserModule =
        FModuleManager::LoadModuleChecked<FContentBrowserModule> (TEXT ("ContentBrowser"));

    return ContentBrowserModule.GetAllAssetViewContextMenuExtenders ();
}

#undef LOCTEXT_NAMESPACE