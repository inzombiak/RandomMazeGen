#ifndef MATH_H
#define MATH_H

#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>

namespace sfmath
{
	struct Ray
	{
		MazeDefs::Vector2f origin;
		MazeDefs::Vector2f direction;
	};

	struct Simplex
	{
		MazeDefs::Vector2f pos;
		MazeDefs::Vector2f dir;
		MazeDefs::Vector2f aSupp;
		MazeDefs::Vector2f bSupp;

		inline MazeDefs::Vector2f operator - (const Simplex& other)
		{
			return pos - other.pos;
		}
	};

	float Dot(const MazeDefs::Vector2f& v1, const MazeDefs::Vector2f& v2);

	float Length(const MazeDefs::Vector2f& v);
	float Length2(const MazeDefs::Vector2f& v);
	float Cross(const MazeDefs::Vector2f& v1, const MazeDefs::Vector2f& v2);
	float Angle(const MazeDefs::Vector2f& v1, const MazeDefs::Vector2f& v2);

	bool LineLineIntersect(const MazeDefs::Vector2f vA, const MazeDefs::Vector2f vB, const MazeDefs::Vector2f uA, const MazeDefs::Vector2f uB, MazeDefs::Vector2f& intersectPoint);
	bool RayLineIntersect(const Ray& ray, const MazeDefs::Vector2f a, const MazeDefs::Vector2f b);

	MazeDefs::Vector2f GetSupportPoint(const std::vector<MazeDefs::Vector2f>& vertices, const MazeDefs::Vector2f& dir);
	sfmath::Simplex GetSimplex(const std::vector<MazeDefs::Vector2f>& verticesA, const std::vector<MazeDefs::Vector2f>& verticesB, const MazeDefs::Vector2f& dir);
	MazeDefs::Vector2f ProjectOntoVector(const MazeDefs::Vector2f& vec, const MazeDefs::Vector2f& target);
	MazeDefs::Vector2f Normalize(const MazeDefs::Vector2f& vec);

	bool SameSideOfLine(const MazeDefs::Vector2f& a, const MazeDefs::Vector2f& b, const MazeDefs::Vector2f& p1, const MazeDefs::Vector2f& p2);
	bool PointInTriangle(const MazeDefs::Vector2f& p0, const MazeDefs::Vector2f& p1, const MazeDefs::Vector2f& p2, const MazeDefs::Vector2f& p);
	bool SameDirection(const MazeDefs::Vector2f& a, const MazeDefs::Vector2f& b);

	bool IsReflex(const MazeDefs::Vector2f& p, const MazeDefs::Vector2f& prev, const MazeDefs::Vector2f& next, bool counterClockwise = true);
	std::vector<MazeDefs::Vector2f> InvertShape(const std::vector<MazeDefs::Vector2f>& vertices, MazeDefs::Vector2f origin = MazeDefs::Vector2f(0.f, 0.f));
	int Mod(int i, int base);
	struct Vector2fComperator
	{
		bool operator()(MazeDefs::Vector2f const& a, MazeDefs::Vector2f const& b)
		{
			return (a.x < b.x) || (a.x == b.x && a.y < b.y);
		}
	};


}

#endif