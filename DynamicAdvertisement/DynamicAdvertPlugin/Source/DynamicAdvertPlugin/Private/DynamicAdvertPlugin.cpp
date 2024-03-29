// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "IDynamicAdvertPlugin.h"


class FDynamicAdvertPlugin : public IDynamicAdvertPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FDynamicAdvertPlugin, DynamicAdvertPlugin)



void FDynamicAdvertPlugin::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FDynamicAdvertPlugin::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}



