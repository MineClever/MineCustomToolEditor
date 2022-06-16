#pragma once
#include "IDetailCustomization.h"

class AExampleActor;

/*
*   The details customization implements IDetailCustomization interface.
* In the main entry point CustomizeDetails function,
* we first hide original properties option 1 and option 2
* (you can comment out those two lines and see how it works).
*
*   Then we add our custom widget, here the "RadioButton" is purely a visual style,
* it has nothing to do with mutually exclusive logic.
* You can implement the same logic with other visuals like regular check box, buttons, etc.
* In the widget functions for check box,
* IsModeRadioChecked and OnModeRadioChanged we add extra parameters "actor" and "optionIndex",
* so we can pass in the editing object and specify option when we construct the widget.
*/
class FExampleActorDetails : public IDetailCustomization
{
public:
    /** Makes a new instance of this detail layout class for a specific detail view requesting it */
    static TSharedRef<IDetailCustomization> MakeInstance ();

    /** IDetailCustomization interface */
    virtual void CustomizeDetails (IDetailLayoutBuilder &DetailLayout) override;

protected:
    // widget functions

    ECheckBoxState IsModeRadioChecked (AExampleActor *Actor, int OptionIndex) const;
    void OnModeRadioChanged (ECheckBoxState CheckType, AExampleActor *Actor, int OptionIndex);
};