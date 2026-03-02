extends Node2D

@export_group("Required Nodes")
@export var ground_mesh: MeshInstance3D
@export var snowball: Node3D
@export var sub_viewport: SubViewport
@export var paint_canvas: Node2D

@export_group("Brush Settings")
@export var brush_texture: Texture2D
@export var brush_size: Vector2 = Vector2(0.2, 0.2)
@export var brush_opacity: float = 0.5

var ground_size: Vector2
var ground_origin: Vector2
var last_paint_pos: Vector3 = Vector3.ZERO
var is_ready_to_paint: bool = false
var dynamic_min_distance: float = 0.1

func _ready():
	if not _validate_setup(): return
	
	# 1. ground size for map
	var aabb = ground_mesh.get_aabb()
	ground_size = Vector2(aabb.size.x, aabb.size.z)
	ground_origin = Vector2(aabb.position.x, aabb.position.z)
	
	# 2. brush size
	dynamic_min_distance = (brush_size.x * ground_size.x) * 0.4
	
	# 3. viewport setting
	sub_viewport.render_target_clear_mode = SubViewport.CLEAR_MODE_NEVER
	sub_viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	
	# bind viewport texture to shader
	var viewport_tex = sub_viewport.get_texture()
	var mat = ground_mesh.get_surface_override_material(0)
	if not mat:
		mat = ground_mesh.mesh.surface_get_material(0)
	if mat is ShaderMaterial:
		mat.set_shader_parameter("mask_texture", viewport_tex)
		print("TrailPainter: Bind ViewportTexture to 'mask_texture' successfully")
	else:
		push_warning("TrailPainter: No ShaderMaterial，mask_texture bind failed")
	
	_prepare_canvas()

func _prepare_canvas():
	var bg = ColorRect.new()
	bg.size = sub_viewport.size
	bg.color = Color.WHITE
	paint_canvas.add_child(bg)
	
	await get_tree().process_frame
	await get_tree().process_frame
	
	bg.queue_free()
	
	last_paint_pos = snowball.global_position
	is_ready_to_paint = true

func _process(_delta):
	if not is_ready_to_paint: return
	
	# prevent the accumulaion of alpha channel
	var current_dist = snowball.global_position.distance_to(last_paint_pos)
	if current_dist > dynamic_min_distance:
		_draw_trail()
		last_paint_pos = snowball.global_position

func _draw_trail():
	var local_pos = ground_mesh.to_local(snowball.global_position)
	var uv_x = (local_pos.x - ground_origin.x) / ground_size.x
	var uv_y = (local_pos.z - ground_origin.y) / ground_size.y
	
	if uv_x < 0 or uv_x > 1 or uv_y < 0 or uv_y > 1: return
		
	var paint_pos = Vector2(uv_x * sub_viewport.size.x, uv_y * sub_viewport.size.y)
	_stamp_brush(paint_pos)

func _stamp_brush(pos: Vector2):
	var sprite = Sprite2D.new()
	sprite.texture = brush_texture
	sprite.position = pos
	sprite.scale = brush_size
	sprite.modulate = Color(0, 0, 0, brush_opacity) 
	sprite.rotation = randf() * TAU
	
	paint_canvas.add_child(sprite)
	
	# waitting for write
	await get_tree().process_frame
	if is_instance_valid(sprite):
		sprite.queue_free()

func _validate_setup() -> bool:
	if not ground_mesh or not snowball or not sub_viewport or not paint_canvas:
		return false
	return true
