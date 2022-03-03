extends Node

const GdRackClient = preload("res://gdnative/gdrackclient.gdns")
const SIZE = 8

onready var client = GdRackClient.new()

func update(index: int, voltage: float):
	assert(index < SIZE and index >= 0)
	assert(voltage >= -10.0 and voltage <= 10.0)
	
	var data = "{index};{voltage}".format({
		"index": index, 
		"voltage": voltage 
	})
	
	client.send_data(data)
