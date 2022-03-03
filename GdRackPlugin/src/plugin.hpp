#pragma once

#include <rack.hpp>
#include <logger.hpp>

using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern rack::Plugin *pluginInstance;

// Declare each Model, defined in each module source file
extern rack::Model *modelGdRackModule;