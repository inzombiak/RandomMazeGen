#include "Math.h"

float sfmath::Dot(const MazeDefs::Vector2f& v1, const MazeDefs::Vector2f& v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

float sfmath::Dot(const MazeDefs::Vector3f& v1, const MazeDefs::Vector3f& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

float sfmath::Length2(const MazeDefs::Vector2f& v)
{
	return (v.x * v.x + v.y  * v.y);
}
float sfmath::Length(const MazeDefs::Vector2f& v)
{
	return sqrt(Length2(v));
}

float sfmath::Length2(const MazeDefs::Vector3f& v)
{
	return (v.x * v.x + v.y  * v.y + v.z * v.z);
}
float sfmath::Length(const MazeDefs::Vector3f& v)
{
	return sqrt(Length2(v));
}

float sfmath::Cross(const MazeDefs::Vector2f& v1, const MazeDefs::Vector2f& v2)
{
	float result = (v1.x * v2.y) - (v1.y * v2.x);

	result = roundf(result * 100) / 100;

	return result;
}

MazeDefs::Vector3f sfmath::Cross3D(const MazeDefs::Vector3f& v1, const MazeDefs::Vector3f& v2)
{
	MazeDefs::Vector3f result;

	result.x = v1.y * v2.z - v1.z * v2.y;
	result.y = v1.z * v2.x - v1.x * v2.z;
	result.z = v1.x * v2.y - v1.y * v2.x;

	return result;
}

bool sfmath::SameSideOfLine(const MazeDefs::Vector2f& a, const MazeDefs::Vector2f& b, const MazeDefs::Vector2f& p1, const MazeDefs::Vector2f& p2)
{
	float cp1 = Cross(b - a, p1 - a);
	float cp2 = Cross(b - a, p2 - a);

	return ((cp1 < 0) != (cp2 < 0));

}

bool sfmath::PointInTriangle(const MazeDefs::Vector2f& p0, const MazeDefs::Vector2f& p1, const MazeDefs::Vector2f& p2, const MazeDefs::Vector2f& p)
{
	float s = p0.y*p2.x - p0.x*p2.y + (p2.y - p0.y)*p.x + (p0.x - p2.x)*p.y;
	float t = p0.x*p1.y - p0.y*p1.x + (p0.y - p1.y)*p.x + (p1.x - p0.x)*p.y;

	if ((s < 0) != (t < 0))
		return false;

	float A = -p1.y * p2.x + p0.y * (p2.x - p1.x) + p0.x * (p1.y - p2.y) + p1.x * p2.y;
	if (A < 0.0)
	{
		s = -s;
		t = -t;
		A = -A;
	}
	return s > 0 && t > 0 && (s + t) <= A;
}

bool sfmath::LineLineIntersect(const MazeDefs::Vector2f vA, const MazeDefs::Vector2f vB, const MazeDefs::Vector2f uA, const MazeDefs::Vector2f uB, MazeDefs::Vector2f& intersectPoint)
{
	MazeDefs::Vector2f vDir = (vB - vA);
	MazeDefs::Vector2f uDir = (uB - uA);

	//Cross product of the directions
	float dirCross = Cross(vDir, uDir);
	MazeDefs::Vector2f originDiff = uA - vA;

	if (dirCross == 0 && Cross(originDiff, vDir) == 0)
	{
		float vDot = Dot(vDir, vDir);
		float t0 = Dot(originDiff, vDir) / vDot;
		float t1 = t0 + (Dot(uDir, vDir) / vDot);
					
		if (t0 >= 0 && t0 <= 1)
		{
			t0 = roundf(t0 * 100) / 100;
			intersectPoint = vA + t0 * vDir;
			return true;
		}
		else if (t1 >= 0 && t1 <= 1)
		{
			t1 = roundf(t1 * 100) / 100;
			intersectPoint = vA + t1 * vDir;
			return true;
		}
			return false;

	}
	else
	{
		float t = (Cross(originDiff, uDir)) / dirCross;
		float u = (Cross(originDiff, vDir)) / dirCross;
		if (t >= 0 && t <= 1 && u >= 0 && u <= 1)
		{
			t = roundf(t * 100) / 100;
			intersectPoint = vA + (float)t * vDir;
			return true;
		}
	}
	
	return false;
}

bool sfmath::RayLineIntersect(const Ray& ray, const MazeDefs::Vector2f a, const MazeDefs::Vector2f b)
{
	//Read this http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect/565282#565282
	//The direction of the line, normalized
	MazeDefs::Vector2f lineDir = b - a;

	//Cross product of the directions
	float dirCross = Cross(ray.direction, lineDir);
	if (dirCross == 0)
		return false;

	MazeDefs::Vector2f originDiff = a - ray.origin;
	float t = (Cross(originDiff, lineDir)) / dirCross;
	float u = (Cross(originDiff, ray.direction)) / dirCross;

	//Since t is for ray it just needs to be >= 0 and u needs to between 0 and 1
	return ((t >= 0) & (u >= 0) & (u <= 1));
}

bool sfmath::IsReflex(const MazeDefs::Vector2f& p, const MazeDefs::Vector2f& prev, const MazeDefs::Vector2f& next, bool counterClockwise)
{
	MazeDefs::Vector2f vec1, vec2;
	float angle;
	vec1 = prev - p;
	vec2 = next - p;

	angle = Angle(vec1, vec2);
	printf("Angle: %lf \n", angle);
	if (angle > 0)
		return false;

	return true;
}

std::vector<MazeDefs::Vector2f> sfmath::InvertShape(const std::vector<MazeDefs::Vector2f>& vertices, MazeDefs::Vector2f origin)
{
	std::vector<MazeDefs::Vector2f> result;
	result.reserve(vertices.size());
	for (unsigned int i = 0; i < vertices.size(); ++i)
	{
		result.push_back(2.f*origin - vertices[i]);
	}
	return result;
}

int sfmath::Mod(int i, int base)
{
	return ((i % base) + base) % base;

}

float sfmath::Angle(const MazeDefs::Vector2f& v1, const MazeDefs::Vector2f& v2)
{
	return atan2(sfmath::Cross(v1, v2), sfmath::Dot(v1, v2));
}

MazeDefs::Vector2f sfmath::Normalize(const MazeDefs::Vector2f& vec)
{
	return vec / Length(vec);
}

MazeDefs::Vector3f sfmath::Normalize(const MazeDefs::Vector3f& vec)
{
	return vec / Length(vec);
}

bool sfmath::SameDirection(const MazeDefs::Vector2f& a, const MazeDefs::Vector2f& b)
{
	return (sfmath::Dot(a, b) > 0);
}

bool sfmath::SameDirection(const MazeDefs::Vector3f& a, const MazeDefs::Vector3f& b)
{
	return (sfmath::Dot(a, b) > 0);
}

sfmath::Simplex sfmath::GetSimplex(const std::vector<MazeDefs::Vector2f>& verticesA, const std::vector<MazeDefs::Vector2f>& verticesB, const MazeDefs::Vector2f& dir)
{
	Simplex result;
	result.aSupp = GetSupportPoint(verticesA, dir);
	result.bSupp = GetSupportPoint(verticesB, -dir);
	result.dir = dir;
	result.pos = result.aSupp - result.bSupp;

	return  result; 
}

MazeDefs::Vector2f sfmath::GetSupportPoint(const std::vector<MazeDefs::Vector2f>& vertices, const MazeDefs::Vector2f& dir)
{
	float max = -FLT_MAX;
	MazeDefs::Vector2f result;
	float dot;

	for (int i = 0; i < vertices.size(); ++i)
	{
		dot = sfmath::Dot(dir, vertices[i]);
		if (dot > max)
		{
			max = dot;
			result = vertices[i];
		}
	}

	return result;
}

MazeDefs::Vector2f sfmath::ProjectOntoVector(const MazeDefs::Vector2f& vec, const MazeDefs::Vector2f& target)
{
	return ((Dot(vec, target)) * target) / Length2(target);
}