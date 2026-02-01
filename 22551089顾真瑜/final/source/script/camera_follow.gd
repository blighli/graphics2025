extends Node3D

# character
@export var target : Node3D

# orbit
@export var base_height : float = 10.0
@export var orbit_radius : float = 8.0
@export var orbit_smoothness : float = 8.0
@export var height_smoothness : float = 4.0

@export var mouse_sensitivity : float = 0.002
@export var zoom_sensitivity : float = 0.8
@export var min_height : float = 2.0
@export var max_height : float = 20.0

var _current_orbit_angle : float = 0.0	# radian
var _current_camera_height : float = 0.0
var _target_camera_height : float = 0.0
var _is_first_frame : bool = true           

func _ready():
	_target_camera_height = base_height
	_current_camera_height = base_height
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	_update_camera_position(true)

func _input(event):
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		_current_orbit_angle -= event.relative.x * mouse_sensitivity
	
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_target_camera_height = clamp(
				_target_camera_height - zoom_sensitivity,
				min_height,
				max_height
			)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_target_camera_height = clamp(
				_target_camera_height + zoom_sensitivity,
				min_height,
				max_height
			)
	
	if event.is_action_pressed("ui_cancel"):
		if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
			Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
		else:
			Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
			get_viewport().warp_mouse(get_viewport().get_visible_rect().size * 0.5)

func _physics_process(delta):
	if not target:
		return
	
	_current_camera_height = lerp(
		_current_camera_height, 
		_target_camera_height, 
		height_smoothness * delta
	)
	
	_update_camera_position(false, delta)

func _update_camera_position(instant: bool = false, delta: float = 1.0):
	if not target:
		return
	
	var horizontal_offset = Vector3(
		sin(_current_orbit_angle) * orbit_radius,
		0,
		cos(_current_orbit_angle) * orbit_radius
	)
	
	var target_position = target.global_position + horizontal_offset
	target_position.y = _current_camera_height
	
	if instant or _is_first_frame:
		global_position = target_position
		_is_first_frame = false
	else:
		global_position = global_position.lerp(
			target_position, 
			orbit_smoothness * delta
		)
	
	look_at(target.global_position, Vector3.UP)
