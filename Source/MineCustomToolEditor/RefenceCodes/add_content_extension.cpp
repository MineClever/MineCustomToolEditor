#include "ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "Engine/Engine.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

struct AddContentBroserExtension
{
    void AddExtension ()
    {
        /* 1. Find ContentBrowser*/
        FContentBrowserModule &ContentBrowserModule =
            FModuleManager::LoadModuleChecked<FContentBrowserModule> (TEXT ("ContentBrowser"));

        /* 2. Find SelectedAssets Menu Extensions*/
        TArray<FContentBrowserMenuExtender> &CBMenuExtenderDelegates =
            ContentBrowserModule.GetAllAssetViewViewMenuExtenders ();

        /* 3. Add the Menu to path right click */
        TSharedPtr<FExtender> AssetMenuExtender = MakeShareable (new FExtender);
        FMenuExtensionDelegate &&MenuExtensionDelegate =
            FMenuExtensionDelegate::CreateRaw (this, &AddContentBroserExtension::MakeContentPathMenu);

        AssetMenuExtender->AddMenuExtension (
            "Section",
            EExtensionHook::After,
            nullptr,
            MenuExtensionDelegate
        );

        FContentBrowserMenuExtender &&AssetMenuExtenderDelegate = FContentBrowserMenuExtender::CreateLambda (
            [&AssetMenuExtender]() {return AssetMenuExtender.ToSharedRef (); });

        /* 4. Add Extender to Extension Callback */
        CBMenuExtenderDelegates.Add (AssetMenuExtenderDelegate);
    }

    void MakeContentPathMenu (FMenuBuilder &menuBuilder)
    {
        // just a frame for tools to fill in
        menuBuilder.BeginSection ("Section", FText::FromString (FString("Test")));
        menuBuilder.AddMenuSeparator (FName ("Section_1"));
        menuBuilder.EndSection ();

        GEngine->AddOnScreenDebugMessage (-1, 5.f, FColor::Blue, TEXT ("TEST"));

    }
};
