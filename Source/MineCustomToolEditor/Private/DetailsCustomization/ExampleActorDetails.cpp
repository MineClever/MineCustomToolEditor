#include "DetailsCustomization/ExampleActorDetails.h"


TSharedRef<IDetailCustomization> FExampleActorDetails::MakeInstance ()
{
    return MakeShareable (new FExampleActorDetails);
}

void FExampleActorDetails::CustomizeDetails (IDetailLayoutBuilder &DetailLayout)
{
    TArray<TWeakObjectPtr<UObject>> Objects;
    DetailLayout.GetObjectsBeingCustomized (Objects);
    if (Objects.Num () != 1) {
        // skip customization if select more than one objects
        return;
    }
    AExampleActor *Actor = static_cast<AExampleActor*>(Objects[0].Get());

    // hide original property
    DetailLayout.HideProperty (DetailLayout.GetProperty (GET_MEMBER_NAME_CHECKED (AExampleActor, bOption1)));
    DetailLayout.HideProperty (DetailLayout.GetProperty (GET_MEMBER_NAME_CHECKED (AExampleActor, bOption2)));

    // add custom widget to "Options" category
    IDetailCategoryBuilder &OptionsCategory = 
        DetailLayout.EditCategory ("Options", FText::FromString (""), ECategoryPriority::Important);
    OptionsCategory.AddCustomRow (FText::FromString ("Options"))
    .WholeRowContent ()
    [
        SNew (SHorizontalBox)
        + SHorizontalBox::Slot ()
        .AutoWidth ()
        .VAlign (VAlign_Center)
        [
            SNew (SCheckBox)
            .Style (FEditorStyle::Get (), "RadioButton")
            .IsChecked (this, &FExampleActorDetails::IsModeRadioChecked, Actor, 1)
            .OnCheckStateChanged (this, &FExampleActorDetails::OnModeRadioChanged, Actor, 1)
            [
                SNew (STextBlock).Text (FText::FromString ("Option 1"))
            ]
        ]

        + SHorizontalBox::Slot ()
        .AutoWidth ()
        .Padding (10.f, 0.f, 0.f, 0.f)
        .VAlign (VAlign_Center)
        [
            SNew (SCheckBox)
            .Style (FEditorStyle::Get (), "RadioButton")
            .IsChecked (this, &FExampleActorDetails::IsModeRadioChecked, Actor, 2)
            .OnCheckStateChanged (this, &FExampleActorDetails::OnModeRadioChanged, Actor, 2)
            [
                SNew (STextBlock).Text (FText::FromString ("Option 2"))
            ]
        ]

    ];
}

ECheckBoxState FExampleActorDetails::IsModeRadioChecked (AExampleActor *Actor, int OptionIndex) const
{
    bool bFlag = false;
    if (Actor) {
        if (OptionIndex == 1)
            bFlag = Actor->bOption1;
        else if (OptionIndex == 2)
            bFlag = Actor->bOption2;
    }
    return bFlag ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FExampleActorDetails::OnModeRadioChanged (ECheckBoxState CheckType, AExampleActor *Actor, int OptionIndex)
{
    bool bFlag = (CheckType == ECheckBoxState::Checked);
    if (Actor) {
        Actor->Modify ();
        if (bFlag) {
            // clear all options first
            Actor->bOption1 = false;
            Actor->bOption2 = false;
        }
        if (OptionIndex == 1)
            Actor->bOption1 = bFlag;
        else if (OptionIndex == 2)
            Actor->bOption2 = bFlag;
    }
}