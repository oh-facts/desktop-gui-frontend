/* date = July 23rd 2024 2:36 pm */

#ifndef SAOIRSE_CAMERA_H
#define SAOIRSE_CAMERA_H

enum CAMERA_PROJ
{
	CAMERA_PROJ_UN,
	CAMERA_PROJ_PERS,
	CAMERA_PROJ_ORTHO,
	CAMERA_PROJ_COUNT
};

#define WORLD_UP (v3f){{0,1,0}}
#define WORLD_FRONT (v3f){{0,0,-1}}

union quat
{
	struct
	{
		f32 r,i,j,k;
	};
	
	struct
	{
		f32 x,y,z,w;
	};
	
	f32 e[4];
};

internal quat operator*(quat x, quat y)
{
	return (quat){
		.r = x.r * y.r - x.i * y.i - x.j * y.j - x.k * y.k,
		.i = x.r * y.i + x.i * y.r + x.j * y.k - x.k * y.j,
		.j = x.r * y.j - x.i * y.k + x.j * y.r + x.k * y.i,
		.k = x.r * y.k + x.i * y.j - x.j * y.i + x.k * y.r
	};
}

struct Camera
{
	v3f pos;
	v3f target;
	v3f up;
	f32 pitch;
	f32 yaw;
	f32 zoom;
	v3f mv;
	v3f input_rot;
	f32 speed;
	f32 aspect;
	CAMERA_PROJ proj;
};

internal m4f quat_to_matrix(quat q)
{
	f32 xx = q.i * q.i;
	f32 yy = q.j * q.j;
	f32 zz = q.k * q.k;
	f32 xy = q.i * q.j;
	f32 xz = q.i * q.k;
	f32 yz = q.j * q.k;
	f32 wx = q.r * q.i;
	f32 wy = q.r * q.j;
	f32 wz = q.r * q.k;
	
	return (m4f) {
		{
			{1 - 2 * (yy + zz), 2 * (xy - wz),     2 * (xz + wy),     0},
			{2 * (xy + wz),     1 - 2 * (xx + zz), 2 * (yz - wx),     0},
			{2 * (xz - wy),     2 * (yz + wx),     1 - 2 * (xx + yy), 0},
			{0,                 0,                 0,                 1}
		}
	};
}

internal m4f m4f_make_perspective(f32 fov, f32 aspect, f32 near, f32 far)
{
	f32 tanHalfFov = tan(fov / 2.0f);
	f32 range = near - far;
	
	return (m4f) {
		{
			{1.0f / (aspect * tanHalfFov), 0, 0, 0},
			{0, 1.0f / tanHalfFov, 0, 0},
			{0, 0, (near + far) / range, 2 * near * far / range},
			{0, 0, -1, 0}
		}
	};
}

internal m4f_ortho_proj cam_get_proj_inv(Camera *cam)
{
	f32 z = cam->zoom;
	f32 za = z * cam->aspect;
	
	m4f_ortho_proj out = m4f_ortho(-za, za, -z, z, 0.001, 1000);
	return out;
}

internal m4f cam_get_proj(Camera *cam)
{
	switch(cam->proj)
	{
		case CAMERA_PROJ_ORTHO:
		{
			f32 z = cam->zoom;
			f32 za = z * cam->aspect;
			
			m4f out = m4f_ortho(-za, za, -z, z, 0.001, 1000).fwd;
			return out;
			
		}break;
		case CAMERA_PROJ_PERS:
		{
			quat rot_x = {
				.r = cosf(cam->input_rot.x / 2),
				.i = sinf(cam->input_rot.x / 2)
			};
			quat rot_y = {
				.r = cosf(cam->input_rot.y / 2),
				.j = sinf(cam->input_rot.y / 2),
			};
			
			
			quat rotation = rot_x * rot_y;
			
			
			m4f rotation_matrix = quat_to_matrix(rotation);
			
			m4f persp = m4f_make_perspective(90, 16.f/9, 0.001,1000);
			
			return persp * rotation_matrix;
			
		}break;
		default:
		{
			INVALID_CODE_PATH();
		}
	}
	
	INVALID_CODE_PATH();
	return m4f_identity();
}

internal m4f cam_get_view(Camera *cam)
{
	return m4f_look_at(cam->pos, cam->pos + cam->target, cam->up);
}

internal void cam_input(Camera *cam, Input *input)
{
	switch(cam->proj)
	{
		case CAMERA_PROJ_ORTHO:
		{
			cam->mv.x = 0;
			cam->mv.y = 0;
			
			if(input_is_key_held(input, 'W'))
			{
				cam->mv.y = 1;
			}
			if(input_is_key_held(input, 'S'))
			{
				cam->mv.y = -1;
			}
			if(input_is_key_held(input, 'D'))
			{
				cam->mv.x = 1;
			}
			if(input_is_key_held(input, 'A'))
			{
				cam->mv.x = -1;
			}
			
		}break;
		case CAMERA_PROJ_PERS:
		{
			cam->mv.x = 0;
			cam->mv.y = 0;
			cam->mv.z = 0;
			
			if(input_is_key_held(input, 'W'))
			{
				cam->mv.z = 1;
			}
			if(input_is_key_held(input, 'S'))
			{
				cam->mv.z = -1;
			}
			if(input_is_key_held(input, 'D'))
			{
				cam->mv.x = 1;
			}
			if(input_is_key_held(input, 'A'))
			{
				cam->mv.x = -1;
			}
			if(input_is_key_held(input, 'Q'))
			{
				cam->mv.y = 1;
			}
			if(input_is_key_held(input, 'E'))
			{
				cam->mv.y = -1;
			}
			
			v2i mv = input_get_mouse_mv(input);
			
			
			cam->input_rot.x += mv.y * 0.001;
			cam->input_rot.y += mv.x * 0.001;
			
		}break;
		default:
		{
			INVALID_CODE_PATH();
		}
	}
	
}

internal void cam_update(Camera *cam, f32 delta)
{
	
	switch(cam->proj)
	{
		case CAMERA_PROJ_ORTHO:
		{
			cam->pos += cam->mv * cam->speed * delta;
			
		}break;
		case CAMERA_PROJ_PERS:
		{
			quat rot_x = {
				.r = cosf(cam->input_rot.x / 2),
				.i = sinf(cam->input_rot.x / 2)
			};
			quat rot_y = {
				.r = cosf(cam->input_rot.y / 2),
				.j = sinf(cam->input_rot.y / 2),
			};
			
			
			quat rotation = rot_x * rot_y;
			
			
			m4f rotation_matrix = quat_to_matrix(rotation);
			
			cam->pos += (rotation_matrix * (v4f){.xyz = cam->mv * cam->speed * delta}).xyz;
			
		}break;
		default:
		{
			INVALID_CODE_PATH();
		}
	}
}

#endif //SAOIRSE_CAMERA_H
