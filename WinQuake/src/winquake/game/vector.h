/*  Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	See file, 'COPYING', for details.
*/

#pragma once

struct Vector3D
{
	constexpr Vector3D() = default;
	constexpr Vector3D(const Vector3D&) = default;

	constexpr Vector3D(vec_t x, vec_t y, vec_t z)
		: x(x)
		, y(y)
		, z(z)
	{
	}

	constexpr Vector3D& operator=(const Vector3D&) = default;

	constexpr operator const float* () const { return &x; }
	constexpr operator float* () { return &x; }

	constexpr Vector3D operator+(const Vector3D& other) const
	{
		Vector3D result{*this};

		result.x += other.x;
		result.y += other.y;
		result.z += other.z;

		return result;
	}

	constexpr Vector3D operator-(const Vector3D& other) const
	{
		Vector3D result{*this};

		result.x -= other.x;
		result.y -= other.y;
		result.z -= other.z;

		return result;
	}

	constexpr Vector3D operator*(float scalar) const
	{
		Vector3D result{*this};

		result.x *= scalar;
		result.y *= scalar;
		result.z *= scalar;

		return result;
	}

	vec_t x = 0;
	vec_t y = 0;
	vec_t z = 0;
};

constexpr Vector3D operator*(float scalar, const Vector3D& vec)
{
	return vec * scalar;
}

//Treat vec3_t as Vector3D with this.
inline Vector3D& AsVector(vec3_t vec)
{
	return *reinterpret_cast<Vector3D*>(vec);
}
