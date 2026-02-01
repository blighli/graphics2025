extends CharacterBody3D

@export var speed : float = 5.0
@export var ball_visual : MeshInstance3D
@export var ball_radius : float = 1.0

var gravity : float = ProjectSettings.get_setting("physics/3d/default_gravity")
@export var camera_pivot : Node3D

func _physics_process(delta):
	if not is_on_floor():
		velocity.y -= gravity * delta
	
	# direction based on camera and movement
	var input_dir := Input.get_vector("move_left", "move_right", "move_forward", "move_back")
	var direction := (camera_pivot.global_transform.basis * Vector3(input_dir.x, 0, input_dir.y)).normalized()

	if direction:
		velocity.x = direction.x * speed
		velocity.z = direction.z * speed
		_rolling_effect(direction, delta)
	else:
		velocity.x = move_toward(velocity.x, 0, speed)
		velocity.z = move_toward(velocity.z, 0, speed)
		var current_velocity_ground = Vector3(velocity.x, 0, velocity.z)
		if current_velocity_ground.length() > 0.1:
			_rolling_effect(current_velocity_ground.normalized(), delta)

	move_and_slide()

func _rolling_effect(dir: Vector3, delta: float):
	if not ball_visual: return
	var horizontal_velocity = Vector3(velocity.x, 0, velocity.z).length()
	
	# rotation = arc length / radius
	# distance = speed * delta
	var rotation_amount = (horizontal_velocity * delta) / ball_radius
	var rotation_axis = Vector3(dir.z, 0, -dir.x).normalized()
	
	if rotation_axis.length() > 0:
		ball_visual.global_rotate(rotation_axis, rotation_amount)
