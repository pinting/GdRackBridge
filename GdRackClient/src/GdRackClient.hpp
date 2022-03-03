#ifndef RACKCLIENT_H
#define RACKCLIENT_H

#include <Godot.hpp>

#include <Input.hpp>
#include <Reference.hpp>
#include <Sprite.hpp>

class GdRackClient : public godot::Reference
{
	GODOT_CLASS(GdRackClient, godot::Reference)

public:
	static void _register_methods();

	void _init();
	void sendData(godot::String data);
};

#endif