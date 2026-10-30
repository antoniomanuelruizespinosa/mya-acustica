#include "MainTabsComponent.h"

MainTabsComponent::MainTabsComponent(RTAPanel::DataProvider provider) : rta(std::move(provider)) {
 addAndMakeVisible(rta);
 setVisible(false);
}

void MainTabsComponent::showRTA(bool visible) { setVisible(visible); rta.setActive(visible); }
void MainTabsComponent::resized() { rta.setBounds(getLocalBounds()); }
