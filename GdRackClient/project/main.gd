extends Node

onready var index_control = $IndexControl
onready var voltage_control = $VoltageControl

var current_index = 0;

func _ready() -> void:
	current_index = index_control.value
	
	index_control.connect("value_changed", self, "_index_changed")
	voltage_control.connect("value_changed", self, "_voltage_changed")

func _voltage_changed(value):
	RackControl.update(current_index, value)

func _index_changed(value):
	current_index = value;
