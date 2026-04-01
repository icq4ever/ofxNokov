#pragma once

#include "ofxOSC.h"
#include "RigidBodyInfo.h"

class OSCSending
{
public:
	void setDestinationSettingsFile(std::filesystem::path &filepath);
	void setupDestinations(std::vector<std::pair<const std::string, unsigned short>> dst);
	void send(int ID, const RigidBodyInfo &rb, bool as_bundle=false);
private:
	std::vector<std::shared_ptr<ofxOscSender>> senders;
};
