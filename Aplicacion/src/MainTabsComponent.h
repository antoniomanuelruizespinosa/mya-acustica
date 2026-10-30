#pragma once
#include "RTAPanel.h"

// The existing navigation commands select the view; this component owns the
// independent RTA screen while the other screens are migrated separately.
class MainTabsComponent : public juce::Component {
public:
 explicit MainTabsComponent(RTAPanel::DataProvider provider);
 void showRTA(bool visible);
 void resized() override;
private:
 RTAPanel rta;
};
