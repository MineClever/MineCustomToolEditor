#include "TabTools/TabToolPanel.h"

void TabToolPanel::Construct (const FArguments &InArgs)
{
    Tool = InArgs._Tool;
    if (Tool.IsValid ()) {
        // do anything you need from tool object
    }

    ChildSlot
    [
        SNew (SScrollBox)
        + SScrollBox::Slot ()
        .VAlign (VAlign_Top)
        .Padding (5)
        [
            SNew (SBorder)
            .BorderBackgroundColor (FColor (192, 192, 192, 255))
            .Padding (15.0f)
            [
                SNew (STextBlock)
                .Text (FText::FromString (TEXT ("This is a tab example.")))
            ]
        ]
        + SScrollBox::Slot ()
        .VAlign (VAlign_Top)
        .Padding (15.0f)
        [
            SNew (SButton)
            .VAlign (VAlign_Top)
            .HAlign(HAlign_Left)
            .Text (FText::FromString (TEXT ("Unused Botton")))
            .ForegroundColor (FColor (0, 255, 127, 255))
        ]

    ];
}